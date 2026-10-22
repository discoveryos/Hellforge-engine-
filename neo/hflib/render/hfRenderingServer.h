#ifndef __HF_RENDER_RENDERING_SERVER_H__
#define __HF_RENDER_RENDERING_SERVER_H__

// GPU-neutral "RenderingServer" in the Godot Servers sense: RID-handled
// resources (textures/materials/meshes) + a per-frame command builder that
// resolves the cull list + lights + environment into hfDrawCommand
// (one command == one draw call's uniforms). Ships a tiny CPU reference
// rasterizer so the whole path is verifiable headlessly.

#include "hfMaterial.h"
#include "../hfEcs.h"
#include "../scene/hfRenderCullSystem.h"
#include "../scene/hfLightComponent.h"

namespace HF_Render {

static const int HF_RENDER_MAX_TEXTURES	= 256;
static const int HF_RENDER_MAX_MATERIALS	= 256;
static const int HF_RENDER_MAX_DRAW			= 2048;
static const int HF_RENDER_MAX_LIGHTS		= 64;

struct hfRid {
	int		index;
	HF_INLINE hfRid() : index( -1 ) {}
	HF_INLINE bool Valid() const { return index >= 0; }
};

// One resolved draw call. The vec4 lanes match the uber shader §4 rpUserN
// uniforms 1:1 so the GL/Vulkan front-end can memcpy this struct to a UBO.
struct hfDrawCommand {
	HF_Ecs::Entity	entity;
	int				meshId;
	hfMat4			worldMatrix;
	hfVec3			worldCenter;
	float			distanceSq;

	// rpUser1 = (roughness, metallic, ao, emissiveScale)
	hfVec4			rpUser1;
	// rpUser4 = (ambient.rgb, ambientScale)
	hfVec4			rpUser4;
	hfVec4			baseColor;
	hfVec3			emissiveColor;

	hfRid			albedoTexture;
	hfRid			normalTexture;
	bool			noShadow;
	bool			transparent;
	bool			twoSided;
	bool			polygonOffset;

	float			fogFactor;		// per-command fog blend in [0,1]
	hfVec3			fogColor;
};

class hfRenderingServer {
public:
	hfRenderingServer()
		: m_texCount( 0 ), m_matCount( 0 ), m_cmdCount( 0 ) {}

	// ---- resource registry (RIDs) ----

	hfRid CreateTexture( unsigned int nameHash ) {
		hfRid r;
		if ( m_texCount >= HF_RENDER_MAX_TEXTURES ) { return r; }
		for ( int i = 0; i < m_texCount; i++ ) {
			if ( m_texHashes[i] == nameHash ) { r.index = i; return r; }
		}
		m_texHashes[ m_texCount ] = nameHash;
		r.index = m_texCount++;
		return r;
	}

	hfRid CreateMaterial( const hfPbrMaterial &m ) {
		hfRid r;
		if ( m_matCount >= HF_RENDER_MAX_MATERIALS ) { return r; }
		m_materials[ m_matCount ] = m;
		r.index = m_matCount++;
		return r;
	}

	const hfPbrMaterial &GetMaterial( const hfRid &r ) const {
		static const hfPbrMaterial kDefault;
		return ( r.index >= 0 && r.index < m_matCount ) ? m_materials[ r.index ] : kDefault;
	}

	int NumTextures() const { return m_texCount; }
	int NumMaterials() const { return m_matCount; }

	// ---- frame builder ----

	// Resolve a cull draw list + lights + environment into commands.
	// matLookup(materialId, ctx) -> hfRid; NULL falls back to slot 0.
	// texLookup(resourceId, ctx) -> texture hfRid; NULL leaves RIDs invalid.
	void BuildFrame( const HF_Scene::hfDrawEntry *draws, int drawCount,
					 const HF_Scene::hfWorldLight *lights, int lightCount,
					 const hfEnvironment &env,
					 hfRid ( *matLookup )( int materialId, void *ctx ),
					 void *ctx,
					 hfRid ( *texLookup )( int resourceId, void *texCtx ) = NULL,
					 void *texCtx = NULL ) {
		( void )lights;
		( void )lightCount;
		m_cmdCount = 0;
		for ( int i = 0; i < drawCount && i < HF_RENDER_MAX_DRAW; i++ ) {
			const HF_Scene::hfDrawEntry &e = draws[i];
			hfDrawCommand &c = m_cmds[ m_cmdCount ];
			c.entity			= e.entity;
			c.meshId			= e.meshId;
			c.worldMatrix		= e.worldMatrix;
			c.worldCenter		= e.worldBounds.GetCenter();
			c.distanceSq		= e.distanceSq;

			hfRid mat;
			if ( matLookup ) { mat = matLookup( e.materialId, ctx ); }
			const hfPbrMaterial &m = GetMaterial( mat );

			c.rpUser1			= hfVec4( m.roughness, m.metallic, m.ao, m.emissiveScale );
			c.rpUser4			= hfVec4( m.ambientColor.x, m.ambientColor.y, m.ambientColor.z, m.ambientScale );
			c.baseColor			= m.baseColor;
			c.emissiveColor		= m.emissiveColor;
			c.albedoTexture		= ( m.albedoTexId >= 0 && texLookup ) ? texLookup( m.albedoTexId, texCtx ) : hfRid();
			c.normalTexture		= ( m.normalTexId >= 0 && texLookup ) ? texLookup( m.normalTexId, texCtx ) : hfRid();
			c.noShadow			= m.noShadow;
			c.transparent		= m.transparent;
			c.twoSided			= m.twoSided;
			c.polygonOffset		= m.polygonOffset;

			float dist = sqrtf( c.distanceSq );
			c.fogFactor			= ComputeFogFactor( env.fog, dist );
			c.fogColor			= env.fog.color;
			m_cmdCount++;
		}
	}

	int GetCommandCount() const { return m_cmdCount; }
	const hfDrawCommand &GetCommand( int i ) const { return m_cmds[ i ]; }

	// ---- CPU reference rasterizer (headless validation) ----

	static const int RAST_W = 64;
	static const int RAST_H = 64;

	// Rasterize one command's material onto a flat-white triangle lit by a
	// single directional light; output is a tone-mapped sRGB uint8 buffer.
	// Vertex geometry: unit triangle centered on screen, plane facing +Z.
	void RasterReference( const hfDrawCommand &cmd,
						  const hfVec3 &lightDir, const hfVec3 &lightColor,
						  byte *rgb, int w, int h ) const;

private:
	unsigned int		m_texHashes[ HF_RENDER_MAX_TEXTURES ];
	int					m_texCount;
	hfPbrMaterial			m_materials[ HF_RENDER_MAX_MATERIALS ];
	int					m_matCount;
	hfDrawCommand		m_cmds[ HF_RENDER_MAX_DRAW ];
	int					m_cmdCount;
};

// ---- CPU rasterizer implementation ----

HF_INLINE void hfRenderingServer::RasterReference( const hfDrawCommand &cmd,
												   const hfVec3 &lightDir, const hfVec3 &lightColor,
												   byte *rgb, int w, int h ) const {
	// triangle in NDC
	const float vx[3] = { -0.8f, 0.8f, 0.0f };
	const float vy[3] = { -0.6f, -0.6f, 0.8f };
	const float vz[3] = { 0.0f, 0.0f, 0.0f };

	for ( int y = 0; y < h; y++ ) {
		for ( int x = 0; x < w; x++ ) {
			// ndc pixel center
			float px = ( 2.0f * ( x + 0.5f ) / w ) - 1.0f;
			float py = 1.0f - ( 2.0f * ( y + 0.5f ) / h );
			rgb[ ( y * w + x ) * 3 + 0 ] = 0;
			rgb[ ( y * w + x ) * 3 + 1 ] = 0;
			rgb[ ( y * w + x ) * 3 + 2 ] = 40;	// dark background

			// barycentric
			float e0 = ( vx[1] - vx[0] ) * ( py - vy[0] ) - ( vy[1] - vy[0] ) * ( px - vx[0] );
			float e1 = ( vx[2] - vx[1] ) * ( py - vy[1] ) - ( vy[2] - vy[1] ) * ( px - vx[1] );
			float e2 = ( vx[0] - vx[2] ) * ( py - vy[2] ) - ( vy[0] - vy[2] ) * ( px - vx[2] );
			const float area = ( vx[1] - vx[0] ) * ( vy[2] - vy[0] ) - ( vy[1] - vy[0] ) * ( vx[2] - vx[0] );
			bool inside = ( area >= 0.0f && e0 >= 0.0f && e1 >= 0.0f && e2 >= 0.0f )
				|| ( area <= 0.0f && e0 <= 0.0f && e1 <= 0.0f && e2 <= 0.0f );
			if ( !inside ) { continue; }

			// flat normal +Z; diffuse/specular
			hfVec3 N( 0.0f, 0.0f, 1.0f );
			hfVec3 L = lightDir;
			L.Normalize();
			float ndl = ( N * L ) > 0.0f ? ( N * L ) : 0.0f;
			hfVec3 albedo( cmd.baseColor.x, cmd.baseColor.y, cmd.baseColor.z );
			hfVec3 lit( albedo.x * lightColor.x * ndl, albedo.y * lightColor.y * ndl, albedo.z * lightColor.z * ndl );
			lit = lit + cmd.emissiveColor * cmd.rpUser1.w;
			// simple ambient: color * scale * base * (1 - metallic*0.5) * ao
			float ambGain = cmd.rpUser4.w * 0.2f * ( 1.0f - cmd.rpUser1.y * 0.5f ) * cmd.rpUser1.z;
			hfVec3 amb = cmd.rpUser4.ToVec3() * ambGain;
			lit.x += amb.x * albedo.x; lit.y += amb.y * albedo.y; lit.z += amb.z * albedo.z;
			// fog (linear blend in HDR, then tonemap)
			float f = cmd.fogFactor;
			lit.x = lit.x * ( 1.0f - f ) + cmd.fogColor.x * f;
			lit.y = lit.y * ( 1.0f - f ) + cmd.fogColor.y * f;
			lit.z = lit.z * ( 1.0f - f ) + cmd.fogColor.z * f;
			hfVec3 out = ACESTonemap( lit );
			rgb[ ( y * w + x ) * 3 + 0 ] = (byte)( out.x * 255.0f );
			rgb[ ( y * w + x ) * 3 + 1 ] = (byte)( out.y * 255.0f );
			rgb[ ( y * w + x ) * 3 + 2 ] = (byte)( out.z * 255.0f );
		}
	}
}

} // namespace HF_Render

#endif