/* Virtual texture ("Megatexture") implementation - August 2007
 *
 * We have:
 *    Physical textures: these store the actual raw pixels
 *    Page tables: these are GL_NEAREST textures mapping virtual to physical
 *    Virtual space: a texture coordinate space that's indirected
 *    Virtual textures: what we apply to objects
 *
 * Physical texture:
 *    Suppose we use a 4KB x 2KB physical texture (for lower-spec, we use 2k x 2k).
 *    If we want to avoid having to do S3TC on the fly, we need 4 bytes pp.
 *    This is 16*4 = 32MB for a single physical texture.
 *
 *    Suppose we have the following surface textures (based on HL2):
 *        BaseTex (RGB) 
 *        BumpmapTex (RGB)
 *        EmissiveTex (RGB)
 *
 *    If all three are 4 bytes, this requires 96MB. Note that if we
 *    aggressively use the double-ended allocation approach, we can do
 *    better, but we're limited by the fact that everything has a bump-map.
 *
 *    Suppose we have 5 2k x 2k RGBA textures: that requires 16MB * 5 = 64MB.
 *
 *    Here are the two orderings:
 *        1. RGB=diffuse, A=specular
 *        2. RGB=normal, A=metal/plastic
 *        3. RGB=emissive_mirror, A=mirror/emissive       3. RGB=emissive_mirror, A=mirror/emissive
 *        4. RGB=emissive_mirror, A=mirror/emissive       4. RGB=normal
 *                                                        5. RGB=diffuse, A=mirror
 *
 *
 * A page is a block of texels with padding and with mipmaps. Without mipmapping,
 * each page-sized block of virtual texels maps to a page-sized block of physical
 * textures. With mipmapping, 2x2 pages of virtual texture might map to a single
 * 1x1 page of physical texture, or 4x4 pages, etc.
 *
 * To support multitexture, a virtual texture might consist of multiple identically
 * indexed textures. The the physical texture works identically.
 *
 * Because different virtual textures might use different numbers of multitextures,
 * and we would like to optimize the sharing, we allow reusing the same physical
 * page for multiple spaces if they don't overlap. So for example you might use
 * the first K multitex or the last K for arbitrary K for each virtual texture;
 * then two virtual textures that use non-overlapping textures can use the same page.
 * (E.g. suppose you have 7 stacked textures--some with alpha, some without, etc.
 * You arranged some preferred order so you always use the first N, and maybe add
 * one more stacked texture on the end. Then a 1-texture thing always uses the last
 * one; a 7 texture always use the first 7; a 4-texture uses the last 4, and a 3-texture
 * uses the first 3. This may result in higher utilization.)
 * 
 * Each virtual texture coordinate space can be shared by multiple textures
 * (they must use non-overlapping portions of it, though). These textures
 * can use differing numbers of multitexture (and freely first or last K).
 *
 * svtexset
 *   A set of physical textures are a svtexset. The svtexset tracks what pages
 *   are in use. It tracks abandoned pages in LRU order and allows them to be
 *   'restored' if they're not overwritten first. Each physical texture in the
 *   set is called a 'plane'. Each page can be allocated twice if the set of planes
 *   needed for that page are disjoint; you must make sure this is possible to
 *   get this efficiency.
 *
 * svtexspace
 *   A svtexspace is a virtual coordinate space with a page table mapping it
 *   onto a single svtexset. It keeps track of which coordinates are mapping
 *   to which pages, and allows you to determine the delta to go from the
 *   current situation to the future. You can have multiple svtexspaces targetting one svtexset.
 *   It allows you to specify local repeats (so you can have short-range-wrapping
 *   textures), and other reuse scenarios (e.g. multiple model instances with limited
 *   variation).
 *
 * svtextex
 *   A svtextex is a virtual texture.
 *
 * Preliminary rendering: we initially render an RGBA texture using the default
 *   texture coordinates. We need to know which pages are needed, so we need to
 *   recover:
 *         page_x
 *         page_y
 *         miplevel
 *         svtexspace_id
 *
 *   We can roll svtexspace_id into the main coordinates. The idea is that we
 *   assemble all the svtexspaces into one logical coordinate space; i.e. we
 *   encode the svtexspace_id in the top bits of page_x and page_y.
 *
 *   To clarify:
 *         page_x = R + (B>>4)*256  // 2^12
 *         page_y = G + (B&15)*256  // 2^12
 *         id_hi  = a & 15
 *         miplevel = a >> 4  // 0..15
 *
 *   If a page is 128x128, then a full megaspace above is 512K x 512K.
 *   If we call a full space like that '4096x4096' (i.e. in pages), then
 *   we are limited to 16 4Kx4K megaspaces. However, each megaspace which
 *   is smaller than 4Kx4K only needs to use part of the coordinate space.
 *   For example, four 4Kx1K megaspaces can share the same id_hi, using
 *   the top two bits of the Y coordinate to distinguish themselves.
 *
 *   How to render this? Well, we can use a pixel shader to generate it
 *   directly. Or we can get tricky and use a couple of multitextures
 *   that are added--but we'll need to replicate the coordinates in the
 *   vertex shader or the caller:
 *      A_hi: one texture with mipmapping on the raw coordinates
 *      RG  : one 2^8 x 2^8 texture on page coordinates * 16
 *      B1  : one 2^4 x 2^4 texture on page coordinates
 *      B2  : constant color with low bits of id
 *      A_lo: constant color with high bits of id
 *      RG  : constant color with really-low bits of id if very small space
 *   Pixel shader is faster? Except maybe for computing G1.
 *
 */

#include <stdlib.h>
#include <assert.h>

#include "all.h"
#include "svt_phystex.h"
#include "svt_vtex.h"
#include "svt_sparsetex.h"
#include "prof.h"

// if a page is 64x64, and our physical texture is 4K x 4K, it will only
// require 2^12/2^6 = 2^6 possible coordinates per channel, so each can
// actually fit in 8 bits. 8 bits works up to 32x32 with 8K x 8K.
typedef struct
{
   uint8 x,y;
} svtex_pagecoord;

#define SPARSE_LIMIT 8

// if a svt_texset is 4096x4096 and uses 32x32 pages, it will
// have only 16K slots, so even using 32 bytes per slot we
// only need 512KB. Since the megaset uses 8MB/texture, and
// say an average of 3 textures, then that's 512KB for 24MB
// of texture, which is fine.
//
// And in practice we expect 4096x2048 using 64x64 pages,
// which means 2K slots, so it's only 64KB.
struct svtexset_usage_st
{
   svtexset_usage *next, **prev;
   uint32 id;
   uint16 which_tex;
}; // 28

struct svtexset_st
{
   int w,h, wlog2,hlog2;
   svtexset_usage *pages;
   svtexset_usage head;
   svtex **tex;
};

#define svtex_tex_null     0xffff
#define svtex_tex_head      0xfffe

svtexset * svtexset_new(int w, int h)
{
   int i;
   svtexset *ms = malloc(sizeof(*ms));
   if (!ms) return NULL;
   ms->pages = malloc(sizeof(*ms->pages) * w * h);
   if (!ms->pages) { free(ms); return NULL; }
   ms->w = w;
   ms->h = h;
   ms->wlog2 = stb_log2_floor(w);
   ms->hlog2 = stb_log2_floor(h);
   ms->tex = NULL;
   for (i=0; i < w*h; ++i) {
      svtexset_usage *u = &ms->pages[i];
      u->next = &ms->pages[i+1];
      u->prev = &ms->pages[i-1].next;
      u->which_tex = svtex_tex_null;
   }
   ms->pages[w*h-1].next = &ms->head;
   ms->pages[0].prev = &ms->head.next;
   ms->head.next = &ms->pages[0];
   ms->head.prev = &ms->pages[w*h-1].next;
   ms->head.which_tex = svtex_tex_head;
   ms->tex = NULL;
   return ms;
}

int svtexset_count(svtexset *set)
{
   return set->w * set->h;
}


int svtexset_add_tex(svtexset *set, svtex *tex)
{
   stb_arr_push(set->tex, tex);
   return stb_arr_len(set->tex)-1;
}

static void svtexset_usage_unlink(svtexset_usage *u)
{
   u->next->prev = u->prev;
   *(u->prev) = u->next;
   u->next = NULL;
   u->prev = NULL;
}

Bool svtexset_check_page(svtexset *ms, svtex_pagecoord pc, uint16 which_tex, uint32 id)
{
   int n = pc.x + (pc.y << ms->wlog2);
   svtexset_usage *u = &ms->pages[n];
   if (u->id == id && u->which_tex == which_tex) {
      if (u->next) {
         // if in LRU queue, unlink it so it's active again!
         svtexset_usage_unlink(u);
      }
      return True;
   }
   return False;
}

static void svtexset_usage_link(svtexset *ms, svtexset_usage *u)
{
   // insert u in list k
   u->next = &ms->head;
   u->prev = ms->head.prev;
   *(u->prev) = u;
   ms->head.prev = &u->next;
}

extern void svtex_abandon_page(svtex *tex, uint32 id);

void svtexset_abandon(svtexset *ms, uint16 tex, uint32 id)
{
   svtex_abandon_page(ms->tex[tex], id);
}

svtex_pagecoord svtexset_alloc_page(svtexset *ms, uint16 which_tex, uint32 id, int planes)
{
   int n;
   svtexset_usage *u=NULL;
   svtex_pagecoord pc = { 255,255 };
   u = ms->head.next;
   if (u->which_tex == svtex_tex_head)
      return pc;

   svtexset_usage_unlink(u);
   if (u->which_tex != svtex_tex_null)
      svtexset_abandon(ms, u->which_tex, u->id);

   u->id = id;
   u->which_tex = which_tex;

   n = u - ms->pages;
   pc.x = n & (ms->w - 1);
   pc.y = n >> ms->wlog2; 
   return pc;
}

void svtexset_flush(svtexset *set)
{
   svtexset_usage *u = set->head.next;
   while (u != &set->head) {
      if (u->which_tex != svtex_tex_null) {
         svtexset_abandon(set, u->which_tex, u->id);
         u->which_tex = svtex_tex_null;
      }
      u = u->next;
   }
}

void svtexset_release_page(svtexset *ms, svtex_pagecoord pc, uint16 which_tex, uint32 id)
{
   int n = pc.x + (pc.y << ms->wlog2);
   // move to tail of free list
   svtexset_usage *u = &ms->pages[n];
   assert(u->id == id);
   assert(u->which_tex == which_tex);
   assert(u->next == NULL);
   svtexset_usage_link(ms, u);
}

typedef struct
{
   svtex_pagecoord pc;
   uint8 mip_level;
   uint8 padding;
} svt_pagetable; // 4 bytes

struct svtexspace_st
{
   int w,h, wlog2, hlog2;
   int max_mip;
   Bool dirty;
   Bool really_dirty;
   svtexset *set;
   uint page_texture;
   svt_pagetable **page_table;
   uint8 **page_data;
   int hlog[MAX_VTEX_MIP_LEVEL];
   svt_sparseupdater *updater[MAX_VTEX_MIP_LEVEL];
};

#define pagecomp(x,y)   (*(int *)&(x) == *(int *)&(y))

struct foo
{
   svtexspace *set;
   int m;
};

int page_dl;
static void download_update(int x0, int y0, int x1, int y1, void *p)
{  Prof_Begin(download_update)
   struct foo *z = p;
   int m = z->m;
   svtexspace *s = z->set;
   glTexSubImage2D(GL_TEXTURE_2D, m, x0,y0, x1-x0,y1-y0, GL_RGBA, GL_UNSIGNED_BYTE, &s->page_table[m][(y0 << s->hlog[m]) + x0]);
   page_dl += (y1-y0) * (x1-x0);
   Prof_End
}
//   ts->page_table[m][x + (y << ts->hlog[m])] = p;

void svtexspace_download(svtexspace *set)
{  Prof_Begin(svtexspace_download)
   int i,w,h;
   if (set->really_dirty) {
      glBindTexture(GL_TEXTURE_2D, set->page_texture);
      for (i=0; i < set->max_mip; ++i) {
         w = set->w >> i;
         h = set->h >> i;
         glTexImage2D(GL_TEXTURE_2D, i, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, set->page_table[i]);
         page_dl += w*h;
      }
   } else {
      glBindTexture(GL_TEXTURE_2D, set->page_texture);
      for (i=0; i < SPARSE_LIMIT; ++i) {
         struct foo z = { set, i };      
         if (i >= set->max_mip) break;
         glPixelStorei(GL_UNPACK_ROW_LENGTH, (1 << set->hlog[i]));
         svt_sparse_process_blocks(set->updater[i], download_update, &z);
      }
      glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
      for (; i < set->max_mip; ++i) {
         w = set->w >> i;
         h = set->h >> i;
         glTexImage2D(GL_TEXTURE_2D, i, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, set->page_table[i]);
         page_dl += w*h;
      }
   }
   set->dirty = False;
   set->really_dirty = False;
   Prof_End
}

void svtexspace_sync(svtexspace *set)
{  Prof_Begin(svtexspace_sync)
   if (set->dirty || set->really_dirty)
      svtexspace_download(set);
   Prof_End
}

svtexspace *svtexspace_new_limit(svtexset *set, int w, int h, int limit)
{
   int i;
   svtexspace *ms = malloc(sizeof(*ms));
   if (ms == NULL) return NULL;
   memset(ms, 0, sizeof(*ms));
   ms->set = set;
   ms->w = w;
   ms->h = h;
   ms->wlog2 = stb_log2_floor(w);
   ms->hlog2 = stb_log2_floor(h);
   ms->max_mip = MAX_VTEX_MIP_LEVEL;
   if (ms->max_mip > ms->wlog2+1) ms->max_mip = ms->wlog2+1;
   if (ms->max_mip > ms->hlog2+1) ms->max_mip = ms->hlog2+1;
   if (limit) {
      limit = stb_log2_floor(limit);
      if (ms->max_mip > limit) ms->max_mip = limit;
   }
   ms->page_table = malloc(sizeof(*ms->page_table) * ms->max_mip);
   ms->page_data  = malloc(sizeof(*ms->page_data ) * ms->max_mip);
   if (ms->page_table == NULL || ms->page_data == NULL) {
      if (ms->page_table) free(ms->page_table);
      if (ms->page_data) free(ms->page_data);
      free(ms);
      return NULL;
   }
   ms->page_texture = svt_tex_alloc(w, h, ZXTEX_rgba8);
   svt_tex_filter(ms->page_texture, ZXTEX_mipmap_nearest, ZXTEX_nearest, ZXTEX_nearest);
   glBindTexture(GL_TEXTURE_2D, ms->page_texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, ms->max_mip-1);
   for (i=0; i < ms->max_mip; ++i) {
      int j;
      ms->page_table[i] = malloc(sizeof(*ms->page_table[i]) * w * h);
      ms->page_data[i]  = calloc(sizeof(*ms->page_data [i]),  w * h);
      for (j=0; j < w*h; ++j)
         ms->page_table[i][j].mip_level = 255;
      ms->hlog[i] = ms->wlog2 - i;
      ms->updater[i] = svt_sparse_new(w,h);
      w >>= 1;
      h >>= 1;
   }
   ms->really_dirty = True;
   svtexspace_sync(ms);
   return ms;
}

svtexspace *svtexspace_new(svtexset *set, int w, int h)
{
   return svtexspace_new_limit(set, w, h, 0);
}

int svtexspace_num_levels(svtexspace *space)
{
   return space->max_mip;
}

float svtexspace_bias(svtexspace *space)
{
   return 5 - (10 - space->max_mip)/2.0;
}

extern void svtexspace_pagetablesize(svtexspace *ms, float *out2)
{
   out2[0] = ms->w;
   out2[1] = ms->h;
}

void svtexset_physsizerecip(svtexset *set, float *out2)
{
   out2[0] = 1.0 / set->w;
   out2[1] = 1.0 / set->h;
}

uint svtexspace_pagetex(svtexspace *space)
{
   return space->page_texture;
}

int ptouch;
void svtexspace_set_block_rec(svtexspace *ts, svt_pagetable p, int x, int y, int m, int t)
{
   ts->page_table[m][x + (y << ts->hlog[m])] = p;
   ++ptouch;
   if (m < SPARSE_LIMIT) svt_sparse_add(ts->updater[m], x,y);
   if (m > t) {
      int xy,s;
      m -= 1;
      x <<= 1;
      y <<= 1;
      xy = x + (y << ts->hlog[m]);
      s = 1 << ts->hlog[m];
      if (ts->page_table[m][xy    ].mip_level > m) svtexspace_set_block_rec(ts, p, x  , y  , m, t);
      if (ts->page_table[m][xy+1  ].mip_level > m) svtexspace_set_block_rec(ts, p, x+1, y  , m, t);
      if (ts->page_table[m][xy+s  ].mip_level > m) svtexspace_set_block_rec(ts, p, x  , y+1, m, t);
      if (ts->page_table[m][xy+s+1].mip_level > m) svtexspace_set_block_rec(ts, p, x+1, y+1, m, t);
   }
}

void svtexspace_abandon_page_rec(svtexspace *ts, svt_pagetable p, svt_pagetable sub, int x, int y, int m)
{
   assert(pagecomp(p, ts->page_table[m][x + (y << ts->hlog[m])]));
   ts->page_table[m][x + (y << ts->hlog[m])] = sub;
   ++ptouch;
   if (m < SPARSE_LIMIT) svt_sparse_add(ts->updater[m], x,y);
   if (m) {
      int xy,s;
      m -= 1;
      x += x;
      y += y;
      s = 1 << ts->hlog[m];
      xy = x + (y << ts->hlog[m]);
      if (pagecomp(p, ts->page_table[m][xy    ])) svtexspace_abandon_page_rec(ts, p, sub, x  , y  , m);
      if (pagecomp(p, ts->page_table[m][xy+1  ])) svtexspace_abandon_page_rec(ts, p, sub, x+1, y  , m);
      if (pagecomp(p, ts->page_table[m][xy+s  ])) svtexspace_abandon_page_rec(ts, p, sub, x  , y+1, m);
      if (pagecomp(p, ts->page_table[m][xy+s+1])) svtexspace_abandon_page_rec(ts, p, sub, x+1, y+1, m);
   }
}

void svtexspace_abandon_page(svtexspace *ts, int x, int y, int m)
{  Prof_Begin(svtexspace_abandon_page)
   int x2 = x >> 1, y2 = y >> 1;
   svt_pagetable p    =  ts->page_table[m  ][x  + (y  << ts->hlog[m  ])];
   svt_pagetable sub  =  ts->page_table[m+1][x2 + (y2 << ts->hlog[m+1])];

   // you can't abandon the topmost page!
   assert(m+1 < ts->max_mip);

   svtexspace_abandon_page_rec(ts, p, sub, x, y, m);
   ts->dirty = True;
   Prof_End
}

struct svtex_st
{
   int w,h,x,y;
   int which_tex;
   int planes,k;
   svtexspace *space;
};

svtex *svtex_new(svtexspace *sp, int x, int y, int w, int h)
{
   svtex *t = malloc(sizeof(*t));
   if (t == NULL) return NULL;
   t->space = sp;
   t->which_tex = svtexset_add_tex(sp->set, t);
   t->x = x;
   t->y = y;
   t->w = w;
   t->h = h;
   t->planes = 1;
   t->k =  0;
   return t;
}

typedef struct
{
   uint32 x:14;
   uint32 y:14;
   uint32 mip:4;
} svtex_vloc;

void svtex_abandon_page(svtex *tex, uint32 id)
{
   svtex_vloc z = *(svtex_vloc *) &id;
   svtexspace_abandon_page(tex->space, (tex->x >> z.mip) + z.x, (tex->y >> z.mip) + z.y, z.mip);
}

Bool svtex_alloc_locked_page(svtex *tex, int x, int y, int m, int *ox, int *oy, int t)
{  Prof_Begin(svtex_alloc_locked)
   svtex_vloc z = { x,y,m };
   svtex_pagecoord pc;
   assert(m < tex->space->max_mip);
   pc = svtexset_alloc_page(tex->space->set, tex->which_tex, *(uint32 *) &z, tex->planes);
   if (pc.x != 255 || pc.y != 255) {
      // fill in the mip levels for it!
      svt_pagetable p;
      p.pc = pc;
      p.mip_level = m;
      p.padding = 255;
      if (t < 0) t = 0;
      x += tex->x >> m;
      y += tex->y >> m;
      { Prof_Begin(svtexspace_set_block_rec)
      svtexspace_set_block_rec(tex->space, p, x, y, m, t);
      Prof_End }
      assert(sizeof(p) == 4);
      tex->space->dirty = True;
      *ox = pc.x;
      *oy = pc.y;
      Prof_End
      return True;
   }
   Prof_End
   return False;
}

void svtex_unlock_page(svtex *tex, int x, int y, int m)
{
   svtex_vloc id = { x,y,m };
   svt_pagetable p;
   svtexspace *s = tex->space;
   x += tex->x >> m;
   y += tex->y >> m;
   p = s->page_table[m][x + (y << s->hlog[m])];
   assert(p.mip_level == m);
   svtexset_release_page(s->set, p.pc, tex->which_tex, *(uint32 *) &id);
}

Bool svtex_lock_page(svtex *tex, int x, int y, int m)
{
   svtex_vloc id = { x,y,m  };
   Bool res;
   svtexspace *s = tex->space;
   svt_pagetable p;
   assert(m < tex->space->max_mip);
   x += tex->x >> m;
   y += tex->y >> m;
   p = s->page_table[m][x + (y << s->hlog[m])];
   if (p.mip_level != m) return False;
   res = svtexset_check_page(s->set, p.pc, tex->which_tex, *(uint32 *) &id);
   assert(res);
   p = s->page_table[m][x + (y << s->hlog[m])];
   assert(p.mip_level == m);
   return True;
}

void zvtex_propagate_parent(svtex *tex, int x, int y, int m)
{  Prof_Begin(zvtex_propogate_parent)
   svtexspace *s = tex->space;
   svt_pagetable *p;
   x += tex->x >> m;
   y += tex->y >> m;
   p = &s->page_table[m][x + (y << s->hlog[m])];
   if (p->mip_level > m+1) {
      svt_pagetable *q = &s->page_table[m+1][(x>>1) + ((y>>1) << s->hlog[m+1])];
      if (p->mip_level != q->mip_level) {
         *p = *q;
         ++ptouch;
         if (m < SPARSE_LIMIT) svt_sparse_add(s->updater[m], x,y);
         s->dirty = True;
      }
   }
   Prof_End
}

uint8 *svtex_page_data(svtex *tex, int x, int y, int m)
{
   svtexspace *s = tex->space;
   x += (tex->x >> m);
   y += (tex->y >> m);
   return &s->page_data[m][x + (y << s->hlog[m])];
}
