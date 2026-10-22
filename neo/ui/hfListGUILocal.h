#ifndef __LISTGUILOCAL_H__
#define __LISTGUILOCAL_H__

/*
===============================================================================

	feed data to a listDef
	each item has an id and a display string

===============================================================================
*/

class hfListGUILocal : protected hfList<hfStr, TAG_OLD_UI>, public hfListGUI {
public:
						hfListGUILocal() { m_pGUI = NULL; m_water = 0; m_stateUpdates = true; }

	// hfListGUI interface
	void				Config( hfUserInterface *pGUI, const char *name ) { m_pGUI = pGUI; m_name = name; }
	void				Add( int id, const hfStr& s );
						// use the element count as index for the ids
	void				Push( const hfStr& s );
	bool				Del( int id );
	void				Clear();
	int					Num() { return hfList<hfStr, TAG_OLD_UI>::Num(); }
	int					GetSelection( char *s, int size, int sel = 0 ) const; // returns the id, not the list index (or -1)
	void				SetSelection( int sel );
	int					GetNumSelections();
	bool				IsConfigured() const;
	void				SetStateChanges( bool enable );
	void				Shutdown();

private:
	hfUserInterface *	m_pGUI;
	hfStr				m_name;
	int					m_water;
	hfList<int, TAG_OLD_UI>			m_ids;
	bool				m_stateUpdates;

	void				StateChanged();
};

#endif /* !__LISTGUILOCAL_H__ */

