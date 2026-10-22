#ifndef __HF_RENDER_MATERIAL_H__
#define __HF_RENDER_MATERIAL_H__

// CPU-side PBR material. Every field maps 1:1 to a GLSL uniform
// (rpUser1, rpUser4) documented in GLSL_uber_shaders.md §4.

#include "../hfStr.h"
#include "../hfAssetStore.h"
#include "../math/hfVector.h"

namespace HF_Render {

struct hfPbrMaterial {
	hfVec4	baseColor;			// linear RGBA
	float	metallic;			// 0..1
	float	roughness;			// 0..1
	float	ao;					// ambient occlusion 0..1
	float	emissiveScale;		// multiplier for emissiveColor
	hfVec3	emissiveColor;		// linear RGB
	hfVec3	ambientColor;		// ambient tint RGB   (GLSL: rpUser4.rgb)
	float	ambientScale;		// ambient multiplier (GLSL: rpUser4.w)
	int		albedoTexId;		// resource id (-1 = flat white)
	int		normalTexId;		// resource id (-1 = flat up)
	bool	noShadow;
	bool	transparent;		// blend alpha or blend
	bool	twoSided;
	bool	polygonOffset;

	HF_INLINE hfPbrMaterial() {
		baseColor		= hfVec4( 1.0f, 1.0f, 1.0f, 1.0f );
		metallic		= 0.0f;
		roughness		= 0.55f;
		ao				= 1.0f;
		emissiveScale	= 0.0f;
		emissiveColor	= hfVec3( 0.0f, 0.0f, 0.0f );
		ambientColor	= hfVec3( 0.55f, 0.55f, 0.6f );
		ambientScale	= 1.0f;
		albedoTexId		= -1;
		normalTexId		= -1;
		noShadow		= false;
		transparent		= false;
		twoSided		= false;
		polygonOffset	= false;
	}
};

// Build a hfPbrMaterial from a parsed .mtr definition.
// texLookup( name, ctx ) returns a resource id for a texture name, or -1.
HF_INLINE bool BuildMaterial( const HF_Asset::mtrDef_t &def,
							   hfPbrMaterial &out,
							   int ( *texLookup )( const char *name, void *ctx ),
							   void *ctx = NULL ) {
	out = hfPbrMaterial();
	if ( !def.parseOk ) { return false; }
	out.noShadow	= def.noShadows;
	out.polygonOffset = def.polygonOffset;

	// stage 0 = albedo
	if ( def.numStages > 0 && def.stages[0].textureName.Length() > 0 ) {
		if ( texLookup ) {
			out.albedoTexId = texLookup( def.stages[0].textureName.c_str(), ctx );
		}
		int b = def.stages[0].blend;
		out.transparent = ( b == HF_Asset::MBLEND_ALPHA || b == HF_Asset::MBLEND_BLEND );
	}

	// stage 1+ with texGen "normalmap" or "normal"
	for ( int s = 1; s < def.numStages && s < 8; s++ ) {
		const HF_Asset::mtrStage_t &st = def.stages[s];
		if ( st.texGen.Length() == 0 ) { continue; }
		if ( st.texGen.Icmp( "normalmap" ) == 0 || st.texGen.Icmp( "normal" ) == 0 ) {
			if ( texLookup && st.textureName.Length() > 0 ) {
				out.normalTexId = texLookup( st.textureName.c_str(), ctx );
			}
			break;
		}
	}
	return true;
}

// ---- Fog ----

enum hfFogMode { HF_FOG_NONE = 0, HF_FOG_LINEAR, HF_FOG_EXP2 };

struct hfFogParams {
	hfFogMode	mode;
	hfVec3		color;
	float		density;		// exp2
	float		start, end;		// linear

	HF_INLINE hfFogParams() {
		mode	= HF_FOG_NONE;
		color	= hfVec3( 0.7f, 0.8f, 0.9f );
		density	= 0.01f;
		start	= 10.0f;
		end		= 100.0f;
	}
};

// Returns fog blend factor in [0,1]: 0 = clear, 1 = fully fogged.
HF_INLINE float ComputeFogFactor( const hfFogParams &fog, float dist ) {
	switch ( fog.mode ) {
	case HF_FOG_LINEAR: {
		float t = ( dist - fog.start ) / ( fog.end - fog.start );
		return t < 0.0f ? 0.0f : ( t > 1.0f ? 1.0f : t );
	}
	case HF_FOG_EXP2: {
		float d = fog.density * dist;
		return 1.0f - expf( -2.0f * d * d * 0.6931472f );
	}
	default: return 0.0f;
	}
}

// ---- Tone mapping (ACES filmic, matches GLSL doc §4 compute post) ----

HF_INLINE hfVec3 ACESFilmic( const hfVec3 &x ) {
	const float a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
	return hfVec3(
		( x.x * ( a * x.x + b ) ) / ( x.x * ( c * x.x + d ) + e ),
		( x.y * ( a * x.y + b ) ) / ( x.y * ( c * x.y + d ) + e ),
		( x.z * ( a * x.z + b ) ) / ( x.z * ( c * x.z + d ) + e )
	);
}

HF_INLINE hfVec3 ACESTonemap( const hfVec3 &hdr ) {
	hfVec3 c = ACESFilmic( hdr );
	return hfVec3(
		c.x < 0.0f ? 0.0f : ( c.x > 1.0f ? 1.0f : c.x ),
		c.y < 0.0f ? 0.0f : ( c.y > 1.0f ? 1.0f : c.y ),
		c.z < 0.0f ? 0.0f : ( c.z > 1.0f ? 1.0f : c.z )
	);
}

// ---- Environment (global frame knobs) ----

struct hfEnvironment {
	float		exposure;
	hfVec3		ambientColor;
	float		ambientScale;
	hfFogParams	fog;

	HF_INLINE hfEnvironment() {
		exposure		= 1.0f;
		ambientColor	= hfVec3( 0.55f, 0.55f, 0.6f );
		ambientScale	= 1.0f;
	}
};

} // namespace HF_Render

#endif
