#ifndef __HFOPENGLPOSTPROCESS_H__
#define __HFOPENGLPOSTPROCESS_H__

#include "../hflib/hfprecompiled.h"

class hfCVar;

// Enables the compute-shader based HDR post-processing pass (idTech 6 style).
// Requires OpenGL 4.3 (compute shaders + image load/store). When unavailable,
// the renderer silently falls back to the legacy fullscreen-quad postprocess.
extern hfCVar r_computePostprocess;
extern hfCVar r_postProcessExposure;
extern hfCVar r_postProcessGamma;
extern hfCVar r_bloom;
extern hfCVar r_bloomBrightThreshold;
extern hfCVar r_bloomIntensity;
extern hfCVar r_computeDepthBlit;
extern hfCVar r_ssao;
extern hfCVar r_ssaoStrength;
extern hfCVar r_ssr;
extern hfCVar r_ssrScale;
extern hfCVar r_ssrSteps;
extern hfCVar r_depthOfField;
extern hfCVar r_dofStrength;
extern hfCVar r_dofFocusDist;

// Scratch textures managed for the offscreen HDR capture and the LDR result.
// Returns 0 while unavailable or before BeginHDRCapture.
GLuint hfComputePost_HDRTexture();
GLuint hfComputePost_ResultTexture();
GLuint hfComputePost_DepthTexture();	// scene depth blitted into the HDR target (SSAO/SSR/DoF)
GLuint hfComputePost_AOTexture();		// full-res SSAO mask

// One-time capability detection. Compiles and links the embedded compute
// program and resolves the GL 4.3 entry points via dlsym. Returns false (and
// prints a note) when the driver does not support compute-based post.
bool hfComputePost_Available();

// Resizes/recreates the internal textures to match the current render size and
// redirects rendering into the RGBA32F HDR target. Returns false when compute
// post is unavailable. The caller must restore state it cares about after the
// offscreen pass.
bool hfComputePost_BeginHDRCapture( int width, int height );

// Binds back the default framebuffer after the offscreen HDR capture pass.
void hfComputePost_EndHDRCapture();

// Runs the compute tonemap: reads the HDR target as an image, applies ACES
// curve + exposure/gamma, writes the LDR result, then emits the appropriate
// memory barriers so the result can be sampled by the present pass.
void hfComputePost_Dispatch( int width, int height, float exposure );

// Copies the view/projection matrix into module state. Row-major float[16]
// (element i*4+j = row i, column j), evaluated as clip = M * vec4(viewPos,1).
// Used for view-space reconstruction in the SSAO / SSR compute passes.
void hfComputePost_SetProjectionMatrix( const float m[16] );

// Releases the GL objects (textures, framebuffer, program).
void hfComputePost_Shutdown();

#endif