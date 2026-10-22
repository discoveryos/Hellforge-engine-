#ifndef __HF_SCENE_LIGHT_SYSTEM_H__
#define __HF_SCENE_LIGHT_SYSTEM_H__

#include "hfSystem.h"
#include "hfLightComponent.h"
#include "hfTransformComponent.h"

namespace HF_Scene {

static const int HF_MAX_WORLD_LIGHTS = 256;

// Resolves per-frame world-space light state for every enabled light
// entity. Points inherit position; spots/directionals inherit both position
// and the entity's world forward (a light looks down its own -Z, matching
// camera convention). Runs after the transform system.
class hfLightSystem : public hfSystem {
public:
	enum { SYSTEM_PRIORITY = 15 };

	hfLightSystem()
		: m_lightCount( 0 ) {}

	const char *GetName() const override { return "LightSystem"; }
	int GetPriority() const override { return SYSTEM_PRIORITY; }

	void OnRegister( HF_Ecs::Registry &registry ) override {
		( void )registry;
		m_lightCount = 0;
	}

	void Update( HF_Ecs::Registry &registry, float dt ) override {
		( void )dt;
		m_lightCount = 0;
		registry.Each< hfLightComponent >(
			[ this, &registry ]( HF_Ecs::Entity ent, hfLightComponent &light ) {
				( void )ent;
				if ( !light.enabled || m_lightCount >= HF_MAX_WORLD_LIGHTS ) { return; }
				hfTransformComponent *t = registry.GetComponent< hfTransformComponent >( ent );
				hfWorldLight &wl = m_lights[ m_lightCount ];
				wl.position = t ? t->WorldPosition() : hfVec3( 0.0f, 0.0f, 0.0f );
				wl.direction = t ? t->TransformDirection( hfVec3( 0.0f, 0.0f, -1.0f ) )
								 : hfVec3( 0.0f, 0.0f, -1.0f );
				wl.color = light.color * light.intensity;
				wl.range = light.range;
				wl.priority = light.intensity;
				wl.castsShadow = light.castsShadow;
				light.direction = wl.direction;
				m_lightCount++;
			} );
	}

	int NumLights() const { return m_lightCount; }
	const hfWorldLight &GetLight( int i ) const { return m_lights[ i ]; }

private:
	hfWorldLight		m_lights[ HF_MAX_WORLD_LIGHTS ];
	int					m_lightCount;
};

} // namespace HF_Scene

#endif