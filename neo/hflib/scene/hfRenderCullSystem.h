#ifndef __HF_SCENE_RENDER_CULL_SYSTEM_H__
#define __HF_SCENE_RENDER_CULL_SYSTEM_H__

#include "hfSystem.h"
#include "hfMeshRendererComponent.h"
#include "hfTransformComponent.h"
#include "hfCameraComponent.h"
#include "../bv/hfCollide.h"

namespace HF_Scene {

static const int HF_MAX_DRAW = 2048;

// GPU-neutral result of per-frame frustum culling. Fills an ordered draw
// list (camera-agnostic order, one entry per visible mesh renderer) that a
// GL/Vulkan front-end walks to issue draw calls.
struct hfDrawEntry {
	HF_Ecs::Entity	entity;
	int				meshId;
	int				materialId;
	hfMat4			worldMatrix;
	hfBounds		worldBounds;
	float			distanceSq;		// from camera, for back-to-front sorting
};

class hfRenderCullSystem : public hfSystem {
public:
	enum { SYSTEM_PRIORITY = 30 };

	hfRenderCullSystem()
		: m_cameraEntity( HF_Ecs::ENTITY_NULL ), m_drawCount( 0 ) {}

	const char *GetName() const override { return "RenderCullSystem"; }
	int GetPriority() const override { return SYSTEM_PRIORITY; }

	void OnRegister( HF_Ecs::Registry &registry ) override {
		( void )registry;
		m_cameraEntity = HF_Ecs::ENTITY_NULL;
		m_drawCount = 0;
	}

	// World-space AABB of a model-space bounds under an arbitrary TRS matrix.
	static HF_INLINE void TransformBounds( const hfBounds &b, const hfMat4 &m, hfBounds &out ) {
		hfVec3 pts[ 8 ];
		for ( int i = 0; i < 8; i++ ) {
			hfVec3 p(
				b[ ( i & 1 ) ? 1 : 0 ].x,
				b[ ( i & 2 ) ? 1 : 0 ].y,
				b[ ( i & 4 ) ? 1 : 0 ].z );
			pts[ i ] = m * p;
		}
		out.FromPoints( pts, 8 );
	}

	void Update( HF_Ecs::Registry &registry, float dt ) override {
		( void )dt;
		m_drawCount = 0;
		m_cameraEntity = HF_Ecs::ENTITY_NULL;

		// The first camera in entity order is the active one this frame.
		registry.Each< hfCameraComponent >(
			[ this ]( HF_Ecs::Entity ent, hfCameraComponent &cam ) {
				( void )cam;
				if ( m_cameraEntity == HF_Ecs::ENTITY_NULL ) { m_cameraEntity = ent; }
			} );
		if ( m_cameraEntity == HF_Ecs::ENTITY_NULL ) { return; }

		hfCameraComponent *cam = registry.GetComponent< hfCameraComponent >( m_cameraEntity );
		if ( !cam ) { return; }
		// clip = projection * view; the frustum is extracted from that product.
		hfMat4 combined = cam->projectionMatrix * cam->viewMatrix;
		hfFrustum frustum;
		frustum.FromViewProjMatrix( combined );

		hfTransformComponent *camT = registry.GetComponent< hfTransformComponent >( m_cameraEntity );
		hfVec3 camPos = camT ? camT->WorldPosition() : hfVec3( 0, 0, 0 );

		registry.Each< hfMeshRendererComponent >(
			[ this, &registry, &frustum, &camPos ]( HF_Ecs::Entity ent, hfMeshRendererComponent &mr ) {
				if ( !mr.visible || mr.meshId < 0 || m_drawCount >= HF_MAX_DRAW ) { return; }
				hfTransformComponent *t = registry.GetComponent< hfTransformComponent >( ent );
				hfDrawEntry &e = m_draw[ m_drawCount ];
				e.entity = ent;
				e.meshId = mr.meshId;
				e.materialId = mr.materialId;
				e.worldMatrix.Identity();
				e.worldBounds = mr.localBounds;
				if ( t ) {
					e.worldMatrix = t->worldMatrix;
					TransformBounds( mr.localBounds, t->worldMatrix, e.worldBounds );
				}
				hfVec3 center = e.worldBounds.GetCenter();
				e.distanceSq = ( center - camPos ).LengthSqr();
				if ( !hfCollide::TestBoundsFrustum( e.worldBounds, frustum ) ) {
					return;	// outside the view volume
				}
				m_drawCount++;
			} );
	}

	int GetDrawCount() const { return m_drawCount; }
	const hfDrawEntry &GetDraw( int i ) const { return m_draw[ i ]; }
	HF_Ecs::Entity GetCameraEntity() const { return m_cameraEntity; }

private:
	HF_Ecs::Entity		m_cameraEntity;
	hfDrawEntry			m_draw[ HF_MAX_DRAW ];
	int					m_drawCount;
};

} // namespace HF_Scene

#endif