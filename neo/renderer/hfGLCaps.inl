#pragma once
// Header-only implementation of the pure hfGLCaps helpers so the CPU-only
// test harness (which does not link hfGLCaps.cpp or pull in the GL wrapper)
// can validate the capability math offline. hfGLCaps.cpp uses the same
// inline definitions, keeping engine and tests on identical logic.

static inline bool hfGLCaps_ParseVersionString_Impl( const char * str, int * major, int * minor ) {
	if ( str == NULL ) {
		return false;
	}
	const char * p = str;
	while ( *p && !( *p >= '0' && *p <= '9' ) ) {
		p++;
	}
	if ( *p == '\0' ) {
		return false;
	}
	*major = *p - '0';
	p++;
	if ( *p == '.' ) {
		p++;
		*minor = 0;
		while ( *p >= '0' && *p <= '9' ) {
			*minor = *minor * 10 + ( *p - '0' );
			p++;
		}
	} else {
		*minor = 0;
	}
	return true;
}

static inline hfGLCapabilityFlags hfGLCaps_FlagsForVersion_Impl( int glMajor, int glMinor, int glslMajor, int glslMinor ) {
	int flags = HF_GL_CAP_NONE;
	if ( glMajor >= 2 ) {
		flags |= HF_GL_CAP_GLSL_120;
	}
	if ( ( glMajor >= 3 && glMinor >= 1 ) || glslMajor >= 140 ) {
		flags |= HF_GL_CAP_GLSL_140;
	}
	if ( glMajor >= 3 && glMinor >= 3 ) {
		flags |= HF_GL_CAP_GLSL_330;
	}
	if ( glMajor >= 4 ) {
		flags |= HF_GL_CAP_GLSL_400;
	}
	if ( glMajor >= 4 && glMinor >= 3 ) {
		flags |= HF_GL_CAP_GLSL_430;
		flags |= HF_GL_CAP_COMPUTE;
		flags |= HF_GL_CAP_SSBO;
	}
	if ( glMajor >= 3 && glMinor >= 1 ) {
		flags |= HF_GL_CAP_UBO;
	}
	if ( glMajor >= 3 ) {
		flags |= HF_GL_CAP_FBO;
		flags |= HF_GL_CAP_BLIT_FRAMEBUFFER;
		flags |= HF_GL_CAP_TEXTURE_FLOAT;
	}
	if ( glMajor >= 4 && glMinor >= 3 ) {
		flags |= HF_GL_CAP_INSTANCING;
	}
	return ( hfGLCapabilityFlags ) flags;
}

inline bool hfGLCaps::ParseVersionString( const char * str, int * major, int * minor ) {
	return hfGLCaps_ParseVersionString_Impl( str, major, minor );
}

inline hfGLCapabilityFlags hfGLCaps::FlagsForVersion( int glMajor, int glMinor, int glslMajor, int glslMinor ) {
	return hfGLCaps_FlagsForVersion_Impl( glMajor, glMinor, glslMajor, glslMinor );
}