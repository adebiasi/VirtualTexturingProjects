#include <math.h>
#include <ctype.h>
#include <stdarg.h>
#include "all.h"
#include "vec3f.h"
#include "svt_vtex.h"
#include "svt_phystex.h"
#include "pgen_bitmap.h"
#include "stb_image.h"
#include "prof.h"
#define STB_DEFINE
#include "stb_gl.h"
#define STB_WINMAIN
#include "stb_wingraph.h"

#define READ_RGBA

void fatal_error(char *s, char *file, int line)
{
   stbwingraph_ods("%s %d: %s", file, line, s);
   assert(0);  // catch it in the debugger
   exit(1);
}


int slide_mode;


int scr_w, scr_h;

#define MAX_UPDATES_NORMAL 20
#define MAX_UPDATES_FAST   32

void computeRelativeTranslation(vec3f *out, vec3f *src, vec3f *angle)
{
  GLdouble matrix[16];
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glRotatef(-angle->y,  0.0f, 1.0f, 0.0f);
  glRotatef(-angle->x,  1.0f, 0.0f, 0.0f);
  glRotatef(-angle->z,  0.0f, 0.0f, 1.0f);
  glGetDoublev(GL_MODELVIEW_MATRIX, matrix);
  glPopMatrix();

  out->x = matrix[0]*src->x + matrix[1]*src->y + matrix[2]*src->z;
  out->y = matrix[4]*src->x + matrix[5]*src->y + matrix[6]*src->z;
  out->z = matrix[8]*src->x + matrix[9]*src->y + matrix[10]*src->z;
}

vec3f vel, ang_vel;
vec3f camera_ang = { 0,0,-45} , camera_loc = { 25,25, 50 };

void camera_sim(float simtime)
{
   vec3f rot_vel;
   computeRelativeTranslation(&rot_vel, &vel, &camera_ang);
   vec3f_addeq_scale(&camera_ang, &ang_vel, simtime);
   vec3f_addeq_scale(&camera_loc, &rot_vel, simtime);
}

static void clear(void)
{
   glViewport(0,0,scr_w,scr_h);
   glClearColor(0.75,0.45,0.85,1);    // blue
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);
}

int mipsample_shift = 4;
static void clear2(void)
{
   glDepthMask(GL_TRUE);
   glViewport(0,0,scr_w >> mipsample_shift,scr_h >> mipsample_shift);
   glClearColor(1,1,1,1);      // not a valid ID!
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);
}

static void start(void)
{
   glViewport(0,0,scr_w,scr_h);
   glClear(GL_DEPTH_BUFFER_BIT);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);
}

static void setup_view(void)
{
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(74, scr_w/(float)scr_h, 0.25, 5000);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glRotatef(-90, 1,0,0);
   glRotatef(-camera_ang.y, 0,1,0);
   glRotatef(-camera_ang.x, 1,0,0);
   glRotatef(-camera_ang.z, 0,0,1);
   glTranslatef(-camera_loc.x, -camera_loc.y, -camera_loc.z);
}

void glprint(int x, int y, char *str, ...)
{
   char buffer[4096];
   va_list v;
   va_start(v, str);
   vsprintf(buffer, str, v);
   va_end(v);
   glListBase(4096);
   glColor3f(0,0,0);
   glRasterPos2i(x+1,y+1);
   glCallLists(strlen(buffer), GL_UNSIGNED_BYTE, buffer);
   glColor3f(1,1,1);
   glRasterPos2i(x,y);
   glCallLists(strlen(buffer), GL_UNSIGNED_BYTE, buffer);
   glListBase(0);
}

int opt_shader;

GLhandleARB frag_shade, mip_shade, frag_shade_unlit;
GLuint frag_prog, mip_prog, frag_prog_unlit;
GLuint base_tex, norm_tex, base_tex2, norm_tex2, mipmap_tex, sky[6];
uint8 *page_sample_buffer;

float page_table_size2[2] = { 4,4};
float phys_size_recip2[2] = { 1/4.0, 1/2.0};
float virt_tex_size[2];
float page_size_log2f;

svtexset    *vset;
svtexspace  *vspace, *vspace2, *slide_space;
svtex       *vtex, *vtex2;

float mip_bias_debug = 0;
Bool do_clear = True;
extern Bool main_block(int tex, int x, int y, int m, int t);

//#define svtex foo foo

typedef struct
{
   uint16 x,y;
   uint8 m, texture;
   uint8 valid, goal;
} Block;

Block *live_blocks;
Block *consider[MAX_VTEX_MIP_LEVEL];

int active_blocks, requested_blocks, updated_blocks, visited_blocks, forced_blocks;

svtex *texlist[256];
int tex_mip_limit[256];
int tex_min_mip[256]; // minimum mip level while waiting for source data to load

struct
{
   int x0,y0;
   int x1,y1;
} tex_pagerange[256];

Bool request_block1(int tex, int x, int y, int m)
{
   ++requested_blocks;
   if (svtex_lock_page(texlist[tex], x,y,m)) {
      Block b = { x,y,m,tex };
      stb_arr_push(live_blocks, b);
      ++active_blocks;
      return True;
   }
   return False;
}

enum
{
   RESULT_pending,
   RESULT_done,
   RESULT_abandoned,
};

typedef struct
{
   int16 x;
   int16 y;
   uint16 tex;
   uint16 mip;
} BlockKey;

typedef struct
{
   BlockKey key;
   Bool  active;
   Bool  still_want_it;
   int   result_flag;
   uint8 block[64][64][4];
   uint8 bmip[32][32][4];
} BlockRequest;

#define MAX_PENDING_BLOCKS 256

BlockRequest block_queue[MAX_PENDING_BLOCKS];

stb_define_hash(BlockRequestMap, brm_, BlockKey *, (void *) 0, (void *) 1, return stb_hash_number(k->x + k->y*65536 + k->mip*23471 + k->tex*3783);, BlockRequest *);

BlockRequestMap *block_map;

void flush_pending_updates(void)
{
   Prof_Begin(flush_pending_updates)
   int i;
   for (i=0; i < MAX_PENDING_BLOCKS; ++i)
      block_queue[i].still_want_it = False;

   for (i=0; i < MAX_PENDING_BLOCKS; ++i) {
      if (block_queue[i].active && block_queue[i].result_flag) {
         BlockRequest *z;
         brm_remove(block_map, &block_queue[i].key, &z);
         z->active = False;
      }
   }
   Prof_End
}

Bool enqueue_block(Block *c)
{
#if 0
   int i;
   BlockKey bk = { c->x, c->y, c->texture, c->m };
   BlockRequest *z;
   volatile BlockRequest *b;
   if (brm_get_flag(block_map, &bk, &z)) {
      assert(0);
   }
   for (i=0; i < MAX_PENDING_BLOCKS; ++i)
      if (!block_queue[i].active)
         break;
   if (i == MAX_PENDING_BLOCKS) return False;

   b = &block_queue[i];
   b->key = bk;
   b->result_flag = RESULT_pending;
   b->still_want_it = True;
   stb_barrier();
   b->active = True;
#endif
   return True;
}

Bool is_block_avail(Block *c)
{
#if 1
return TRUE;
#else
   BlockKey bk = { c->x, c->y, c->texture, c->m };
   BlockRequest *z;
   return brm_get_flag(block_map, &bk, &z);
#endif
}

Bool request_block2(int tex, int x, int y, int m, int t)
{
#if 1
   if (main_block(tex, x,y,m,t)) {
      Block b = { x,y,m,tex };
      stb_arr_push(live_blocks, b);
      ++active_blocks;
      return True;
   }
#else
   BlockKey bk = { x,y,tex,m };
   BlockRequest *z;
   if (brm_get_flag(block_map, &bk, &z) && z->result_flag) {
      if (z->result_flag == RESULT_done) {
         Block b = { x,y,m,tex };
         stb_arr_push(live_blocks, b);
         ++active_blocks;
         return True;
      } else {
         z->still_want_it = False;
         z->active = False;
         brm_remove(block_map, &bk, &z);
      }
   }
#endif
   return False;
}

void unlock_blocks(void)
{
   Prof_Begin(unlock_blocks)
   while (stb_arr_len(live_blocks)) {
      Block b = stb_arr_pop(live_blocks);
      svtex_unlock_page(texlist[b.texture], b.x, b.y, b.m);
   }
   Prof_End
}

#define GOAL_OFFSET  (16)

float mip_sample_bias_base = - 0.25;
int use_program = 0;

#define svtex foo foo
uint max_updates=0xffffffff;
void update_vtex(void)
{
   Prof_Begin(update_vtex)
   int n = svtexset_count(vset);
   uint updates=0;
   int total=0;
   Bool download_limit = False;
   int i,j;
   uint8 *p = page_sample_buffer;

{ Prof_Begin(up_v_analyze)
   for (i=0; i < scr_w * scr_h >> (2*mipsample_shift); ++i, p += 4) {
      uint8 *w;
      Block b;
      int x,y,t,m;
      #ifdef READ_RGBA
      x = p[0] + ((p[2] & 0x0f) << 8);   
      y = p[1] + ((p[2] & 0xf0) << 4);
      m = p[3] & 15;
      t = p[3] & ~15;
      #else
      x = p[3] + ((p[1] & 0x0f) << 8);   
      y = p[2] + ((p[1] & 0xf0) << 4);
      m = p[0] & 15;
      t = p[0] & ~15;
      #endif
      if (m < 0) m = 0;
      if (*(uint32 *)p == 0xffffffff) continue;
      if (x < tex_pagerange[t].x0 || x >= tex_pagerange[t].x1 || y < tex_pagerange[t].y0 || y >= tex_pagerange[t].y1) {
         int i;
         for (i=1; i < 16; ++i) {
            if (x >= tex_pagerange[i+t].x0 && x < tex_pagerange[i+t].x1 && y >= tex_pagerange[i+t].y0 && y < tex_pagerange[i+t].y1) {
               t = i+t;
               x -= tex_pagerange[t].x0;
               y -= tex_pagerange[t].y0;
               break;
            }
         }
         if (i == 16)
            continue;
      }
      if (m < tex_min_mip[t]) m = tex_min_mip[t];
      if (m >= tex_mip_limit[t])
         continue; // we always leave this locked
      x >>= m;
      y >>= m;
      w = svtex_page_data(texlist[t], x,y,m);
      if (!*w) {
         b.x = x;
         b.y = y;
         b.m = m;
         b.texture = t;
         b.valid = True;
         *w = (m-1) + GOAL_OFFSET;
         assert(b.m < tex_mip_limit[b.texture]);
         stb_arr_push(consider[m], b);
         ++visited_blocks;
         ++total;
      }
   }
Prof_End}

{ Prof_Begin(up_v_force_mipmaps)
   // force mipmaps
   for (j=0; j < MAX_VTEX_MIP_LEVEL-2; ++j) {
      for (i=0; i < stb_arr_len(consider[j]); ++i) {
         int m = consider[j][i].m+1;
         int x = consider[j][i].x >> 1;
         int y = consider[j][i].y >> 1;
         int t = consider[j][i].texture;
         uint8 *data, *data2;
         if (m >= tex_mip_limit[t]) continue;
         data = svtex_page_data(texlist[t], x, y, m);
         data2 = svtex_page_data(texlist[t], consider[j][i].x, consider[j][i].y, consider[j][i].m);
         assert(*data2);
         if (!*data) {
            Block b;
            b.x = x;
            b.y = y;
            b.m = m;
            b.texture = t;
            b.valid = True;
            *data = *data2;
            assert(b.m < tex_mip_limit[b.texture]);
            stb_arr_push(consider[m], b);
            ++total;
            ++forced_blocks;
         } else if (*data > *data2)
            *data = *data2;
      }
   }
   stb_barrier(); // make sure 'tex_mip_limit' is synched to its data
Prof_End}

   if (total > n) {
      Prof_Begin(up_v_findbest)
      for (j=0; j < MAX_VTEX_MIP_LEVEL; ++j) {
         for (i=0; i < stb_arr_len(consider[j]); ++i) {
            int t = consider[j][i].texture;
            int m;
            consider[j][i].valid = 0;
            m = consider[j][i].m+1;
            --total;
            if (m < tex_mip_limit[t]) {
               int x = consider[j][i].x >> 1;
               int y = consider[j][i].y >> 1;
               uint8 *data = svtex_page_data(texlist[t], x, y, m);
               uint8 *data2 = svtex_page_data(texlist[t], consider[j][i].x, consider[j][i].y, consider[j][i].m);
               assert(*data2);
               if (!*data) {
                  Block b;
                  b.x = x;
                  b.y = y;
                  b.m = m;
                  b.texture = t;
                  b.valid = True;
                  *data = *data2;
                  b.goal = consider[j][i].goal;
                  assert(b.m < tex_mip_limit[b.texture]);
                  stb_arr_push(consider[m], b);
                  ++total;
               } else if (*data > *data2)
                  *data = *data2;
            }
            if (total <= n-20)
               break;
         }
         if (total <= n)
            break;
      }
      Prof_End
   }

{ Prof_Begin(up_v_req1)
   for (j=0; j < MAX_VTEX_MIP_LEVEL; ++j) 
      for (i=0; i < stb_arr_len(consider[j]); ++i)
         if (consider[j][i].valid)
            if (request_block1(consider[j][i].texture, consider[j][i].x, consider[j][i].y, consider[j][i].m))
               consider[j][i].valid = False;
Prof_End }

   flush_pending_updates();

{ Prof_Begin(up_v_req2)
   updates = 0;
   for (j=MAX_VTEX_MIP_LEVEL-1; j >= 0; --j) {
      for (i=0; i < stb_arr_len(consider[j]); ++i) {
         Block *c = &consider[j][i];
         if (c->valid) {
            if (updates < max_updates && is_block_avail(c) && request_block2(c->texture, c->x, c->y, c->m, *svtex_page_data(texlist[c->texture],c->x,c->y,c->m) - GOAL_OFFSET)) {
               ++updates;
            } else {
               enqueue_block(c);
               zvtex_propagate_parent(texlist[c->texture], c->x, c->y, c->m);
            }
         }
      }
   }
Prof_End }

   for (i=0; i < MAX_VTEX_MIP_LEVEL; ++i) {
      while (stb_arr_len(consider[i])) {
         Block b = stb_arr_pop(consider[i]);
         *svtex_page_data(texlist[b.texture], b.x, b.y, b.m) = 0;
      }
   }

   if (opt_shader)
      max_updates = MAX_UPDATES_FAST;
   else
      max_updates = MAX_UPDATES_NORMAL;
   Prof_End
}
#undef svtex

void svertex(float r, float s, float t, float us, float uc, float vs, float vc, float s0, float t0, float s1, float t1)
{
   float vsr = vs;
   vec3f n = { uc * vs, us * vs, vc}, p;
   vec3f tn,bt;
   vec3f_scale(&p, &n, r);
   tn.x = -us;
   tn.y = uc;
   tn.z = 0;
   vec3f_cross(&bt, &n, &tn);
   glTexCoord2f(s0 + s*s1, t0 + t*t1);
   if (opt_shader)
      glNormal3fv(&n.x);
   else {
      glMultiTexCoord3fv(GL_TEXTURE1_ARB, &n.x);
      glMultiTexCoord3fv(GL_TEXTURE2_ARB, &tn.x);
      glMultiTexCoord3fv(GL_TEXTURE3_ARB, &bt.x);
   }
   glVertex3fv(&p.x);
}

void do_draw_sphere(float r, float s0, float t0, float s1, float t1)
{
   float vc[200], vs[200], uc[200],us[200], u[200], v[200];
   int i,j,h,w;
   h = 60;
   w = 40;
   for (j=0; j < h; ++j) {
      vc[j] = cos(j * M_PI / h);
      vs[j] = sin(j * M_PI / h);
      v[j] = j / (float) h;
   }
   vc[j] = -1;
   vs[j] = 0;
   v[j] = 1;
   for (i=0; i < w; ++i) {
      uc[i] = cos(i * 2 * M_PI / w);
      us[i] = sin(i * 2 * M_PI / w);
      u[i] = i / (float) w;
   }
   uc[i] = uc[0];
   us[i] = us[0];
   u[i] = 1;
   glBegin(GL_TRIANGLE_FAN);
      svertex(r,u[0],v[0], us[0],uc[0], vs[0],vc[0], s0,t0,s1,t1);
      for (i=0; i <= w; ++i) {
         svertex(r, u[i],v[1], us[i],uc[i], vs[1],vc[1], s0,t0,s1,t1);
      }
   glEnd();
   for (j=1; j < h-1; ++j) {
      glBegin(GL_TRIANGLE_STRIP);
      for (i=0; i <= w; ++i) {
         svertex(r, u[i],v[j], us[i],uc[i], vs[j],vc[j], s0,t0,s1,t1);
         svertex(r, u[i],v[j+1], us[i],uc[i], vs[j+1],vc[j+1], s0,t0,s1,t1);
      }
      glEnd();
   }
}

void draw_sphere(float x, float y, float z, float r, float s0, float t0, float s1, float t1)
{  Prof_Begin(draw_sphere)
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glTranslatef(x,y,z);
   do_draw_sphere(r,s0,t0,s1,t1);
   glPopMatrix();
Prof_End
}

vec3f mnorm[257][257];
float map[257][257];
float mtc[257][257][2];
float mscale = 4;

vec3f ttan[257][257];
vec3f btan[257][257];

float height_scale=1.0;
void vertex(int x, int y)
{
   glTexCoord2f(mtc[y][x][0], mtc[y][x][1]);
   if (opt_shader)
      glNormal3fv(&mnorm[y][x].x);
   else {
      glMultiTexCoord3fv(GL_TEXTURE1_ARB, &mnorm[y][x].x);
      glMultiTexCoord3fv(GL_TEXTURE2_ARB, &ttan[y][x].x);
      glMultiTexCoord3fv(GL_TEXTURE3_ARB, &btan[y][x].x);
   }
   glVertex3f(x*mscale, y*mscale, map[y][x] * height_scale);
}

void do_draw(void)
{
   int x,y,s=1;
   if (opt_shader) s= 4;
   for (y=0; y < 256; y += s) {
      glBegin(GL_TRIANGLE_STRIP);
      for (x=0; x <= 256; x += s) {
         vertex(x,y+s);
         vertex(x,y);
      }
      glEnd();
   }
}

Bool rebuild = True;
GLuint world_dlist;
void predraw_world(void)
{
   if (!world_dlist)
      world_dlist = glGenLists(1);
   glNewList(world_dlist, GL_COMPILE);
   do_draw();
   glEndList();
   rebuild = False;
}

GLuint cur_shad;

void uni1f(char *name, float p)
{
   if (glUniform1fARB) {
      GLuint z = glGetUniformLocationARB(cur_shad, name);
      glUniform1fARB(z, p);
   }
}

void uni2fv(char *name, float *p)
{
   if (glUniform2fvARB) {
      GLuint z = glGetUniformLocationARB(cur_shad, name);
      glUniform2fvARB(z, 1, p);
   }
}

void unisampler2(char *name, int unit, GLuint tex)
{
   if (glUniform1iARB) {
      GLuint z = glGetUniformLocationARB(cur_shad, name);
      glUniform1iARB(z, unit);
   }
   glActiveTextureARB(GL_TEXTURE0_ARB + unit);
   glBindTexture(GL_TEXTURE_2D, tex);
   glActiveTextureARB(GL_TEXTURE0_ARB);
}

void virtual_uniforms(svtexset *set, svtexspace *space, Bool compute_mip, int space_id)
{
   int i;
   svtexset_physsizerecip(set, phys_size_recip2);
   svtexspace_pagetablesize(space, page_table_size2);
   for (i=0; i < 2; ++i)
      virt_tex_size[i] = page_table_size2[i] * 60;

   uni2fv("phys_size_recip", phys_size_recip2);
   uni1f("page_size_log2", page_size_log2f);
   uni2fv("page_table_size", page_table_size2);
   uni2fv("virt_tex_size", virt_tex_size);

   glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 1, phys_size_recip2[0], phys_size_recip2[1],0,0);
   glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 2, page_size_log2f, 0,0,0);
   glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 3, page_table_size2[0], page_table_size2[1],0,0);
   glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 4, virt_tex_size[0], virt_tex_size[1], 0,0);
   glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 5, 60/64.0,60/64.0,0,0);
   glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 6, 2/64.0,2/64.0,0,0);
   glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 7, virt_tex_size[0]/1024,virt_tex_size[1]/1024,0,0);

   if (compute_mip) {
      uni1f("space_id", space_id);
      glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 6, space_id + 1.0/8,0,0,0);
   } else
      unisampler2("page_tex", 0, svtexspace_pagetex(space));
}

void draw_slide_raw(float s0, float t0, float s1, float t1, float x, float y, float z, float sx, float sy, float sz, float tx, float ty, float tz)
{
   glBegin(GL_POLYGON);
      glTexCoord2f(s0,t0); glVertex3f(x,y,z);
      glTexCoord2f(s1,t0); glVertex3f(x+sx,y+sy,z+sz);
      glTexCoord2f(s1,t1); glVertex3f(x+sx+tx,y+sy+ty,z+sz+tz);
      glTexCoord2f(s0,t1); glVertex3f(x+tx,y+ty,z+tz);
   glEnd();
}

#define SLIDE_W (512 * 60)
#define SLIDE_H (512 * 60)
#define SLIDE_WIDTH  1088
#define SLIDE_HEIGHT  816

vec3f slideshow = { 135,240,50};

// originally this function was going to draw one slide,
// but now it draws all the slides, so it's only actually
// meaingful with n=0
void draw_slide(int n)
{  Prof_Begin(draw_slide)
   int x,y;
   float s0,t0,s1,t1;
   y = n & 15;
   x = n >> 4;
   s0 =  x * (float) SLIDE_WIDTH / SLIDE_W;
   s1 = ((x+16) * (float) SLIDE_WIDTH) / SLIDE_W;
   t0 =  y * (float) SLIDE_WIDTH / SLIDE_W;
   t1 = ((y+16) * (float) SLIDE_WIDTH) / SLIDE_W;

   t1 = t1 * 3/4;

   draw_slide_raw(s0,t1,s1,t0, slideshow.x, slideshow.y, slideshow.z, 128,0,0, 0,0,96);
   Prof_End
}

float mip_trilerp_bias = 0, texture_mip_bias = 0;
Bool show_normals=False;
void draw_world_setup(Bool compute_mip)
{
   float z=0;
   // setup -- we only need to do once, global for scene
   if (compute_mip) {
      z = mip_sample_bias_base - (mipsample_shift * 1.1f);
      uni1f("mip_sample_bias", z);
      glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0, 0,0,z,texture_mip_bias);
      glBindTexture(GL_TEXTURE_2D, mipmap_tex);
   } else {
      uni1f("mip_debug_bias", mip_bias_debug);
      uni1f("mip_trilerp_bias", mip_trilerp_bias);
      glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0, mip_bias_debug, mip_trilerp_bias,mip_bias_debug + page_size_log2f - 0.5f,mip_bias_debug + mip_trilerp_bias);
      unisampler2("base_tex", 1, base_tex);
      if (!opt_shader)
         unisampler2("norm_tex", 2, norm_tex);
   }
}

int cur_slide = 0;
int overlay;
// if compute_mip is true, we're writing out page-IDs and mip levels for readback
void draw_world(Bool compute_mip)
{
   Prof_Begin(draw_world)
   draw_world_setup(compute_mip);

   // now draw everything in each virtual texture space


{Prof_Begin(draw_world_do)

   virtual_uniforms(vset, vspace, compute_mip, 0);
   if (rebuild)
      predraw_world();
   glCallList(world_dlist);

Prof_End}

   virtual_uniforms(vset, vspace2, compute_mip, 16);
   draw_sphere(200,120,60,40,   0,0  ,  1,0.5);
   draw_sphere(250,200,50,30,   0,0.5,  0.5,0.25);
   draw_sphere(700,450,80,90,   0,0.75, 0.5,0.25);

   glDisable(GL_LIGHTING);

   if (!compute_mip) {
      if (use_program) {
         glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, frag_prog_unlit);
         glEnable(GL_FRAGMENT_PROGRAM_ARB);
      } else {
         glUseProgramObjectARB(cur_shad = frag_shade_unlit);
         glProgramEnvParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, 0, mip_bias_debug, mip_trilerp_bias,mip_bias_debug + page_size_log2f - 0.5f,mip_bias_debug + mip_trilerp_bias);
         unisampler2("base_tex", 1, base_tex);
         if (!opt_shader)
            unisampler2("norm_tex", 2, norm_tex);
      }
   }

   virtual_uniforms(vset, slide_space, compute_mip, 32);

   if (overlay && slide_mode && !compute_mip) {
      int x = cur_slide >> 4;
      int y = cur_slide & 15;
      float s0,t0,s1,t1;
      s0 = x * (float) SLIDE_WIDTH / SLIDE_W;
      s1 = s0 + 1024.0 / SLIDE_W;
      t0 = y * (float) SLIDE_HEIGHT / SLIDE_H;
      t1 = t0 + 768.0 / SLIDE_H;

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      gluOrtho2D(0,1024,768,0);
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glDisable(GL_DEPTH_TEST);
      glDisable(GL_CULL_FACE);
      glDepthMask(GL_FALSE);

      draw_slide_raw(s0,t0,s1,t1,0,0,1,1024,0,0,0,768,0);
   } else
      draw_slide(0);
   Prof_End
}

void draw_normals(void)
{
   int x,y;
   glColor3f(1,0,0);
   glDisable(GL_LIGHTING);
   glBegin(GL_LINES);
   for (y=0; y <= 256; y += 4)
   for (x=0; x <= 256; x += 4) {
      vec3f p = { x*mscale, y*mscale, map[y][x] * height_scale };
      //vec3f_addeq_scale(&p, &mnorm[y][x], 0.25);
      glVertex3fv(&p.x);
      //vec3f_addeq_scale(&p, &ttan[y][x], mscale * 2);
      vec3f_addeq_scale(&p, &mnorm[y][x], mscale * 2);
      glVertex3fv(&p.x);
   }
   glEnd();
}

float skyvec[8][3] =
{
   { -1,-1,-1 },
   { -1,-1, 1 },
   { -1, 1,-1 },
   { -1, 1, 1 },
   {  1,-1,-1 },
   {  1,-1, 1 },
   {  1, 1,-1 },
   {  1, 1, 1 },
};

int skyv[6][4] =
{
   { 7,5,4,6 },
   { 5,1,0,4 },
   { 1,3,2,0 },
   { 3,7,6,2 },
   { 1,5,7,3 },
   { 0,2,6,4 },
};

void draw_sky_box(void)
{
   Prof_Begin(draw_skybox)
   int i;
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glTranslatef(camera_loc.x, camera_loc.y, camera_loc.z);
   glRotatef(-45, 0,0,1);
   for (i=0; i < 6; ++i) {
      glBindTexture(GL_TEXTURE_2D, sky[i]);
      glBegin(GL_POLYGON);
      glTexCoord2f(0,0); glVertex3fv(skyvec[skyv[i][0]]);
      glTexCoord2f(0,1); glVertex3fv(skyvec[skyv[i][3]]);
      glTexCoord2f(1,1); glVertex3fv(skyvec[skyv[i][2]]);
      glTexCoord2f(1,0); glVertex3fv(skyvec[skyv[i][1]]);
      glEnd();
   }
   glPopMatrix();
   Prof_End
}

extern double Prof_get_time(void);
double start_time;
void start_timer(void)
{
   start_time = Prof_get_time();
}

int show_info = 0, update=1, profile_update=1, show_profile = 0;
GLuint debug_tex;

void my_print(float sx, float sy, char *text)
{
   glDisable(GL_BLEND);
   glListBase(4096);
   glRasterPos2f(sx,sy-1);
   glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
   glListBase(0);
   glEnable(GL_BLEND);
}

float my_width(char *text)
{
   return strlen(text) * 10;
}

static int which = 0;
extern int page_dl, ptouch;
int update_sync=1, hud=0;
extern void draw_diagram(int n);
int special[500], diagram[500];
int show_time = TRUE;

int demoRunLoopmode_base(float simTime)
{
   Prof_Begin(demoRunLoopmode)
   Prof_update(profile_update);
   Prof_set_report_mode(Prof_CALL_GRAPH);
   if (simTime > 0.125) simTime = 0.125;

   if (simTime) camera_sim(simTime);

   if (show_info)
      mipsample_shift = show_info==1 ? 0 : 3;
   else
      mipsample_shift = 3;

   { Prof_Begin(draw_one)
   clear2();
   setup_view();

   if (glUseProgramObjectARB == NULL)
      use_program = TRUE;

   if (use_program) {
      glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, mip_prog);
      glEnable(GL_FRAGMENT_PROGRAM_ARB);
   } else {
      glUseProgramObjectARB(cur_shad = mip_shade);
   }
   glDisable(GL_LIGHTING);
   draw_world(TRUE);
   Prof_End }
   
   if (page_sample_buffer == NULL) 
      page_sample_buffer = malloc(4 * scr_w * scr_h);

   if (update || show_info) {
      Prof_Begin(glReadPixels)
      #ifdef READ_RGBA
      glReadPixels(0,0,scr_w >> mipsample_shift,scr_h >> mipsample_shift, GL_RGBA    , GL_UNSIGNED_BYTE, page_sample_buffer);
      #else
      glReadPixels(0,0,scr_w >> mipsample_shift,scr_h >> mipsample_shift, GL_ABGR_EXT, GL_UNSIGNED_BYTE, page_sample_buffer);
      #endif
      Prof_End
   }

   if (!show_info) {
      if (do_clear)
         clear();
      else
         start();

      glDepthMask(GL_FALSE);
      glDisable(GL_DEPTH_TEST);
      glDisable(GL_FRAGMENT_PROGRAM_ARB);
      if (glUseProgramObjectARB) glUseProgramObjectARB(0);
      glDisable(GL_LIGHTING);
      glActiveTextureARB(GL_TEXTURE0_ARB);
      glEnable(GL_TEXTURE_2D);
      glColor3f(1,1,1);
      draw_sky_box();
      glDepthMask(GL_TRUE);
      glEnable(GL_DEPTH_TEST);
   }

   stbgl_SimpleLight(0, 0.6, 0.557, -0.577, 0.577);
   {
      float color[4] = { 0.8,0.6,0.4,0 };
      glLightfv(GL_LIGHT0, GL_DIFFUSE, color);
   }
   stbgl_GlobalAmbient(0.25,0.40,0.55);
   glEnable(GL_LIGHTING);

   if (1 || !opt_shader) {
      if (update) {
         Prof_Begin(update_all)
         update_vtex();
         svtexspace_sync(vspace);
         svtexspace_sync(vspace2); 
         svtexspace_sync(slide_space);
         Prof_End
      }
   }

   if (use_program) {
      glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, frag_prog);
      glEnable(GL_FRAGMENT_PROGRAM_ARB);
   } else {
      glUseProgramObjectARB(cur_shad = frag_shade);
   }

   { Prof_Begin(draw_2)
   if (!show_info)
      draw_world(FALSE);
   Prof_End }

   { Prof_Begin(unlock_blocks)
        unlock_blocks();
     Prof_End
   }

   if (0 && opt_shader) {
      if (update) {
         Prof_Begin(update_all)
         update_vtex();
         svtexspace_sync(vspace);
         svtexspace_sync(vspace2); 
         Prof_End
      }
   }

   glDisable(GL_LIGHTING);

   if (glUseProgramObjectARB)
      glUseProgramObjectARB(0);
   glDisable(GL_FRAGMENT_PROGRAM_ARB);

   if (!show_info && show_normals)
      draw_normals();

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluOrtho2D(0,1024,768,0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_CULL_FACE);
   glDepthMask(GL_FALSE);

   glDisable(GL_TEXTURE_2D);
   glColor3f(1,1,1);

   if (slide_mode && diagram[cur_slide]) {
      draw_diagram(diagram[cur_slide]);
   }

   if (show_info) {
      int i, n = (scr_w * scr_h) >> (mipsample_shift*2);
      glRasterPos2f(0,768);
      for (i=0; i < n; ++i) {
         uint8 *p = page_sample_buffer + i*4;
         if (p[3] != 255) {
            p[2] = p[0] >> 1;
            p[1] = p[1] >> 1;
            p[0] = ((p[3] >> 4) + (p[3] << 4)) << 1;
         }
      }
      glDrawPixels(scr_w >> mipsample_shift,scr_h >> mipsample_shift, GL_RGBA, GL_UNSIGNED_BYTE, page_sample_buffer);
   }

   if (hud) {
      glColor3f(1,0.25,0.25);
      glprint(20, 30, "frame time(ms): %03d  |  ptouch: %d  | pageDLs: %d", (int) (simTime*1000), ptouch, page_dl);
      glprint(20, 60, "allocated: %d   updated: %d    desired: %d   mipforced: %d", requested_blocks, updated_blocks, visited_blocks, forced_blocks);
   }

   if (debug_tex) {
      float z = (which == 3 ? 0.25 : 1);
      int x = which == 1 ? 500 : 800;
      int y = which == 1 ? 500 : 400;
      glBindTexture(GL_TEXTURE_2D, debug_tex);
      glEnable(GL_TEXTURE_2D);
      glColor3f(1,1,1);
      glBegin(GL_POLYGON);
         glTexCoord2f(0,z/2); glVertex2i(0,768);
         glTexCoord2f(z,z/2); glVertex2i(x,768);
         glTexCoord2f(z,0); glVertex2i(x,768-y);
         glTexCoord2f(0,0); glVertex2i(0,768-y);
      glEnd();
      glDisable(GL_TEXTURE_2D);
   }

   if (show_profile) {
      glDisable(GL_TEXTURE_2D);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_BLEND);
      Prof_draw_gl(40,120,600,300,24,3,my_print,my_width);
      glDisable(GL_BLEND);
   }

   if (slide_mode && show_time) {
      float delta_time;
      glDisable(GL_TEXTURE_2D);
      glDisable(GL_BLEND);
      if (start_time == 0) {
         glColor3f(0,0,1);
         glBegin(GL_LINES);
         glVertex2i(0,767);
         glVertex2i(1023,767);
         glEnd();
      } else {
         float g,r,pos;
         #define MINUTES 55
         double end = Prof_get_time();
         delta_time = end - start_time;
         if (delta_time > MINUTES*60)
            glColor3f(0.25f,0,0);
         else {
            g = stb_linear_remap(delta_time, 0, MINUTES*60, 0.5,0);
            r = stb_linear_remap(delta_time, 0, MINUTES*60, 0,1.75);
            r /= 2;
            g /= 2;
            glColor3f(r,g,0);
         }
         pos = stb_linear_remap(delta_time, 0,MINUTES*60, 1024,0);
         glBegin(GL_LINES);
         glVertex2i(0,767);
         glVertex2i(pos,767);
         glEnd();
      }
   }


   active_blocks = 0;
   requested_blocks = 0;
   visited_blocks = 0;
   forced_blocks = 0;
   page_dl = 0;
   ptouch = 0;

   stbwingraph_SwapBuffers(0);
   Prof_End

   if (updated_blocks) {
      updated_blocks = 0;
      return FALSE;
   }
   updated_blocks = 0;

   if (slide_mode && !hud && !show_profile)
      return STBWINGRAPH_update_pause; // don't run animation loop

   if (vel.x || vel.y || vel.z || ang_vel.x || ang_vel.y || ang_vel.z)
      return FALSE;

   if (hud || show_profile)
      return FALSE;

   return STBWINGRAPH_update_pause;
}

int hack;
void next_slide(int step, int space)
{
   int sx,sy;
   cur_slide = stb_clamp(cur_slide + step, 0, 255);
   sy = cur_slide & 15;
   sx = cur_slide >> 4;

   if (cur_slide == 1 && step == 1 && start_time == 0)
      start_timer();

   camera_loc.x = slideshow.x + 128.0 * (1.0/16*sx + (1024.0 / (SLIDE_WIDTH*16))/2);
   camera_loc.y = slideshow.y - 3.75 + .0040f; // ad hoc, determined experimentally
   camera_loc.z = slideshow.z + 96.0 - 96.0 * (SLIDE_HEIGHT/(SLIDE_WIDTH*3.0/4)/16*sy + (768.0 / (SLIDE_WIDTH*16*3/4))/2);

   camera_ang.x = 0;
   camera_ang.y = 0;
   camera_ang.z = 0;
   
   max_updates = 1000;
}

int save_slides = 0;

int demoRunLoopmode(float simTime)
{
   if (save_slides) {
      char filename[512];
      char *buffer = malloc(scr_w * scr_h * 4);
      int j;
      int i;
      for (j=0; j < 256; ++j) {
         glReadPixels(0,0,scr_w,scr_h,GL_RGB,GL_UNSIGNED_BYTE,buffer);
         for (i=0; i*2 < scr_h; ++i)
            stb_swap(buffer+i*scr_w*3, buffer+(scr_h-i-1)*scr_w*3, scr_w*3);
         sprintf(filename, "slides/slide_%03d.bmp", cur_slide);
         stbi_write_bmp(filename, scr_w, scr_h, 3, buffer);
         next_slide(1,FALSE);
         demoRunLoopmode_base(0);
      }
      free(buffer);
      save_slides = FALSE;
   }
   return demoRunLoopmode_base(simTime);
}

int trilinear = 1;

void set_trilinear(int mode)
{
   trilinear = mode;
   glBindTexture(GL_TEXTURE_2D, base_tex);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, trilinear ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
   glBindTexture(GL_TEXTURE_2D, 0);
}

Bool show_mips, show_pages, show_trilerp, coerce_mip=True;
int demoProcessCharacter(int ch)
{
   // global functions
   switch(ch) {
      case '!': start_timer(); break;
      case 'T': set_trilinear(!trilinear); break;
      case '9': Prof_move_cursor(-1); break;
      case '0': Prof_move_cursor( 1); break;
      case '\r': Prof_select(); break;
      case '\\': Prof_select_parent(); break;
      case 'P': profile_update = !profile_update; break;
      case 'p': show_profile = !show_profile; break;
      case '(': Prof_move_frame(-1); break;
      case ')': Prof_move_frame( 1); break;
      case 'h': hud = !hud; break;
      case 'm': show_mips = !show_mips; svtexset_flush(vset); max_updates = -1; break;
      case 't': show_trilerp = !show_trilerp; svtexset_flush(vset); max_updates = -1; break;
      case 'y': show_pages = !show_pages; svtexset_flush(vset); max_updates = -1; break;
      case 'f': use_program = !use_program; break;
      case 'o': show_info = (show_info+1) % 3; break;
      case '>': ++hack; break;
      case '<': --hack; break;
      case 'B': show_time = !show_time; break;
      case 'b': coerce_mip = !coerce_mip; svtexset_flush(vset); max_updates = -1; break;
      #ifdef _DEBUG
      case '&': {
         save_slides = TRUE;
         break;
      }
      #endif
      case 'i': {
         switch (++which) {
            default: debug_tex = 0; which = 0; break;
            case 1: debug_tex = svtexspace_pagetex(vspace); break;
            case 2: debug_tex = base_tex; break;
            case 3: debug_tex = base_tex; break;
         }
         break;
      }
   }

   if (slide_mode) {
      switch(ch) {
         case ' ': next_slide(1, TRUE); break;
         case 27 : slide_mode = 0; set_trilinear(TRUE); break;
         case '/': overlay = !overlay; break;
         case 'S': next_slide(0, FALSE); break;
      }      
   } else {
      switch(ch) {
         case 27 : break;//return TRUE;

         #define ANG_STEP 8
         #define LOC_STEP 1

         case '=': case '+': mip_bias_debug += 0.5; break;
         case '-':           mip_bias_debug -= 0.5; break;

         case '.': mip_trilerp_bias += 1.0 / 8; break;
         case ',': mip_trilerp_bias -= 1.0 / 8; break;

         case '{': mip_sample_bias_base += 1.0 / 4; break;
         case '}': mip_sample_bias_base -= 1.0 / 4; break;

         case '[': height_scale /= 1.05; rebuild = True; break;
         case ']': height_scale *= 1.05; rebuild = True; break;


   //      case 't': test_vtex(); break;
         case 'C': do_clear = !do_clear; break;
         case 'u': update = !update; break;
         case 'U': update_sync = !update_sync; break;

         case 'a': ang_vel.z += ANG_STEP; return FALSE;
         case 'd': ang_vel.z -= ANG_STEP; return FALSE;
         case 'r': ang_vel.x += ANG_STEP; return FALSE;
         case 'v': ang_vel.x -= ANG_STEP; return FALSE;
         case 'E': ang_vel.y += ANG_STEP; return FALSE;
         case 'Q': ang_vel.y -= ANG_STEP; return FALSE;

         case 'w': vel.y += LOC_STEP; return FALSE;
         case 'x': vel.y -= LOC_STEP; return FALSE;
         case 'z': vel.x -= LOC_STEP; return FALSE;
         case 'c': vel.x += LOC_STEP; return FALSE;
         case 'q': vel.z += LOC_STEP; return FALSE;
         case 'e': vel.z -= LOC_STEP; return FALSE;

         case 'S': slide_mode = 1; set_trilinear(0); next_slide(0,FALSE); break;

         case '\n': camera_loc.x = camera_loc.y = 0; camera_loc.z = 8;
                   camera_ang.x = camera_ang.y = camera_ang.z = 0;
                   /* FALLTHROUGH */
         case ' ': vel.x = vel.y = vel.z = 0;
                   ang_vel.x = ang_vel.y = ang_vel.z = 0;
                   return FALSE;
      }
   }
   return 0;
}

void demoScreenInit(int x, int y)
{
   scr_w = x;
   scr_h = y;
}

void demoResizeViewport(int x, int y)
{
   scr_w = x;
   scr_h = y;
   free(page_sample_buffer);
   page_sample_buffer = NULL;
}

#define SZ (1024+256)

// trivial bump map texture
uint8 texbump[SZ][SZ];
pgen_bitmap big_splat[16], big_splat2[16];
pgen_bitmap borderv[16], borderh[16];

void main_tex(void)
{
   pgen_bitmap splat;
   int x,y,i;
   for (y=0; y < SZ; ++y)
   for (x=0; x < SZ; ++x) {
      int r2 = (x - 500) * (x - 500) + (y - 550) * (y-550);
      float r = sqrt(r2);
      float z = fabs(1 - (r/75 - floor(r/75))*2)*3;
      if (z > 1) z = 1;
      texbump[y][x] = z * 128 + 64;
   }

   splat = pgen_bitmap_alloc4(64,64);
   for (y=0; y < 64; ++y)
   for (x=0; x < 64; ++x) {
      float dx = fabs(x - 32)/32.0;
      float dy = fabs(y - 32)/32.0;
      float p = 0.75;
      float d = pow(pow(dx,p)+pow(dy,p),1/p);
      float w = stb_clamp(1-d,0,1);
      w = 3*w*w - 2*w*w*w;
      pgen_bitmap_set_pixel4(&splat,x,y, x*3+64,y*3+64,64,w*255);
   }
   pgen_bitmap_premultiply(&splat, &splat);

   big_splat[0] = pgen_bitmap_alloc4(250,200);
   big_splat2[0] = pgen_bitmap_alloc4(250,200);
   for (y=0; y < 200; ++y)
   for (x=0; x < 250; ++x) {
      int r,g,b;
      float dx = fabs(x - 125)/125.0;
      float dy = fabs(y - 100)/100.0;
      float p = 2;
      float d = pow(pow(dx,p)+pow(dy,p),1/p);
      float w = stb_clamp(2*(1-d),0,1);
      w = 3*w*w - 2*w*w*w;
      if ((x ^ y) & 16)
         r = 64, g = 127, b = 255;
      else
         r = 32, g = 32, b = 127;
      pgen_bitmap_set_pixel4(&big_splat[0],x,y, r,g,b,w*160);
      if ((x ^ y) & 16)
         r = 192, g = 32, b = 255;
      else
         r = 96, g = 32, b = 96;
      pgen_bitmap_set_pixel4(&big_splat2[0],x,y, r,g,b,w*160);
   }
   pgen_bitmap_premultiply(&big_splat[0], &big_splat[0]);
   pgen_bitmap_premultiply(&big_splat2[0], &big_splat2[0]);
   for (i=1; i < 12; ++i) {
      big_splat[i] = pgen_bitmap_mipmap_arbitrary(big_splat[i-1]);
      big_splat2[i] = pgen_bitmap_mipmap_arbitrary(big_splat2[i-1]);
   }
}

static void average(uint8 *p1, uint8 *p2)
{
   int i;
   for (i=0; i < 4; ++i)
      p1[i] = (p1[i] + p2[i]) >> 1;
}

uint8 *mipmaps_bump[12];

void make_mipmap(uint8 *out, uint8 *in, int w, int h)
{
   uint8 color1[4],color2[4];
   int i,j;
   h >>= 1;
   w >>= 1;
   for (j=0; j < h; ++j) {
      for (i=0; i < w; ++i) {
         memcpy(color1, in, 4);
         average(color1, in+4);
         memcpy(color2, in+w*8, 4);
         average(color2, in+w*8+4);
         average(color1, color2);
         memcpy(out, color1, 4);
         out += 4;
         in += 8;
      }
      in += w*8;
   }
}

void recolor(uint8 *data, int w, int h, int r, int g, int b)
{
   int i;
   if (r >= 128) ++r;
   if (g >= 128) ++g;
   if (b >= 128) ++b;
   for (i=0; i < w*h; ++i) {
      data[0] = ((data[0] * r) >> 8) + (256-r);
      data[1] = ((data[1] * g) >> 8) + (256-g);
      data[2] = ((data[2] * b) >> 8) + (256-b);
      data += 4;
   }
}

void main_mip(void)
{
   int n=0, w=SZ,h=SZ;
   mipmaps_bump[n] = texbump[0];
   while (w > 1 && h > 1) {
      uint8 *data = malloc(w*h);
      pgen_bitmap src,dest;
      src = pgen_bitmap_make1(w, h, mipmaps_bump[n], w);
      dest = pgen_bitmap_mipmap(src);
      mipmaps_bump[n+1] = dest.pixels;
      assert(data);
      ++n;
      w >>= 1;
      h >>= 1;
   }
}

char *load_file_with_includes(char *name)
{
   char *data = stb_file(name, NULL);
   char *p;
   if (!data) return NULL;
   p = strstr(data, "#include");
   while (p) {
      char inc_text[280];
      char inc_name[256];
      char *inc_data, *z, *q;
      q = p;
      p += 8;
      if (isspace(*p)) {
         int n=0;
         while (isspace(*p)) ++p;
         if (*p != '"') return data;
         ++p;
         strcpy(inc_name, "glsl/");
         n = 5;
         while (*p != '"') {
            if (!*p) return data;
            inc_name[n++] = *p++;
            if (n == sizeof(inc_name)) return data;
         }
         ++p;
         inc_name[n++] = 0;
         inc_data = stb_file(inc_name, NULL);
         if (inc_data == NULL) return data;
         memcpy(inc_text, q, p-q);
         inc_text[p-q] = 0;
         n  = p - data;
         z = stb_dupreplace(data, inc_text, inc_data);
         free(data);
         data = z;
         p = data + n;
      }
      p = strstr(p, "#include");
   }
   return data;
}

void download_block(uint tex, int level, int x, int y, uint8* data, uint w, uint h, uint stride)
{  Prof_Begin(download_block)
   glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
   glBindTexture(GL_TEXTURE_2D, tex);
   glTexSubImage2D(GL_TEXTURE_2D, level, x,y,w,h,GL_RGBA, GL_UNSIGNED_BYTE, data);
   glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
   Prof_End
}

static char dummy[16*1024*1024];

void download_block_recolor(uint tex, int level, int x, int y, uint8* data, uint w, uint h, uint stride, int n, int z, int svtex)
{
   unsigned int i;
   int color[][3] = {
       { 255,255,255 },
       { 127,255,255 },
       { 255,127,255 },
       { 255,255,127 },
       { 127,127,255 },
       { 127,255,127 },
       { 255,127,127 },
   };
   while (n > 6) n -= 6;
   for (i=0; i < h; ++i)
      memcpy(dummy + i*w*4, data + i*stride*4, w*4);
   if (svtex)
      recolor(dummy, w, h, 180,255,180);
   if (show_mips)
      recolor(dummy, w, h, color[n][0], color[n][1], color[n][2]);
   if (show_pages && z)
      recolor(dummy, w, h, 128,128,128);
   if (show_trilerp && w == 32)
      memset(dummy, 255, w*h*4);
   download_block(tex, level, x, y, dummy, w, h, 0);
}

typedef struct
{
   vec3f normal, tangent, bitangent;
} Frame;

//Frame tsamp[513][513]; // 2^22 * 12 = 2^20 * 48 = 48MB!

uint8 block[64][64][4];
uint8 bmip[32][32][4]; // mipmapped version of same block

pgen_bitmap mainbm_bump(int m)
{
   int w = SZ >> m, h = SZ >> m;
   if (w == 0) w=1;
   if (h == 0) h=1;
   return pgen_bitmap_make1(w, h, mipmaps_bump[m], w);
}

void write_fixed_pixel(uint8 *pixel, int x, int y)
{
   memcpy(block[y][x], pixel, 4);
}

void compute_fixed_pixel(uint8 *pixel, int x, int y)
{
   int i;
   uint8 mip[4];
   int x1,x3,y1,y3;
   x3 = x >> 1;
   x1 = x & 1 ? x3+1 : x3-1;
   y3 = y >> 1;
   y1 = y & 1 ? y3+1 : y3-1;
   for (i=0; i < 4; ++i)
      mip[i] = (bmip[y3][x3][i]*9 + bmip[y3][x1][i]*3 + bmip[y1][x3][i]*3 + bmip[y1][x1][i]) >> 4;
   for (i=0; i < 4; ++i) {
      uint8 c = block[y][x][i];
      int t = c >> 4;
      if (t < 4) t = 4;
      if (abs(c - mip[i]) > t) {
         // move towards the mip value
         pixel[i] = (mip[i]*11 + 5*c) >> 4;
      } else
         pixel[i] = c;
   }
}

// this function tweaks the fringes of each page to match the
// mipmap to reduce the cracking caused by bad mip calculation
void fix_cracks(void)
{  Prof_Begin(fix_cracks)
   uint8 buffer[128*8*4], *p;
   int i;

   p = buffer;
   for (i=1; i < 63; ++i) {
      compute_fixed_pixel(p, i, 1); p += 4;
      compute_fixed_pixel(p, i, 2); p += 4;
      compute_fixed_pixel(p, i,61); p += 4;
      compute_fixed_pixel(p, i,62); p += 4;
   }
   for (i=3; i < 61; ++i) {
      compute_fixed_pixel(p, 1, i); p += 4;
      compute_fixed_pixel(p, 2, i); p += 4;
      compute_fixed_pixel(p,61, i); p += 4;
      compute_fixed_pixel(p,62, i); p += 4;
   }
   p = buffer;
   for (i=1; i < 63; ++i) {
      write_fixed_pixel(p, i, 1); p += 4;
      write_fixed_pixel(p, i, 2); p += 4;
      write_fixed_pixel(p, i,61); p += 4;
      write_fixed_pixel(p, i,62); p += 4;
   }
   for (i=3; i < 61; ++i) {
      write_fixed_pixel(p, 1, i); p += 4;
      write_fixed_pixel(p, 2, i); p += 4;
      write_fixed_pixel(p,61, i); p += 4;
      write_fixed_pixel(p,62, i); p += 4;
   }
   Prof_End
}

uint8 *rmod[129];
int rmod_mask[129];
unsigned short shuffled[2048];
unsigned short rdata[2048];

#define BLOCK_EXTENT  1024
#define BLOCK_LOG2    10

void splat_loc(int x, int y, int *ox, int *oy)
{  Prof_Begin(splat_loc)
   uint32 z = (x << 16) + y;
   z = stb_hash_number(z);
   *ox = (z % 500);
   z = stb_rehash_improved(z);
   *oy = (z % 500);
   Prof_End
}

void main_loc(int x, int y, int *ox, int *oy)
{  Prof_Begin(main_loc)
   uint32 z = (x << 16) + y;
   z = stb_hash_number(z);
   *ox = (z & 15) & ~3;
   z = stb_rehash_improved(z);
   *oy = ((z >> 12) & 15) & ~3;
   Prof_End
}

int cid(int x, int y)
{
   int choice[32] = { 0,1,2,4,3,0,1,2,5,3,0,1,2,4,3,0,1,2,5,3,0,1,2,4,3,0,1,2,3,0,1,2 };
   uint32 z = (x << 8)  + y;
   z = stb_hash_number(z);
   z += x;
   return z % 14;//choice[(z >> 3) & 31];
}

pgen_bitmap_optimized **clusters;

#define CLUSTER_SPACING  240
#define CLUSTER_OVERHANG 400
void generate(int xo, int yo, int m)
{  Prof_Begin(generate)
   pgen_bitmap dest, destm;
   int x,y,xb,xe,yb,ye;
   { Prof_Begin(generate__divides)
   xb = ((xo<<m) - CLUSTER_OVERHANG) / CLUSTER_SPACING, xe = (((xo+72) << m) + (CLUSTER_SPACING)/2) / CLUSTER_SPACING;
   yb = ((yo<<m) - CLUSTER_OVERHANG) / CLUSTER_SPACING, ye = ((yo+72) << m) / CLUSTER_SPACING;
   Prof_End
   }

   { Prof_Begin(generate__init)
   dest  = pgen_bitmap_make4(64,64,block[0][0],0);
   destm = pgen_bitmap_make4(32,32,bmip[0][0],0);

   memset(block,0,sizeof(block));
   memset(bmip,0,sizeof(bmip));
   Prof_End }

   // generate all the needed blocks
   for (y=ye; y >= yb; --y) {
      int py = (y * CLUSTER_SPACING) >> m;
      for (x=xe; x >= xb; --x) {
         int px = (x * CLUSTER_SPACING) >> m;
         int px2, py2, kx,ky, id;
         int xm = x & 511;
         int ym = y & 511,r1,r2,r3;
         pgen_bitmap_optimized *stack;

         { Prof_Begin(generate_randomn)
         r1 = shuffled[xm + 13];
         r2 = shuffled[xm + 25];
         r3 = shuffled[xm + 48];
         r1 = rdata[r1 + ym + 11];
         r2 = rdata[r2 + ym + 23];
         r3 = rdata[r3 + ym + 4];

         #ifdef _DEBUG
         id = 0;
         #else
         id = rmod[14][r1 & rmod_mask[14]];
         #endif

         px2 = r2 & 12;
         py2 = (r2 >> 4) & 12;
         kx = r3 >> 8;
         ky = r3 & 255;
         Prof_End }

         { Prof_Begin(generate_final)
         stack = clusters[id];
         if (y & 1) px += (CLUSTER_SPACING >> 1) >> m;
         px2 = (px2 >> m) + px;
         py2 = (py2 >> m) + py;
         if (m < stb_arr_len(stack))
            pgen_bitmap_overlay_opt(&dest, (px2-xo), (py2-yo), &stack[m]);
         if (m+1 < stb_arr_len(stack))
            pgen_bitmap_overlay_opt(&destm,(px2-xo)>>1,(py2-yo)>>1,&stack[m+1]);
         if ((r2 & (7 << 10)) == 0) {
            kx = (kx >> m) + px;
            ky = (ky >> m) + py;
            if (r2 & (3 << 14)) {
               pgen_bitmap_overlay(&dest, (kx-xo), (ky-yo), &big_splat[m]);
               pgen_bitmap_overlay(&destm, (kx-xo)>>1, (ky-yo)>>1, &big_splat[m+1]);
            } else {
               pgen_bitmap_overlay(&dest, (kx-xo), (ky-yo), &big_splat2[m]);
               pgen_bitmap_overlay(&destm, (kx-xo)>>1, (ky-yo)>>1, &big_splat2[m+1]);
            }
         }
         Prof_End }
      }
   }
   if (coerce_mip)
      fix_cracks();
   Prof_End
}

#define BUMP_EXTENT   1024
#define BUMP_LOG2     10

uint8 bump[65][65];
uint8 bumpmip[33][33];

void generate_bump(int xo, int yo, int m)
{  Prof_Begin(generate_bump)
   pgen_bitmap dest, destm;
   pgen_bitmap src, srcm;
   int x,y;
   int xb = (xo<<m) >> BUMP_LOG2, xe = ((xo+65) << m) >> BUMP_LOG2;
   int yb = (yo<<m) >> BUMP_LOG2, ye = ((yo+65) << m) >> BUMP_LOG2;

   src  = mainbm_bump(m);
   srcm = mainbm_bump(m+1);

   dest  = pgen_bitmap_make1(65,65,bump[0],0);
   destm = pgen_bitmap_make1(33,33,bumpmip[0],0);

   // generate all the needed blocks
   for (y=yb; y <= ye; ++y) {
      int py = (y << BLOCK_LOG2) >> m;
      for (x=xb; x <= xe; ++x) {
         int px = (x << BLOCK_LOG2) >> m;
         pgen_bitmap_replace(&dest, (px-xo), (py-yo), &src);
         pgen_bitmap_replace(&destm,(px-xo)>>1,(py-yo)>>1,&srcm);
      }
   }

{
   int z[16] = { 31, 60, 90, 100, 110, 120, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 };
   int p = z[m];
   // finite difference
   for (y=0; y < 64; ++y)
   for (x=0; x < 64; ++x) {
      block[y][x][0] = ((bump[y][x+1] - bump[y][x])>>1) + 128;
      block[y][x][1] = ((bump[y+1][x] - bump[y][x])>>1) + 128;
      block[y][x][2] = 128 + p;
   }

   p = z[m+1];
   for (y=0; y < 32; ++y)
   for (x=0; x < 32; ++x) {
      bmip[y][x][0] = ((bumpmip[y][x+1] - bumpmip[y][x])>>1) + 128;
      bmip[y][x][1] = ((bumpmip[y+1][x] - bumpmip[y][x])>>1) + 128;
      bmip[y][x][2] = 128 + p;
   }
}

   //fix_cracks();
   Prof_End
}

pgen_bitmap slide_hack[10];
void memset32(void *dest, uint32 value, int bytes)
{
   uint32 *foo = dest;
   bytes >>= 2;
   while (bytes > 0) {
      *foo++ = value;
      --bytes;
   }
}

void draw_bitmap(pgen_bitmap *dest, pgen_bitmap *destm, int m, int x, int y, pgen_bitmap *p, int darken)
{
   if (darken) {
      if (m < stb_arr_len(p)) pgen_bitmap_overlay_dark(dest , x>>m, y>>m, &p[m]);
      if (m+1<stb_arr_len(p)) pgen_bitmap_overlay_dark(destm, x>>(m+1), y>>(m+1), &p[m+1]);
   } else {
      if (m < stb_arr_len(p)) pgen_bitmap_overlay(dest , x>>m, y>>m, &p[m]);
      if (m+1<stb_arr_len(p)) pgen_bitmap_overlay(destm, x>>(m+1), y>>(m+1), &p[m+1]);
   }
}

void draw_bitmap_const(pgen_bitmap *dest, pgen_bitmap *destm, int m, int x, int y, pgen_bitmap *p, uint8 *color)
{
   if (m < stb_arr_len(p)) pgen_bitmap_overlay_a_const(dest , x>>m, y>>m, color, &p[m]);
   if (m+1<stb_arr_len(p)) pgen_bitmap_overlay_a_const(destm, x>>(m+1), y>>(m+1), color, &p[m+1]);
}

extern pgen_bitmap *get_char(int slot, int ch);
void draw_text(pgen_bitmap *dest, pgen_bitmap *destm, int m, int x, int y, char *str, int font, uint8 *color)
{  Prof_Begin(draw_text)
   while (*str) {
      pgen_bitmap *p = get_char(font, *str);
      if (*str != ' ') {
         if (m < stb_arr_len(p)) pgen_bitmap_overlay_a_const(dest , x>>m, y>>m, color, &p[m]);
         if (m+1<stb_arr_len(p)) pgen_bitmap_overlay_a_const(destm, x>>(m+1), y>>(m+1), color, &p[m+1]);
      }
      x += p->w;
      ++str;
   }
   Prof_End
}


#ifdef ZP
pgen_bitmap zp;
pgen_bitmap wosat[11];
#endif
pgen_bitmap hborder[12], vborder[12];
pgen_bitmap *bullet;

int num_slides;
char **slides[500];

int string_height(int slot)
{
   pgen_bitmap *p = get_char(slot, 'g');
   return p->h;
}

int string_width(int slot, char *str)
{
   int w=0;
   while (*str) {
      pgen_bitmap *p = get_char(slot, *str++);
      w += p->w;
   }
   return w;
}

typedef struct
{
   int x,y,font;
   char *text;
   pgen_bitmap *image;
   uint8 color[4];
   int is_const;
} Blob;

Blob *slide_blobs[500];

pgen_bitmap *bm[4][5][8][8];

int page_size[4] = { 50,50,32,25 };

void add_bitmap_set(int slot, pgen_bitmap base)
{
   int mip=0;
   int size = page_size[slot];
   int i,j,m;
   pgen_bitmap_premultiply(&base, &base);
   for (mip=0; mip < 5; ++mip) {
      for (j=0; j < (base.h / size); ++j)
         for (i=0; i < (base.w / size); ++i) {
            pgen_bitmap *stack = NULL;
            stb_arr_push(stack, pgen_bitmap_subregion(&base, i*size,j*size,size,size));
            for (m=1; m < 10; ++m) {
               pgen_bitmap b = pgen_bitmap_mipmap(stack[m-1]);
               stb_arr_push(stack, b);
            }
            assert(slot < 4);
            assert(mip < 5);
            assert(j < 8);
            assert(i < 8);
            bm[slot][mip][j][i] = stack;
         }

      base = pgen_bitmap_mipmap(base);
      if (base.w < size || base.h < size)
         break;
   }
}

void add_text(int dest, int x, int y, char *str, int font, uint8 *color)
{
   Blob b;
   assert(str != NULL);
   b.x = x;
   b.y = y;
   b.text = str;
   b.font = font;
   b.image = 0;
   memcpy(b.color, color, 4);
   stb_arr_push(slide_blobs[dest], b);
}

void add_image_const(int dest, int x, int y, pgen_bitmap *image, uint8 *color)
{
   Blob b;
   b.x = x;
   b.y = y;
   b.image = image;
   assert(image != NULL);
   b.text = 0;
   memcpy(b.color, color, 4);
   b.is_const = 1;
   stb_arr_push(slide_blobs[dest], b);
}

void add_image(int dest, int x, int y, pgen_bitmap *image, int darken)
{
   Blob b;
   b.x = x;
   b.y = y;
   assert(image != NULL);
   b.image = image;
   b.text = 0;
   b.color[0] = darken;
   b.is_const = 0;
   stb_arr_push(slide_blobs[dest], b);
}

static char *get_string(char *str)
{
   char *s;
   str = strchr(str, '"');
   if (!str) return "[ERROR]";
   ++str;
   s = strrchr(str, '"');
   if (s) *s = 0;
   return str;
}

static int x_off, y_off, ival, jval, sbm;
static int p_font, box_width, page_x, page_step, page_x_off, page_y_off;

void parse_into(int dest, char *t)
{
   uint8 text_color[4] = { 0,0,0,255 };
   int p0,p1,p2,p3,p4;
   if (sscanf(t,"text %d,%d", &p0,&p1) == 2) {
      char *s = get_string(t);
      add_text(dest, p0+x_off, p1+y_off, strdup(s), p_font, text_color);
   } else if (sscanf(t,"textr %d,%d", &p0,&p1) == 2) {
      char *s = get_string(t);
      int n = string_width(p_font, s);
      add_text(dest, p0+x_off-n, p1+y_off, strdup(s), p_font, text_color);
   } else if (sscanf(t,"textc %d,%d", &p0,&p1) == 2) {
      char *s = get_string(t);
      int n = string_width(p_font, s);
      add_text(dest, p0+x_off-(n>>1), p1+y_off, strdup(s), p_font, text_color);
   }
   else if (sscanf(t, "grid %d,%d,%d,%d", &p0,&p1,&p2,&p3) == 4) {
      t = strchr(t, '|');
      if (t == 0) {
         add_text(dest, x_off, y_off, "[ERROR]", p_font, text_color);
      } else {
         int i,j;
         t = stb_skipwhite(t+1);
         for (j=0; j < p1; ++j) {
            jval = j;
            for (i=0; i < p0; ++i) {
               ival = i;
               parse_into(dest, t);
               x_off += p2;
            }
            x_off -= p2 * p0;
            y_off += p3;
         }
         y_off -= p3 * p1;
      }
   }
   else if (sscanf(t,"jval %d,%d", &p0,&p1)==2) {
      int n;
      char text[20];
      sprintf(text, "%d", jval);
      n = string_width(p_font, text);
      add_text(dest, p0+x_off-n, p1+y_off, strdup(text), p_font, text_color);
   } else if (sscanf(t,"ival %d,%d", &p0,&p1)==2) {
      int n;
      char text[20];
      sprintf(text, "%d", ival);
      n = string_width(p_font, text);
      add_text(dest, p0+x_off-n, p1+y_off, strdup(text), p_font, text_color);
   }
   else if (sscanf(t,"page %d,%d,%d,%d,%d", &p0,&p1,&p2,&p3,&p4)==5) { add_image(dest,p0+x_off,p1+y_off,bm[sbm][p2][p4][p3], 0); }
   else if (sscanf(t,"dark %d,%d,%d,%d,%d", &p0,&p1,&p2,&p3,&p4)==5) { add_image(dest,p0+x_off,p1+y_off,bm[sbm][p2][p4][p3], 1); }
   else if (sscanf(t,"page %d,%d,%d,%d,%d",         &p2,&p3,&p4)==3) { add_image(dest,x_off+page_x_off,y_off+page_y_off,bm[sbm][p2][p4][p3], 0); page_x_off += page_step; }
   else if (sscanf(t,"dark %d,%d,%d,%d,%d",         &p2,&p3,&p4)==3) { add_image(dest,x_off+page_x_off,y_off+page_y_off,bm[sbm][p2][p4][p3], 1); page_x_off += page_step; }
   else if (!strcmp(t,"endpage")) { page_y_off += page_step; page_x_off = 0; }
   else if (sscanf(t,"pagestep %d", &p0) == 1) { page_step = p0; }
   else if (sscanf(t,"bitmap %d", &p0) == 1) { sbm = p0; }
   else if (sscanf(t,"grabber %d,%d,%d", &p0,&p1,&p2)==3) {  }
   else if (sscanf(t,"label %d,%d,%d", &p0,&p1,&p2)==3) {  }
   else if (sscanf(t,"box %d,%d,%d,%d", &p0,&p1,&p2,&p3)==4) {  }
   else if (sscanf(t,"disk %d,%d,%d", &p0,&p1,&p2)==3) {  }
   else if (sscanf(t,"xbase %d", &p0) == 1) { x_off  = p0; page_x_off = 0; }
   else if (sscanf(t,"ybase %d", &p0) == 1) { y_off  = p0; page_y_off = 0; }
   else if (sscanf(t,"xoff %d" , &p0) == 1) { x_off += p0; page_x_off = 0; }
   else if (sscanf(t,"yoff %d" , &p0) == 1) { y_off += p0; page_y_off = 0; }
   else if (sscanf(t,"font %d" , &p0) == 1) { p_font = p0; }
   else if (sscanf(t,"width %d", &p0) == 1) { box_width = p0; }
   else if (sscanf(t,"line %d,%d,%d,%d" , &p0,&p1,&p2,&p3)==4) {  }
}

void parse_diagram_into_slide(int dest, int n)
{
   unsigned char color[4] = { 128,128,128,255};
   char filename[512];
   char **f;
   int i,len;
   
   // center along x, below title
   p_font = 6;
   box_width = 3;
   x_off = 512;
   y_off = 200;
   page_step = 0;
   sbm = 0;

   sprintf(filename, "data/diagram_%02d.txt", n);
   f = stb_stringfile(filename, &len);
   if (f == NULL) return;

   glColor3f(0,0,0);
   for (i=0; i < len; ++i) {
      if (f[i][0] != '#' && f[i][0]) {
         parse_into(dest, f[i]);
      }
   }
   #ifdef _DEBUG
   add_text(dest, 5,768-20,strdup(filename), 2,color);
   #endif
   free(f);
}

#define TITLE 8
#define NORMAL 7

#define INITIAL 2
pgen_bitmap *mipsub[100];

int layout_slide(int s, int stop, int dest)
{
   int code_not_done=0, code_no_hilight=0, base_x=0,base_y=0;
   int offset=0;
   int codesize=26;
   uint8 code_c[4] = { 30,45,120,0 };
   uint8 code_hilight[4] = { 192,0,64,0 };
   uint8 color[4] = { 0,0,0,0 };
   int a=0,b=0;
   int bullet_sizes[] = { NORMAL, 7, 7, 6, 6, 5, 5, 4,4,3,3,2,2,2,2 };
   int size = NORMAL;
   int n=0;
   int suppress=0;

   char *str;
   str = slides[s][n++];

   b += 80 - (68 >> INITIAL);
   while (*str == '=') ++str;
   while (*str == ' ') ++str;
   if (*str) {
      int n = string_width(TITLE, str);
      add_text(dest, a + 512 - (n >> 1), b, str, TITLE, color);
      b += string_height(TITLE) + 20;
      if (n < stb_arr_len(slides[s]) && slides[s][n][0] == 0)
         ++n;
   }

   while (n < stb_arr_len(slides[s])) {
      int pos, type = 0, code=0;
      str = slides[s][n++];
      if (*str == '#') continue;
      if (*str == '!') {
         int spec;
         if (sscanf(str, "!special %d", &spec) == 1) {
            if (stop == 1)
               special[dest] = spec;
         }
         else if (sscanf(str, "!diagram %d", &spec) == 1) {
            if (stop == 1) {
               diagram[dest] = spec;
               parse_diagram_into_slide(dest, spec);
            }
         }
         else if (sscanf(str, "!mipsub %d", &spec) == 1) {
            if (stop == 1) {
               pgen_bitmap *p = mipsub[spec];
               if (p) {
                  add_image(dest, (1024 - p->w) >> 1, 760 - p->h, p, 0);
               }
            }
         }
         else sscanf(str, "!base %d,%d", &base_x, &b);
         continue;
      }
      while (*str == ' ') ++str;
      if (*str == 0) { b += 30; continue; }
      if (*str == '*' || (*str == 'o' && (str[1] == 'o' || str[1] == ' '))) {
         int count = 0;
         type = *str;
         if (type == 'o') {
            --stop;
            if (stop < 0) return 0;
            suppress = (stop != 0);
         }
         while (*str == type) { ++count; ++str; }
         while (*str == ' ') ++str;
         size = bullet_sizes[count];
         pos = 80 + count*40;
      } else {
         size = NORMAL;
         if (*str == '^') {
            pos = 80;
            ++str;
         } else
            pos = 1024;
      }
      while (*str == '>') { offset += 20; ++str; }
      while (*str == '<') { offset -= 20; ++str; }
      if (*str == '+' || *str == '-') {
         while (*str == '+') { ++size; ++str; }
         while (*str == '-') { --size; ++str; }
         if (*str == '`') { size = (size-NORMAL) + 26; size = stb_clamp(size,19,26); codesize = size; }
      }
      if (*str == '`') {
         size = codesize;
         ++str;
         pos = 120;
         if (*str == '`') {
            --stop;
            ++str;
            if (*str == '`') {
               code_no_hilight = 1;
               ++str;
               ++stop;
            } else {
               code_no_hilight = 0;
               code_not_done = (stop != 0);
            }
         }
         code = 1;
      }
      if (code)
         size = stb_clamp(size,20,26);
      else
         size = stb_clamp(size, 2, 8);
      if (pos == 1024) {
         int n = string_width(size, str);
         pos = 512 - (n >> 1);
      }

      if (!suppress || type != '*') {
         uint8 *c = color;
         int z = string_height(size), z2;
         pos += offset + base_x;
         z2 = z >> INITIAL;

         if (code)
            if (stop == 0 && !code_no_hilight)
               c = code_hilight;
            else
               c = code_c;

         b += z2;
         add_text(dest, a+pos, b, str, size, c);
         if (type) {
            add_image_const(dest, a+pos-40,b+(z>>1)-7,bullet, color);
         }
         z -= z2;
         if      (size <  4) b += z+1;
         else if (size <  5) b += z + 6;
         else if (size < 16) b += z + 10;

         else if (size < 20) b += z - 5;
         else if (size < 22) b += z + 1;
         else if (size < 23) b += z + 2;
         else if (size < 24) b += z + 3;
         else if (size < 25) b += z + 5;
         else if (size < 26) b += z + 6;
         else                b += z + 8;
      }
   }
   return !code_not_done;
}

void load_slides(void)
{
   int skip=0;
   int i,n,s=0,dest;
   char **tex = stb_stringfile("data/talk.txt", &n);

   for (i=0; i < 500; ++i) {
      slides[i] = NULL;
      slide_blobs[i] = NULL;
      special[i] = 0;
      diagram[i] = 0;
   }
   for (i=0; i < n; ++i) {
      if (tex[i][0] == '=') {
         if (tex[i][1] == '-') {
            skip = 1;
         } else {
            skip = 0;
            if (s != 0 || stb_arr_len(slides[0]))
               ++s;
         }
      }
      if (!skip)
         stb_arr_push(slides[s], tex[i]);  
   }
   num_slides = s+1;

   dest=0;
   for (i=0; i < num_slides; ++i) {
      int s = 1;
      while (!layout_slide(i, s, dest++)) {
         ++s;
      }
   }
   num_slides = dest;
   if (vset) svtexset_flush(vset);
   max_updates = -1;
}

int demoProcessKeydown(int ch, int shift, int ctrl, int alt)
{
   switch (ch) {
      case 'R':
         if (ctrl) load_slides();
         break;
      case 39:
         if (shift)
            next_slide(5,FALSE);
         else
            next_slide(1,FALSE);
         break;
      case 37:
         if (shift)
            next_slide(-5,FALSE);
         else
            next_slide(-1,FALSE);
         break;

   }
   return 0;
}

void draw_slide_tex(pgen_bitmap *dest, pgen_bitmap *destm, int m, int s, int x, int y)
{  Prof_Begin(draw_slide_tex)
   int i;
   for (i=0; i < stb_arr_len(slide_blobs[s]); ++i) {
      Blob *b = &slide_blobs[s][i];
      if (b->image) {
         if (b->is_const)
            draw_bitmap_const(dest, destm, m, x+b->x, y+b->y, b->image, b->color);
         else
            draw_bitmap(dest, destm, m, x+b->x, y+b->y, b->image, b->color[0]);
      } else {
         draw_text(dest, destm, m, x+b->x, y+b->y, b->text, b->font, b->color);
      }
   }
   Prof_End
}

typedef struct
{
   int x,y,c;
} Decal;

#ifdef ZP
Decal *zp_decals;
#endif

void generate_slide(int xo, int yo, int m)
{  Prof_Begin(generate_slide)
   pgen_bitmap dest, destm;
   int x,y,xb,xe,yb,ye,xf,yf;

   xf = (xo << m);
   yf = (yo << m);

   { Prof_Begin(generate_slide_divides)
   xb = (xf - 4) / SLIDE_WIDTH; xe = (xf + (64 << m) + SLIDE_WIDTH-1+4)/SLIDE_WIDTH;
   yb = (yf - 4) / SLIDE_HEIGHT; ye = (yf + (64 << m) + SLIDE_HEIGHT-1+4)/SLIDE_HEIGHT;
   Prof_End
   }

   { Prof_Begin(generate_slide_init)

   dest  = pgen_bitmap_make4(64,64,block[0][0],0);
   destm = pgen_bitmap_make4(32,32,bmip[0][0],0);

   memset32(block,0xfff8f0,sizeof(block));
   memset32(bmip ,0xfff8f0,sizeof(bmip));
   Prof_End }

#ifdef ZP
   for (i=0; i < stb_arr_len(zp_decals); ++i) {
      uint8 color[4];
      int a,b,c;
      Decal *d = &zp_decals[i];
      a = (d->x - xf) >> m;
      b = (d->y - yf) >> m;
      switch (m) {
         case 0: c = 235; break;
         case 1: c = (235*15 + 1*d->c) >> 4; break;
         case 2: c = (235*14 + 2*d->c) >> 4; break;
         case 3: c = (235*12 + 4*d->c) >> 4; break;
         case 4: c = (235*8  + 8*d->c) >> 4; break;
         default: c = d->c; break;
      }
      color[0] = (0xf0 * c) >> 8;
      color[1] = 0xf8;//(0xf8 * c) >> 8;
      color[2] = (0xff * c) >> 8;
      pgen_bitmap_overlay_a_const(&dest, a, b, color, &wosat[m]);
      switch (m+1) {
         case 0: c = 235; break;
         case 1: c = (235*15 + 1*d->c) >> 4; break;
         case 2: c = (235*14 + 2*d->c) >> 4; break;
         case 3: c = (235*12 + 4*d->c) >> 4; break;
         case 4: c = (235*8  + 8*d->c) >> 4; break;
         default: c = d->c; break;
      }
      color[0] = (0xf0 * c) >> 8;
      color[1] = 0xf8;//(0xf8 * c) >> 8;
      color[2] = (0xff * c) >> 8;
      pgen_bitmap_overlay_a_const(&destm, a>>1, b>>1, color, &wosat[m+1]);
   }
#endif

   // generate all the needed blocks
   for (y=ye; y >= yb; --y) {
      int py = (y * SLIDE_HEIGHT);
      for (x=xe; x >= xb; --x) {
         int m2 = m+1;
         int px = (x * SLIDE_WIDTH);

         // draw slide borders
         int a,b;
         a = px - xf - 4;
         b = py - yf - 4;
         pgen_bitmap_overlay(&dest , a>>m , b>>m , &hborder[m  ]);
         pgen_bitmap_overlay(&destm, a>>m2, b>>m2, &hborder[m+1]);
         b += 8;
         pgen_bitmap_overlay(&dest , a>>m , b>>m , &vborder[m  ]);
         pgen_bitmap_overlay(&destm, a>>m2, b>>m2, &vborder[m+1]);
         b += 760;
         pgen_bitmap_overlay(&dest , a>>m , b>>m , &hborder[m  ]);
         pgen_bitmap_overlay(&destm, a>>m2, b>>m2, &hborder[m+1]);
         a = px - xf + 1020;
         b = py - yf + 4;
         pgen_bitmap_overlay(&dest , a>>m , b>>m , &vborder[m  ]);
         pgen_bitmap_overlay(&destm, a>>m2, b>>m2, &vborder[m+1]);

         if (x >= 0 && x < 16 && y >= 0 && y < 16) {
            int s = x*16 + y;
            a = px - xf;
            b = py - yf;
            draw_slide_tex(&dest, &destm, m, s, a,b);
         }
      }
   }
   Prof_End
}


#define BLOCK_SIZE 60
#define BLOCK_OFFSET 2

pgen_bitmap earth[10];
pgen_bitmap mars[10];
pgen_bitmap jupiter[10];


void download_bitmap(GLuint tex, int level, int tx, int ty, pgen_bitmap *bm, int bx, int by, int w, int h)
{  Prof_Begin(download_bitmap)
   if (bx < 0 || by < 0 || bx+w >= bm->w || by+h >= bm->h) {
      pgen_bitmap temp = pgen_bitmap_make4(64,64,block[0][0],0);
      int i,j;
      assert(w <= 64 && h <= 64);
      for (j = -1; j <= 1; ++j)
      for (i = -1; i <= 1; ++i) {
         pgen_bitmap_replace(&temp, -bx + i*bm->w, -by + j*bm->h, bm);
      }
      download_block(tex, level, tx, ty, block[0][0], w, h, 64);
   } else {
      uint8 *start = bm->pixels + bx*4 + by*bm->stride_in_bytes;
      download_block(tex, level, tx, ty, start, w, h, bm->stride_in_bytes>>2);
   }
   Prof_End
}

void do_main_block(int a, int b, int tex, int x, int y, int m, GLuint base, GLuint norm)
{  Prof_Begin(do_main_block)
   int off = (1024 >> m) - BLOCK_OFFSET;
   int xp = x*BLOCK_SIZE + off;
   int yp = y*BLOCK_SIZE + off;

   if (tex == 0) {
      generate(xp,yp,m);
      download_block_recolor(base, 0, a*64,b*64, block[0][0], 64,64,64,m,(x^y)&1, 0);
      download_block_recolor(base, 1, a*32,b*32, bmip[0][0], 32,32,32,m,(x^y)&1, 0);
   } else if (tex >= 16 && tex <= 31) {
      int xp1 = x*BLOCK_SIZE - BLOCK_OFFSET;
      int yp1 = y*BLOCK_SIZE - BLOCK_OFFSET;
      int xp2 = xp1 >> 1;
      int yp2 = yp1 >> 1;
      pgen_bitmap *planet;
      if (tex == 16) planet = earth;
      else if (tex == 17) planet = mars;
      else planet = jupiter;
      if (planet[m  ].pixels) download_bitmap(base, 0, a*64,b*64, &planet[m], xp1, yp1, 64,64);
      if (planet[m+1].pixels) download_bitmap(base, 1, a*32,b*32, &planet[m+1], xp2, yp2, 32,32);
   } else if (tex == 32) {
      int xp1 = x*BLOCK_SIZE - BLOCK_OFFSET;
      int yp1 = y*BLOCK_SIZE - BLOCK_OFFSET;
      generate_slide(xp1,yp1,m);
      download_block_recolor(base, 0, a*64,b*64, block[0][0], 64,64,64,m,(x^y)&1, 0);
      download_block_recolor(base, 1, a*32,b*32, bmip[0][0], 32,32,32,m,(x^y)&1, 0);
      //pgen_bitmap *planet = slide_hack;
      //if (planet[m  ].pixels) download_bitmap(base, 0, a*64,b*64, &planet[m], xp1, yp1, 64,64);
      //if (planet[m+1].pixels) download_bitmap(base, 1, a*32,b*32, &planet[m+1], xp2, yp2, 32,32);
   }

   if (norm) {
      if (tex == 0) {
         generate_bump(xp,yp,m);
      } else {
         int i;
         memset(block, 128, sizeof(block));
         memset(bmip, 128, sizeof(bmip));
         for (i=0; i < 64*64; ++i)
            block[0][i][2] = 255;
         for (i=0; i < 32*32; ++i)
            bmip[0][i][2] = 255;
      }
      download_block(norm, 0, a*64,b*64, block[0][0], 64,64,64);
      download_block(norm, 1, a*32,b*32, bmip[0][0], 32,32,32);
   }
   Prof_End
}

Bool main_block(int tex, int x, int y, int m, int t)
{  Prof_Begin(main_block)
   svtex *svtex = texlist[tex];
   int a,b;
   if (svtex_alloc_locked_page(svtex, x,y,m, &a,&b, t)) {
      do_main_block(a,b, tex,x,y,m, base_tex, norm_tex);
      ++updated_blocks;
      Prof_End
      return True;
   }
   Prof_End
   return False;
}

void fail(GLuint obj)
{
   GLint length;
   char *error;
   glGetObjectParameterivARB(obj, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
   error = malloc(length+1);
   glGetInfoLogARB(obj, length, NULL, error);
   fatal(error);
}

GLhandleARB compile_fragment_shader(char *shader)
{
   int success;
   GLhandleARB prog;
   GLhandleARB fshad;
   if (glCreateProgramObjectARB == NULL) return 0;
   prog = glCreateProgramObjectARB();
   fshad = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

   glShaderSourceARB(fshad, 1, &shader, NULL);

   glCompileShaderARB(fshad);

   glGetObjectParameterivARB(fshad, GL_OBJECT_COMPILE_STATUS_ARB, &success);
   if (!success) fail(fshad);

   glAttachObjectARB(prog, fshad);
   glDeleteObjectARB(fshad);
   glLinkProgramARB(prog);

   glGetObjectParameterivARB(prog, GL_OBJECT_LINK_STATUS_ARB, &success);
   if (!success) fail(prog);

   return prog;
}

GLuint compile_fragment_program(char *program)
{
   int epos, is_native;
   GLuint p;
   const char *s;
   glGenProgramsARB(1, &p);
   glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, p);
   glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(program), program);
   s = glGetString(GL_PROGRAM_ERROR_STRING_ARB);
   glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &epos);
   if (s) { OutputDebugString(s); }
   if (epos != -1) { assert(0); return 0; } // leaks p
   glGetProgramivARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &is_native);
   if (!is_native) { return 0; } // stb_fatal("Fragment program cannot run on this hardware");
   return p;
}

pgen_bitmap build_masked(pgen_bitmap src, pgen_bitmap mask, int sz)
{
   pgen_bitmap b;
   int i,j;
   // find the extent of non-zeroness in the mask
   int min_x=mask.w, min_y = mask.h, max_x = 0, max_y = 0;
   for (j=0; j < mask.h; ++j)
      for (i=0; i < mask.w; ++i)
         if (mask.pixels[j*mask.stride_in_bytes+i]) {
            min_x = min(i,min_x);
            max_x = max(i,max_x);
            min_y = min(j,min_y);
            max_y = max(j,max_y);
         }

   b = pgen_bitmap_alloc4(max_x - min_x + 1, max_y - min_y + 1);
   src  = pgen_bitmap_subregion(&src, min_x, min_y, b.w, b.h);
   mask = pgen_bitmap_subregion(&mask,min_x, min_y, b.w, b.h);
   for (j=0; j < sz; ++j)
   for (i=0; i < sz; ++i)
      mask.pixels[j*mask.stride_in_bytes + i*mask.channels] = 255;
   pgen_bitmap_replace(&b, 0,0, &src);
   pgen_bitmap_premultiply(&b, &mask);
   return b;
}

void force_minimum(pgen_bitmap data, int size, int m)
{
   int i;
   int s2 = (size >> m) << m;
   // if we're a matching power, we're fine
   if (s2 == size) return;

   s2 = (size >> m);
   // force it so we're opaque out to the difference
   assert(data.w > s2 && data.h >= s2);
   for (i=0; i <= s2; ++i) {
      uint8 *p = &data.pixels[s2*4 + i*data.stride_in_bytes];
      if (p[3] != 255) {
         p[0] = (255 * p[0] / p[3]);
         p[1] = (255 * p[1] / p[3]);
         p[2] = (255 * p[2] / p[3]);
         p[3] = 255;
      }
      p = &data.pixels[s2*data.stride_in_bytes + i*4];
      if (p[3] != 255) {
         p[0] = (255 * p[0] / p[3]);
         p[1] = (255 * p[1] / p[3]);
         p[2] = (255 * p[2] / p[3]);
         p[3] = 255;
      }
   }
}

void add_cluster(pgen_bitmap data, char *file, int sz)
{
   int i;
   pgen_bitmap_optimized *stack = NULL;
   pgen_bitmap mask = pgen_bitmap_load(file,1);
   pgen_bitmap z = build_masked(data, mask, sz);
   pgen_bitmap last = z;

   stb_arr_push(stack, pgen_bitmap_optimize(&last));
   for (i=0; i < 10; ++i) {
      pgen_bitmap p;
      p = pgen_bitmap_mipmap(last);
      if (last.w == 1 && last.h == 1)
         memcpy(p.pixels, last.pixels, 4);
      force_minimum(p, sz, i+1);
      if (p.pixels[3] != 255) {
         p.pixels[0] = (255 * p.pixels[0] / p.pixels[3]);
         p.pixels[1] = (255 * p.pixels[1] / p.pixels[3]);
         p.pixels[2] = (255 * p.pixels[2] / p.pixels[3]);
         p.pixels[3] = 255;
      }
      last = p;
      stb_arr_push(stack, pgen_bitmap_optimize(&last));
   }
   stb_arr_push(clusters, stack);
   free(mask.pixels);
}

void mwrite(char *filename, pgen_bitmap z)
{
   stbi_write_tga(filename, z.w, z.h, z.channels, z.pixels);
}

void load_clusters(void)
{
   #if 0
   pgen_bitmap data = pgen_bitmap_load("tex/t_09_pebbles.jpg", 4);
   add_cluster(data, "tex/t_09_m01.png", 256);
   add_cluster(data, "tex/t_09_m02.png", 256);
   add_cluster(data, "tex/t_09_m03.png", 256);
   add_cluster(data, "tex/t_09_m04.png", 256);
   add_cluster(data, "tex/t_09_m05.png", 256);
   add_cluster(data, "tex/t_09_m06.png", 256);
   add_cluster(data, "tex/t_09_m07.png", 256);
   add_cluster(data, "tex/t_09_m08.png", 256);
   add_cluster(data, "tex/t_09_m09.png", 256);
   add_cluster(data, "tex/t_09_m10.png", 256);
   add_cluster(data, "tex/t_09_m11.png", 256);
   add_cluster(data, "tex/t_09_m12.png", 256);
   add_cluster(data, "tex/t_09_m13.png", 256);
   #else
   pgen_bitmap data = pgen_bitmap_load("tex/t_03_g_dg.jpg", 4);
   add_cluster(data, "tex/t_03_m01.png", 256);
   #ifndef _DEBUG
   add_cluster(data, "tex/t_03_m02.png", 256);
   add_cluster(data, "tex/t_03_m03.png", 256);
   add_cluster(data, "tex/t_03_m04.png", 256);
   add_cluster(data, "tex/t_03_m05.png", 256);
   add_cluster(data, "tex/t_03_m06.png", 256);
   add_cluster(data, "tex/t_03_m07.png", 256);
   add_cluster(data, "tex/t_03_m08.png", 256);
   add_cluster(data, "tex/t_03_m09.png", 256);
   add_cluster(data, "tex/t_03_m10.png", 256);
   add_cluster(data, "tex/t_03_m11.png", 256);
   add_cluster(data, "tex/t_03_m12.png", 256);
   add_cluster(data, "tex/t_03_m13.png", 256);
   add_cluster(data, "tex/t_03_m14.png", 256);
   #endif
   #endif
   free(data.pixels);
}

GLuint create_phys_tex(int w, int h, GLuint type)
{
   GLuint tex, e;
   glGetError();
//   tex = svt_tex_alloc(w, h, GL_RGBA);
   tex = stbgl_TexImage2D(0, w,h,NULL,"NONE t");
   e = glGetError();
   if (e != GL_NO_ERROR) {
      const char *s = gluErrorString(e);
      stb_fatal("Whoops: %s", s);
   }
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
   glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, w>>1, h>>1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
   return tex;
}

GLuint create_mipmap_tex(int w, int h)
{
   int level = 0, bias = 0;
   char *data;
   GLuint tex;
   glGenTextures(1, &tex);
   glBindTexture(GL_TEXTURE_2D, tex);
   data = malloc(w * h);
   for(;;) {
      memset(data, stb_clamp(bias,0,255), w*h);
      glTexImage2D(GL_TEXTURE_2D, level, GL_ALPHA8, w, h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, data);
      ++level;
      bias += 16;
      if (w == 1 && h == 1) break;
      if (w>1) w >>= 1;
      if (h>1) h >>= 1;
   }
   free(data);
   svt_tex_filter(tex, ZXTEX_mipmap_linear, ZXTEX_nearest, ZXTEX_nearest);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
   return tex;
}

void load_splat(pgen_bitmap *bm, char *filename, int mip)
{
   int i;
   bm[0] = pgen_bitmap_load(filename, 4);
   pgen_bitmap_premultiply(&bm[0], &bm[0]);
   for (i=1; i < mip; ++i)
      bm[i] = pgen_bitmap_mipmap(bm[i-1]);
}

void load_splat_arb(pgen_bitmap *bm, char *filename, int mip)
{
   int i;
   bm[0] = pgen_bitmap_load(filename, 4);
   pgen_bitmap_premultiply(&bm[0], &bm[0]);
   for (i=1; i < mip; ++i)
      bm[i] = pgen_bitmap_mipmap_arbitrary(bm[i-1]);
}

void load_planet(int tex, pgen_bitmap *bm, char *filename, int mip)
{
   bm[mip] = pgen_bitmap_load(filename, 4);
   if (bm[mip].pixels) {
      int i;
      for (i=mip+1; i < 10; ++i) {
         if (bm[i].pixels) break;
         bm[i] = pgen_bitmap_mipmap(bm[i-1]);
      }
      stb_barrier();
      //tex_min_mip[tex] = mip;
   }
}

void* bg_data_load(void *p)
{
   load_planet(16, earth,   "tex/earth.jpg", 0);
   load_planet(17, mars,    "tex/mars.jpg", 0);
   load_planet(18, jupiter, "tex/jupiter.jpg", 0);

   return p;
}

extern void  build_map_full(void);


extern void init_fonts(void);
void demoInit(void)
{
   char *shader;
   pgen_bitmap b;
   int size_x, size_y;
   int psize,x,y,z,i;
   int page_size_log2 = 6;
   page_size_log2f = page_size_log2;

   init_fonts();
   stb_arr_setlen(bullet, 12);
   bullet[0] = pgen_bitmap_load("tex/bullet.png", 1);
   for (i=1; i < 12; ++i)
      bullet[i] = pgen_bitmap_mipmap_arbitrary(bullet[i-1]);

   for (z=0; z < 100; ++z) {
      char filename[100];
      pgen_bitmap b;
      sprintf(filename, "data/mipmap_substitution_%02d.png", z);
      b = pgen_bitmap_load(filename, 4);
      if (b.pixels) {
         pgen_bitmap_premultiply(&b,&b);
         stb_arr_push(mipsub[z], b);
         for (i=1; i < 12; ++i)
            stb_arr_push(mipsub[z], pgen_bitmap_mipmap(mipsub[z][i-1]));
      }
   }

   block_map = brm_create();

   //shader = (char *) glGetString(GL_EXTENSIONS);
   //stb_filewrite("extensions.txt", shader, strlen(shader));

   b = pgen_bitmap_load("data/smiley_face.jpg", 4);
   add_bitmap_set(0, b);
   b = pgen_bitmap_load("data/smiley_face_visible.jpg", 4);
   add_bitmap_set(1, b);
   b = pgen_bitmap_load("data/smiley_face_small.jpg", 4);
   add_bitmap_set(2, b);
   b = pgen_bitmap_load("data/smiley_face_mip.jpg", 4);
   add_bitmap_set(3, b);
   load_slides();

   glextdefInit();
   //glCreateProgramObjectARB = NULL;

   if (glCreateProgramObjectARB) {
      shader = load_file_with_includes("glsl/fragshad.txt");
      frag_shade = compile_fragment_shader(shader);
      free(shader);

      shader = load_file_with_includes("glsl/frag_unlit.txt");
      frag_shade_unlit = compile_fragment_shader(shader);
      free(shader);

      shader = load_file_with_includes("glsl/mip_shader.txt");
      mip_shade = compile_fragment_shader(shader);
      free(shader);

      // these are pretty much functional equivalents of the above
      shader = stb_file("frag_prog/fragprog.txt", NULL);
      frag_prog = compile_fragment_program(shader);
      free(shader);

      shader = stb_file("frag_prog/fragprog_unlit.txt", NULL);
      frag_prog_unlit = compile_fragment_program(shader);
      free(shader);

      shader = stb_file("frag_prog/mip_prog.txt", NULL);
      mip_prog = compile_fragment_program(shader);
      free(shader);
   } else {
      opt_shader = 1;
      use_program = 1;

      // if there's no glCreateProgramObjectARB, then use
      // a simpler shader as well
      shader = stb_file("frag_prog/fragprog_opt.txt", NULL);
      frag_prog = compile_fragment_program(shader);
      frag_prog_unlit = frag_prog;
      free(shader);

      shader = stb_file("frag_prog/mip_prog_opt.txt", NULL);
      mip_prog = compile_fragment_program(shader);
      free(shader);
   }

   load_planet(16, earth, "tex/earth_mip3.jpg", 3);
   load_planet(17, mars , "tex/mars_mip3.jpg", 3);
   load_planet(18, jupiter, "tex/jupiter_mip3.jpg", 3);


   for (i=0; i < 6; ++i) {
      char filename[512];
      sprintf(filename, "tex/skyrender%04d.jpg",i+1);
      sky[i] = stbgl_LoadTexture(filename, "c");
   }

   for (i=1; i < 128; ++i) {
      int n,j;
      if (stb_is_pow2(i)) {
         n = i;
      } else {
         n = 1 << (stb_log2_floor(i*4));
      }
      rmod[i] = malloc(n);
      rmod_mask[i] = n-1;
      for (j=0; j < n; ++j)
         rmod[i][j] = (unsigned char) (j % i);
   }

   for (i=0; i < 1024; ++i)
      shuffled[i] = i;
   stb_shuffle(shuffled, 1024, 2, 0);
   for (i=1024; i < 2048; ++i)
      shuffled[i] = shuffled[i-1024];
   for (i=0; i < 1024; ++i)
      rdata[i] = stb_rand();
   for (i=1024; i < 2048; ++i)
      rdata[i] = rdata[i-1024];

   mipmap_tex = create_mipmap_tex(1 << 10, 1 << 10);

   if (opt_shader) {
      size_x = 2048;
      size_y = 2048;
   } else {
      size_x = 4096;
      size_y = 2048;
   }

   build_map_full();
   
   vset   = svtexset_new(size_x >> page_size_log2, size_y >> page_size_log2);
            
   if (opt_shader)
      psize = 1024;
   else
      psize = 2048;
   vspace = svtexspace_new(vset, psize,psize);
   vtex   = svtex_new(vspace, 0,0,psize,psize);
   texlist[0] = vtex;

   load_clusters();

   main_tex();
   main_mip();
   tex_min_mip[0] = 0;

   base_tex = create_phys_tex(size_x, size_y, GL_RGBA);
   if (!opt_shader)
      norm_tex = create_phys_tex(size_x, size_y, GL_RGBA);

   // build all top-level blocks
   z = svtexspace_num_levels(vspace);

   for (y=0; y < psize >> (z-1); ++y)
   for (x=0; x < psize >> (z-1); ++x)
      main_block(0,x,y,z-1,z-2);
   for (y=0; y < psize >> (z-2); ++y)
   for (x=0; x < psize >> (z-2); ++x)
      main_block(0,x,y,z-2,0);

   svtexspace_sync(vspace);

   tex_mip_limit[0] = z-2;
   tex_pagerange[0].x0 = 0;
   tex_pagerange[0].y0 = 0;
   tex_pagerange[0].x1 = psize;
   tex_pagerange[0].y1 = psize;

   vspace2 = svtexspace_new_limit(vset, 128,128, 32);
   vtex2 = svtex_new(vspace2, 0,0,128,64);
   texlist[16] = vtex2;

   z = svtexspace_num_levels(vspace2);
   tex_mip_limit[16] = z-1;
   tex_pagerange[16].x0 = 0;
   tex_pagerange[16].y0 = 0;
   tex_pagerange[16].x1 = 128;
   tex_pagerange[16].y1 = 64;

   for (y=0; y < 64 >> (z-1); ++y)
   for (x=0; x < 128 >> (z-1); ++x)
      main_block(16,x,y,z-1,0);

   texlist[17] = svtex_new(vspace2, 0,64,64,32);
   tex_mip_limit[17] = z-1;
   tex_pagerange[17].x0 = 0;
   tex_pagerange[17].y0 = 64;
   tex_pagerange[17].x1 = 64;
   tex_pagerange[17].y1 = 96;

   for (y=0; y < 32 >> (z-1); ++y)
   for (x=0; x < 64 >> (z-1); ++x)
      main_block(17,x,y,z-1,0);

   texlist[18] = svtex_new(vspace2,  0,96,64,32);
   tex_mip_limit[18] = z-1;
   tex_pagerange[18].x0 = 0;
   tex_pagerange[18].y0 = 96;
   tex_pagerange[18].x1 = 64;
   tex_pagerange[18].y1 = 128;

   for (y=0; y < 32 >> (z-1); ++y)
   for (x=0; x < 64 >> (z-1); ++x)
      main_block(18,x,y,z-1,0);

   svtexspace_sync(vspace2);

   load_planet(32, slide_hack, "sample_slide_1.png", 0);

#ifdef ZP
   zp = pgen_bitmap_load("tex/zp.png", 1);
   {
      Decal z;
      z.x = 720;
      z.y = 140;
      z.c = 128;
      stb_arr_push(zp_decals, z);
   }
   for (y=0; y < zp.h; ++y)
   for (x=0; x < zp.w; ++x)
      if (zp.pixels[y*zp.stride_in_bytes+x] < 255) {
         Decal z;
         z.x = x * 170 + (stb_rand() % 50) + 1300;
         z.y = y * 170 + (stb_rand() % 50) + 700;
         z.c = (int) (255 - (255-zp.pixels[y*zp.stride_in_bytes+x]) * 0.85f);
         stb_arr_push(zp_decals, z);
      }
#endif

#ifdef ZP
   wosat[0] = pgen_bitmap_load("tex/wosat.png", 1);
   wosat[0] = pgen_bitmap_mipmap(wosat[0]); // leak
   for (i=1; i < 11; ++i)
      wosat[i] = pgen_bitmap_mipmap(wosat[i-1]);
#endif

   load_splat(vborder, "tex/border_vert.png" , 12);
   load_splat(hborder, "tex/border_horiz.png", 12);

   slide_space = svtexspace_new(vset, 512, 512);
   texlist[32] = svtex_new(slide_space, 0,0,512,512);
   z = svtexspace_num_levels(slide_space);

   tex_mip_limit[32] = z-1;
   tex_pagerange[32].x0 = 0;
   tex_pagerange[32].y0 = 0;
   tex_pagerange[32].x1 = 512;
   tex_pagerange[32].y1 = 512;

   for (y=0; y < 512 >> (z-1); ++y)
   for (x=0; x < 512 >> (z-1); ++x)
      main_block(32,x,y,z-1,0);
   svtexspace_sync(slide_space);


//   stb_work(bg_data_load, NULL, NULL);
#ifndef _DEBUG
   bg_data_load(NULL);
#endif
}

int winproc(void *data, stbwingraph_event *e)
{
   static int left,right;
   switch (e->type) {
      case STBWGE_create:
         demoInit();
         slide_mode = 1;
         set_trilinear(0);
         next_slide(0,FALSE);
         break;

      case STBWGE_char:
         if (demoProcessCharacter(e->key))
            return STBWINGRAPH_winproc_exit;
         break;

      case STBWGE_keydown:
         demoProcessKeydown(e->key, e->shift, e->ctrl, e->alt);
         break;

      case STBWGE_size:
         demoResizeViewport(e->width, e->height);
         break;

      case STBWGE_draw:
         demoRunLoopmode(0);
         break;

      case STBWGE_leftdown: left = 1; break;
      case STBWGE_leftup  : left = 0; break;
      case STBWGE_rightdown : right = 1; break;
      case STBWGE_rightup   : right = 0; break;

      case STBWGE_mousemove:
         if (left || right) {
            vec3f move = {0}, rmove;
            if (slide_mode) {
               slide_mode = 0;
               set_trilinear(TRUE);
            }
            if (left && right) {
               move.x = e->dx / 10.0;
               move.z = -e->dy / 10.0;
            } else if (left) {
               camera_ang.z += -e->dx / 6.0;
               camera_ang.x += -e->dy / 6.0;
            } else {
               move.z += -e->dy / 20.0;
               move.y = -e->dx / 20.0;
            }            
            computeRelativeTranslation(&rmove, &move, &camera_ang);
            vec3f_addeq(&camera_loc, &rmove);
            return STBWINGRAPH_winproc_update;
         }
         break;

      default:
         return STBWINGRAPH_unprocessed;
   }
   return 0;
}

void stbwingraph_main(void)
{
   #ifdef Prof_ENABLED
   stb_force_uniprocessor();
   #endif
   stbwingraph_CreateWindow(1, winproc, NULL, "SVTdemo", 1024, 768, 0, 1, 8, 0);
   stbwingraph_MakeFonts(NULL, 4096);
   stbwingraph_MainLoop(demoRunLoopmode, 0.01555f);
}

