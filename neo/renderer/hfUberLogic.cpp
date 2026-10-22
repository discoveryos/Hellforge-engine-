/*
	Header-only implementation of the HF_Uber logic (see hfUberLogic.h). One
	translation unit — included by the engine and by the CPU-only test harness.
*/

#include "hfUberLogic.h"

namespace HF_Uber {

static const char * const s_coveredPrograms[] = {
	"color.vfp",
	"simpleshade.vfp",
	"texture.vfp",
	"texture_color.vfp",
	"texture_color_skinned.vfp",
	"texture_color_texgen.vfp",
	"gui.vfp",
	"interaction.vfp",
	"interaction_skinned.vfp",
	"interactionAmbient.vfp",
	"interactionAmbient_skinned.vfp",
	NULL
};

static struct defaultFeatures_t {
	const char * name;
	uint32 features;
} s_defaultFeatures[] = {
	// GUI tint comes through rpColor; the legacy gui program does not
	// modulate the surface vertex color.
	{ "gui.vfp",
		HF_UBER_DIFFUSE },
	// flat color, no texture modulation
	{ "color.vfp",
		0 },
	// flat shading with diffuse color applied through rpColor
	{ "simpleshade.vfp",
		0 },
	{ "texture.vfp",
		HF_UBER_DIFFUSE },
	{ "texture_color.vfp",
		HF_UBER_DIFFUSE | HF_UBER_VERTEXCOLOR },
	// the color attribute carries the joint indices, never vertex color
	{ "texture_color_skinned.vfp",
		HF_UBER_DIFFUSE },
	{ "texture_color_texgen.vfp",
		HF_UBER_DIFFUSE | HF_UBER_VERTEXCOLOR | HF_UBER_TEXGEN },
	// alpha test stays on unconditionally: rpAlphaTest is 0 when the material
	// has no cut-out, exactly like the legacy interaction program.
	{ "interaction.vfp",
		HF_UBER_DIFFUSE | HF_UBER_BUMP | HF_UBER_SPECULAR | HF_UBER_VERTEXCOLOR
		| HF_UBER_ALPHATEST | HF_UBER_DIFFUSEMOD | HF_UBER_LIGHTED },
	{ "interaction_skinned.vfp",
		HF_UBER_DIFFUSE | HF_UBER_BUMP | HF_UBER_SPECULAR
		| HF_UBER_ALPHATEST | HF_UBER_DIFFUSEMOD | HF_UBER_LIGHTED },
	{ "interactionAmbient.vfp",
		HF_UBER_DIFFUSE | HF_UBER_BUMP | HF_UBER_SPECULAR | HF_UBER_VERTEXCOLOR
		| HF_UBER_ALPHATEST | HF_UBER_DIFFUSEMOD | HF_UBER_LIGHTED },
	{ "interactionAmbient_skinned.vfp",
		HF_UBER_DIFFUSE | HF_UBER_BUMP | HF_UBER_SPECULAR
		| HF_UBER_ALPHATEST | HF_UBER_DIFFUSEMOD | HF_UBER_LIGHTED },
};

const char * FeatureName( uint32 feature ) {
	switch ( feature ) {
		case HF_UBER_DIFFUSE:		return "diffuse";
		case HF_UBER_BUMP:			return "bump";
		case HF_UBER_SPECULAR:		return "specular";
		case HF_UBER_VERTEXCOLOR:	return "vertexColor";
		case HF_UBER_ALPHATEST:		return "alphaTest";
		case HF_UBER_TEXGEN:		return "texgen";
		case HF_UBER_DIFFUSEMOD:	return "diffuseMod";
		case HF_UBER_LIGHTED:		return "lighted";
		case HF_UBER_PBR:			return "pbr";
		default:					return "unknown";
	}
}

bool IsCovered( const char * name ) {
	if ( name == NULL || name[0] == '\0' ) {
		return false;
	}
	for ( int i = 0; s_coveredPrograms[i] != NULL; i++ ) {
		if ( hfStr::Icmp( name, s_coveredPrograms[i] ) == 0 ) {
			return true;
		}
	}
	return false;
}

const char * const * CoveredList() {
	return s_coveredPrograms;
}

uint32 DefaultFeaturesFor( const char * name ) {
	if ( name == NULL || name[0] == '\0' ) {
		return 0;
	}
	for ( size_t i = 0; i < sizeof( s_defaultFeatures ) / sizeof( s_defaultFeatures[0] ); i++ ) {
		if ( hfStr::Icmp( name, s_defaultFeatures[i].name ) == 0 ) {
			return s_defaultFeatures[i].features;
		}
	}
	return 0;
}

uint32 FeaturesFromStage( bool hasDiffuse, bool vertexColorModulate, bool hasAlphaTest, bool screenTexgen ) {
	uint32 features = 0;
	if ( hasDiffuse ) {
		features |= HF_UBER_DIFFUSE;
	}
	if ( vertexColorModulate ) {
		features |= HF_UBER_VERTEXCOLOR;
	}
	if ( hasAlphaTest ) {
		features |= HF_UBER_ALPHATEST;
	}
	if ( screenTexgen ) {
		features |= HF_UBER_TEXGEN;
	}
	return features;
}

} // namespace HF_Uber