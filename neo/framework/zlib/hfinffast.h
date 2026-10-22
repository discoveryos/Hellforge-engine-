/* hfinffast.h -- header to use hfinffast.c
 * Copyright (C) 1995-2003 Mark Adler
 * For conditions of distribution and use, see copyright notice in hfzlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use hfzlib.h.
 */

void inflate_fast OF((z_streamp strm, unsigned start));
