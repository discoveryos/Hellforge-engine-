#ifndef __HF_PHYSICS_LIMITS_H__
#define __HF_PHYSICS_LIMITS_H__

// Shared physics budgets and the overlap event record. They live in their own
// header because both the solver in hfPhysicsSystem and the Jolt adapter in
// hfJoltBackend size their buffers from them, and both need to be usable
// independently of each other.

namespace HF_Physics {

static const int HF_MAX_PHYSICS_BODIES		= 1024;
static const int HF_MAX_OVERLAPS			= 512;
static const int HF_MAX_OVERLAP_EVENTS		= 512;

struct hfOverlapEvent {
	HF_Ecs::Entity	a;
	HF_Ecs::Entity	b;
	int				state;		// 1 = enter, 0 = stay, -1 = exit
};

} // namespace HF_Physics

#endif
