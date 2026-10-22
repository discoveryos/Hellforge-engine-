#ifndef __HF_SCENE_TAG_COMPONENT_H__
#define __HF_SCENE_TAG_COMPONENT_H__

#include "../hfStr.h"

// Human-readable entity name (SynapseEngine TagComponent equivalent).
// World::FindEntityByTag() scans these.

namespace HF_Scene {

struct hfTagComponent {
	hfStr			name;

	HF_INLINE hfTagComponent() {}
	HF_INLINE hfTagComponent( const char *tag ) { name = tag; }
};

} // namespace HF_Scene

#endif