#ifndef __HF_PHYSICS_RIGID_BODY_COMPONENT_H__
#define __HF_PHYSICS_RIGID_BODY_COMPONENT_H__

#include "../math/hfVector.h"

// Game-facing rigid body (modern-engine conventions): static / dynamic /
// kinematic types, sphere or box colliders, a material block (mass /
// restitution / friction / gravity scale), trigger volumes, collision
// layers (layer x mask) and sleeping. The PhysicsSystem integrates and
// resolves these every fixed step. Mirrors the role of Jolt/Bullet's
// RigidBody when a native back-end eventually replaces it.

namespace HF_Physics {

enum hfBodyType {
	HF_BT_STATIC = 0,	// never moves, still collides
	HF_BT_DYNAMIC,		// gravity + impulses
	HF_BT_KINEMATIC		// moved by the caller, pushes dynamic bodies
};

enum hfShapeType {
	HF_SHAPE_SPHERE = 0,
	HF_SHAPE_BOX
};

struct hfRigidBodyComponent {
	hfBodyType		type;
	hfShapeType		shape;

	float			radius;			// sphere collider
	hfVec3			halfExtents;	// box collider
	hfVec3			offset;			// collider volume offset from the entity origin (local frame)

	float			mass;			// dynamic mass (kg)
	float			invMass;		// derived: 0 = infinite mass
	float			restitution;	// bounciness [0..1]; a pair uses the max
	float			friction;		// tangential damping [0..1]; a pair uses the max
	float			gravityScale;	// 0 disables gravity for this body
	bool			isTrigger;		// overlap-only: records events, no response
	unsigned int	layer;			// collision group
	unsigned int	mask;			// groups this body collides with (bit test)

	// dynamic state owned by the solver
	hfVec3			velocity;		// world space, m/s
	hfVec3			angularVelocity;// world space, rad/s
	bool			asleep;
	float			sleepTimer;

	HF_INLINE hfRigidBodyComponent() {
		type = HF_BT_DYNAMIC;
		shape = HF_SHAPE_SPHERE;
		radius = 0.5f;
		halfExtents = hfVec3( 0.5f, 0.5f, 0.5f );
		offset = hfVec3( 0.0f, 0.0f, 0.0f );
		mass = 1.0f;
		invMass = 1.0f;
		restitution = 0.3f;
		friction = 0.3f;
		gravityScale = 1.0f;
		isTrigger = false;
		layer = 1u;
		mask = 0xFFFFFFFFu;
		velocity = hfVec3( 0.0f, 0.0f, 0.0f );
		angularVelocity = hfVec3( 0.0f, 0.0f, 0.0f );
		asleep = false;
		sleepTimer = 0.0f;
	}

	// Call after changing type/mass; static + kinematic are infinite-mass.
	HF_INLINE void RefreshMass() {
		invMass = ( type == HF_BT_DYNAMIC && mass > 0.0f ) ? ( 1.0f / mass ) : 0.0f;
	}
};

} // namespace HF_Physics

#endif