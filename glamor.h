/*
 * Copyright © 2008 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#ifndef GLAMOR_H
#define GLAMOR_H

#include <scrnintstr.h>
#ifdef GLAMOR_FOR_XORG
#include <xf86str.h>
#endif
#include <pixmapstr.h>
#include <windowstr.h>
#include <gcstruct.h>
#include <picturestr.h>
#include <fb.h>
#include <fbpict.h>

/*
 * glamor_pixmap_type : glamor pixmap's type.
 * @MEMORY: pixmap is in memory.
 * @TEXTURE_DRM: pixmap is in a texture created from a DRM buffer.
 * @SEPARATE_TEXTURE: The texture is created from a DRM buffer, but
 * 		      the format is incompatible, so this type of pixmap
 * 		      will never fallback to DDX layer.
 * @DRM_ONLY: pixmap is in a external DRM buffer.
 * @TEXTURE_ONLY: pixmap is in an internal texture.
 */
typedef enum  glamor_pixmap_type {
	GLAMOR_MEMORY,
	GLAMOR_TEXTURE_DRM,
	GLAMOR_SEPARATE_TEXTURE,
	GLAMOR_DRM_ONLY,
	GLAMOR_TEXTURE_ONLY
} glamor_pixmap_type_t;

#define GLAMOR_EGL_EXTERNAL_BUFFER 3
#define GLAMOR_INVERTED_Y_AXIS  	1
#define GLAMOR_USE_SCREEN		(1 << 1)
#define GLAMOR_USE_PICTURE_SCREEN 	(1 << 2)
#define GLAMOR_USE_EGL_SCREEN		(1 << 3)
#define GLAMOR_VALID_FLAGS      (GLAMOR_INVERTED_Y_AXIS  		\
				 | GLAMOR_USE_SCREEN 			\
                                 | GLAMOR_USE_PICTURE_SCREEN		\
				 | GLAMOR_USE_EGL_SCREEN)

/* @glamor_init: Initialize glamor internal data structure.
 *
 * @screen: Current screen pointer.
 * @flags:  Please refer the flags description above.
 *
 * 	@GLAMOR_INVERTED_Y_AXIS:
 * 	set 1 means the GL env's origin (0,0) is at top-left.
 * 	EGL/DRM platform is an example need to set this bit.
 * 	glx platform's origin is at bottom-left thus need to
 * 	clear this bit.
 *
 * 	@GLAMOR_USE_SCREEN:
 *	If running in an pre-existing X environment, and the
 * 	gl context is GLX, then you should set this bit and
 * 	let the glamor to handle all the screen related
 * 	functions such as GC ops and CreatePixmap/DestroyPixmap.
 *
 * 	@GLAMOR_USE_PICTURE_SCREEN:
 * 	If don't use any other underlying DDX driver to handle
 * 	the picture related rendering functions, please set this
 * 	bit on. Otherwise, clear this bit. And then it is the DDX
 * 	driver's responsibility to determine how/when to jump to
 * 	glamor's picture compositing path.
 *
 * 	@GLAMOR_USE_EGL_SCREEN:
 * 	If you are using EGL layer, then please set this bit
 * 	on, otherwise, clear it.
 *
 * This function initializes necessary internal data structure
 * for glamor. And before calling into this function, the OpenGL
 * environment should be ready. Should be called before any real
 * glamor rendering or texture allocation functions. And should
 * be called after the DDX's screen initialization or at the last
 * step of the DDX's screen initialization.
 */
extern _X_EXPORT Bool glamor_init(ScreenPtr screen, unsigned int flags);
extern _X_EXPORT void glamor_fini(ScreenPtr screen);

/* This function is used to free the glamor private screen's
 * resources. If the DDX driver is not set GLAMOR_USE_SCREEN,
 * then, DDX need to call this function at proper stage, if
 * it is the xorg DDX driver,then it should be called at free
 * screen stage not the close screen stage. The reason is after
 * call to this function, the xorg DDX may need to destroy the
 * screen pixmap which must be a glamor pixmap and requires
 * the internal data structure still exist at that time.
 * Otherwise, the glamor internal structure will not be freed.*/
extern _X_EXPORT Bool glamor_close_screen(int idx, ScreenPtr screen);


/* Let glamor to know the screen's fbo. The low level
 * driver should already assign a tex
 * to this pixmap through the set_pixmap_texture. */
extern _X_EXPORT void glamor_set_screen_pixmap(PixmapPtr screen_pixmap);

/* @glamor_glyphs_init: Initialize glyphs internal data structures.
 *
 * @pScreen: Current screen pointer.
 *
 * This function must be called after the glamor_init and the texture
 * can be allocated. An example is to call it when create the screen
 * resources at DDX layer.
 */
extern _X_EXPORT Bool glamor_glyphs_init(ScreenPtr pScreen);

extern _X_EXPORT void glamor_set_pixmap_texture(PixmapPtr pixmap,
						unsigned int tex);

extern _X_EXPORT void glamor_set_pixmap_type(PixmapPtr pixmap, glamor_pixmap_type_t type);
extern _X_EXPORT void glamor_destroy_textured_pixmap(PixmapPtr pixmap);
extern _X_EXPORT void glamor_block_handler(ScreenPtr screen);
extern _X_EXPORT PixmapPtr glamor_create_pixmap(ScreenPtr screen, int w, int h, int depth,
						unsigned int usage);

extern _X_EXPORT void glamor_egl_screen_init(ScreenPtr screen);

extern _X_EXPORT void glamor_egl_make_current(ScreenPtr screen);
extern _X_EXPORT void glamor_egl_restore_context(ScreenPtr screen);

#ifdef GLAMOR_FOR_XORG

#define GLAMOR_EGL_MODULE_NAME  "glamoregl"

/* @glamor_egl_init: Initialize EGL environment.
 *
 * @scrn: Current screen info pointer.
 * @fd:   Current drm fd.
 *
 * This function creates and intialize EGL contexts.
 * Should be called from DDX's preInit function.
 * Return TRUE if success, otherwise return FALSE.
 * */
extern _X_EXPORT Bool glamor_egl_init(ScrnInfoPtr scrn, int fd);

/* @glamor_egl_init_textured_pixmap: Initialization for textured pixmap allocation.
 *
 * @screen: Current screen pointer.
 *
 * This function must be called before any textured pixmap's creation including
 * the screen pixmap. Could be called from DDX's screenInit function after the calling
 * to glamor_init..
 */
extern _X_EXPORT Bool glamor_egl_init_textured_pixmap(ScreenPtr screen);

/* @glamor_egl_create_textured_screen: Create textured screen pixmap.
 *
 * @screen: screen pointer to be processed.
 * @handle: screen pixmap's BO handle.
 * @stride: screen pixmap's stride in bytes.
 *
 * This function is similar with the create_textured_pixmap. As the
 * screen pixmap is a special, we handle it separately in this function.
 */
extern _X_EXPORT Bool glamor_egl_create_textured_screen(ScreenPtr screen,
							int handle,
							int stride);
/*
 * @glamor_egl_create_textured_pixmap: Try to create a textured pixmap from
 * 				       a BO handle.
 *
 * @pixmap: The pixmap need to be processed.
 * @handle: The BO's handle attached to this pixmap at DDX layer.
 * @stride: Stride in bytes for this pixmap.
 *
 * This function try to create a texture from the handle and attach
 * the texture to the pixmap , thus glamor can render to this pixmap
 * as well. Return true if successful, otherwise return FALSE.
 */
extern _X_EXPORT Bool glamor_egl_create_textured_pixmap(PixmapPtr pixmap,
							int handle,
							int stride);

extern _X_EXPORT void glamor_egl_destroy_textured_pixmap(PixmapPtr pixmap);
#endif

extern _X_EXPORT int glamor_create_gc(GCPtr gc);

extern _X_EXPORT void glamor_validate_gc(GCPtr gc, unsigned long changes, DrawablePtr drawable);
/* Glamor rendering/drawing functions with XXX_nf.
 * nf means no fallback within glamor internal if possible. If glamor
 * fail to accelerate the operation, glamor will return a false, and the
 * caller need to implement fallback method. Return a true means the
 * rendering request get done successfully. */
extern _X_EXPORT Bool glamor_fill_spans_nf(DrawablePtr drawable,
					   GCPtr gc,
					   int n, DDXPointPtr points,
					   int *widths, int sorted);

extern _X_EXPORT Bool glamor_poly_fill_rect_nf(DrawablePtr drawable,
					       GCPtr gc,
					       int nrect,
					       xRectangle * prect);

extern _X_EXPORT Bool glamor_put_image_nf(DrawablePtr drawable,
					  GCPtr gc, int depth, int x, int y,
					  int w, int h, int left_pad,
					  int image_format, char *bits);

extern _X_EXPORT Bool glamor_copy_n_to_n_nf(DrawablePtr src,
					    DrawablePtr dst,
					    GCPtr gc,
					    BoxPtr box,
					    int nbox,
					    int dx,
					    int dy,
					    Bool reverse,
					    Bool upsidedown, Pixel bitplane,
					    void *closure);

extern _X_EXPORT Bool glamor_composite_nf(CARD8 op,
					  PicturePtr source,
					  PicturePtr mask,
					  PicturePtr dest,
					  INT16 x_source,
					  INT16 y_source,
					  INT16 x_mask,
					  INT16 y_mask,
					  INT16 x_dest, INT16 y_dest,
					  CARD16 width, CARD16 height);

extern _X_EXPORT Bool glamor_trapezoids_nf(CARD8 op,
					   PicturePtr src, PicturePtr dst,
					   PictFormatPtr mask_format,
					   INT16 x_src, INT16 y_src,
					   int ntrap, xTrapezoid * traps);

extern _X_EXPORT Bool glamor_glyphs_nf(CARD8 op,
				       PicturePtr src,
				       PicturePtr dst,
				       PictFormatPtr mask_format,
				       INT16 x_src,
				       INT16 y_src, int nlist,
				       GlyphListPtr list, GlyphPtr * glyphs);

extern _X_EXPORT Bool glamor_triangles_nf(CARD8 op,
					  PicturePtr pSrc,
					  PicturePtr pDst,
					  PictFormatPtr maskFormat,
					  INT16 xSrc, INT16 ySrc,
					  int ntris, xTriangle * tris);


extern _X_EXPORT void glamor_glyph_unrealize(ScreenPtr screen, GlyphPtr glyph);

extern _X_EXPORT Bool glamor_set_spans_nf(DrawablePtr drawable, GCPtr gc, char *src,
					  DDXPointPtr points, int *widths, int n, int sorted);

extern _X_EXPORT Bool glamor_get_spans_nf(DrawablePtr drawable, int wmax,
					  DDXPointPtr points, int *widths, int count, char *dst);

extern _X_EXPORT Bool glamor_composite_rects_nf (CARD8         op,
						 PicturePtr    pDst,
						 xRenderColor  *color,
						 int           nRect,
						 xRectangle    *rects);

extern _X_EXPORT Bool glamor_get_image_nf(DrawablePtr pDrawable, int x, int y, int w, int h,
					  unsigned int format, unsigned long planeMask, char *d);

extern _X_EXPORT Bool glamor_add_traps_nf(PicturePtr pPicture,
					  INT16 x_off,
					  INT16 y_off, int ntrap, xTrap * traps);

extern _X_EXPORT Bool glamor_copy_plane_nf(DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
					   int srcx, int srcy, int w, int h, int dstx, int dsty,
					   unsigned long bitPlane, RegionPtr *pRegion);

extern _X_EXPORT Bool glamor_image_glyph_blt_nf(DrawablePtr pDrawable, GCPtr pGC,
						int x, int y, unsigned int nglyph,
						CharInfoPtr * ppci, pointer pglyphBase);

extern _X_EXPORT Bool glamor_poly_glyph_blt_nf(DrawablePtr pDrawable, GCPtr pGC,
					       int x, int y, unsigned int nglyph,
					       CharInfoPtr * ppci, pointer pglyphBase);

extern _X_EXPORT Bool glamor_push_pixels_nf(GCPtr pGC, PixmapPtr pBitmap,
					    DrawablePtr pDrawable, int w, int h, int x, int y);

extern _X_EXPORT Bool glamor_poly_point_nf(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt,
					   DDXPointPtr ppt);

extern _X_EXPORT Bool glamor_poly_segment_nf(DrawablePtr pDrawable, GCPtr pGC, int nseg,
					     xSegment *pSeg);

extern _X_EXPORT Bool glamor_poly_line_nf(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt,
					  DDXPointPtr ppt);

extern _X_EXPORT Bool glamor_poly_lines_nf(DrawablePtr drawable, GCPtr gc, int mode, int n,
					   DDXPointPtr points);

#endif /* GLAMOR_H */
