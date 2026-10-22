#pragma once

#include "../hflib/hfprecompiled.h"

/*
    hfGLCaps — Lightweight runtime capability detection for broad GPU support.

    Parses GL_VERSION / GLSL_VERSION at init time and exposes capability flags
    that let the renderer pick the best shader and post-process path for the
    current driver. Every flag degrades to an older code path when false, so
    the engine works on GL 2.1 through GL 4.6 without compile-time or
    runtime failures.
*/

enum hfGLCapabilityFlags {
    HF_GL_CAP_NONE              = 0,
    // GLSL version parsed from the driver string
    HF_GL_CAP_GLSL_120          = (1 << 0),   // GL 2.1  — minimum for converted shaders
    HF_GL_CAP_GLSL_140          = (1 << 1),   // GL 3.1  — native uber vertex/fragment
    HF_GL_CAP_GLSL_330          = (1 << 2),   // GL 3.3  — better error messages
    HF_GL_CAP_GLSL_400          = (1 << 3),   // GL 4.0  — tessellation
    HF_GL_CAP_GLSL_430          = (1 << 4),   // GL 4.3  — compute, image load/store
    // Extensions / features
    HF_GL_CAP_COMPUTE           = (1 << 5),   // GL 4.3 or ARB_compute_shader
    HF_GL_CAP_UBO               = (1 << 6),   // GL 3.1 or ARB_uniform_buffer_object
    HF_GL_CAP_SSBO              = (1 << 7),   // GL 4.3 or ARB_shader_storage_buffer_object
    HF_GL_CAP_FBO               = (1 << 8),   // GL 3.0 or ARB_framebuffer_object
    HF_GL_CAP_BLIT_FRAMEBUFFER  = (1 << 9),   // GL 3.0 or ARB_framebuffer_blit
    HF_GL_CAP_TEXTURE_FLOAT     = (1 << 10),  // GL 3.0 or ARB_texture_float
    HF_GL_CAP_INSTANCING        = (1 << 11),  // GL 3.3 or ARB_instanced_arrays
    HF_GL_CAP_MAX_FLAGS
};

class hfGLCaps {
public:
    static void             Init();
    static bool             HasFlag( hfGLCapabilityFlags flag );

    static int              GetGLMajorVersion()     { return s_glMajor; }
    static int              GetGLMinorVersion()     { return s_glMinor; }
    static int              GetGLSLMajorVersion()   { return s_glslMajor; }
    static int              GetGLSLMinorVersion()   { return s_glslMinor; }
    static hfGLCapabilityFlags GetFlags()           { return s_flags; }

    // Pure helpers (no GL context required): used by Init() and by the CPU
    // test harness to validate the capability math offline.
    // Reads a "major.minor" prefix out of a driver string like
    // "4.6.0 NVIDIA 470.86" / "3.3 ATI" / "2.1 Mesa 21.2".
    static bool             ParseVersionString( const char * str, int * major, int * minor );

    // Core (non-extension) flag set derived purely from parsed versions.
    // Extension-based flags (compute/ubo/ssbo/fbo/blit/float/instancing) are
    // layered on top by Init() after scanning GL_EXTENSIONS.
    static hfGLCapabilityFlags FlagsForVersion( int glMajor, int glMinor, int glslMajor, int glslMinor );

private:
    static hfGLCapabilityFlags s_flags;
    static int s_glMajor;
    static int s_glMinor;
    static int s_glslMajor;
    static int s_glslMinor;
};

#include "hfGLCaps.inl"
