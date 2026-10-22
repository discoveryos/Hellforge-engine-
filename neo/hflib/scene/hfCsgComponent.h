#ifndef __HF_CSG_COMPONENT_H__
#define __HF_CSG_COMPONENT_H__

#include "../hfEcs.h"
#include "../math/hfVector.h"
#include "../bv/hfBounds.h"

/*
===============================================================================

	hfCsgComponent — procedural brush (box/sphere/quad) expressed as a scene
	entity the way a map tool would author it.  Phase 5's CSG system
	tessellates dirty brushes into a triangle soup that renderers and the
	content pipeline can consume (mesh renderer local bounds, physics
	primitives later, glTF-style export later).

===============================================================================
*/

namespace HF_Scene {

enum HfCsgShape {
	HF_CSG_NONE = 0,
	HF_CSG_BOX,
	HF_CSG_SPHERE,
	HF_CSG_QUAD
};

struct hfCsgComponent {
	HfCsgShape			shape;
	hfVec3				halfExtents;	// box half extents / sphere radius (x)
	int					materialId;
	int					triCount;
	hfBounds			localBounds;
	bool				dirty;			// set by the editor/pipeline to retessellate

	HF_INLINE			hfCsgComponent()
	: shape( HF_CSG_NONE ), halfExtents( 1, 1, 1 ), materialId( -1 ), triCount( 0 ), dirty( true ) {
		localBounds.Clear();
	}
};

} // namespace HF_Scene

#endif