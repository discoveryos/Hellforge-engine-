#ifndef __HF_SCENE_HIERARCHY_COMPONENT_H__
#define __HF_SCENE_HIERARCHY_COMPONENT_H__

// Marker component: presence means the entity participates in the scene
// hierarchy.  Parent/child linkage itself lives in HierarchySystem
// (indexed by entity, so it survives component-store repacking).

namespace HF_Scene {

struct hfHierarchyComponent {
	HF_INLINE hfHierarchyComponent() {}
};

} // namespace HF_Scene

#endif