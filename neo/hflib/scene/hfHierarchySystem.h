#ifndef __HF_SCENE_HIERARCHY_SYSTEM_H__
#define __HF_SCENE_HIERARCHY_SYSTEM_H__

#include "../hfEcs.h"
#include "hfSystem.h"
#include "hfTransformComponent.h"

// Owns the parent/child linkage of the scene graph.  Data is indexed by
// entity index (the ECS component stores free to swap-pop), so destroyed
// entities are detached explicitly via PreDestroy().

namespace HF_Scene {

class hfWorld;

class hfHierarchySystem : public hfSystem {
public:
	HF_INLINE hfHierarchySystem() {
		m_parent = NULL;
		m_firstChild = NULL;
		m_nextSibling = NULL;
		m_capacity = 0;
		m_registry = NULL;
	}

	HF_INLINE ~hfHierarchySystem() {
		delete[] m_parent;
		delete[] m_firstChild;
		delete[] m_nextSibling;
	}

	HF_INLINE virtual const char *GetName() const { return "hfHierarchy"; }
	HF_INLINE virtual int GetPriority() const { return 0; }

	HF_INLINE virtual void OnRegister( HF_Ecs::Registry &registry ) {
		m_registry = &registry;
	}

	HF_INLINE virtual void Update( HF_Ecs::Registry &registry, float dt ) {
		(void)registry; (void)dt;
		Validate();
	}

	void					SetParent( HF_Ecs::Entity child, HF_Ecs::Entity parent );
	void					PreDestroy( HF_Ecs::Entity ent );
	void					Validate();
	void					Clear();

	HF_Ecs::Entity			GetParent( HF_Ecs::Entity ent ) const {
		unsigned int idx = ent & HF_Ecs::INDEX_MASK;
		return idx < ( unsigned int )m_capacity ? m_parent[idx] : HF_Ecs::ENTITY_NULL;
	}
	HF_Ecs::Entity			GetFirstChild( HF_Ecs::Entity ent ) const {
		unsigned int idx = ent & HF_Ecs::INDEX_MASK;
		return idx < ( unsigned int )m_capacity ? m_firstChild[idx] : HF_Ecs::ENTITY_NULL;
	}
	HF_Ecs::Entity			GetNextSibling( HF_Ecs::Entity ent ) const {
		unsigned int idx = ent & HF_Ecs::INDEX_MASK;
		return idx < ( unsigned int )m_capacity ? m_nextSibling[idx] : HF_Ecs::ENTITY_NULL;
	}
	int						GetChildren( HF_Ecs::Entity parent, HF_Ecs::Entity *out, int max ) const {
		int n = 0;
		for ( HF_Ecs::Entity c = GetFirstChild( parent ); c != HF_Ecs::ENTITY_NULL && n < max; c = GetNextSibling( c ) ) {
			out[n++] = c;
		}
		return n;
	}
	bool					IsDescendant( HF_Ecs::Entity maybeChild, HF_Ecs::Entity ancestor ) const {
		for ( HF_Ecs::Entity p = GetParent( maybeChild ); p != HF_Ecs::ENTITY_NULL; p = GetParent( p ) ) {
			if ( p == ancestor ) { return true; }
		}
		return false;
	}

private:
	void					EnsureIndex( unsigned int idx );
	void					DetachFromParent( HF_Ecs::Entity ent );

	HF_Ecs::Entity *		m_parent;
	HF_Ecs::Entity *		m_firstChild;
	HF_Ecs::Entity *		m_nextSibling;
	int						m_capacity;
	HF_Ecs::Registry *		m_registry;
};

HF_INLINE void hfHierarchySystem::EnsureIndex( unsigned int idx ) {
	if ( idx < ( unsigned int )m_capacity ) { return; }
	int cap = m_capacity ? m_capacity : 16;
	while ( ( unsigned int )cap <= idx ) { cap *= 2; }
	HF_Ecs::Entity *p = new HF_Ecs::Entity[ cap ];
	HF_Ecs::Entity *f = new HF_Ecs::Entity[ cap ];
	HF_Ecs::Entity *n = new HF_Ecs::Entity[ cap ];
	for ( int i = 0; i < m_capacity; i++ ) {
		p[i] = m_parent[i];
		f[i] = m_firstChild[i];
		n[i] = m_nextSibling[i];
	}
	for ( int i = m_capacity; i < cap; i++ ) {
		p[i] = HF_Ecs::ENTITY_NULL;
		f[i] = HF_Ecs::ENTITY_NULL;
		n[i] = HF_Ecs::ENTITY_NULL;
	}
	delete[] m_parent;
	delete[] m_firstChild;
	delete[] m_nextSibling;
	m_parent = p;
	m_firstChild = f;
	m_nextSibling = n;
	m_capacity = cap;
}

HF_INLINE void hfHierarchySystem::SetParent( HF_Ecs::Entity child, HF_Ecs::Entity parent ) {
	if ( child == HF_Ecs::ENTITY_NULL || child == parent ) { return; }
	unsigned int cIdx = child & HF_Ecs::INDEX_MASK;
	EnsureIndex( cIdx );

	// clear the child out of the linker state first
	if ( m_parent[cIdx] != HF_Ecs::ENTITY_NULL ) {
		DetachFromParent( child );
		m_parent[cIdx] = HF_Ecs::ENTITY_NULL;
	}
	if ( parent == HF_Ecs::ENTITY_NULL ) {
		return;
	}
	if ( IsDescendant( parent, child ) ) {		// would form a cycle
		return;
	}
	unsigned int pIdx = parent & HF_Ecs::INDEX_MASK;
	EnsureIndex( pIdx );
	m_parent[cIdx] = parent;
	m_nextSibling[cIdx] = m_firstChild[pIdx];
	m_firstChild[pIdx] = child;

	// make sure the parent entity looks the part in the world
	hfTransformComponent *pc = m_registry ? m_registry->GetComponent< hfTransformComponent >( parent ) : NULL;
	hfTransformComponent *cc = m_registry ? m_registry->GetComponent< hfTransformComponent >( child ) : NULL;
	if ( pc ) { pc->dirty = true; }
	if ( cc ) { cc->dirty = true; }
}

HF_INLINE void hfHierarchySystem::DetachFromParent( HF_Ecs::Entity ent ) {
	unsigned int idx = ent & HF_Ecs::INDEX_MASK;
	if ( idx >= ( unsigned int )m_capacity ) { return; }
	HF_Ecs::Entity parent = m_parent[idx];
	if ( parent == HF_Ecs::ENTITY_NULL ) { return; }
	unsigned int pIdx = parent & HF_Ecs::INDEX_MASK;
	if ( m_firstChild[pIdx] == ent ) {
		m_firstChild[pIdx] = m_nextSibling[idx];
	} else {
		HF_Ecs::Entity prev = m_firstChild[pIdx];
		while ( prev != HF_Ecs::ENTITY_NULL && m_nextSibling[prev & HF_Ecs::INDEX_MASK] != ent ) {
			prev = m_nextSibling[prev & HF_Ecs::INDEX_MASK];
		}
		if ( prev != HF_Ecs::ENTITY_NULL ) {
			m_nextSibling[prev & HF_Ecs::INDEX_MASK] = m_nextSibling[idx];
		}
	}
	m_nextSibling[idx] = HF_Ecs::ENTITY_NULL;
}

HF_INLINE void hfHierarchySystem::PreDestroy( HF_Ecs::Entity ent ) {
	unsigned int idx = ent & HF_Ecs::INDEX_MASK;
	if ( idx >= ( unsigned int )m_capacity ) { return; }

	// adopt the children out to our parent, then drop ourselves from the tree
	HF_Ecs::Entity parent = m_parent[idx];
	DetachFromParent( ent );
	m_parent[idx] = HF_Ecs::ENTITY_NULL;

	// collect children before reparenting them (their links change while we walk)
	HF_Ecs::Entity kids[ 64 ];
	int n = 0;
	for ( HF_Ecs::Entity c = m_firstChild[idx]; c != HF_Ecs::ENTITY_NULL && n < 64; c = m_nextSibling[c & HF_Ecs::INDEX_MASK] ) {
		kids[n++] = c;
	}
	m_firstChild[idx] = HF_Ecs::ENTITY_NULL;
	for ( int i = 0; i < n; i++ ) {
		SetParent( kids[i], parent );
	}
}

HF_INLINE void hfHierarchySystem::Validate() {
	for ( int i = 0; i < m_capacity; i++ ) {
		HF_Ecs::Entity parent = m_parent[i];
		if ( parent == HF_Ecs::ENTITY_NULL ) { continue; }
		HF_Ecs::Entity self = ( HF_Ecs::Entity )( i );			// gen-0 index, used only for the self-link case
		bool bad = ( parent == self );
		if ( !bad && m_registry ) { bad = !m_registry->IsAlive( parent ); }
		if ( bad ) {
			DetachFromParent( self );
			m_parent[ i ] = HF_Ecs::ENTITY_NULL;
		}
	}
}

HF_INLINE void hfHierarchySystem::Clear() {
	for ( int i = 0; i < m_capacity; i++ ) {
		m_parent[ i ] = HF_Ecs::ENTITY_NULL;
		m_firstChild[ i ] = HF_Ecs::ENTITY_NULL;
		m_nextSibling[ i ] = HF_Ecs::ENTITY_NULL;
	}
}

} // namespace HF_Scene

#endif