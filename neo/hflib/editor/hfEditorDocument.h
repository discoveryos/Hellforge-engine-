#ifndef __HF_EDITOR_DOCUMENT_H__
#define __HF_EDITOR_DOCUMENT_H__

/*
===============================================================================

	Phase 11 — the editor document.

	The piece the roadmap actually validates: *open → edit → save → reload
	keeps semantics, and the round-trip diff of the text scene file is
	empty*. Everything else in the editor hangs off that contract.

	`hfEditorDocument` owns one `hfWorld` and one `hfEditorTree`, and offers
	the four things a document has to do:

	  open   `hfSceneReader::Read` into a cleared world, then rebuild the tree.
	  edit   selection + a flat inspector sheet + structural operations. Every
	         mutation bumps the dirty flag and rebuilds the tree, so the tree
	         is never stale relative to the world.
	  save   `hfSceneWriter::Write` back to text.
	  reload is just open(text), which is what the test does — there is no
	         separate path that could preserve something a real load would drop.

	The sheet is the inspector's model: for the selected entity it lists every
	registered component that entity actually has, in registration order, as
	flat `(class, property)` pairs. A UI binds row *i* to a widget by
	`hfEditorProperty::type` and writes back through `SetPropertyValue`. The
	document never knows what the widget is; a headless test drives the same
	API, which is why this can be verified without a window.

	What is deliberately *not* here: gizmos, play-in-editor and the import
	view. Those need the renderer or a running sim loop and are recorded as
	known gaps rather than stubbed out.

===============================================================================
*/

#include "../hfprecompiled.h"
#include "../scene/hfWorld.h"
#include "../scene/hfSceneSerializer.h"
#include "hfEditorReflection.h"
#include "hfEditorTree.h"

namespace HF_Editor {

struct hfEditorSheetEntry {
	int		classIndex;
	int		propIndex;
};

class hfEditorDocument {
public:
	hfEditorDocument() : m_selection( -1 ), m_sheetSize( 0 ),
		m_dirty( false ), m_editCount( 0 ) {
		hfEditorRegisterBuiltins();
		m_tree.Clear();
	}

	// ---- document lifecycle --------------------------------------------

	// Replace the world with the scene described by 'text'.
	bool			Open( const char *text ) {
		m_world.Clear();
		bool ok = HF_Scene::hfSceneReader::Read( m_world, text ? text : "" );
		RefreshTree();
		m_selection = -1;
		m_sheetSize = 0;
		m_dirty = false;
		m_editCount = 0;
		return ok;
	}

	bool			OpenEmpty() { return Open( "" ); }

	bool			Save( hfStr &out ) {
		return HF_Scene::hfSceneWriter::Write( m_world, out );
	}

	// Rebuild the outline from the world. Call after any structural change.
	void			RefreshTree() {
		m_tree.Build( m_world );
		if ( m_selection >= m_tree.GetCount() ) {
			m_selection = -1;
			m_sheetSize = 0;
		}
	}

	HF_Scene::hfWorld &		GetWorld() { return m_world; }
	const hfEditorTree &	GetTree() const { return m_tree; }

	bool			IsDirty() const { return m_dirty; }
	void			MarkClean() { m_dirty = false; }
	int				GetEditCount() const { return m_editCount; }

	// ---- selection ------------------------------------------------------

	bool			Select( int node ) {
		if ( node < 0 || node >= m_tree.GetCount() ) { return false; }
		m_selection = node;
		RebuildSheet();
		return true;
	}

	bool			SelectByTag( const char *tag ) { return Select( m_tree.FindByTag( tag ) ); }
	void			ClearSelection() { m_selection = -1; m_sheetSize = 0; }

	int				GetSelection() const { return m_selection; }

	HF_Ecs::Entity	GetSelectedEntity() const {
		return ( m_selection >= 0 && m_selection < m_tree.GetCount() )
			 ? m_tree.Get( m_selection ).entity : HF_Ecs::ENTITY_NULL;
	}

	// ---- inspector sheet -------------------------------------------------

	void			RebuildSheet() {
		m_sheetSize = 0;
		HF_Ecs::Entity e = GetSelectedEntity();
		if ( e == HF_Ecs::ENTITY_NULL ) { return; }
		int classes = hfEditorReflection::GetClassCount();
		for ( int c = 0; c < classes && m_sheetSize < HF_EDITOR_MAX_SHEET; c++ ) {
			const hfEditorComponentClass *cls = hfEditorReflection::GetClass( c );
			if ( !cls || !cls->has || !cls->has( m_world.GetRegistry(), e ) ) { continue; }
			for ( int p = 0; p < cls->propCount && m_sheetSize < HF_EDITOR_MAX_SHEET; p++ ) {
				m_sheet[ m_sheetSize ].classIndex = c;
				m_sheet[ m_sheetSize ].propIndex = p;
				m_sheetSize++;
			}
		}
	}

	int				GetSheetSize() const { return m_sheetSize; }

	bool			GetSheetEntry( int i, const hfEditorComponentClass **cls,
								   const hfEditorProperty **prop ) const {
		if ( i < 0 || i >= m_sheetSize ) { return false; }
		const hfEditorComponentClass *c =
			hfEditorReflection::GetClass( m_sheet[i].classIndex );
		if ( !c ) { return false; }
		if ( cls ) { *cls = c; }
		if ( prop ) { *prop = &c->props[ m_sheet[i].propIndex ]; }
		return true;
	}

	bool			GetPropertyValue( int i, hfEditorValue &out ) {
		if ( i < 0 || i >= m_sheetSize ) { return false; }
		const hfEditorComponentClass *c =
			hfEditorReflection::GetClass( m_sheet[i].classIndex );
		if ( !c ) { return false; }
		return hfEditorReflection::ReadValue( *c, m_world.GetRegistry(),
			GetSelectedEntity(), m_sheet[i].propIndex, out );
	}

	bool			SetPropertyValue( int i, const hfEditorValue &in ) {
		if ( i < 0 || i >= m_sheetSize ) { return false; }
		const hfEditorComponentClass *c =
			hfEditorReflection::GetClass( m_sheet[i].classIndex );
		if ( !c ) { return false; }
		if ( !hfEditorReflection::WriteValue( *c, m_world.GetRegistry(),
			 GetSelectedEntity(), m_sheet[i].propIndex, in ) ) { return false; }
		Touch();
		return true;
	}

	// convenience: locate a property by "Component.Property"
	int				FindSheetEntry( const char *qualified ) const {
		if ( !qualified ) { return -1; }
		const char *dot = strchr( qualified, '.' );
		if ( !dot ) { return -1; }
		hfStr compName;
		for ( const char *p = qualified; p && p != dot; p++) { compName += *p; }
		for ( int i = 0; i < m_sheetSize; i++ ) {
			const hfEditorComponentClass *cls = NULL;
			const hfEditorProperty *prop = NULL;
			if ( !GetSheetEntry( i, &cls, &prop ) ) { continue; }
			if ( hfStr::Icmp( cls->name, compName.c_str() ) ) { continue; }
			if ( !hfStr::Icmp( prop->name, dot + 1 ) ) { return i; }
		}
		return -1;
	}

	// ---- structural edits -----------------------------------------------

	HF_Ecs::Entity	CreateEntity( const char *tag, int parentNode ) {
		HF_Ecs::Entity e = ( tag && *tag ) ? m_world.CreateEntity( tag ) : m_world.CreateEntity();
		if ( e == HF_Ecs::ENTITY_NULL ) { return e; }
		if ( parentNode >= 0 && parentNode < m_tree.GetCount() ) {
			m_world.SetParent( e, m_tree.Get( parentNode ).entity );
		}
		Touch();
		return e;
	}

	bool			Reparent( int node, int newParent ) {
		if ( node < 0 || node >= m_tree.GetCount() ) { return false; }
		if ( newParent >= 0 ) {
			if ( newParent >= m_tree.GetCount() || newParent == node ) { return false; }
			// walking up from the proposed parent must never reach 'node',
			// otherwise we would hang the hierarchy off its own descendant
			int a = newParent;
			for ( int guard = 0; a >= 0 && guard < HF_EDITOR_MAX_NODES;
				  guard++ ) {
				if ( a == node ) { return false; }
				a = m_tree.Get( a ).parent;
			}
		}
		HF_Ecs::Entity child = m_tree.Get( node ).entity;
		HF_Ecs::Entity parent = ( newParent >= 0 ) ? m_tree.Get( newParent ).entity
												   : HF_Ecs::ENTITY_NULL;
		m_world.SetParent( child, parent );
		Touch();
		return true;
	}

	bool			DeleteEntity( int node ) {
		if ( node < 0 || node >= m_tree.GetCount() ) { return false; }
		HF_Ecs::Entity e = m_tree.Get( node ).entity;
		if ( m_selection == node ) { m_selection = -1; m_sheetSize = 0; }
		else if ( m_selection > node ) { m_selection--; }
		m_world.DestroyEntity( e );
		Touch();
		return true;
	}

	bool			AddComponent( int node, const char *className ) {
		if ( node < 0 || node >= m_tree.GetCount() ) { return false; }
		const hfEditorComponentClass *cls = hfEditorReflection::FindByName( className );
		if ( !cls || !cls->add ) { return false; }
		HF_Ecs::Entity e = m_tree.Get( node ).entity;
		if ( !cls->has || !cls->has( m_world.GetRegistry(), e ) ) {
			cls->add( m_world.GetRegistry(), e );
		}
		Touch();
		if ( m_selection == node ) { RebuildSheet(); }
		return true;
	}

	bool			RemoveComponent( int node, const char *className ) {
		// hfEcs stores components by entity with no per-type removal, so the
		// editor can add a component but cannot take one back off. Refusing
		// here is better than reporting success and saving it anyway.
		( void )node; ( void )className;
		return false;
	}

	// rename: creates the tag component when the entity has none
	bool			SetName( int node, const char *name ) {
		if ( node < 0 || node >= m_tree.GetCount() || !name ) { return false; }
		HF_Ecs::Entity e = m_tree.Get( node ).entity;
		HF_Scene::hfTagComponent *tag = m_world.GetComponent< HF_Scene::hfTagComponent >( e );
		if ( !tag ) { tag = &m_world.AddComponent< HF_Scene::hfTagComponent >( e ); }
		tag->name = name;
		Touch();
		return true;
	}

private:
	void			Touch() {
		m_dirty = true;
		m_editCount++;
		m_tree.Build( m_world );
		if ( m_selection >= 0 && m_selection < m_tree.GetCount() ) {
			RebuildSheet();
		} else {
			m_sheetSize = 0;
		}
	}

	HF_Scene::hfWorld		m_world;
	hfEditorTree			m_tree;
	hfEditorSheetEntry		m_sheet[ HF_EDITOR_MAX_SHEET ];
	int						m_sheetSize;
	int						m_selection;
	bool					m_dirty;
	int						m_editCount;
};

} // namespace HF_Editor

#endif
