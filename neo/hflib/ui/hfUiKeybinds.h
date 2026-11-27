#ifndef __HF_UI_KEYBINDS_H__
#define __HF_UI_KEYBINDS_H__

/*
===============================================================================

	Phase 17 — hfUiKeybinds.

	A small action -> key map with conflict detection.  The engine already has
	key numbers (keyNum_t) and a cvar-backed bind system for the legacy input
	path; this is the UI/game-facing equivalent: named actions, one primary and
	one secondary key, and a hard refusal to put two actions on the same key.

	Conflicts are checked on every Bind(), so remapping cannot silently steal a
	key from another action.  Parse() trusts the file and does not reject
	duplicates — a hand-edited config is repaired by the next Bind(), and the
	loader reports nothing it cannot prove.

	Text format:

		move_up = 17,0     # K_W
		fire    = 286,0    # K_MOUSE1
		dodge   = 57,0     # K_SPACE

	`key = primary,secondary`, key numbers as integers (0 = unbound).
	Symbolic `K_*` names are for C++ code; Parse() expects integers.

===============================================================================
*/

#include "../hfprecompiled.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

namespace HF_Ui {

class hfUiKeybinds {
public:
	enum {
		HF_UI_KEYBIND_MAX_ACTIONS = 32,
		HF_UI_KEYBIND_MAX_NAME = 32
	};

	hfUiKeybinds() : m_count( 0 ), m_lastConflict( -1 ) {}

	void Clear() {
		m_count = 0;
		m_lastConflict = -1;
	}

	void SetDefaults() {
		Clear();
		Bind( "move_up", K_W );
		Bind( "move_down", K_S );
		Bind( "move_left", K_A );
		Bind( "move_right", K_D );
		Bind( "fire", K_MOUSE1 );
		Bind( "dodge", K_SPACE );
		Bind( "pause", K_ESCAPE );
		Bind( "confirm", K_ENTER );
	}

	int Count() const { return m_count; }

	const char *ActionAt( int index ) const {
		if ( index < 0 || index >= m_count ) { return NULL; }
		return m_entries[ index ].action;
	}

	int Primary( int index ) const {
		if ( index < 0 || index >= m_count ) { return 0; }
		return m_entries[ index ].primary;
	}

	int Secondary( int index ) const {
		if ( index < 0 || index >= m_count ) { return 0; }
		return m_entries[ index ].secondary;
	}

	int Find( const char *action ) const {
		for ( int i = 0; i < m_count; i++ ) {
			if ( hfStr::Icmp( m_entries[ i ].action, action ) == 0 ) { return i; }
		}
		return -1;
	}

	// First action bound to a key (primary or secondary), or NULL.
	const char *ActionForKey( int key ) const {
		if ( key == 0 ) { return NULL; }
		for ( int i = 0; i < m_count; i++ ) {
			if ( m_entries[ i ].primary == key || m_entries[ i ].secondary == key ) {
				return m_entries[ i ].action;
			}
		}
		return NULL;
	}

	// NULL when the key is free for this action, otherwise the action that
	// already owns it.
	const char *FindConflict( const char *action, int key ) const {
		if ( key == 0 ) { return NULL; }
		for ( int i = 0; i < m_count; i++ ) {
			if ( hfStr::Icmp( m_entries[ i ].action, action ) == 0 ) { continue; }
			if ( m_entries[ i ].primary == key || m_entries[ i ].secondary == key ) {
				return m_entries[ i ].action;
			}
		}
		return NULL;
	}

	// slot 0 = primary, 1 = secondary.  Fails (and records the conflict) when
	// another action already owns the key.
	bool Bind( const char *action, int key, int slot = 0 ) {
		if ( action == NULL || action[ 0 ] == '\0' ) { return false; }
		const char *conflict = FindConflict( action, key );
		if ( conflict != NULL ) {
			const int index = Find( conflict );
			m_lastConflict = index;
			return false;
		}

		int index = Find( action );
		if ( index < 0 ) {
			if ( m_count >= HF_UI_KEYBIND_MAX_ACTIONS ) { return false; }
			index = m_count++;
			hfStr::Copynz( m_entries[ index ].action, action, sizeof( m_entries[ index ].action ) );
			m_entries[ index ].primary = 0;
			m_entries[ index ].secondary = 0;
		}

		if ( slot == 1 ) {
			m_entries[ index ].secondary = key;
		} else {
			m_entries[ index ].primary = key;
			// keep the pair from aliasing when rebinding the primary
			if ( m_entries[ index ].secondary == key ) {
				m_entries[ index ].secondary = 0;
			}
		}
		m_lastConflict = -1;
		return true;
	}

	bool Unbind( const char *action ) {
		const int index = Find( action );
		if ( index < 0 ) { return false; }
		m_entries[ index ].primary = 0;
		m_entries[ index ].secondary = 0;
		return true;
	}

	int GetLastConflict() const { return m_lastConflict; }
	const char *GetLastConflictAction() const {
		return ( m_lastConflict >= 0 && m_lastConflict < m_count ) ? m_entries[ m_lastConflict ].action : NULL;
	}

	// --- file format ------------------------------------------------------

	int Serialize( char *out, int maxSize ) const {
		if ( out == NULL || maxSize <= 0 ) { return 0; }
		out[ 0 ] = '\0';
		for ( int i = 0; i < m_count; i++ ) {
			const int used = ( int )strlen( out );
			if ( used >= maxSize - 1 ) { break; }
			snprintf( out + used, ( size_t )( maxSize - used ), "%s = %d,%d\n",
					  m_entries[ i ].action, m_entries[ i ].primary, m_entries[ i ].secondary );
		}
		return ( int )strlen( out );
	}

	bool Parse( const char *text ) {
		if ( text == NULL ) { return false; }
		Clear();
		int loaded = 0;
		const char *p = text;
		while ( *p != '\0' ) {
			const char *lineEnd = p;
			while ( *lineEnd != '\0' && *lineEnd != '\n' ) { lineEnd++; }

			char line[ 128 ];
			int len = ( int )( lineEnd - p );
			if ( len > ( int )sizeof( line ) - 1 ) { len = ( int )sizeof( line ) - 1; }
			memcpy( line, p, len );
			line[ len ] = '\0';
			p = ( *lineEnd == '\n' ) ? lineEnd + 1 : lineEnd;

			char *comment = strchr( line, '#' );
			if ( comment != NULL ) { *comment = '\0'; }

			char *eq = strchr( line, '=' );
			if ( eq == NULL ) { continue; }
			*eq = '\0';

			char *action = line;
			while ( *action == ' ' || *action == '\t' ) { action++; }
			char *actionEnd = action + strlen( action );
			while ( actionEnd > action && ( actionEnd[ -1 ] == ' ' || actionEnd[ -1 ] == '\t' || actionEnd[ -1 ] == '\r' ) ) {
				*( --actionEnd ) = '\0';
			}

			char *value = eq + 1;
			while ( *value == ' ' || *value == '\t' ) { value++; }
			if ( *action == '\0' || *value == '\0' ) { continue; }

			int primary = 0, secondary = 0;
			if ( sscanf( value, "%d , %d", &primary, &secondary ) < 1 ) { continue; }

			if ( m_count >= HF_UI_KEYBIND_MAX_ACTIONS ) { break; }
			hfStr::Copynz( m_entries[ m_count ].action, action, sizeof( m_entries[ m_count ].action ) );
			m_entries[ m_count ].primary = primary;
			m_entries[ m_count ].secondary = secondary;
			m_count++;
			loaded++;
		}
		return loaded > 0;
	}

private:
	struct Entry {
		char	action[ HF_UI_KEYBIND_MAX_NAME ];
		int		primary;
		int		secondary;
	};

	Entry	m_entries[ HF_UI_KEYBIND_MAX_ACTIONS ];
	int		m_count;
	int		m_lastConflict;
};

} // namespace HF_Ui

#endif
