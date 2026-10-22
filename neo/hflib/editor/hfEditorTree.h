#ifndef __HF_EDITOR_TREE_H__
#define __HF_EDITOR_TREE_H__

/*
===============================================================================

	Phase 11 — the scene tree.

	An editor does not browse the registry; it browses a *tree*. The registry
	holds a flat, generation-tagged set of entities and the hierarchy
	component holds parent/child links, neither of which is an ordered
	structure you can walk top-down. `hfEditorTree` flattens both into one
	dense, depth-annotated array in a fixed order:

	  - entities come out of `Each< hfTransformComponent >`, which is the same
	    set `hfSceneWriter` serialises, so the tree and the file describe the
	    same scene;
	  - children keep creation order, so two runs over one scene build the
	    same tree and any dump of it can be diffed;
	  - an entity whose parent is not in that set is a root.

	Every structural operation the editor offers (select, reparent, delete,
	create) resolves through `FindByEntity` / `FindByTag` into an index in
	this array, and the tree is rebuilt after the scene changes. The array is
	the model, not a cache of anything the registry could answer better.

`DumpText` writes the tree as an indented name list. It exists so the harness
can compare two trees by text instead of by field, and so a tree can be
printed next to the scene file it came from.

===============================================================================
*/

#include "../hfprecompiled.h"
#include "../hfEcs.h"
#include "../scene/hfWorld.h"

namespace HF_Editor {

static const int HF_EDITOR_MAX_NODES = 512;

struct hfEditorTreeNode {
	HF_Ecs::Entity		entity;
	int					parent;			// -1 for a root
	int					firstChild;		// -1 when leafless
	int					nextSibling;	// -1 at the end of the sibling list
	int					depth;
	int					childCount;
	hfStr				name;			// hfTagComponent, empty when untagged
};

class hfEditorTree {
public:
	hfEditorTree() { Clear(); }

	void			Clear() {
		m_count = 0;
		m_rootCount = 0;
	}

	// Flatten 'world' into the node array. Deterministic for a given scene.
	void			Build( HF_Scene::hfWorld &world ) {
		Clear();
		HF_Ecs::Registry &reg = world.GetRegistry();

		reg.Each< HF_Scene::hfTransformComponent >(
			[ & ]( HF_Ecs::Entity e, HF_Scene::hfTransformComponent & ) {
				if ( m_count >= HF_EDITOR_MAX_NODES ) { return; }
				hfEditorTreeNode &n = m_nodes[ m_count ];
				n.entity = e;
				n.parent = -1;
				n.firstChild = -1;
				n.nextSibling = -1;
				n.depth = 0;
				n.childCount = 0;
				n.name = "";
				HF_Scene::hfTagComponent *tag =
					reg.GetComponent< HF_Scene::hfTagComponent >( e );
				if ( tag ) { n.name = tag->name; }
				m_count++;
			} );

		// resolve parents against the same set (a parent outside the set
		// makes the node a root, which is what the writer does too)
		for ( int i = 0; i < m_count; i++ ) {
			HF_Ecs::Entity p = world.GetParent( m_nodes[i].entity );
			if ( p == HF_Ecs::ENTITY_NULL ) { continue; }
			int pi = FindByEntity( p );
			if ( pi >= 0 && pi != i ) { m_nodes[i].parent = pi; }
		}

		// link children in entity order: walk to the current tail, append
		for ( int i = 0; i < m_count; i++ ) {
			int p = m_nodes[i].parent;
			if ( p < 0 ) { m_rootCount++; continue; }
			if ( m_nodes[p].firstChild < 0 ) {
				m_nodes[p].firstChild = i;
			} else {
				int c = m_nodes[p].firstChild;
				int guard = 0;
				while ( m_nodes[c].nextSibling >= 0 && guard++ < HF_EDITOR_MAX_NODES ) {
					c = m_nodes[c].nextSibling;
				}
				m_nodes[c].nextSibling = i;
			}
			m_nodes[p].childCount++;
		}

		// depth, with a guard so a malformed cycle cannot spin forever
		for ( int i = 0; i < m_count; i++ ) {
			int d = 0;
			int p = m_nodes[i].parent;
			int guard = 0;
			while ( p >= 0 && guard++ < HF_EDITOR_MAX_NODES ) {
				d++;
				p = m_nodes[p].parent;
			}
			m_nodes[i].depth = d;
		}
	}

	int				GetCount() const { return m_count; }
	int				GetRootCount() const { return m_rootCount; }

	const hfEditorTreeNode &Get( int i ) const { return m_nodes[ i ]; }

	int				FindRoot( int n ) const {
		int guard = 0;
		while ( n >= 0 && m_nodes[n].parent >= 0 && guard++ < HF_EDITOR_MAX_NODES ) {
			n = m_nodes[n].parent;
		}
		return n;
	}

	int				FindByEntity( HF_Ecs::Entity e ) const {
		for ( int i = 0; i < m_count; i++ ) {
			if ( m_nodes[i].entity == e ) { return i; }
		}
		return -1;
	}

	int				FindByTag( const char *tag ) const {
		if ( !tag || !*tag ) { return -1; }
		for ( int i = 0; i < m_count; i++ ) {
			if ( m_nodes[i].name.Length() && !hfStr::Icmp( m_nodes[i].name.c_str(), tag ) ) { return i; }
		}
		return -1;
	}

	// DFS over firstChild/nextSibling — the order an outline view shows
	void			Walk( void ( *fn )( int node, void *user ), void *user ) const {
		if ( !fn ) { return; }
		for ( int i = 0; i < m_count; i++ ) {
			if ( m_nodes[i].parent >= 0 ) { continue; }
			WalkSubtree( i, fn, user );
		}
	}

	// indented outline; two trees of one scene dump to identical text
	void			DumpText( hfStr &out ) const {
		out = "";
		for ( int i = 0; i < m_count; i++ ) {
			if ( m_nodes[i].parent >= 0 ) { continue; }
			DumpNode( i, out );
		}
	}

private:
	void			WalkSubtree( int n, void ( *fn )( int, void * ), void *user ) const {
		fn( n, user );
		int guard = 0;
		for ( int c = m_nodes[n].firstChild; c >= 0 && guard++ < HF_EDITOR_MAX_NODES;
			  c = m_nodes[c].nextSibling ) {
			WalkSubtree( c, fn, user );
		}
	}

	void			DumpNode( int n, hfStr &out ) const {
		const hfEditorTreeNode &node = m_nodes[n];
		for ( int d = 0; d < node.depth; d++ ) { out += "  "; }
		out += node.name.Length() ? node.name.c_str() : "(untagged)";
		out += "\n";
		int guard = 0;
		for ( int c = node.firstChild; c >= 0 && guard++ < HF_EDITOR_MAX_NODES;
			  c = m_nodes[c].nextSibling ) {
			DumpNode( c, out );
		}
	}

	hfEditorTreeNode	m_nodes[ HF_EDITOR_MAX_NODES ];
	int					m_count;
	int					m_rootCount;
};

} // namespace HF_Editor

#endif
