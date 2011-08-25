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

#include "scrnintstr.h"
#include "xf86str.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "gcstruct.h"
#include "picturestr.h"
#include "fb.h"
#include "fbpict.h"

#endif /* GLAMOR_H */


#define GLAMOR_INVERTED_Y_AXIS  1
#define GLAMOR_HOSTX            2
#define GLAMOR_VALID_FLAGS      (GLAMOR_INVERTED_Y_AXIS | GLAMOR_HOSTX)

#define GLAMOR_EGL_EXTERNAL_BUFFER 3

extern _X_EXPORT Bool glamor_init(ScreenPtr screen, unsigned int flags);
extern _X_EXPORT void glamor_fini(ScreenPtr screen);
extern _X_EXPORT void glamor_set_screen_pixmap_texture(ScreenPtr screen, int w, int h, unsigned int tex);

extern _X_EXPORT Bool glamor_egl_init(ScrnInfoPtr scrn, int fd);
extern _X_EXPORT Bool glamor_create_egl_screen_image(ScreenPtr screen, int handle, int stride);
extern _X_EXPORT Bool glamor_close_egl_screen(ScreenPtr screen);
extern _X_EXPORT void glamor_free_egl_screen(int scrnIndex, int flags);

