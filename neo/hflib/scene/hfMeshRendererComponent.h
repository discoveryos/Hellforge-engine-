#ifndef __HF_SCENE_MESH_RENDERER_COMPONENT_H__
#define __HF_SCENE_MESH_RENDERER_COMPONENT_H__

#include "../bv/hfBounds.h"
#include "../hfStr.h"

namespace HF_Scene {

struct hfMeshRendererComponent {
	int				meshId;			// resource id into the pack (-1 = none)
	int				materialId;		// material id (-1 = default material)
	hfStr			meshName;		// asset name, for editor/debug display
	hfBounds		localBounds;	// model-space bounds the cull system expands
	bool			castsShadow;
	bool			receiveShadow;
	bool			visible;		// participates in the draw list?

	HF_INLINE hfMeshRendererComponent() {
		meshId = -1;
		materialId = -1;
		localBounds.Zero();
		castsShadow = true;
		receiveShadow = true;
		visible = true;
	}
};

} // namespace HF_Scene

#endif