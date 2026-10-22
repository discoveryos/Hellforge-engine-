#ifndef __HF_SCENE_TRANSFORM_SYSTEM_H__
#define __HF_SCENE_TRANSFORM_SYSTEM_H__

#include "../hfEcs.h"
#include "hfSystem.h"
#include "hfTransformComponent.h"
#include "hfHierarchySystem.h"

// Computes the world (TRS-composed, hierarchy-compounded) matrix for every
// entity that has a TransformComponent, walking the scene tree parent-first
// so children see an up-to-date parent matrix.

namespace HF_Scene {

class hfWorld;

class hfTransformSystem : public hfSystem {
public:
	HF_INLINE hfTransformSystem() { m_hierarchy = NULL; m_registry = NULL; }

	HF_INLINE virtual const char *GetName() const { return "hfTransform"; }
	HF_INLINE virtual int GetPriority() const { return 10; }

	HF_INLINE virtual void OnRegister( HF_Ecs::Registry &registry ) {
		m_registry = &registry;
	}

	// Called by the world after it links the built-in systems together.
	HF_INLINE void LinkHierarchy( hfHierarchySystem *hierarchy ) { m_hierarchy = hierarchy; }

	HF_INLINE virtual void Update( HF_Ecs::Registry &registry, float dt ) {
		(void)dt;
		m_registry = &registry;
		if ( !m_hierarchy ) { return; }
		Prepare( registry );
	}

	// One-shot recompute of a single node (used when a component is edited
	// out of frame and the caller just wants the matrix refreshed).
	HF_INLINE hfMat4 ComputeStandalone( HF_Ecs::Entity e ) {
		HF_Ecs::Registry &reg = *m_registry;
		hfTransformComponent *t = reg.GetComponent< hfTransformComponent >( e );
		HF_Ecs::Entity parent = m_hierarchy->GetParent( e );
		if ( parent != HF_Ecs::ENTITY_NULL ) {
			hfTransformComponent *pt = reg.GetComponent< hfTransformComponent >( parent );
			if ( pt ) {
				t->worldMatrix = pt->worldMatrix * t->LocalMatrix();
			} else {
				t->worldMatrix = t->LocalMatrix();
			}
		} else {
			t->worldMatrix = t->LocalMatrix();
		}
		t->dirty = false;
		return t->worldMatrix;
	}

private:
	HF_INLINE void Prepare( HF_Ecs::Registry &registry ) {
		registry.Each< hfTransformComponent >(
			[ this ]( HF_Ecs::Entity e, hfTransformComponent & t ) {
				if ( this->m_hierarchy->GetParent( e ) != HF_Ecs::ENTITY_NULL ) { return; }	// reached via parent DFS
				t.worldMatrix = t.LocalMatrix();
				t.dirty = false;
				VisitChildren( e, t.worldMatrix );
			} );
	}

	HF_INLINE void VisitChildren( HF_Ecs::Entity parent, const hfMat4 &parentWorld ) {
		for ( HF_Ecs::Entity c = m_hierarchy->GetFirstChild( parent ); c != HF_Ecs::ENTITY_NULL; c = m_hierarchy->GetNextSibling( c ) ) {
			hfTransformComponent *t = m_registry->GetComponent< hfTransformComponent >( c );
			if ( t ) {
				t->worldMatrix = parentWorld * t->LocalMatrix();
				t->dirty = false;
			}
			VisitChildren( c, t ? t->worldMatrix : parentWorld );
		}
	}

	hfHierarchySystem *		m_hierarchy;
	HF_Ecs::Registry *		m_registry;
};

} // namespace HF_Scene

#endif