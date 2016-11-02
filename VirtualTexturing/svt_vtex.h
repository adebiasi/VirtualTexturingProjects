#ifndef INC_ZX_VTEX_H
#define INC_ZX_VTEX_H

// MAX_VTEX_MIP_LEVELS forces three things:
//   first, we have to have the entire top MIP level loaded, so larger is better
//   second, it quantizes how we can partition a virtual space, since each partition
//       should get its own top-level mipmap, so smaller is better
//   third, it determines the size of the mipmap-computing texture, so smaller is better
#define MAX_VTEX_MIP_LEVEL  10
//   1. with a 512K x 512K virtual texture, 10 forces 512 x 512 of stuff to load
//   2. with a 64x64 page, the min quantization is 64K x 64K
//   3. mipmap texture is 1024x1024 = 1MB ... we'll use pixel shader instead


typedef struct svtexset_usage_st svtexset_usage;
typedef struct svtexspace_st     svtexspace;
typedef struct svtex_st          svtex;
typedef struct svtexset_st       svtexset;

extern svtexset * svtexset_new(int w, int h);
extern void svtexset_flush(svtexset *set);
extern int svtexset_count(svtexset *set);

extern svtexspace *svtexspace_new(svtexset *set, int w, int h);
extern svtexspace *svtexspace_new_limit(svtexset *set, int w, int h, int limit);
extern void svtexspace_sync(svtexspace *set);
extern uint svtexspace_pagetex(svtexspace *space);
extern int svtexspace_num_levels(svtexspace *space);
extern float svtexspace_bias(svtexspace *space);

extern svtex *svtex_new(svtexspace *sp, int x, int y, int w, int h);
extern Bool svtex_alloc_locked_page(svtex *tex, int x, int y, int m, int *out_x, int *out_y, int t);
extern void svtex_unlock_page(svtex *tex, int x, int y, int m);
extern uint8 *svtex_page_data(svtex *tex, int x, int y, int m);
extern void zvtex_propagate_parent(svtex *tex, int x, int y, int m);

// returns False if page has been uncached
extern Bool svtex_lock_page(svtex *tex, int x, int y, int m);

extern void svtexspace_pagetablesize(svtexspace *space, float *out2);
extern void svtexset_physsizerecip(svtexset *set, float *out2);

#endif
