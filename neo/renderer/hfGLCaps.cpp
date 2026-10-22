#include "hfGLCaps.h"

#include "hfqgl.h"

hfGLCapabilityFlags hfGLCaps::s_flags = HF_GL_CAP_NONE;
int hfGLCaps::s_glMajor = 0;
int hfGLCaps::s_glMinor = 0;
int hfGLCaps::s_glslMajor = 0;
int hfGLCaps::s_glslMinor = 0;

#include "hfGLCaps.h"

/*
================
hfGLCaps::Init

Parses the GL and GLSL version strings using the real GL calls, then walks
the extension string to build the capability set. The extension check works
both for the new-style "GL & GL_EXTENSIONS" query and the string search used
by the fixed-function path.
================
*/
void hfGLCaps::Init() {
	if ( s_glMajor != 0 ) {
		return;	// already initialized
	}

	const char * glVersionStr = ( const char * ) qglGetString( GL_VERSION );
	const char * glslVersionStr = ( const char * ) qglGetString( GL_SHADING_LANGUAGE_VERSION );

	hfGLCaps::ParseVersionString( glVersionStr, &s_glMajor, &s_glMinor );
	hfGLCaps::ParseVersionString( glslVersionStr, &s_glslMajor, &s_glslMinor );

	// GLSL derived from GL version when the driver can't report it
	if ( s_glslMajor == 0 ) {
		if ( s_glMajor >= 4 ) {
			s_glslMajor = 400 + ( s_glMinor - 0 ) * 100;
		} else if ( s_glMajor >= 3 ) {
			s_glslMajor = 130 + ( s_glMinor ) * 100;
		} else {
			s_glslMajor = 110 + ( s_glMinor );
		}
	}

	// Core capability flags from parsed versions (shared pure logic so the
	// test harness covers this path, see hfGLCaps.inl)
	int flags = hfGLCaps::FlagsForVersion( s_glMajor, s_glMinor, s_glslMajor, s_glslMinor );

	// Extension-based boosts for older-core-but-newer-feature drivers
	const char * ext = ( const char * ) qglGetString( GL_EXTENSIONS );
	if ( ext != NULL ) {
		if ( hfStr::FindText( ext, "GL_ARB_compute_shader", false ) != -1 ) {
			flags |= HF_GL_CAP_COMPUTE;
			if ( s_glMajor < 4 ) {
				flags |= HF_GL_CAP_GLSL_430;
			}
		}
		if ( hfStr::FindText( ext, "GL_ARB_uniform_buffer_object", false ) != -1 ) {
			flags |= HF_GL_CAP_UBO;
		}
		if ( hfStr::FindText( ext, "GL_ARB_shader_storage_buffer_object", false ) != -1 ) {
			flags |= HF_GL_CAP_SSBO;
		}
		if ( hfStr::FindText( ext, "GL_ARB_framebuffer_object", false ) != -1
				|| hfStr::FindText( ext, "GL_EXT_framebuffer_object", false ) != -1 ) {
			flags |= HF_GL_CAP_FBO;
		}
		if ( hfStr::FindText( ext, "GL_ARB_framebuffer_blit", false ) != -1 ) {
			flags |= HF_GL_CAP_BLIT_FRAMEBUFFER;
		}
		if ( hfStr::FindText( ext, "GL_ARB_texture_float", false ) != -1 ) {
			flags |= HF_GL_CAP_TEXTURE_FLOAT;
		}
		if ( hfStr::FindText( ext, "GL_ARB_instanced_arrays", false ) != -1 ) {
			flags |= HF_GL_CAP_INSTANCING;
		}
	}
	s_flags = ( hfGLCapabilityFlags ) flags;

#if !defined( _DEBUG )
	hfLib::Printf( "hfGLCaps: GL %d.%d, GLSL %d.%d, flags 0x%08x\n",
		s_glMajor, s_glMinor, s_glslMajor, s_glslMinor, ( unsigned ) s_flags );
#endif
}

bool hfGLCaps::HasFlag( hfGLCapabilityFlags flag ) {
	return ( s_flags & flag ) != 0;
}