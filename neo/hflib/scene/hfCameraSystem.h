#ifndef __HF_SCENE_CAMERA_SYSTEM_H__
#define __HF_SCENE_CAMERA_SYSTEM_H__

#include "../hfEcs.h"
#include "hfSystem.h"
#include "hfCameraComponent.h"
#include "hfTransformComponent.h"

// Refreshes each camera's view (world -> view) + projection (view -> clip)
// from the owning entity's transform.  Pure math, so it is fully headless.

namespace HF_Scene {

class hfWorld;

class hfCameraSystem : public hfSystem {
public:
	HF_INLINE hfCameraSystem() : m_registry( NULL ) {}

	HF_INLINE virtual const char *GetName() const { return "hfCamera"; }
	HF_INLINE virtual int GetPriority() const { return 20; }

	HF_INLINE virtual void OnRegister( HF_Ecs::Registry &registry ) { m_registry = &registry; }

	HF_INLINE virtual void Update( HF_Ecs::Registry &registry, float dt ) {
		(void)dt; (void)registry;
		registry.Each< hfCameraComponent >(
			[ this ]( HF_Ecs::Entity e, hfCameraComponent & cam ) {
				hfMat4 camWorld;
				hfTransformComponent *t = m_registry->GetComponent< hfTransformComponent >( e );
				if ( t ) { camWorld = t->worldMatrix; }
				cam.viewMatrix = camWorld.InverseFast();
				cam.ComputeProjection();
			} );
	}

	// World point -> NDC via the camera's current matrices.
	static HF_INLINE hfVec3 ProjectToClip( const hfCameraComponent &cam, const hfVec3 &world ) {
		hfVec4 v = cam.viewMatrix * hfVec4( world.x, world.y, world.z, 1.0f );
		hfVec4 c = cam.projectionMatrix * v;
		if ( c.w == 0.0f ) { return hfVec3( 0.0f, 0.0f, 0.0f ); }
		return hfVec3( c.x / c.w, c.y / c.w, c.z / c.w );
	}

private:
	HF_Ecs::Registry *		m_registry;
};

} // namespace HF_Scene

#endif