#ifndef __HF_SCENE_LIGHT_COMPONENT_H__
#define __HF_SCENE_LIGHT_COMPONENT_H__

// GPU-neutral light component. The LightSystem resolves each light's
// world-space state (position / direction / radius) from its entity
// transform every frame and hands it to consumers — the CPU light-cluster
// front-end in renderer/hfLightCluster.h, or the deferred/cube remappers.

namespace HF_Scene {

enum hfLightType {
	HF_LIGHT_POINT = 0,
	HF_LIGHT_SPOT,
	HF_LIGHT_DIRECTIONAL
};

struct hfLightComponent {
	hfLightType		type;
	hfVec3			color;			// linear color
	float			intensity;		// brightness multiplier (also the cluster sort key)
	float			range;			// point/spot world-space influence radius
	float			spotInnerDeg;	// spot inner cone half-angle, degrees
	float			spotOuterDeg;	// spot outer cone half-angle, degrees
	bool			castsShadow;
	bool			enabled;

	hfVec3			direction;		// world-space, resolved by LightSystem (unused for point)

	HF_INLINE hfLightComponent() {
		type = HF_LIGHT_POINT;
		color = hfVec3( 1.0f, 1.0f, 1.0f );
		intensity = 1.0f;
		range = 10.0f;
		spotInnerDeg = 20.0f;
		spotOuterDeg = 30.0f;
		castsShadow = true;
		enabled = true;
		direction = hfVec3( 0.0f, 0.0f, -1.0f );
	}
};

// Frame-resolved world-space light. Convertible to HF_Cluster::LightEntry
// (position/radius/index/priority) for the tiled front-end.
struct hfWorldLight {
	hfVec3			position;
	hfVec3			direction;
	hfVec3			color;
	float			range;
	float			priority;
	bool			castsShadow;
};

} // namespace HF_Scene

#endif