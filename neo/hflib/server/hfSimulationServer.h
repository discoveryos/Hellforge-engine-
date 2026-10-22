#ifndef __HF_SERVER_SIMULATION_SERVER_H__
#define __HF_SERVER_SIMULATION_SERVER_H__

/*
===============================================================================

	hfSimulationServer — Phase 8, threaded server split.

	CPU/headless server in the Godot Servers sense: it owns the per-frame
	advance of an `hfWorld`, runs that advance on a worker thread, and
	publishes an immutable snapshot for the front end to consume.

	Nobody outside the worker may touch a world while it is stepping, so the
	snapshot is the entire cross-thread surface. Render reads it to build draw
	commands, audio reads it for source positions, networking serializes it —
	none of them need the world, and none of them need to wait for the step
	that is producing it.

	hfSimConfig cfg;
	hfSimulationServer sim( &world, cfg );
	sim.Start();
	for ( int frame = 0; ; frame++ ) {
		sim.Kick();                                  // worker starts step N
		hfSimSnapshot snap;
		sim.CopySnapshot( &snap );                   // step N-1, already done
		... render / audio / network consume snap ...
		sim.WaitForStep();                           // let step N finish
	}
	sim.Stop();

	Without Start() every Kick() runs inline on the calling thread, which is
	what makes the determinism check meaningful: the same scene must produce
	the same snapshot with and without the worker thread.

	Determinism contract: a step is a pure function of the world, the seed and
	the input log. Two runs with the same seed and the same input log produce
	byte-identical snapshots (including the checksum) whatever thread they ran
	on. Nothing in here reads a clock, and no system may, either.

===============================================================================
*/

#include <string.h>

#include "../hfThread.h"
#include "../hfHeadless.h"
#include "../math/hfQuat.h"
#include "../scene/hfWorld.h"
#include "../physics/hfRigidBodyComponent.h"

namespace HF_Server {

static const int HF_SIM_MAX_SERVERS			= 8;
static const int HF_SIM_MAX_BODIES			= 1024;
static const int HF_SIM_MAX_INPUT			= 512;

// handle to a registered server, same shape as HF_Render::hfRid
struct hfSimRid {
	int		index;
	HF_INLINE hfSimRid() : index( -1 ) {}
	HF_INLINE bool Valid() const { return index >= 0; }
};

// one entry of the input log: an impulse the server applies to a body on a
// given step. The log is recorded by the front end and replayed in order, so
// replaying it reproduces the run.
struct hfSimInput {
	unsigned int		step;
	HF_Ecs::Entity		entity;
	hfVec3				impulse;
};

// what the front end consumes: no pointers, no handles into the world, safe to
// copy out while the worker is already building the next one.
//
// `position` is world space (transform->WorldPosition()); `rotation` is the
// component's local rotation, which is what the transform component exposes.
struct hfSimBody {
	HF_Ecs::Entity		entity;
	hfVec3				position;
	hfQuat				rotation;
	hfVec3				velocity;
	bool				asleep;
};

struct hfSimSnapshot {
	unsigned int		sequence;		// increments on every published step
	unsigned int		step;			// fixed timestep index
	float				simTime;		// step * dt
	unsigned int		seed;
	unsigned int		inputsApplied;	// how far through the input log we are
	int					bodyCount;
	hfSimBody			bodies[ HF_SIM_MAX_BODIES ];

	// FNV-1a over every byte before it. Must stay the last member.
	unsigned int		checksum;

	HF_INLINE void Zero() { memset( this, 0, sizeof( *this ) ); }
	HF_INLINE unsigned int ComputeChecksum() const {
		const unsigned char *base = (const unsigned char *)this;
		unsigned int n = (unsigned int)( (const unsigned char *)&checksum - base );
		unsigned int h = 2166136261u;
		for ( unsigned int i = 0; i < n; i++ ) {
			h ^= (unsigned int)base[i];
			h *= 16777619u;
		}
		return h;
	}
};

struct hfSimConfig {
	float				dt;				// fixed timestep, never wall clock
	bool				threaded;		// false = Kick() runs inline
	HF_INLINE hfSimConfig() : dt( 1.0f / 60.0f ), threaded( true ) {}
};

/*
The front end owns the world object but not its advance: build it, hand it
over, then only read snapshots until Stop().
*/
class hfSimulationServer : public hfSysThread {
public:
	HF_INLINE hfSimulationServer( HF_Scene::hfWorld *world, const hfSimConfig &cfg )
		: m_world( world ), m_cfg( cfg ), m_rid(), m_seed( 0 ),
		  m_inputCount( 0 ), m_step( 0 ), m_sequence( 0 ), m_simTime( 0.0f ),
		  m_started( false ) {
		m_published.Zero();
		memset( m_inputs, 0, sizeof( m_inputs ) );
		m_rid = Register( this );
	}

	HF_INLINE ~hfSimulationServer() {
		Stop();
		Unregister();
	}

	// ---- RID registry (mirrors hfRenderingServer's handle pattern) ----

	// One shared slot array for the whole program without needing an
	// out-of-line definition: hflib stays header only for the script JIT.
	static hfSimulationServer **Slots() {
		static hfSimulationServer *s_slots[ HF_SIM_MAX_SERVERS ];
		return s_slots;
	}
	static int &CountRef() { static int s_count = 0; return s_count; }

	static hfSimRid Register( hfSimulationServer *server ) {
		hfSimRid r;
		int &count = CountRef();
		if ( !server ) { return r; }
		// reuse the slot of an unregistered server, so a long session of
		// servers being built and torn down never runs the pool out
		int slot = -1;
		for ( int i = 0; i < count; i++ ) {
			if ( Slots()[ i ] == NULL ) { slot = i; break; }
		}
		if ( slot < 0 ) {
			if ( count >= HF_SIM_MAX_SERVERS ) { return r; }
			slot = count++;
		}
		r.index = slot;
		Slots()[ slot ] = server;
		return r;
	}
	HF_INLINE void Unregister() {
		if ( !m_rid.Valid() ) { return; }
		if ( m_rid.index >= 0 && m_rid.index < HF_SIM_MAX_SERVERS ) { Slots()[ m_rid.index ] = NULL; }
		m_rid.index = -1;
	}
	static HF_INLINE hfSimulationServer *Resolve( const hfSimRid &r ) {
		return ( r.Valid() && r.index < HF_SIM_MAX_SERVERS && r.index < CountRef() ) ?
				Slots()[ r.index ] : NULL;
	}
	static HF_INLINE int Count() { return CountRef(); }
	HF_INLINE hfSimRid GetRid() const { return m_rid; }

	// ---- configuration (front end, before Start) ----

	HF_INLINE void SetSeed( unsigned int seed ) { m_seed = seed; }
	HF_INLINE unsigned int GetSeed() const { return m_seed; }

	// Appends to the input log; call with an increasing step to replay a
	// recorded stream. Inputs must be added before the step they belong to.
	HF_INLINE bool AddInput( const hfSimInput &in ) {
		if ( m_inputCount >= HF_SIM_MAX_INPUT ) { return false; }
		m_inputs[ m_inputCount++ ] = in;
		return true;
	}
	HF_INLINE void ClearInput() { m_inputCount = 0; }
	HF_INLINE int GetInputCount() const { return m_inputCount; }

	// ---- lifecycle ----

	HF_INLINE void Start() {
		if ( m_started || !m_cfg.threaded || !m_world ) { return; }
		m_started = StartWorkerThread( "simServer", CORE_ANY );
	}
	HF_INLINE void Stop() {
		if ( !m_started ) { return; }
		StopThread( true );
		m_started = false;
	}
	HF_INLINE bool IsStarted() const { return m_started; }

	// The engine's --headless switch, from the point of view of the server:
	// the simulation runs to completion with nothing to present to.
	HF_INLINE bool IsHeadless() const { return HF_Lib::IsHeadless(); }

	// ---- one fixed step ----

	// Signals the worker to step, or runs the step inline when there is no
	// worker. Returns without waiting.
	HF_INLINE void Kick() {
		if ( m_started ) { SignalWork(); return; }
		DoStep();
	}
	HF_INLINE void WaitForStep() {
		if ( m_started ) { WaitForThread(); }
	}
	// Kick + wait, for callers that do not want to overlap anything.
	HF_INLINE void StepNow() { Kick(); WaitForStep(); }

	// ---- consumption ----

	// Copies the newest published snapshot out under a short lock. Returns
	// false when nothing has been published yet. Copying rather than handing
	// out a pointer means the caller can never hold a buffer the worker wants
	// to reuse, so there is no read/step ordering to get wrong.
	HF_INLINE bool CopySnapshot( hfSimSnapshot *out ) const {
		if ( !out ) { return false; }
		hfScopedCriticalSection lock( m_lock );
		if ( m_sequence == 0 ) { return false; }
		*out = m_published;
		return true;
	}
	HF_INLINE unsigned int GetSequence() const {
		hfScopedCriticalSection lock( m_lock );
		return m_sequence;
	}
	HF_INLINE unsigned int GetStep() const {
		hfScopedCriticalSection lock( m_lock );
		return m_step;
	}
	HF_INLINE float GetSimTime() const {
		hfScopedCriticalSection lock( m_lock );
		return m_simTime;
	}
	HF_INLINE bool HasWorld() const { return m_world != NULL; }

protected:
	virtual int Run() {
		DoStep();
		return 0;
	}

private:
	// Applies every input logged for this step, advances the world, then
	// publishes. This is the whole of the worker's job: no clocks, no
	// randomness, nothing but the seed, the log and the world.
	HF_INLINE void DoStep() {
		if ( !m_world ) { return; }

		for ( int i = 0; i < m_inputCount; i++ ) {
			if ( m_inputs[i].step != m_step ) { continue; }
			HF_Physics::hfRigidBodyComponent *rb =
				m_world->GetComponent< HF_Physics::hfRigidBodyComponent >( m_inputs[i].entity );
			if ( rb ) { rb->velocity += m_inputs[i].impulse; }
		}

		m_world->Update( m_cfg.dt );

		Publish();
	}

	// Publish is the only place m_step / m_simTime / m_sequence move, and it
	// holds the lock across all of them, so a reader sees the counter and the
	// snapshot it belongs to as one thing rather than two.
	HF_INLINE void Publish() {
		hfSimSnapshot snap;
		snap.Zero();

		hfScopedCriticalSection lock( m_lock );

		m_step++;
		m_simTime += m_cfg.dt;

		snap.sequence		= m_sequence + 1;
		snap.step			= m_step;
		snap.simTime		= m_simTime;
		snap.seed			= m_seed;

		// inputs are applied in log order, so "applied" is how far the log
		// has been consumed — a replay that stops at the same step matches
		unsigned int applied = 0;
		for ( int i = 0; i < m_inputCount; i++ ) {
			if ( m_inputs[i].step < m_step ) { applied++; }
		}
		snap.inputsApplied	= applied;

		snap.bodyCount = 0;
		m_world->GetRegistry().Each< HF_Scene::hfTransformComponent >(
			[ & ]( HF_Ecs::Entity e, HF_Scene::hfTransformComponent &t ) {
				if ( snap.bodyCount >= HF_SIM_MAX_BODIES ) { return; }
				hfSimBody &b = snap.bodies[ snap.bodyCount++ ];
				b.entity	= e;
				b.position	= t.WorldPosition();
				b.rotation	= t.rotation;
				b.velocity	= hfVec3( 0.0f, 0.0f, 0.0f );
				b.asleep	= false;
				HF_Physics::hfRigidBodyComponent *rb =
					m_world->GetComponent< HF_Physics::hfRigidBodyComponent >( e );
				if ( rb ) {
					b.velocity	= rb->velocity;
					b.asleep	= rb->asleep;
				}
			} );

		snap.checksum = snap.ComputeChecksum();

		m_published	= snap;
		m_sequence	= snap.sequence;
	}

	HF_Scene::hfWorld *		m_world;
	hfSimConfig				m_cfg;
	hfSimRid				m_rid;

	unsigned int			m_seed;
	hfSimInput				m_inputs[ HF_SIM_MAX_INPUT ];
	int						m_inputCount;

	unsigned int			m_step;
	unsigned int			m_sequence;
	float					m_simTime;
	bool					m_started;

	// m_published is written by the worker and read by CopySnapshot(); m_lock
	// only guards the assignment and the memcpy, never the step itself.
	hfSimSnapshot			m_published;
	mutable hfSysMutex		m_lock;
};

} // namespace HF_Server

#endif
