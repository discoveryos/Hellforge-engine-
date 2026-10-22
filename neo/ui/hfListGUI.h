#ifndef __LISTGUI_H__
#define __LISTGUI_H__

/*
===============================================================================

	feed data to a listDef
	each item has an id and a display string

===============================================================================
*/

class hfListGUI {
public:
	virtual				~hfListGUI() { }

	virtual void		Config( hfUserInterface *pGUI, const char *name ) = 0;
	virtual void		Add( int id, const hfStr& s ) = 0;
						// use the element count as index for the ids
	virtual void		Push( const hfStr& s ) = 0;
	virtual bool		Del( int id ) = 0;
	virtual void		Clear() = 0;
	virtual int			Num() = 0;
	virtual int			GetSelection( char *s, int size, int sel = 0 ) const = 0; // returns the id, not the list index (or -1)
	virtual void		SetSelection( int sel ) = 0;
	virtual int			GetNumSelections() = 0;
	virtual bool		IsConfigured() const = 0;
						// by default, any modification to the list will trigger a full GUI refresh immediately
	virtual void		SetStateChanges( bool enable ) = 0;
	virtual void		Shutdown() = 0;
};

#endif /* !__LISTGUI_H__ */

