/*
===========================================================================

CPU-only unit tests for the native GLSL uber-material logic and the GL
capability detection math.

Shares the exact same code the renderer uses (hfUberLogic.cpp and the pure
inline helpers in hfGLCaps.h/.inl), so a headless machine can verify the
feature-bit decoding, the per-program default masks, the stage→features
mapping and the capability ladder without a GPU or a display.

Build from the engine root (with hfsharness):
	see hfsharness.cpp header - add ../script/tests/hfstest_render.cpp

===========================================================================
*/
#include "../../hflib/hfprecompiled.h"

// Pull in the pure uber logic (single TU harness; no link against the
// renderer static library is required).
#include "../../renderer/hfUberLogic.cpp"

// Pull in the pure GL capability helpers (inline, no GL symbols referenced).
#include "../../renderer/hfGLCaps.h"

static int g_renderFailures = 0;

#define REPORT( cond )														\
	do {																	\
		if ( !( cond ) ) {													\
			printf( "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond );		\
			g_renderFailures++;												\
		} else {															\
			printf( "  ok   %s\n", #cond );									\
		}																	\
	} while ( 0 )

/*
================
Test_UberFeatureDecode
================
*/
static void Test_UberFeatureDecode() {
	const uint32 all = HF_Uber::HF_UBER_DIFFUSE | HF_Uber::HF_UBER_BUMP
			| HF_Uber::HF_UBER_SPECULAR | HF_Uber::HF_UBER_VERTEXCOLOR
			| HF_Uber::HF_UBER_ALPHATEST | HF_Uber::HF_UBER_TEXGEN
			| HF_Uber::HF_UBER_DIFFUSEMOD | HF_Uber::HF_UBER_LIGHTED
			| HF_Uber::HF_UBER_PBR;

	REPORT( !HF_Uber::FeatureEnabled( 0, HF_Uber::HF_UBER_DIFFUSE ) );

	// Each feature must decode exactly on its own bit and stay silent for
	// every other bit.
	const uint32 bits[9] = {
		HF_Uber::HF_UBER_DIFFUSE, HF_Uber::HF_UBER_BUMP, HF_Uber::HF_UBER_SPECULAR,
		HF_Uber::HF_UBER_VERTEXCOLOR, HF_Uber::HF_UBER_ALPHATEST, HF_Uber::HF_UBER_TEXGEN,
		HF_Uber::HF_UBER_DIFFUSEMOD, HF_Uber::HF_UBER_LIGHTED, HF_Uber::HF_UBER_PBR
	};
	for ( int i = 0; i < 9; i++ ) {
		REPORT( HF_Uber::FeatureEnabled( bits[i], bits[i] ) );
		for ( int j = 0; j < 9; j++ ) {
			if ( i != j ) {
				REPORT( !HF_Uber::FeatureEnabled( bits[i], bits[j] ) );
			}
		}
	}

	// Combined mask: all bits decode active.
	REPORT( HF_Uber::FeatureEnabled( all, HF_Uber::HF_UBER_PBR ) );
	REPORT( HF_Uber::FeatureEnabled( all, HF_Uber::HF_UBER_LIGHTED ) );
	REPORT( HF_Uber::FeatureEnabled( all, HF_Uber::HF_UBER_DIFFUSE ) );

	// Names (used by the testUber console dump).
	REPORT( hfStr::Icmp( HF_Uber::FeatureName( HF_Uber::HF_UBER_DIFFUSE ), "diffuse" ) == 0 );
	REPORT( hfStr::Icmp( HF_Uber::FeatureName( HF_Uber::HF_UBER_PBR ), "pbr" ) == 0 );
	REPORT( hfStr::Icmp( HF_Uber::FeatureName( 0x8000 ), "unknown" ) == 0 );
}

/*
================
Test_UberCoverage
================
*/
static void Test_UberCoverage() {
	REPORT( HF_Uber::IsCovered( "texture.vfp" ) );
	REPORT( HF_Uber::IsCovered( "interaction.vfp" ) );
	REPORT( HF_Uber::IsCovered( "interaction_skinned.vfp" ) );
	REPORT( HF_Uber::IsCovered( "gui.vfp" ) );
	REPORT( HF_Uber::IsCovered( "color.vfp" ) );
	REPORT( !HF_Uber::IsCovered( "fog.vfp" ) );
	REPORT( !HF_Uber::IsCovered( "skybox.vfp" ) );
	REPORT( !HF_Uber::IsCovered( NULL ) );
	REPORT( !HF_Uber::IsCovered( "" ) );
	REPORT( !HF_Uber::IsCovered( "not_a_shader.vfp" ) );
}

/*
================
Test_UberDefaultFeatures
================
*/
static void Test_UberDefaultFeatures() {
	REPORT( HF_Uber::DefaultFeaturesFor( "gui.vfp" ) == HF_Uber::HF_UBER_DIFFUSE );
	REPORT( HF_Uber::DefaultFeaturesFor( "color.vfp" ) == 0 );
	REPORT( HF_Uber::DefaultFeaturesFor( "simpleshade.vfp" ) == 0 );
	REPORT( HF_Uber::DefaultFeaturesFor( "texture.vfp" ) == HF_Uber::HF_UBER_DIFFUSE );
	REPORT( HF_Uber::DefaultFeaturesFor( "texture_color.vfp" )
			== ( HF_Uber::HF_UBER_DIFFUSE | HF_Uber::HF_UBER_VERTEXCOLOR ) );
	// skinned variant: vertex color carries joint indices, no VERTEXCOLOR
	REPORT( HF_Uber::DefaultFeaturesFor( "texture_color_skinned.vfp" ) == HF_Uber::HF_UBER_DIFFUSE );
	REPORT( HF_Uber::DefaultFeaturesFor( "texture_color_texgen.vfp" )
			== ( HF_Uber::HF_UBER_DIFFUSE | HF_Uber::HF_UBER_VERTEXCOLOR | HF_Uber::HF_UBER_TEXGEN ) );

	const uint32 interaction = HF_Uber::HF_UBER_DIFFUSE | HF_Uber::HF_UBER_BUMP
			| HF_Uber::HF_UBER_SPECULAR | HF_Uber::HF_UBER_VERTEXCOLOR
			| HF_Uber::HF_UBER_ALPHATEST | HF_Uber::HF_UBER_DIFFUSEMOD | HF_Uber::HF_UBER_LIGHTED;
	REPORT( HF_Uber::DefaultFeaturesFor( "interaction.vfp" ) == interaction );
	// PBR is not part of the base mask; it is layered on by the renderer when
	// r_usePBR is enabled.
	REPORT( !HF_Uber::FeatureEnabled( HF_Uber::DefaultFeaturesFor( "interaction.vfp" ), HF_Uber::HF_UBER_PBR ) );
	REPORT( HF_Uber::DefaultFeaturesFor( "interaction_skinned.vfp" ) == ( interaction & ~HF_Uber::HF_UBER_VERTEXCOLOR ) );
	REPORT( HF_Uber::DefaultFeaturesFor( "interactionAmbient.vfp" ) == interaction );
	REPORT( HF_Uber::DefaultFeaturesFor( "interactionAmbient_skinned.vfp" ) == ( interaction & ~HF_Uber::HF_UBER_VERTEXCOLOR ) );
	REPORT( HF_Uber::DefaultFeaturesFor( "fog.vfp" ) == 0 );
	REPORT( HF_Uber::DefaultFeaturesFor( NULL ) == 0 );
}

/*
================
Test_UberStageToFeatures
================
*/
static void Test_UberStageToFeatures() {
	REPORT( HF_Uber::FeaturesFromStage( false, false, false, false ) == 0 );

	// classic diffuse map
	REPORT( HF_Uber::FeaturesFromStage( true, false, false, false ) == HF_Uber::HF_UBER_DIFFUSE );

	// diffuse + cut-out (alpha test)
	REPORT( HF_Uber::FeaturesFromStage( true, false, true, false )
			== ( HF_Uber::HF_UBER_DIFFUSE | HF_Uber::HF_UBER_ALPHATEST ) );

	// vertex color (both modulate modes map the same way)
	REPORT( HF_Uber::FeaturesFromStage( false, true, false, false ) == HF_Uber::HF_UBER_VERTEXCOLOR );

	// screen-space texgen
	REPORT( HF_Uber::FeaturesFromStage( false, false, false, true ) == HF_Uber::HF_UBER_TEXGEN );

	// a blend/texgen stage without its own texture keeps texgen but adds no diffuse
	REPORT( HF_Uber::FeaturesFromStage( false, true, true, true )
			== ( HF_Uber::HF_UBER_VERTEXCOLOR | HF_Uber::HF_UBER_ALPHATEST | HF_Uber::HF_UBER_TEXGEN ) );

	// every classic-stage feature at once (no newStage / bump / specular here;
	// those come from the interaction branch, not the stage path)
	const uint32 allStage = HF_Uber::FeaturesFromStage( true, true, true, true );
	REPORT( allStage == ( HF_Uber::HF_UBER_DIFFUSE | HF_Uber::HF_UBER_VERTEXCOLOR
			| HF_Uber::HF_UBER_ALPHATEST | HF_Uber::HF_UBER_TEXGEN ) );
	REPORT( !HF_Uber::FeatureEnabled( allStage, HF_Uber::HF_UBER_LIGHTED ) );
	REPORT( !HF_Uber::FeatureEnabled( allStage, HF_Uber::HF_UBER_PBR ) );
}

/*
================
Test_GLParseVersion
================
*/
static void Test_GLParseVersion() {
	int major = -1, minor = -1;

	REPORT( hfGLCaps::ParseVersionString( "4.6.0 NVIDIA 470.86", &major, &minor ) );
	REPORT( major == 4 && minor == 6 );

	REPORT( hfGLCaps::ParseVersionString( "3.3 ATI", &major, &minor ) );
	REPORT( major == 3 && minor == 3 );

	REPORT( hfGLCaps::ParseVersionString( "2.1 Mesa 21.2.6", &major, &minor ) );
	REPORT( major == 2 && minor == 1 );

	// GLSL strings are "major.minor" with a language prefix, e.g. glslang.
	REPORT( hfGLCaps::ParseVersionString( "4.60", &major, &minor ) );
	REPORT( major == 4 && minor == 60 );

	REPORT( hfGLCaps::ParseVersionString( "OpenGL ES 3.2", &major, &minor ) );
	REPORT( major == 3 && minor == 2 );

	// Multi-digit minor (GL 4.13 style, future-proof).
	REPORT( hfGLCaps::ParseVersionString( "4.13", &major, &minor ) );
	REPORT( major == 4 && minor == 13 );

	REPORT( !hfGLCaps::ParseVersionString( "", &major, &minor ) );
	REPORT( !hfGLCaps::ParseVersionString( NULL, &major, &minor ) );
	REPORT( !hfGLCaps::ParseVersionString( "vendor-only-string", &major, &minor ) );
}

/*
================
Test_GLFlagsVersion
================
*/
static void Test_GLFlagsVersion() {
	// GL 2.1: minimum, converted shaders only
	hfGLCapabilityFlags f = hfGLCaps::FlagsForVersion( 2, 1, 0, 0 );
	REPORT( ( f & HF_GL_CAP_GLSL_120 ) != 0 );
	REPORT( ( f & HF_GL_CAP_GLSL_140 ) == 0 );
	REPORT( ( f & HF_GL_CAP_UBO ) == 0 );
	REPORT( ( f & HF_GL_CAP_COMPUTE ) == 0 );

	// GL 3.0: FBOs, but no UBO/no compute
	f = hfGLCaps::FlagsForVersion( 3, 0, 0, 0 );
	REPORT( ( f & HF_GL_CAP_FBO ) != 0 );
	REPORT( ( f & HF_GL_CAP_UBO ) == 0 );
	REPORT( ( f & HF_GL_CAP_GLSL_140 ) == 0 );

	// GL 3.1: native uber + UBO available
	f = hfGLCaps::FlagsForVersion( 3, 1, 0, 0 );
	REPORT( ( f & HF_GL_CAP_GLSL_140 ) != 0 );
	REPORT( ( f & HF_GL_CAP_UBO ) != 0 );
	REPORT( ( f & HF_GL_CAP_COMPUTE ) == 0 );

	// GL 3.3: +GLSL 330 (still no compute)
	f = hfGLCaps::FlagsForVersion( 3, 3, 0, 0 );
	REPORT( ( f & HF_GL_CAP_GLSL_330 ) != 0 );
	REPORT( ( f & HF_GL_CAP_COMPUTE ) == 0 );

	// GL 4.0: +GLSL 400
	f = hfGLCaps::FlagsForVersion( 4, 0, 0, 0 );
	REPORT( ( f & HF_GL_CAP_GLSL_400 ) != 0 );
	REPORT( ( f & HF_GL_CAP_COMPUTE ) == 0 );

	// GL 4.3: the full idTech7 stack (compute + ssbo)
	f = hfGLCaps::FlagsForVersion( 4, 3, 0, 0 );
	REPORT( ( f & HF_GL_CAP_GLSL_430 ) != 0 );
	REPORT( ( f & HF_GL_CAP_COMPUTE ) != 0 );
	REPORT( ( f & HF_GL_CAP_SSBO ) != 0 );

	// GL 4.6: same capability set as 4.3 for our feature surface
	f = hfGLCaps::FlagsForVersion( 4, 6, 0, 0 );
	REPORT( ( f & HF_GL_CAP_COMPUTE ) != 0 );

	// GLSL cap drives the 140 flag even on GL 2.x-style cores (extension path)
	f = hfGLCaps::FlagsForVersion( 2, 1, 140, 0 );
	REPORT( ( f & HF_GL_CAP_GLSL_140 ) != 0 );
}

/*
================
HfTest_RenderMain

Run all render-logic unit tests, return the failure count (0 = pass).
================
*/
int HfTest_RenderMain() {
	printf( "--- render logic (uber -> feats, GL caps) ---\n" );

	Test_UberFeatureDecode();
	Test_UberCoverage();
	Test_UberDefaultFeatures();
	Test_UberStageToFeatures();
	Test_GLParseVersion();
	Test_GLFlagsVersion();

	printf( "%s\n", g_renderFailures == 0 ? "  render logic: all ok" : "  render logic: FAILURES PRESENT" );
	return g_renderFailures;
}