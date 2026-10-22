#ifndef __HF_JOLT_BACKEND_H__
#define __HF_JOLT_BACKEND_H__

/*
================================

Jolt physics backend

hfPhysicsSystem ships with a small built-in impulse solver so the harness can
run headless with no dependencies at all. This header adds the option to run
the same contract through Jolt instead (vendored under `external/jolt`, MIT),
behind one switch:

    -DHF_PHYSICS_HAS_JOLT=1 -Iexternal/jolt

Nothing outside this header changes. The game-facing pieces keep their
meaning:

  - bodies come from `hfRigidBodyComponent` (sphere or box, local offset,
    material, mass, gravity scale, trigger flag)
  - static / dynamic / kinematic map onto Jolt motion types
  - the `layer` / `mask` bit test keeps working: a layer becomes a Jolt object
    layer and a pair filter runs the engine's own
    `(maskA & layerB) && (maskB & layerA)` rule
  - triggers become Jolt sensors and the enter / stay / exit stream is
    reported in the same order and shape
  - transforms, velocities and sleeping sync both ways

`hfPhysicsSystem` picks the backend at construction and falls back to the
built-in solver whenever Jolt is not part of the build, so the same binary
shape compiles either way.

Jolt is stepped on a single-threaded job system on purpose: the harness needs
determinism (same input, same output) and a physics island solver that is free
to use every core would make results depend on the thread schedule.
================================
*/

#include "../scene/hfSystem.h"
#include "../scene/hfTransformComponent.h"
#include "../math/hfQuat.h"
#include "hfRigidBodyComponent.h"
#include "hfBroadphase.h"
#include "hfPhysicsLimits.h"

#if !defined( HF_PHYSICS_HAS_JOLT )
	#define HF_PHYSICS_HAS_JOLT 0
#endif

#if HF_PHYSICS_HAS_JOLT

// pins the compile time configuration the vendored library was built with, so
// it has to come before any Jolt header
#include "hfJoltConfig.h"

#include <Jolt/Jolt.h>

#include <Jolt/Core/Core.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h>
#include <Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/EActivation.h>
#include <Jolt/RegisterTypes.h>

#include <stdarg.h>
#include <stdio.h>
#include <map>
#include <utility>

// Jolt pulls in <cfloat>/<cmath>, which define FLT_EPSILON and INFINITY as
// macros. The engine deliberately undefines both and declares its own constants
// under the same names (hfMath::FLT_EPSILON, hfMath::INFINITY), so a
// translation unit that reaches Jolt through this header would break on its own
// math. Every Jolt header is parsed by this point, so taking the macros back
// out cannot hurt Jolt.
#ifdef FLT_EPSILON
	#undef FLT_EPSILON
#endif
#ifdef INFINITY
	#undef INFINITY
#endif

namespace HF_Physics {

// how many distinct collision groups the engine's `layer` field can address
static const uint32 HF_JOLT_NUM_OBJECT_LAYERS	= 32;
static const uint32 HF_JOLT_NUM_BP_LAYERS		= 2;
static const uint32 HF_JOLT_BP_MOVING			= 0;
static const uint32 HF_JOLT_BP_STATIC			= 1;

/*
The engine asks "do these two bodies collide" with a per-body bitmask, while
Jolt only hands the filter two object layers. Layers are numbered, so keeping a
mask per layer reproduces the original rule exactly:
	(maskA & layerB) && (maskB & layerA)
================================
*/
class hfJoltLayerFilter : public JPH::ObjectLayerPairFilter {
public:
	HF_INLINE hfJoltLayerFilter() {
		for ( int i = 0; i < (int)HF_JOLT_NUM_OBJECT_LAYERS; i++ ) { mMasks[ i ] = 0xFFFFFFFFu; }
	}
	HF_INLINE void SetMask( uint32 layer, uint32 mask ) { mMasks[ layer & 31u ] = mask; }
	HF_INLINE uint32 GetMask( uint32 layer ) const { return mMasks[ layer & 31u ]; }

	virtual bool ShouldCollide( JPH::ObjectLayer a, JPH::ObjectLayer b ) const override {
		uint32 la = (uint32)a & 31u;
		uint32 lb = (uint32)b & 31u;
		return ( mMasks[ la ] & ( 1u << lb ) ) && ( mMasks[ lb ] & ( 1u << la ) );
	}

private:
	uint32 mMasks[ HF_JOLT_NUM_OBJECT_LAYERS ];
};

/*
Sensor contacts are the only thing the game reads out of the narrowphase, so
this listener turns them into enter / stay / exit events and applies the
engine's "a pair uses the larger of the two materials" rule. Ordinary contacts
are left entirely to Jolt's solver.
================================
*/
struct hfJoltOverlap {
	HF_Ecs::Entity	a;
	HF_Ecs::Entity	b;
	int				state;		// 1 = enter, 0 = stay, -1 = exit
};

class hfJoltContactListener : public JPH::ContactListener {
public:
	HF_INLINE hfJoltContactListener() : m_count( 0 ) {}

	// body id -> what the listener needs to know about that body
	struct BodyRef {
		HF_Ecs::Entity	entity;
		bool			isTrigger;
		float			restitution;
		float			friction;
	};

	HF_INLINE void RegisterBody( uint32 bodyId, HF_Ecs::Entity entity, bool isTrigger,
								 float restitution, float friction ) {
		BodyRef r;
		r.entity = entity;
		r.isTrigger = isTrigger;
		r.restitution = restitution;
		r.friction = friction;
		mBodies[ bodyId ] = r;
	}
	HF_INLINE void ForgetBody( uint32 bodyId ) { mBodies.erase( bodyId ); }

	HF_INLINE void Begin() { m_count = 0; }

	HF_INLINE int Count() const { return m_count; }
	HF_INLINE const hfJoltOverlap &Get( int i ) const { return m_overlaps[ i ]; }

	virtual void OnContactAdded( const JPH::Body &b1, const JPH::Body &b2, const JPH::ContactManifold &,
								 JPH::ContactSettings &settings ) override {
		Record( b1, b2, settings, 1 );
	}
	virtual void OnContactPersisted( const JPH::Body &b1, const JPH::Body &b2, const JPH::ContactManifold &,
									  JPH::ContactSettings &settings ) override {
		Record( b1, b2, settings, 0 );
	}
	virtual void OnContactRemoved( const JPH::SubShapeIDPair &pair ) override {
		uint32 id1 = pair.GetBody1ID().GetIndexAndSequenceNumber();
		uint32 id2 = pair.GetBody2ID().GetIndexAndSequenceNumber();
		std::map<uint32, BodyRef>::const_iterator it1 = mBodies.find( id1 );
		std::map<uint32, BodyRef>::const_iterator it2 = mBodies.find( id2 );
		if ( it1 == mBodies.end() || it2 == mBodies.end() ) { return; }
		Push( it1->second.entity, it2->second.entity, -1 );
	}

private:
	HF_INLINE void Record( const JPH::Body &b1, const JPH::Body &b2, JPH::ContactSettings &settings, int state ) {
		uint32 id1 = b1.GetID().GetIndexAndSequenceNumber();
		uint32 id2 = b2.GetID().GetIndexAndSequenceNumber();
		std::map<uint32, BodyRef>::const_iterator it1 = mBodies.find( id1 );
		std::map<uint32, BodyRef>::const_iterator it2 = mBodies.find( id2 );
		if ( it1 == mBodies.end() || it2 == mBodies.end() ) { return; }
		// only sensor pairs are reported as events
		if ( !it1->second.isTrigger && !it2->second.isTrigger ) { return; }

		// engine rule: a contact pair uses the larger of the two materials
		settings.mCombinedRestitution = it1->second.restitution > it2->second.restitution ?
									   it1->second.restitution : it2->second.restitution;
		settings.mCombinedFriction = it1->second.friction > it2->second.friction ?
									  it1->second.friction : it2->second.friction;

		Push( it1->second.entity, it2->second.entity, state );
	}

	HF_INLINE void Push( HF_Ecs::Entity a, HF_Ecs::Entity b, int state ) {
		if ( m_count >= HF_MAX_OVERLAP_EVENTS ) { return; }
		// stable ordering: the built-in solver reports the lower entity id first
		if ( (uint32)b < (uint32)a ) { HF_Ecs::Entity t = a; a = b; b = t; }
		m_overlaps[ m_count ].a = a;
		m_overlaps[ m_count ].b = b;
		m_overlaps[ m_count ].state = state;
		m_count++;
	}

	std::map<uint32, BodyRef>	mBodies;
	hfJoltOverlap				m_overlaps[ HF_MAX_OVERLAP_EVENTS ];
	int							m_count;
};

class hfJoltBackend {
public:
	HF_INLINE static int GetLayerCount() { return (int)HF_JOLT_NUM_OBJECT_LAYERS; }

	HF_INLINE hfJoltBackend() { Init(); }

	HF_INLINE ~hfJoltBackend() { Shutdown(); }

	HF_INLINE bool IsReady() const { return m_system != NULL; }

	// The default allocator, the shape factory and the type registration are
	// process wide, so they are installed once and never torn down: a second
	// backend (or the built-in one) must still be able to run afterwards.
	HF_INLINE static void RegisterOnce() {
		if ( s_registered ) { return; }
		JPH::RegisterDefaultAllocator();
		JPH::Trace = JoltTrace;
		JPH::Factory::sInstance = new JPH::Factory();
		JPH::RegisterTypes();
		s_registered = true;
	}

	HF_INLINE void Init() {
		if ( m_system ) { return; }
		RegisterOnce();

		m_layerFilter = new hfJoltLayerFilter();
		m_bpTable = new JPH::BroadPhaseLayerInterfaceTable( HF_JOLT_NUM_OBJECT_LAYERS, HF_JOLT_NUM_BP_LAYERS );
		for ( uint32 i = 0; i < HF_JOLT_NUM_OBJECT_LAYERS; i++ ) {
			m_bpTable->MapObjectToBroadPhaseLayer( JPH::ObjectLayer( i ), JPH::BroadPhaseLayer( i == 0 ? 0 : 1 ) );
		}
		m_ovbTable = new JPH::ObjectVsBroadPhaseLayerFilterTable(
			*m_bpTable, HF_JOLT_NUM_BP_LAYERS, *m_layerFilter, HF_JOLT_NUM_OBJECT_LAYERS );
		m_temp = new JPH::TempAllocatorImpl( 8 * 1024 * 1024 );
		m_jobs = new JPH::JobSystemSingleThreaded( JPH::cMaxPhysicsJobs );

		m_system = new JPH::PhysicsSystem();
		m_system->Init( HF_MAX_PHYSICS_BODIES, 0, HF_MAX_PHYSICS_BODIES * 2, 2048,
						*m_bpTable, *m_ovbTable, *m_layerFilter );
		m_bodyInterface = &m_system->GetBodyInterface();
		m_system->SetContactListener( &m_listener );
		m_system->SetGravity( JPH::Vec3( 0.0f, 0.0f, -9.81f ) );
	}

	HF_INLINE void Shutdown() {
		if ( !m_system ) { return; }
		for ( std::map<uint32, Entry>::iterator it = m_bodies.begin(); it != m_bodies.end(); ++it ) {
			m_bodyInterface->RemoveBody( it->second.id );
			m_bodyInterface->DestroyBody( it->second.id );
			m_listener.ForgetBody( it->second.id.GetIndexAndSequenceNumber() );
		}
		m_bodies.clear();
		m_system->SetContactListener( NULL );
		delete m_system;
		m_system = NULL;
		m_bodyInterface = NULL;
		delete m_jobs;			m_jobs = NULL;
		delete m_temp;			m_temp = NULL;
		delete m_ovbTable;		m_ovbTable = NULL;
		delete m_bpTable;		m_bpTable = NULL;
		delete m_layerFilter;	m_layerFilter = NULL;
	}

	HF_INLINE void SetGravity( const hfVec3 &g ) { m_system->SetGravity( JPH::Vec3( g.x, g.y, g.z ) ); }

	// ---- one fixed step ------------------------------------------------
	//
	// A body is created in Jolt the first time the step sees it, then kept in
	// sync: the engine keeps writing velocities and, for kinematic bodies,
	// target positions. After the step the simulated state is written back.
	HF_INLINE void Step( HF_Ecs::Registry &registry, float dt ) {
		m_listener.Begin();

		HF_Ecs::Entity seen[ HF_MAX_PHYSICS_BODIES ];
		int numSeen = 0;

		registry.Each< hfRigidBodyComponent >(
			[ & ]( HF_Ecs::Entity ent, hfRigidBodyComponent &rb ) {
				if ( numSeen >= HF_MAX_PHYSICS_BODIES ) { return; }
				HF_Scene::hfTransformComponent *t = registry.GetComponent< HF_Scene::hfTransformComponent >( ent );
				if ( !t ) { return; }
				rb.RefreshMass();
				seen[ numSeen++ ] = ent;
				SyncBody( ent, rb, *t, dt );
			} );

		m_system->Update( dt, 1, m_temp, m_jobs );

		for ( int i = 0; i < numSeen; i++ ) {
			HF_Scene::hfTransformComponent *t = registry.GetComponent< HF_Scene::hfTransformComponent >( seen[ i ] );
			hfRigidBodyComponent *rb = registry.GetComponent< hfRigidBodyComponent >( seen[ i ] );
			if ( !t || !rb ) { continue; }
			FetchBody( seen[ i ], *rb, *t );
		}
	}

	// ---- overlap events -------------------------------------------------
	HF_INLINE int NumEvents() const { return m_listener.Count(); }
	HF_INLINE const hfJoltOverlap *Events() const { return &m_listener.Get( 0 ); }

private:
	struct Entry {
		JPH::BodyID		id;
		HF_Ecs::Entity	entity;
		uint32			layer;
		bool			isTrigger;
		bool			asleep;
		float			offsetX, offsetY, offsetZ;
	};

	HF_INLINE static JPH::Vec3 ToJolt( const hfVec3 &v ) { return JPH::Vec3( v.x, v.y, v.z ); }
	HF_INLINE static JPH::Quat ToJolt( const hfQuat &q ) { return JPH::Quat( q.x, q.y, q.z, q.w ); }
	HF_INLINE static void JoltTrace( const char *fmt, ... ) { ( void )fmt; }

	// Jolt positions a body at its center of mass, the engine at its entity
	// origin, and `offset` moves the collider away from the origin, so the two
	// are related by that single rotation.
	HF_INLINE void SyncBody( HF_Ecs::Entity ent, hfRigidBodyComponent &rb,
							 HF_Scene::hfTransformComponent &t, float dt ) {
		uint32 layer = (uint32)rb.layer & 31u;
		m_layerFilter->SetMask( layer, rb.mask );

		JPH::EMotionType motion = JPH::EMotionType::Dynamic;
		if ( rb.type == HF_BT_STATIC ) { motion = JPH::EMotionType::Static; }
		else if ( rb.type == HF_BT_KINEMATIC ) { motion = JPH::EMotionType::Kinematic; }

		const hfVec3 origin = t.WorldPosition();
		const hfQuat rot = t.rotation;
		const hfVec3 com = origin + rot * rb.offset;

		std::map<uint32, Entry>::iterator it = m_bodies.find( (uint32)ent );
		if ( it != m_bodies.end() ) {
			Entry &e = it->second;
			if ( e.isTrigger != rb.isTrigger ) {
				e.isTrigger = rb.isTrigger;
				m_bodyInterface->SetMotionType( e.id, motion, JPH::EActivation::Activate );
			}
			if ( motion == JPH::EMotionType::Kinematic ) {
				hfVec3 target = origin + rb.velocity * dt;
				m_bodyInterface->MoveKinematic( e.id, ToJolt( target + rot * rb.offset ), ToJolt( rot ), dt );
			} else if ( motion == JPH::EMotionType::Dynamic ) {
				if ( rb.asleep ) {
					m_bodyInterface->DeactivateBody( e.id );
					e.asleep = true;
				} else {
					// pushing the state in is what wakes a sleeping body, and it
					// only happens when the game actually moved it
					m_bodyInterface->SetPositionAndRotation( e.id, ToJolt( com ), ToJolt( rot ), JPH::EActivation::Activate );
					m_bodyInterface->SetLinearVelocity( e.id, ToJolt( rb.velocity ) );
					m_bodyInterface->SetAngularVelocity( e.id, ToJolt( rb.angularVelocity ) );
					e.asleep = false;
				}
			}
			return;
		}

		// ---- new body ----
		JPH::ShapeSettings::ShapeResult shape;
		if ( rb.shape == HF_SHAPE_SPHERE ) {
			shape = JPH::SphereShapeSettings( rb.radius ).Create();
		} else {
			shape = JPH::BoxShapeSettings( ToJolt( rb.halfExtents ) ).Create();
		}
		if ( shape.HasError() ) { return; }

		JPH::BodyCreationSettings settings( shape.Get(), ToJolt( com ), ToJolt( rot ),
											motion, JPH::ObjectLayer( layer ) );
		settings.mGravityFactor = rb.gravityScale;
		settings.mRestitution = rb.restitution;
		settings.mFriction = rb.friction;
		settings.mIsSensor = rb.isTrigger;
		settings.mAllowSleeping = true;
		// the component is the authority on the state a body starts in, and the
		// built-in solver starts a body with the velocity the game gave it
		settings.mLinearVelocity = ToJolt( rb.velocity );
		settings.mAngularVelocity = ToJolt( rb.angularVelocity );
		// the built-in solver applies no damping, so neither does this one:
		// the two backends stay comparable in the parity tests
		settings.mLinearDamping = 0.0f;
		settings.mAngularDamping = 0.0f;
		settings.mUserData = (uint64)(uint32)ent;
		if ( motion == JPH::EMotionType::Dynamic && rb.mass > 0.0f ) {
			settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
			settings.mMassPropertiesOverride.mMass = rb.mass;
		}

		JPH::EActivation activation = rb.asleep ? JPH::EActivation::DontActivate : JPH::EActivation::Activate;
		JPH::BodyID id = m_bodyInterface->CreateAndAddBody( settings, activation );
		if ( id.IsInvalid() ) { return; }

		Entry e;
		e.id = id;
		e.entity = ent;
		e.layer = layer;
		e.isTrigger = rb.isTrigger;
		e.asleep = rb.asleep;
		m_bodies[ (uint32)ent ] = e;
		m_listener.RegisterBody( id.GetIndexAndSequenceNumber(), ent, rb.isTrigger,
								 rb.restitution, rb.friction );
	}

	HF_INLINE void FetchBody( HF_Ecs::Entity ent, hfRigidBodyComponent &rb, HF_Scene::hfTransformComponent &t ) {
		std::map<uint32, Entry>::const_iterator it = m_bodies.find( (uint32)ent );
		if ( it == m_bodies.end() ) { return; }
		const Entry &e = it->second;

		// sensors never move, so there is nothing to read back
		if ( rb.isTrigger || rb.type == HF_BT_STATIC ) { return; }

		JPH::RVec3 com = m_bodyInterface->GetCenterOfMassPosition( e.id );
		JPH::Quat q = m_bodyInterface->GetRotation( e.id );
		hfQuat rot( q.GetX(), q.GetY(), q.GetZ(), q.GetW() );
		hfVec3 comH( com.GetX(), com.GetY(), com.GetZ() );
		t.SetPosition( comH - rot * rb.offset );
		t.SetRotation( rot );

		if ( rb.type == HF_BT_DYNAMIC ) {
			JPH::Vec3 v = m_bodyInterface->GetLinearVelocity( e.id );
			JPH::Vec3 w = m_bodyInterface->GetAngularVelocity( e.id );
			bool active = m_bodyInterface->IsActive( e.id );
			rb.velocity = hfVec3( v.GetX(), v.GetY(), v.GetZ() );
			rb.angularVelocity = hfVec3( w.GetX(), w.GetY(), w.GetZ() );
			// Jolt owns sleeping: mirror its decision so the component and the
			// game logic agree, and zero the state exactly like the built-in
			// solver does
			if ( !active ) {
				rb.asleep = true;
				rb.velocity = hfVec3( 0.0f, 0.0f, 0.0f );
				rb.angularVelocity = hfVec3( 0.0f, 0.0f, 0.0f );
			} else if ( rb.asleep ) {
				rb.asleep = false;
				rb.sleepTimer = 0.0f;
			}
		}
	}

	static inline bool	s_registered = false;

	hfJoltContactListener					m_listener;
	hfJoltLayerFilter *						m_layerFilter = NULL;
	JPH::BroadPhaseLayerInterfaceTable *		m_bpTable = NULL;
	JPH::ObjectVsBroadPhaseLayerFilterTable *m_ovbTable = NULL;
	JPH::PhysicsSystem *						m_system = NULL;
	JPH::BodyInterface *						m_bodyInterface = NULL;
	JPH::TempAllocatorImpl *					m_temp = NULL;
	JPH::JobSystemSingleThreaded *			m_jobs = NULL;
	std::map<uint32, Entry>					m_bodies;
};

} // namespace HF_Physics

#endif // HF_PHYSICS_HAS_JOLT

#endif
