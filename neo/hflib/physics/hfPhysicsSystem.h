#ifndef __HF_PHYSICS_SYSTEM_H__
#define __HF_PHYSICS_SYSTEM_H__

#include "../scene/hfSystem.h"
#include "../scene/hfTransformComponent.h"
#include "../math/hfQuat.h"
#include "hfRigidBodyComponent.h"
#include "hfBroadphase.h"
#include "hfPhysicsLimits.h"
#include "hfJoltBackend.h"

#include <math.h>

namespace HF_Physics {

// budgets and hfOverlapEvent live in hfPhysicsLimits.h, shared with the Jolt
// adapter

// Game physics system, modern-engine conventions, fully header-only and
// headless-testable:
//   - fixed internal timestep (default 60 Hz) behind an accumulator
//   - SAP broadphase -> per-shape narrowphase (sphere/sphere, sphere/OBB,
//     OBB/OBB via SAT)
//   - impulse + positional-correction solver with restitution / friction
//   - trigger volumes produce enter / stay / exit events (no response)
//   - sleeping bodies skip integration until bumped by a moving body
// The solver can be swapped for Jolt (vendored, MIT) without changing any of
// that contract: see hfJoltBackend.h. Builds without Jolt keep the built-in
// solver, which is also what the harness falls back to.
// Runs at priority 25 (after cameras, before render cull). Bodies are
// top-level entities for now; the renderer sees one frame of lag, which is
// standard for fixed-step physics.
class hfPhysicsSystem : public HF_Scene::hfSystem {
public:
	enum { SYSTEM_PRIORITY = 25 };
	static const int MAX_STEPS_PER_UPDATE = 4;

	enum BackendType {
		BACKEND_BUILTIN = 0,	// the impulse solver in this header
		BACKEND_JOLT			// only selectable when Jolt is in the build
	};

	// Which solver to use. Falls back to the built-in one when Jolt was not
	// compiled in, so asking for it can never break a build.
	HF_INLINE void SetBackend( BackendType b ) {
		if ( b == BACKEND_JOLT && !IsBackendAvailable( BACKEND_JOLT ) ) { b = BACKEND_BUILTIN; }
		if ( b == m_backend ) { return; }
		DropBackend();
		m_backend = b;
		CreateBackend();
		m_gravityKnown = false;
	}

	HF_INLINE BackendType GetBackend() const { return m_backend; }
	HF_INLINE const char *GetBackendName() const {
		return m_backend == BACKEND_JOLT ? "jolt" : "builtin";
	}

	static HF_INLINE bool IsBackendAvailable( BackendType b ) {
#if HF_PHYSICS_HAS_JOLT
		( void )b;
		return true;
#else
		( void )b;
		return false;
#endif
	}
	// the strongest solver this build can offer, used by default
	static HF_INLINE BackendType PreferredBackend() {
		return IsBackendAvailable( BACKEND_JOLT ) ? BACKEND_JOLT : BACKEND_BUILTIN;
	}

	HF_INLINE hfPhysicsSystem() {
		m_step = 1.0f / 60.0f;
		m_gravity = hfVec3( 0.0f, 0.0f, -9.81f );	// +Z up
		m_accum = 0.0f;
		m_sleepLinear = 0.02f;
		m_sleepAngular = 0.02f;
		m_sleepTime = 1.0f;
		m_activeCount = 0;
		m_eventCount = 0;
		m_backend = BACKEND_BUILTIN;
		m_jolt = NULL;
		m_gravityKnown = false;
		CreateBackend();
	}

	HF_INLINE ~hfPhysicsSystem() { DropBackend(); }

	const char *GetName() const override { return "PhysicsSystem"; }
	int GetPriority() const override { return SYSTEM_PRIORITY; }

	HF_INLINE void OnRegister( HF_Ecs::Registry &registry ) override {
		( void )registry;
		m_accum = 0.0f;
		m_activeCount = 0;
		m_eventCount = 0;
	}

	// ---- tuning ------------------------------------------------------
	HF_INLINE void SetGravity( const hfVec3 &g ) {
		m_gravity = g;
#if HF_PHYSICS_HAS_JOLT
		if ( m_jolt ) { m_jolt->SetGravity( g ); }
#endif
		( void )m_jolt;
	}
	HF_INLINE void SetFixedStep( float hz ) { if ( hz > 1.0f ) { m_step = 1.0f / hz; } }
	HF_INLINE void SetSleepThresholds( float lin, float ang, float seconds ) {
		m_sleepLinear = lin; m_sleepAngular = ang; m_sleepTime = seconds;
	}

	// ---- trigger results ---------------------------------------------
	HF_INLINE int NumActiveOverlaps() const { return m_activeCount; }
	HF_INLINE void GetActiveOverlap( int i, HF_Ecs::Entity &a, HF_Ecs::Entity &b ) const {
		if ( i >= 0 && i < m_activeCount ) { a = m_active[i].a; b = m_active[i].b; }
	}
	HF_INLINE int NumEvents() const { return m_eventCount; }
	HF_INLINE const hfOverlapEvent &GetEvent( int i ) const { return m_events[i]; }

	HF_INLINE void Update( HF_Ecs::Registry &registry, float dt ) override {
		m_eventCount = 0;	// per-frame event buffer
		m_accum += dt;
		int steps = 0;
		while ( m_accum >= m_step && steps < MAX_STEPS_PER_UPDATE ) {
			Step( registry, m_step );
			m_accum -= m_step;
			steps++;
		}
		if ( steps == MAX_STEPS_PER_UPDATE ) { m_accum = 0.0f; }	// spiral of death guard
	}

	// ---- narrowphase (public for the harness) ----------------------
	static HF_INLINE bool TestSphereSphere( const hfVec3 &ca, float ra, const hfVec3 &cb, float rb,
											hfVec3 &normal, float &depth ) {
		hfVec3 d = cb - ca;
		float distSq = d.LengthSqr();
		float rr = ra + rb;
		if ( distSq >= rr * rr ) { return false; }
		float dist = sqrtf( distSq );
		if ( dist > 1e-6f ) {
			normal = d * ( 1.0f / dist );
		} else {
			normal = hfVec3( 0.0f, 0.0f, 1.0f );
		}
		depth = rr - dist;
		return true;
	}

	// Closest point on an OBB (center c, axes u[3], half-extents h) to p.
	static HF_INLINE hfVec3 ClosestPointOBB( const hfVec3 &p, const hfVec3 &c,
											 const hfVec3 u[3], const hfVec3 &h ) {
		hfVec3 d = p - c;
		hfVec3 q = c;
		for ( int i = 0; i < 3; i++ ) {
			float dist = d * u[i];
			if ( dist > h[i] ) { dist = h[i]; }
			if ( dist < -h[i] ) { dist = -h[i]; }
			q = q + u[i] * dist;
		}
		return q;
	}

	// sphere vs OBB. Returns normal pointing from the box toward the sphere.
	static HF_INLINE bool TestSphereOBB( const hfVec3 &cs, float r, const hfVec3 &cb,
										 const hfVec3 u[3], const hfVec3 &h,
										 hfVec3 &normal, float &depth ) {
		hfVec3 d = cs - cb;
		hfVec3 qp = ClosestPointOBB( cs, cb, u, h );
		hfVec3 rel = cs - qp;
		float distSq = rel.LengthSqr();
		if ( distSq >= r * r ) { return false; }
		if ( distSq < 1e-8f ) {
			// center inside the box: push out along the nearest face axis
			int best = 0;
			float bestPen = -1.0f;
			for ( int i = 0; i < 3; i++ ) {
				float proj = d * u[i];
				float pen = ( h[i] - fabsf( proj ) ) + r;
				if ( bestPen < 0.0f || pen < bestPen ) { bestPen = pen; best = i; }
			}
			hfVec3 o = u[best];
			if ( d * o >= 0.0f ) { normal = o; } else { normal = -o; }
			depth = bestPen;
			return true;
		}
		float dist = sqrtf( distSq );
		normal = rel * ( 1.0f / dist );	// from box toward sphere
		depth = r - dist;
		return true;
	}

	// OBB vs OBB via SAT. Returns normal oriented from A toward B.
	static HF_INLINE bool TestBoxBox( const hfVec3 &ca, const hfVec3 ua[3], const hfVec3 &ha,
									  const hfVec3 &cb, const hfVec3 ub[3], const hfVec3 &hb,
									  hfVec3 &normal, float &depth ) {
		hfVec3 t = cb - ca;
		const int kMax = 15;
		hfVec3 axes[ kMax ];
		int nAxes = 0;
		for ( int i = 0; i < 3; i++ ) { axes[nAxes++] = ua[i]; }
		for ( int i = 0; i < 3; i++ ) { axes[nAxes++] = ub[i]; }
		for ( int i = 0; i < 3; i++ ) {
			for ( int j = 0; j < 3; j++ ) {
				hfVec3 cross = ua[i].Cross( ub[j] );
				if ( cross.LengthSqr() > 1e-8f ) {
					cross.Normalize();
					axes[nAxes++] = cross;
				}
			}
		}
		float bestDepth = 1.0e30f;
		hfVec3 bestAxis = hfVec3( 1.0f, 0.0f, 0.0f );
		for ( int a = 0; a < nAxes; a++ ) {
			const hfVec3 &l = axes[a];
			float ra = 0.0f, rb = 0.0f;
			for ( int i = 0; i < 3; i++ ) {
				ra += ha[i] * fabsf( ua[i] * l );
				rb += hb[i] * fabsf( ub[i] * l );
			}
			float dist = fabsf( t * l );
			if ( dist > ra + rb ) { return false; }
			float pen = ( ra + rb ) - dist;
			if ( pen < bestDepth ) { bestDepth = pen; bestAxis = l; }
		}
		normal = bestAxis;
		if ( normal * t < 0.0f ) { normal = -normal; }	// orient A -> B
		depth = bestDepth;
		return true;
	}

private:
	struct Body {
		HF_Ecs::Entity			e;
		hfRigidBodyComponent *	rb;
		HF_Scene::hfTransformComponent *	t;
		hfVec3					c;			// entity world position
		hfVec3					cEff;		// collider world center (c + rot*offset)
		hfVec3					u[3];		// collider world axes (rotated)
		hfQuat					rot;
		hfBounds				box;
		bool					integrated;
	};

	HF_INLINE static bool Overlap( const Body &A, const Body &B, hfVec3 &normal, float &depth ) {
		const hfShapeType sa = A.rb->shape, sb = B.rb->shape;
		if ( sa == HF_SHAPE_SPHERE && sb == HF_SHAPE_SPHERE ) {
			return TestSphereSphere( A.cEff, A.rb->radius, B.cEff, B.rb->radius, normal, depth );
		}
		if ( sa == HF_SHAPE_SPHERE && sb == HF_SHAPE_BOX ) {
			bool hit = TestSphereOBB( A.cEff, A.rb->radius, B.cEff, B.u, B.rb->halfExtents, normal, depth );
			if ( hit ) { normal = -normal; }	// orient A(sphere) -> B(box)
			return hit;
		}
		if ( sa == HF_SHAPE_BOX && sb == HF_SHAPE_SPHERE ) {
			return TestSphereOBB( B.cEff, B.rb->radius, A.cEff, A.u, A.rb->halfExtents, normal, depth );
		}
		return TestBoxBox( A.cEff, A.u, A.rb->halfExtents, B.cEff, B.u, B.rb->halfExtents, normal, depth );
	}

	HF_INLINE static void ComputeAabb( const Body &b, hfBounds &out ) {
		hfVec3 mn, mx;
		if ( b.rb->shape == HF_SHAPE_SPHERE ) {
			float r = b.rb->radius;
			hfVec3 v( r, r, r );
			mn = b.cEff - v; mx = b.cEff + v;
		} else {
			hfVec3 ext( 0.0f, 0.0f, 0.0f );
			const hfVec3 &h = b.rb->halfExtents;
			for ( int i = 0; i < 3; i++ ) {
				hfVec3 w = b.u[i] * h[i];
				ext.x += fabsf( w.x ); ext.y += fabsf( w.y ); ext.z += fabsf( w.z );
			}
			mn = b.cEff - ext; mx = b.cEff + ext;
		}
		out = hfBounds( mn, mx );
	}

	struct Pair { HF_Ecs::Entity a, b; };

	HF_INLINE bool SamePair( const Pair &x, const Pair &y ) const {
		return x.a == y.a && x.b == y.b;
	}

	void Step( HF_Ecs::Registry &registry, float dt ) {
#if HF_PHYSICS_HAS_JOLT
		if ( m_jolt ) {
			StepJolt( registry, dt );
			return;
		}
#endif
		StepBuiltin( registry, dt );
	}

#if HF_PHYSICS_HAS_JOLT
	// Jolt owns integration, the solver and sleeping; the engine keeps its
	// accumulator, its event buffer and its component semantics.
	HF_INLINE void StepJolt( HF_Ecs::Registry &registry, float dt ) {
		m_jolt->Step( registry, dt );
		m_eventCount = 0;
		m_activeCount = 0;
		const hfJoltOverlap *events = m_jolt->Events();
		int n = m_jolt->NumEvents();
		// Jolt reports every touching pair once per step: enter on the step it
		// appears, stay while it lasts, exit on the step it goes away. The
		// active set is therefore rebuilt from the non-negative events, the same
		// way the built-in solver rebuilds it from the broadphase.
		for ( int i = 0; i < n && m_eventCount < HF_MAX_OVERLAP_EVENTS; i++ ) {
			m_events[ m_eventCount ].a = events[ i ].a;
			m_events[ m_eventCount ].b = events[ i ].b;
			m_events[ m_eventCount ].state = events[ i ].state;
			m_eventCount++;
			if ( events[ i ].state >= 0 && m_activeCount < HF_MAX_OVERLAPS ) {
				Pair p;
				p.a = events[ i ].a;
				p.b = events[ i ].b;
				if ( !HasActivePair( p ) ) { m_active[ m_activeCount++ ] = p; }
			}
		}
	}

	HF_INLINE bool HasActivePair( const Pair &p ) const {
		for ( int k = 0; k < m_activeCount; k++ ) {
			if ( SamePair( m_active[k], p ) ) { return true; }
		}
		return false;
	}
#endif

	void StepBuiltin( HF_Ecs::Registry &registry, float dt ) {
		Body bodies[ HF_MAX_PHYSICS_BODIES ];
		int nb = 0;
		hfVec3 delta[ HF_MAX_PHYSICS_BODIES ];
		hfVec3 nPos[ HF_MAX_PHYSICS_BODIES ];
		hfQuat nQuat[ HF_MAX_PHYSICS_BODIES ];

		registry.Each< hfRigidBodyComponent >(
			[ & ]( HF_Ecs::Entity ent, hfRigidBodyComponent &rb ) {
				if ( nb >= HF_MAX_PHYSICS_BODIES ) { return; }
				Body &b = bodies[nb];
				b.e = ent;
				b.rb = &rb;
				b.t = registry.GetComponent< HF_Scene::hfTransformComponent >( ent );
				b.integrated = false;
				delta[nb] = hfVec3( 0.0f, 0.0f, 0.0f );
				if ( !b.t ) { nb++; return; }
				b.rot = b.t->rotation;
				b.c = b.t->WorldPosition();
				b.u[0] = b.rot * hfVec3( 1.0f, 0.0f, 0.0f );
				b.u[1] = b.rot * hfVec3( 0.0f, 1.0f, 0.0f );
				b.u[2] = b.rot * hfVec3( 0.0f, 0.0f, 1.0f );
				b.cEff = b.c + ( b.rot * rb.offset );
				rb.RefreshMass();
				ComputeAabb( b, b.box );

				if ( rb.type == HF_BT_DYNAMIC && !rb.asleep ) {
					// Euler integrate
					rb.velocity += m_gravity * ( rb.gravityScale * dt );
					nPos[nb] = b.c + rb.velocity * dt;
					const hfVec3 &w = rb.angularVelocity;
					float wl = w.Length();
					if ( wl > 1e-6f ) {
						hfVec3 axis = w * ( 1.0f / wl );
						float half = wl * dt * 0.5f;
						float s = sinf( half ), c = cosf( half );
						hfQuat dq( axis.x * s, axis.y * s, axis.z * s, c );
						nQuat[nb] = b.rot * dq;
						nQuat[nb].Normalize();
					} else {
						nQuat[nb] = b.rot;
					}
					b.integrated = true;
				}
				nb++;
			} );

		if ( nb == 0 ) { return; }

		// broadphase
		hfBroadphase bp;
		for ( int i = 0; i < nb; i++ ) {
			hfBroadphaseEntry e;
			e.entity = bodies[i].e;
			e.box = bodies[i].box;
			bp.Add( e );
		}
		bp.Build();

		// trigger overlap set for this step (rebuilt each step)
		Pair m_now[ HF_MAX_OVERLAPS ];
		int nowCount = 0;

		// narrowphase
		for ( int p = 0; p < bp.GetPairCount(); p++ ) {
			int ia, ib;
			bp.GetPair( p, ia, ib );
			Body &A = bodies[ia];
			Body &B = bodies[ib];
			hfRigidBodyComponent &rA = *A.rb, &rB = *B.rb;

			if ( ( rA.mask & rB.layer ) == 0 ) { continue; }
			if ( ( rB.mask & rA.layer ) == 0 ) { continue; }

			hfVec3 normal;
			float depth;
			if ( !Overlap( A, B, normal, depth ) ) { continue; }

			if ( rA.isTrigger || rB.isTrigger ) {
				if ( nowCount < HF_MAX_OVERLAPS ) {
					if ( ia < ib ) { m_now[nowCount].a = A.e; m_now[nowCount].b = B.e; }
					else { m_now[nowCount].a = B.e; m_now[nowCount].b = A.e; }
					nowCount++;
				}
				continue;
			}

			if ( rA.invMass == 0.0f && rB.invMass == 0.0f ) { continue; }

			// wake sleeping dynamic bodies that share a contact with a mover
			if ( rA.asleep && rA.invMass > 0.0f && rB.velocity.Length() > m_sleepLinear ) { rA.asleep = false; rA.sleepTimer = 0.0f; }
			if ( rB.asleep && rB.invMass > 0.0f && rA.velocity.Length() > m_sleepLinear ) { rB.asleep = false; rB.sleepTimer = 0.0f; }

			// positional correction (Baumgarte-style with penetration slop)
			float totalInv = rA.invMass + rB.invMass;
			float corr = depth - 0.005f;
			if ( corr > 0.0f ) {
				corr *= 0.8f;
				float wA = rA.invMass / totalInv;
				float wB = rB.invMass / totalInv;
				delta[ia] -= normal * ( corr * wA );
				delta[ib] += normal * ( corr * wB );
			}

			// normal impulse
			hfVec3 rel = rB.velocity - rA.velocity;
			float vn = rel * normal;
			if ( vn < 0.0f ) {
				float e = rA.restitution > rB.restitution ? rA.restitution : rB.restitution;
				float j = -( 1.0f + e ) * vn / totalInv;
				rA.velocity -= normal * ( j * rA.invMass );
				rB.velocity += normal * ( j * rB.invMass );
			}

			// tangential friction (approximates Coulomb)
			hfVec3 tangent = rel - normal * vn;
			float vt = tangent.Length();
			if ( vt > 1e-6f ) {
				tangent = tangent * ( 1.0f / vt );
				float f = rA.friction > rB.friction ? rA.friction : rB.friction;
				float jt = f * vt / totalInv;
				rA.velocity += tangent * ( jt * rA.invMass );
				rB.velocity -= tangent * ( jt * rB.invMass );
			}
		}

		// write integrated states, apply correction offsets, sleep bookkeeping
		for ( int i = 0; i < nb; i++ ) {
			Body &b = bodies[i];
			if ( b.integrated ) {
				b.t->SetPosition( nPos[i] + delta[i] );
				b.t->SetRotation( nQuat[i] );
			}
			hfRigidBodyComponent &rb = *b.rb;
			if ( rb.type != HF_BT_DYNAMIC ) { continue; }
			if ( rb.asleep ) { continue; }
			float spd = rb.velocity.Length();
			float wspd = rb.angularVelocity.Length();
			if ( spd <= m_sleepLinear && wspd <= m_sleepAngular ) {
				rb.sleepTimer += dt;
				if ( rb.sleepTimer >= m_sleepTime ) {
					rb.asleep = true;
					rb.velocity = hfVec3( 0.0f, 0.0f, 0.0f );
					rb.angularVelocity = hfVec3( 0.0f, 0.0f, 0.0f );
				}
			} else {
				rb.sleepTimer = 0.0f;
			}
		}

		// overlap events: diff this step's overlaps against the active set
		for ( int i = 0; i < nowCount && m_eventCount < HF_MAX_OVERLAP_EVENTS; i++ ) {
			const Pair &p = m_now[i];
			bool wasActive = false;
			for ( int k = 0; k < m_activeCount; k++ ) {
				if ( SamePair( m_active[k], p ) ) { wasActive = true; break; }
			}
			m_events[m_eventCount].a = p.a; m_events[m_eventCount].b = p.b;
			m_events[m_eventCount].state = wasActive ? 0 : 1;
			m_eventCount++;
		}
		for ( int i = 0; i < m_activeCount && m_eventCount < HF_MAX_OVERLAP_EVENTS; i++ ) {
			const Pair &p = m_active[i];
			bool still = false;
			for ( int k = 0; k < nowCount; k++ ) {
				if ( SamePair( m_now[k], p ) ) { still = true; break; }
			}
			if ( !still ) {
				m_events[m_eventCount].a = p.a; m_events[m_eventCount].b = p.b;
				m_events[m_eventCount].state = -1;
				m_eventCount++;
			}
		}
		m_activeCount = nowCount < HF_MAX_OVERLAPS ? nowCount : HF_MAX_OVERLAPS;
		for ( int i = 0; i < m_activeCount; i++ ) { m_active[i] = m_now[i]; }
	}

	hfVec3			m_gravity;
	float			m_step;
	float			m_accum;
	float			m_sleepLinear;
	float			m_sleepAngular;
	float			m_sleepTime;

	Pair			m_active[ HF_MAX_OVERLAPS ];
	int				m_activeCount;
	hfOverlapEvent	m_events[ HF_MAX_OVERLAP_EVENTS ];
	int				m_eventCount;

	BackendType		m_backend;
	bool			m_gravityKnown;

#if HF_PHYSICS_HAS_JOLT
	hfJoltBackend *	m_jolt;
#else
	void *			m_jolt;
#endif

	HF_INLINE void CreateBackend() {
#if HF_PHYSICS_HAS_JOLT
		if ( m_backend == BACKEND_JOLT && !m_jolt ) {
			m_jolt = new hfJoltBackend();
			if ( !m_jolt->IsReady() ) {
				delete m_jolt;
				m_jolt = NULL;
				m_backend = BACKEND_BUILTIN;
			}
		}
#endif
		( void )m_gravityKnown;
	}

	HF_INLINE void DropBackend() {
#if HF_PHYSICS_HAS_JOLT
		if ( m_jolt ) {
			delete m_jolt;
			m_jolt = NULL;
		}
#endif
	}
};

} // namespace HF_Physics

#endif