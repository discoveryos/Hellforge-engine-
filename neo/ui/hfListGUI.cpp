#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfListGUILocal.h"

/*
====================
hfListGUILocal::StateChanged
====================
*/
void hfListGUILocal::StateChanged() {
	int i;

	if ( !m_stateUpdates ) {
		return;
	}

	for( i = 0; i < Num(); i++ ) {
		m_pGUI->SetStateString( va( "%s_item_%i", m_name.c_str(), i ), (*this)[i].c_str() ); 
	}
	for( i = Num() ; i < m_water ; i++ ) {
		m_pGUI->SetStateString( va( "%s_item_%i", m_name.c_str(), i ), "" );
	}
	m_water = Num();
	m_pGUI->StateChanged( Sys_Milliseconds() );
}

/*
====================
hfListGUILocal::GetNumSelections
====================
*/
int hfListGUILocal::GetNumSelections() {
	return m_pGUI->State().GetInt( va( "%s_numsel", m_name.c_str() ) );
}

/*
====================
hfListGUILocal::GetSelection
====================
*/
int hfListGUILocal::GetSelection( char *s, int size, int _sel ) const {
	if ( s ) {		
		s[ 0 ] = '\0';
	}
	int sel = m_pGUI->State().GetInt( va( "%s_sel_%i", m_name.c_str(), _sel ), "-1" );
	if ( sel == -1 || sel >= m_ids.Num() ) {
		return -1;
	}
	if ( s ) {
		hfStr::snPrintf( s, size, m_pGUI->State().GetString( va( "%s_item_%i", m_name.c_str(), sel ), "" ) );
	}
	// don't let overflow
	if ( sel >= m_ids.Num() ) {
		sel = 0;
	}
	m_pGUI->SetStateInt( va( "%s_selid_0", m_name.c_str() ), m_ids[ sel ] ); 
	return m_ids[ sel ];
}

/*
====================
hfListGUILocal::SetSelection
====================
*/
void hfListGUILocal::SetSelection( int sel ) {
	m_pGUI->SetStateInt( va( "%s_sel_0", m_name.c_str() ), sel );
	StateChanged();
}

/*
====================
hfListGUILocal::Add
====================
*/
void hfListGUILocal::Add( int id, const hfStr &s ) {
	int i = m_ids.FindIndex( id );
	if ( i == -1 ) {
		Append( s );
		m_ids.Append( id );
	} else {
		(*this)[ i ] = s;
	}
	StateChanged();
}

/*
====================
hfListGUILocal::Push
====================
*/
void hfListGUILocal::Push( const hfStr& s ) {
	Append( s );
	m_ids.Append( m_ids.Num() );
	StateChanged();
}

/*
====================
hfListGUILocal::Del
====================
*/
bool hfListGUILocal::Del(int id) {
	int i = m_ids.FindIndex(id);
	if ( i == -1 ) {
		return false;
	}
	m_ids.RemoveIndex( i );
	this->RemoveIndex( i );
	StateChanged();
	return true;
}

/*
====================
hfListGUILocal::Clear
====================
*/
void hfListGUILocal::Clear() {
	m_ids.Clear();
	hfList<hfStr, TAG_OLD_UI>::Clear();
	if ( m_pGUI ) {
		// will clear all the GUI variables and will set m_water back to 0
		StateChanged();
	}
}

/*
====================
hfListGUILocal::IsConfigured
====================
*/
bool hfListGUILocal::IsConfigured() const {
	return m_pGUI != NULL;
}

/*
====================
hfListGUILocal::SetStateChanges
====================
*/
void hfListGUILocal::SetStateChanges( bool enable ) {
	m_stateUpdates = enable;
	StateChanged();
}

/*
====================
hfListGUILocal::Shutdown
====================
*/
void hfListGUILocal::Shutdown() {
	m_pGUI = NULL;
	m_name.Clear();
	Clear();
}

