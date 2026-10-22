#ifndef __HF_SCENE_SYSTEM_H__
#define __HF_SCENE_SYSTEM_H__

#include "../hfEcs.h"

// Base class for per-frame scene systems.  A world owns an ordered list of
// systems; Update( dt ) runs them top-down by priority.  Systems talk to the
// world's registry (never the world itself), so they stay header-only and
// reusable.  Mirrors the SynapseEngine System/ hierarchy (TagSystem,
// TransformSystem, ...).

namespace HF_Scene {

class hfWorld;

class hfSystem {
public:
	virtual					~hfSystem() {}
	virtual const char *	GetName() const = 0;
	virtual int				GetPriority() const { return 0; }
	virtual void			OnRegister( HF_Ecs::Registry &registry ) {}
	virtual void			Update( HF_Ecs::Registry &registry, float dt ) = 0;
};

} // namespace HF_Scene

#endif