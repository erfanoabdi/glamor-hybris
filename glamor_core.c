/*
 * Copyright © 2001 Keith Packard
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

/** @file glamor_core.c
 *
 * This file covers core X rendering in glamor.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdlib.h>

#include "glamor_priv.h"

const Bool
glamor_get_drawable_location(const DrawablePtr drawable)
{
  PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
  glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
  glamor_screen_private *glamor_priv =
    glamor_get_screen_private(drawable->pScreen);
  if (pixmap_priv == NULL || pixmap_priv->gl_fbo == 0)
    return 'm';
  if (pixmap_priv->fb == glamor_priv->screen_fbo)
    return 's';
  else
    return 'f';
}


void
glamor_get_transform_uniform_locations(GLint prog,
				       glamor_transform_uniforms *uniform_locations)
{
  uniform_locations->x_bias = glGetUniformLocationARB(prog, "x_bias");
  uniform_locations->x_scale = glGetUniformLocationARB(prog, "x_scale");
  uniform_locations->y_bias = glGetUniformLocationARB(prog, "y_bias");
  uniform_locations->y_scale = glGetUniformLocationARB(prog, "y_scale");
}

/* We don't use a full matrix for our transformations because it's
 * wasteful when all we want is to rescale to NDC and possibly do a flip
 * if it's the front buffer.
 */
void
glamor_set_transform_for_pixmap(PixmapPtr pixmap,
				glamor_transform_uniforms *uniform_locations)
{
  glUniform1fARB(uniform_locations->x_bias, -pixmap->drawable.width / 2.0f);
  glUniform1fARB(uniform_locations->x_scale, 2.0f / pixmap->drawable.width);
  glUniform1fARB(uniform_locations->y_bias, -pixmap->drawable.height / 2.0f);
  glUniform1fARB(uniform_locations->y_scale, -2.0f / pixmap->drawable.height);
}

GLint
glamor_compile_glsl_prog(GLenum type, const char *source)
{
  GLint ok;
  GLint prog;

  prog = glCreateShaderObjectARB(type);
  glShaderSourceARB(prog, 1, (const GLchar **)&source, NULL);
  glCompileShaderARB(prog);
  glGetObjectParameterivARB(prog, GL_OBJECT_COMPILE_STATUS_ARB, &ok);
  if (!ok) {
    GLchar *info;
    GLint size;

    glGetObjectParameterivARB(prog, GL_OBJECT_INFO_LOG_LENGTH_ARB, &size);
    info = malloc(size);

    glGetInfoLogARB(prog, size, NULL, info);
    ErrorF("Failed to compile %s: %s\n",
	   type == GL_FRAGMENT_SHADER ? "FS" : "VS",
	   info);
    ErrorF("Program source:\n%s", source);
    FatalError("GLSL compile failure\n");
  }

  return prog;
}

void
glamor_link_glsl_prog(GLint prog)
{
  GLint ok;

  glLinkProgram(prog);
  glGetObjectParameterivARB(prog, GL_OBJECT_LINK_STATUS_ARB, &ok);
  if (!ok) {
    GLchar *info;
    GLint size;

    glGetObjectParameterivARB(prog, GL_OBJECT_INFO_LOG_LENGTH_ARB, &size);
    info = malloc(size);

    glGetInfoLogARB(prog, size, NULL, info);
    ErrorF("Failed to link: %s\n",
	   info);
    FatalError("GLSL link failure\n");
  }
}


Bool
glamor_prepare_access(DrawablePtr drawable, glamor_access_t access)
{
  PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);

  return glamor_download_pixmap_to_cpu(pixmap, access);
}

void
glamor_init_finish_access_shaders(ScreenPtr screen)
{
  glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
  const char *vs_source =
    "void main()\n"
    "{\n"
    "	gl_Position = gl_Vertex;\n"
    "	gl_TexCoord[0] = gl_MultiTexCoord0;\n"
    "}\n";
  const char *fs_source =
    "varying vec2 texcoords;\n"
    "uniform sampler2D sampler;\n"
    "void main()\n"
    "{\n"
    "	gl_FragColor = texture2D(sampler, gl_TexCoord[0].xy);\n"
    "}\n";
    
  const char *aswizzle_source =
    "varying vec2 texcoords;\n"
    "uniform sampler2D sampler;\n"
    "void main()\n"
    "{\n"
    " gl_FragColor = vec4(texture2D(sampler, gl_TexCoord[0].xy).rgb, 1);\n"
    "}\n";

  GLint fs_prog, vs_prog, avs_prog, aswizzle_prog;

  glamor_priv->finish_access_prog[0] = glCreateProgramObjectARB();
  glamor_priv->finish_access_prog[1] = glCreateProgramObjectARB();

  if (GLEW_ARB_fragment_shader) {
    vs_prog = glamor_compile_glsl_prog(GL_VERTEX_SHADER_ARB, vs_source);
    fs_prog = glamor_compile_glsl_prog(GL_FRAGMENT_SHADER_ARB, fs_source);
    glAttachObjectARB(glamor_priv->finish_access_prog[0], vs_prog);
    glAttachObjectARB(glamor_priv->finish_access_prog[0], fs_prog);

    avs_prog = glamor_compile_glsl_prog(GL_VERTEX_SHADER_ARB, vs_source);
    aswizzle_prog = glamor_compile_glsl_prog(GL_FRAGMENT_SHADER_ARB, aswizzle_source);
    glAttachObjectARB(glamor_priv->finish_access_prog[1], avs_prog);
    glAttachObjectARB(glamor_priv->finish_access_prog[1], aswizzle_prog);
  } else {
    vs_prog = glamor_compile_glsl_prog(GL_VERTEX_SHADER_ARB, vs_source);
    glAttachObjectARB(glamor_priv->finish_access_prog[0], vs_prog);
    ErrorF("Lack of framgment shader support.\n");
  }

  glamor_link_glsl_prog(glamor_priv->finish_access_prog[0]);
  glamor_link_glsl_prog(glamor_priv->finish_access_prog[1]);

  if (GLEW_ARB_fragment_shader) {
    GLint sampler_uniform_location;

    sampler_uniform_location =
      glGetUniformLocationARB(glamor_priv->finish_access_prog[0], "sampler");
    glUseProgramObjectARB(glamor_priv->finish_access_prog[0]);
    glUniform1iARB(sampler_uniform_location, 0);
    glUseProgramObjectARB(0);

    sampler_uniform_location =
      glGetUniformLocationARB(glamor_priv->finish_access_prog[1], "sampler");
    glUseProgramObjectARB(glamor_priv->finish_access_prog[1]);
    glUniform1iARB(sampler_uniform_location, 0);
    glUseProgramObjectARB(0);
  }
}

void
glamor_finish_access(DrawablePtr drawable)
{
  PixmapPtr pixmap = glamor_get_drawable_pixmap(drawable);
  glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
    
  if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
    return;

  if ( pixmap_priv->access_mode != GLAMOR_ACCESS_RO) {
    glamor_restore_pixmap_to_texture(pixmap);
  }

  if (pixmap_priv->pbo != 0 && pixmap_priv->pbo_valid) {
    glBindBufferARB (GL_PIXEL_PACK_BUFFER_EXT, 0);
    glBindBufferARB (GL_PIXEL_UNPACK_BUFFER_EXT, 0);
    pixmap_priv->pbo_valid = FALSE;
    glDeleteBuffersARB(1, &pixmap_priv->pbo);
    pixmap_priv->pbo = 0;
  } else
    free(pixmap->devPrivate.ptr);

  pixmap->devPrivate.ptr = NULL;
}


/**
 * Calls uxa_prepare_access with UXA_PREPARE_SRC for the tile, if that is the
 * current fill style.
 *
 * Solid doesn't use an extra pixmap source, so we don't worry about them.
 * Stippled/OpaqueStippled are 1bpp and can be in fb, so we should worry
 * about them.
 */
Bool
glamor_prepare_access_gc(GCPtr gc)
{
  if (gc->stipple) {
    if (!glamor_prepare_access(&gc->stipple->drawable, GLAMOR_ACCESS_RO))
      return FALSE;
  }
  if (gc->fillStyle == FillTiled) {
    if (!glamor_prepare_access (&gc->tile.pixmap->drawable,
				GLAMOR_ACCESS_RO)) {
      if (gc->stipple)
	glamor_finish_access(&gc->stipple->drawable);
      return FALSE;
    }
  }
  return TRUE;
}

/**
 * Finishes access to the tile in the GC, if used.
 */
void
glamor_finish_access_gc(GCPtr gc)
{
  if (gc->fillStyle == FillTiled)
    glamor_finish_access(&gc->tile.pixmap->drawable);
  if (gc->stipple)
    glamor_finish_access(&gc->stipple->drawable);
}

Bool
glamor_stipple(PixmapPtr pixmap, PixmapPtr stipple,
	       int x, int y, int width, int height,
	       unsigned char alu, unsigned long planemask,
	       unsigned long fg_pixel, unsigned long bg_pixel,
	       int stipple_x, int stipple_y)
{
  glamor_fallback("stubbed out stipple depth %d\n", pixmap->drawable.depth);
  return FALSE;
}

GCOps glamor_gc_ops = {
  .FillSpans = glamor_fill_spans,
  .SetSpans = glamor_set_spans,
  .PutImage = glamor_put_image,
  .CopyArea = glamor_copy_area,
  .CopyPlane = miCopyPlane,
  .PolyPoint = miPolyPoint,
  .Polylines = glamor_poly_lines,
  .PolySegment = miPolySegment,
  .PolyRectangle = miPolyRectangle,
  .PolyArc = miPolyArc,
  .FillPolygon = miFillPolygon,
  .PolyFillRect = glamor_poly_fill_rect,
  .PolyFillArc = miPolyFillArc,
  .PolyText8 = miPolyText8,
  .PolyText16 = miPolyText16,
  .ImageText8 = miImageText8,
  .ImageText16 = miImageText16,
  .ImageGlyphBlt = miImageGlyphBlt,
  .PolyGlyphBlt = miPolyGlyphBlt,
  .PushPixels = miPushPixels,
};

/**
 * uxa_validate_gc() sets the ops to glamor's implementations, which may be
 * accelerated or may sync the card and fall back to fb.
 */
static void
glamor_validate_gc(GCPtr gc, unsigned long changes, DrawablePtr drawable)
{
  /* fbValidateGC will do direct access to pixmaps if the tiling has changed.
   * Preempt fbValidateGC by doing its work and masking the change out, so
   * that we can do the Prepare/finish_access.
   */
#ifdef FB_24_32BIT
  if ((changes & GCTile) && fbGetRotatedPixmap(gc)) {
    gc->pScreen->DestroyPixmap(fbGetRotatedPixmap(gc));
    fbGetRotatedPixmap(gc) = 0;
  }

  if (gc->fillStyle == FillTiled) {
    PixmapPtr old_tile, new_tile;

    old_tile = gc->tile.pixmap;
    if (old_tile->drawable.bitsPerPixel != drawable->bitsPerPixel) {
      new_tile = fbGetRotatedPixmap(gc);
      if (!new_tile ||
	  new_tile ->drawable.bitsPerPixel != drawable->bitsPerPixel)
	{
	  if (new_tile)
	    gc->pScreen->DestroyPixmap(new_tile);
	  /* fb24_32ReformatTile will do direct access of a newly-
	   * allocated pixmap.
	   */
	  glamor_fallback("GC %p tile FB_24_32 transformat %p.\n", gc, old_tile);

	  if (glamor_prepare_access(&old_tile->drawable,
				    GLAMOR_ACCESS_RO)) {
	    new_tile = fb24_32ReformatTile(old_tile,
					   drawable->bitsPerPixel);
	    glamor_finish_access(&old_tile->drawable);
	  }
	}
      if (new_tile) {
	fbGetRotatedPixmap(gc) = old_tile;
	gc->tile.pixmap = new_tile;
	changes |= GCTile;
      }
    }
  }
#endif
  if (changes & GCTile) {
    if (!gc->tileIsPixel) {
      glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(gc->tile.pixmap);
      if ((!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv))
         && FbEvenTile(gc->tile.pixmap->drawable.width *
	  			       drawable->bitsPerPixel))
        {
	  glamor_fallback("GC %p tile changed %p.\n", gc, gc->tile.pixmap);
	  if (glamor_prepare_access(&gc->tile.pixmap->drawable,
				  GLAMOR_ACCESS_RW)) {
	  fbPadPixmap(gc->tile.pixmap);
	  glamor_finish_access(&gc->tile.pixmap->drawable);
	  }
        }
    }
    /* Mask out the GCTile change notification, now that we've done FB's
     * job for it.
     */
    changes &= ~GCTile;
  }

  if (changes & GCStipple && gc->stipple) {
    /* We can't inline stipple handling like we do for GCTile because
     * it sets fbgc privates.
     */
    if (glamor_prepare_access(&gc->stipple->drawable, GLAMOR_ACCESS_RW)) {
      fbValidateGC(gc, changes, drawable);
      glamor_finish_access(&gc->stipple->drawable);
    }
  } else {
    fbValidateGC(gc, changes, drawable);
  }

  gc->ops = &glamor_gc_ops;
}

static GCFuncs glamor_gc_funcs = {
  glamor_validate_gc,
  miChangeGC,
  miCopyGC,
  miDestroyGC,
  miChangeClip,
  miDestroyClip,
  miCopyClip
};

/**
 * exaCreateGC makes a new GC and hooks up its funcs handler, so that
 * exaValidateGC() will get called.
 */
int
glamor_create_gc(GCPtr gc)
{
  if (!fbCreateGC(gc))
    return FALSE;

  gc->funcs = &glamor_gc_funcs;

  return TRUE;
}

RegionPtr
glamor_bitmap_to_region(PixmapPtr pixmap)
{
  RegionPtr ret;
  glamor_fallback("pixmap %p \n", pixmap);
  if (!glamor_prepare_access(&pixmap->drawable, GLAMOR_ACCESS_RO))
    return NULL;
  ret = fbPixmapToRegion(pixmap);
  glamor_finish_access(&pixmap->drawable);
  return ret;
}
