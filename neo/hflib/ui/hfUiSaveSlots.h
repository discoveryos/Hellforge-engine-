#ifndef __HF_UI_SAVE_SLOTS_H__
#define __HF_UI_SAVE_SLOTS_H__

/*
===============================================================================

	Phase 17 — hfUiSaveSlots.

	A small save-slot panel built on the Phase 13 per-account console storage.
	It owns no policy: the host supplies the hfConsoleServices implementation
	and the signed-in user, and the panel renders one row per slot with
	LOAD / SAVE / DELETE buttons.

	The panel is deliberately a *facade plus a mapping*, not an event source.
	After a click the host (or a test) resolves the clicked entity with
	Resolve(), so save/load policy lives in one place — the game — instead of
	being split between the UI and a callback table.

	Slot names are derived from the index (`save1`, `save2`, ...), which keeps
	the console-side keys stable across runs and platforms.

===============================================================================
*/

#include "../hfprecompiled.h"
#include "../hfEcs.h"
#include "../console/hfConsoleServices.h"
#include "hfCanvasComponent.h"
#include "hfUiWidget.h"
#include "hfUiBuilder.h"
#include "hfUiSystem.h"
#include <stdio.h>
#include <string.h>

namespace HF_Ui {

using hfconsole::hfConsoleServices;
using hfconsole::hfConsoleUserId;

class hfUiSaveSlots {
public:
	enum {
		HF_UI_SAVE_SLOT_MAX = 8,
		HF_UI_SAVE_KIND_LOAD = 0,
		HF_UI_SAVE_KIND_SAVE = 1,
		HF_UI_SAVE_KIND_DELETE = 2
	};

	hfUiSaveSlots() {
		m_services = NULL;
		m_user = hfConsoleUserId();
		m_slots = 0;
		m_root = HF_Ecs::ENTITY_NULL;
		for ( int i = 0; i < HF_UI_SAVE_SLOT_MAX; i++ ) {
			m_label[ i ] = m_load[ i ] = m_save[ i ] = m_delete[ i ] = HF_Ecs::ENTITY_NULL;
		}
	}

	// Build (or rebuild) the panel inside `parent`.  Returns the panel root.
	HF_Ecs::Entity Build( HF_Ecs::Registry &registry, hfUiSystem &ui, HF_Ecs::Entity parent,
						  hfConsoleServices *services, hfConsoleUserId user,
						  float x, float y, float w, float h, int slotCount ) {
		m_services = services;
		m_user = user;
		m_slots = slotCount;
		if ( m_slots < 1 ) { m_slots = 1; }
		if ( m_slots > HF_UI_SAVE_SLOT_MAX ) { m_slots = HF_UI_SAVE_SLOT_MAX; }
		for ( int i = 0; i < HF_UI_SAVE_SLOT_MAX; i++ ) {
			m_label[ i ] = m_load[ i ] = m_save[ i ] = m_delete[ i ] = HF_Ecs::ENTITY_NULL;
		}

		m_root = hfUiBuilder::CreatePanel( registry, parent, x, y, w, h, 0.10f, 0.11f, 0.15f, 1.0f );

		const float rowH = 40.0f;
		for ( int i = 0; i < m_slots; i++ ) {
			const float ry = 10.0f + ( float )i * ( rowH + 8.0f );
			char name[ 32 ];
			snprintf( name, sizeof( name ), "SLOT %d", i + 1 );
			m_label[ i ] = hfUiBuilder::CreateLabel( registry, m_root, 12.0f, ry + 8.0f,
													 170.0f, 24.0f, name, 1.25f, 1.0f, 1.0f, 1.0f, 1.0f );

			m_load[ i ] = hfUiBuilder::CreateButton( registry, m_root, 180.0f, ry, 92.0f, rowH, "LOAD" );
			m_save[ i ] = hfUiBuilder::CreateButton( registry, m_root, 280.0f, ry, 92.0f, rowH, "SAVE" );
			m_delete[ i ] = hfUiBuilder::CreateButton( registry, m_root, 380.0f, ry, 92.0f, rowH, "DELETE" );
		}

		Refresh( registry, ui );
		return m_root;
	}

	// Re-read the slot sizes and rewrite the labels ("EMPTY" or "<n> B").
	void Refresh( HF_Ecs::Registry &registry, hfUiSystem &ui ) {
		for ( int i = 0; i < m_slots; i++ ) {
			hfUiLabelComponent *lab = registry.GetComponent< hfUiLabelComponent >( m_label[ i ] );
			if ( lab == NULL ) { continue; }
			char name[ 32 ];
			snprintf( name, sizeof( name ), "SLOT %d: %s", i + 1, m_services != NULL ? SizeText( i ) : "EMPTY" );
			lab->SetText( name );
		}
		ui.Update( registry, 0.0f );
	}

	// Resolve a clicked entity to (slot, kind).  Returns false for entities
	// that are not one of this panel's buttons.
	bool Resolve( HF_Ecs::Entity entity, int &slot, int &kind ) const {
		for ( int i = 0; i < m_slots; i++ ) {
			if ( entity == m_load[ i ] ) { slot = i; kind = HF_UI_SAVE_KIND_LOAD; return true; }
			if ( entity == m_save[ i ] ) { slot = i; kind = HF_UI_SAVE_KIND_SAVE; return true; }
			if ( entity == m_delete[ i ] ) { slot = i; kind = HF_UI_SAVE_KIND_DELETE; return true; }
		}
		return false;
	}

	// --- storage facade ---------------------------------------------------

	bool Save( int slot, const void *data, int dataSize ) {
		if ( m_services == NULL || slot < 0 || slot >= m_slots ) { return false; }
		return m_services->WriteSlot( m_user, SlotName( slot ), ( const byte * )data, dataSize );
	}

	int Load( int slot, void *out, int maxSize ) {
		if ( m_services == NULL || slot < 0 || slot >= m_slots ) { return 0; }
		return m_services->ReadSlot( m_user, SlotName( slot ), ( byte * )out, maxSize );
	}

	bool Delete( int slot ) {
		if ( m_services == NULL || slot < 0 || slot >= m_slots ) { return false; }
		m_services->DeleteSlot( m_user, SlotName( slot ) );
		return true;
	}

	int SlotBytes( int slot ) const {
		if ( m_services == NULL || slot < 0 || slot >= m_slots ) { return 0; }
		return m_services->SlotSize( m_user, SlotName( slot ) );
	}

	static const char *SlotName( int slot ) {
		static char names[ HF_UI_SAVE_SLOT_MAX ][ 16 ];
		if ( slot < 0 || slot >= HF_UI_SAVE_SLOT_MAX ) { return "save0"; }
		snprintf( names[ slot ], sizeof( names[ slot ] ), "save%d", slot + 1 );
		return names[ slot ];
	}

	HF_Ecs::Entity GetRoot() const { return m_root; }
	HF_Ecs::Entity GetLoadButton( int slot ) const { return ( slot >= 0 && slot < m_slots ) ? m_load[ slot ] : HF_Ecs::ENTITY_NULL; }
	HF_Ecs::Entity GetSaveButton( int slot ) const { return ( slot >= 0 && slot < m_slots ) ? m_save[ slot ] : HF_Ecs::ENTITY_NULL; }
	HF_Ecs::Entity GetDeleteButton( int slot ) const { return ( slot >= 0 && slot < m_slots ) ? m_delete[ slot ] : HF_Ecs::ENTITY_NULL; }
	int GetSlotCount() const { return m_slots; }

private:
	const char *SizeText( int slot ) const {
		static char text[ 24 ];
		const int bytes = m_services->SlotSize( m_user, SlotName( slot ) );
		if ( bytes <= 0 ) {
			return "EMPTY";
		}
		snprintf( text, sizeof( text ), "%d B", bytes );
		return text;
	}

	hfConsoleServices *	m_services;
	hfConsoleUserId		m_user;
	int					m_slots;
	HF_Ecs::Entity		m_root;
	HF_Ecs::Entity		m_label[ HF_UI_SAVE_SLOT_MAX ];
	HF_Ecs::Entity		m_load[ HF_UI_SAVE_SLOT_MAX ];
	HF_Ecs::Entity		m_save[ HF_UI_SAVE_SLOT_MAX ];
	HF_Ecs::Entity		m_delete[ HF_UI_SAVE_SLOT_MAX ];
};

} // namespace HF_Ui

#endif
