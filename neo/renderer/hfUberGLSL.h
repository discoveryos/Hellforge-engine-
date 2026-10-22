#ifndef __HFUBERGLSL_H__
#define __HFUBERGLSL_H__

#include "../hflib/hfprecompiled.h"
#include "hfUberLogic.h"

class hfCVar;

// Enables the hand-written native GLSL uber material shader path
// (idTech 6 style) instead of the runtime Cg->GLSL conversion for the common
// material builtins: color, simpleshade, texture, texture_color,
// texture_color_skinned, texture_color_texgen, gui, interaction,
// interaction_skinned, interactionAmbient, interactionAmbient_skinned.
extern hfCVar r_useNativeGLSL;

// Physically-based lighting toggle + defaults consumed by the native uber
// data-binding path (declared here so console tools can read it too).
extern hfCVar r_usePBR;

// Per-draw feature mask. Written into the shared rpUser0 vector uniform once
// per program bind, right where the explicit program permutation selection
// lives in the idTech 6 material compiler. Feature bits are powers of two so
// they can be tested in GLSL with integer modulo/divide. The bit values live
// in HF_Uber (hfUberLogic.h) so the CPU test harness can exercise the same
// math without pulling in the renderer.
enum hfUberFeature {
	HF_UBER_DIFFUSE		= HF_Uber::HF_UBER_DIFFUSE,
	HF_UBER_BUMP		= HF_Uber::HF_UBER_BUMP,
	HF_UBER_SPECULAR	= HF_Uber::HF_UBER_SPECULAR,
	HF_UBER_VERTEXCOLOR	= HF_Uber::HF_UBER_VERTEXCOLOR,
	HF_UBER_ALPHATEST	= HF_Uber::HF_UBER_ALPHATEST,
	HF_UBER_TEXGEN		= HF_Uber::HF_UBER_TEXGEN,
	HF_UBER_DIFFUSEMOD	= HF_Uber::HF_UBER_DIFFUSEMOD,
	HF_UBER_LIGHTED		= HF_Uber::HF_UBER_LIGHTED,
	HF_UBER_PBR			= HF_Uber::HF_UBER_PBR,
};

// Returns true when the given material shader program name is covered by the
// native GLSL uber shader pair instead of the Cg->GLSL converter.
bool HF_UberGLSL_Covered( const char *name );

// Compiles the native uber vertex or fragment shader for the given program
// name. On success the returned GL shader id is stored by the caller and the
// program is flagged nativeGLSL (named-uniforms path). Returns
// HF_UBERGLSL_INVALID_SHADER on any failure (matches the
// hfRenderProgManager::INVALID_PROGID sentinel).
#define HF_UBERGLSL_INVALID_SHADER	0xFFFFFFFFu
GLuint HF_UberGLSL_CompileShader( GLenum target, const char *name, hfList<int> &uniforms );

// Pushes the per-draw feature mask into the shared rpUser0 uniform. Draws with
// special needs may call this at the stage level; plain draws get the default
// mask for the bound builtin from BindShader.
void HF_UberGLSL_SetFeatures( uint32 features );

// Default feature mask for a material builtin program name (0 for programs
// that are not uber-covered).
uint32 HF_UberGLSL_DefaultFeaturesFor( const char *name );

// Feature mask derived from a classic material stage (diffuse/vertexColor/
// alphaTest/texgen). New-style stages always get 0. The stage is passed as
// void* to keep this header independent from the material types.
uint32 HF_UberGLSL_StageFeatures( const void *stage );

// stage-level variant of HF_UberGLSL_SetFeatures: pushes the stage mask when
// the currently bound program is a native uber program, otherwise no-op.
void HF_UberGLSL_SetStageFeatures( const void *stage );

#endif