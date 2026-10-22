Hellforge Power 1 Engine
========================

This file contains the following sections:

GENERAL NOTES
LICENSE

GENERAL NOTES
=============

Hellforge Power 1 is a modern, cross-platform game engine.
It is free of third-party game content and provides a clean
public game module interface, allowing you to build any game on top of it.


Features
--------

* Cross-platform build system (CMake)
* Windows and Linux platform support
* Modern C++11/C++17
* OpenGL renderer
* Clean game module API - plug in any game
* Rendering, physics, collision, and audio systems


Compiling on Linux:
-------------------

Requirements: CMake 3.16+, a C++11 compiler, X11, and OpenGL development headers.

    mkdir build && cd build
    cmake ..
    make

The hellforge executable will be produced in the build directory.


Compiling on Windows:
---------------------

Requirements: CMake 3.16+, Visual Studio (2019 or later), and OpenGL drivers.

    mkdir build && cd build
    cmake -G "Visual Studio 16 2019" ..
    cmake --build . --config Release


Game Data
---------

The Hellforge Power 1 Engine source release does not contain any game data.
Game assets must be provided by the game you build on top of the engine.


LICENSE
=======

See COPYING.txt for the GNU GENERAL PUBLIC LICENSE

ADDITIONAL TERMS:  The Hellforge Power 1 Engine is also subject to certain
additional terms. You should have received a copy of these additional terms
immediately following the terms and conditions of the GNU GPL which
accompanied the Hellforge Power 1 Engine.

EXCLUDED CODE:  The code described below and contained in the Hellforge Power
1 Engine source release is not part of the Program covered by the GPL and is
expressly excluded from its terms.  You are solely responsible for obtaining
from the copyright holder a license for such code and complying with the
applicable license terms.

JPEG library
-----------------------------------------------------------------------------
neo/renderer/jpeg-6/*

Copyright (C) 1991-1995, Thomas G. Lane

Permission is hereby granted to use, copy, modify, and distribute this
software (or portions thereof) for any purpose, without fee, subject to these
conditions:
(1) If any part of the source code for this software is distributed, then this
README file must be included, with this copyright and no-warranty notice
unaltered; and any additions, deletions, or changes to the original files
must be clearly indicated in accompanying documentation.
(2) If only executable code is distributed, then the accompanying
documentation must state that "this software is based in part on the work of
the Independent JPEG Group".
(3) Permission for use of this software is granted only if the user accepts
full responsibility for any undesirable consequences; the authors accept
NO LIABILITY for damages of any kind.

These conditions apply to any software derived from or based on the IJG code,
not just to the unmodified library.  If you use our work, you ought to
acknowledge us.

NOTE: unfortunately the README that came with our copy of the library has
been lost, so the one from release 6b is included instead. There are a few
'glue type' modifications to the library to make it easier to use from
the engine, but otherwise the dependency can be easily cleaned up to a
better release of the library.

zlib library
-----------------------------------------------------------------------------
neo/framework/zlib/*

Copyright (C) 1995-2005 Jean-loup Gailly and Mark Adler

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
 claim that you wrote the original software. If you use this software
 in a product, an acknowledgment in the product documentation would be
 appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
 misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

Base64 implementation
-----------------------------------------------------------------------------
neo/hflib/hfBase64.cpp

Copyright (c) 1996 Lars Wirzenius.  All rights reserved.

June 14 2003: TTimo <ttimo@idsoftware.com>
	modified + endian bug fixes
	http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=197039

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

IO for uncompress .zip files using zlib
-----------------------------------------------------------------------------
neo/framework/hfUnzip.cpp
neo/framework/hfUnzip.h

Copyright (C) 1998 Gilles Vollant
zlib is Copyright (C) 1995-1998 Jean-loup Gailly and Mark Adler

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

MD4 Message-Digest Algorithm
-----------------------------------------------------------------------------
neo/hflib/hashing/hfMD4.cpp
Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD4 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD4 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.

MD5 Message-Digest Algorithm
-----------------------------------------------------------------------------
neo/hflib/hashing/hfMD5.cpp
This code implements the MD5 message-digest algorithm.
The algorithm is due to Ron Rivest.  This code was
written by Colin Plumb in 1993, no copyright is claimed.
This code is in the public domain; do with it what you wish.

CRC32 Checksum
-----------------------------------------------------------------------------
neo/hflib/hashing/hfCRC32.cpp
Copyright (C) 1995-1998 Mark Adler

OpenGL headers
--------------------------------------------------------------------------
neo/renderer/OpenGL/hfglext.h
neo/renderer/OpenGL/hfwglext.h

Copyright (c) 2007-2012 The Khronos Group Inc.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and/or associated documentation files (the
"Materials"), to deal in the Materials without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Materials, and to
permit persons to whom the Materials are furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Materials.

THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
