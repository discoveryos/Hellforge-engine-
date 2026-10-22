#pragma once

/*
	HF_Uber logic — pure, header-only portion of the native GLSL uber material
	path. Kept dependency-free (only hfprecompiled) so the same code runs in
	the live renderer (hfUberGLSL.cpp) and in the CPU-only script harness
	(script/tests), giving us offline unit tests for the feature-mask math.

	Feature bits are powers of two and are tested in GLSL with:
		mod( floor( mask / feature ), 2 ) > 0.5
	which is exactly what FeatureEnabled() simulates on the host.
*/

#include "../hflib/hfprecompiled.h"

namespace HF_Uber {

// Per-draw feature mask bits (mirrors hfUberFeature in hfUberGLSL.h)
enum {
	HF_UBER_DIFFUSE		= 1 << 0,	// modulate the diffuse texture (samp0)
	HF_UBER_BUMP		= 1 << 1,	// per-pixel normal from samp1
	HF_UBER_SPECULAR	= 1 << 2,	// specular term from samp2 * rpSpecularModifier
	HF_UBER_VERTEXCOLOR	= 1 << 3,	// multiply by in_Color * rpVertexColorModulate
	HF_UBER_ALPHATEST	= 1 << 4,	// discard on baseColor.a < rpAlphaTest.x
	HF_UBER_TEXGEN		= 1 << 5,	// use clip-space generated texture coordinates
	HF_UBER_DIFFUSEMOD	= 1 << 6,	// start the base color from rpDiffuseModifier (interactions)
	HF_UBER_LIGHTED		= 1 << 7,	// apply the view-space hemisphere lighting term (interactions)
	HF_UBER_PBR			= 1 << 8,	// physically-based (GGX Cook-Torrance) lighting branch
};

// Host-side simulation of the GLSL decode used by the uber shaders.
inline bool FeatureEnabled( uint32 mask, uint32 feature ) {
	return ( ( mask / feature ) % 2 ) == 1;
}

// Human-readable name for a single feature bit (for testUber / diagnostics).
const char * FeatureName( uint32 feature );

// True when the given program name is covered by the native GLSL uber pair.
bool IsCovered( const char * name );

// NULL-terminated list of every covered builtin program name (for the
// testUber diagnostic and the CPU harness).
const char * const * CoveredList();

// Base default feature mask for an uber-covered builtin program name, without
// the PBR bit (PBR is runtime-flag-dependent and applied by the renderer).
uint32 DefaultFeaturesFor( const char * name );

// Pure stage->features mapping. The renderer translates a shaderStage_t into
// these booleans and the harness can call this directly.
//  hasDiffuse          - stage textures a real image (samp0)
//  vertexColorModulate - SVC_MODULATE / SVC_INVERSE_MODULATE
//  hasAlphaTest        - stage.hasAlphaTest (cut-out)
//  screenTexgen        - TG_SCREEN / TG_SCREEN2
uint32 FeaturesFromStage( bool hasDiffuse, bool vertexColorModulate, bool hasAlphaTest, bool screenTexgen );

} // namespace HF_Uber