#ifndef __EDITWINDOW_H__
#define __EDITWINDOW_H__

#include "hfWindow.h"

const int MAX_EDITFIELD = 4096;

class hfUserInterfaceLocal;
class hfSliderWindow;

class hfEditWindow : public hfWindow {
public:
						hfEditWindow(hfUserInterfaceLocal *gui);
	virtual 			~hfEditWindow();

	virtual void		Draw( int time, float x, float y );
	virtual const char *HandleEvent( const sysEvent_t *event, bool *updateVisuals );
	virtual void		PostParse();
	virtual void		GainFocus();
	virtual size_t		Allocated(){return hfWindow::Allocated();};
	
	virtual hfWinVar *	GetWinVarByName(const char *_name, bool winLookup = false, drawWin_t** owner = NULL );
	
	virtual void 		HandleBuddyUpdate(hfWindow *buddy);
	virtual void		Activate(bool activate, hfStr &act);
	
	void				RunNamedEvent( const char* eventName );
	
private:

	virtual bool		ParseInternalVar(const char *name, hfTokenParser *src);

	void				InitCvar();
						// true: read the updated cvar from cvar system
						// false: write to the cvar system
						// force == true overrides liveUpdate 0
	void				UpdateCvar( bool read, bool force = false );
	
	void				CommonInit();
	void				EnsureCursorVisible();
	void				InitScroller( bool horizontal );
	
	int					maxChars;
	int					paintOffset;
	int					cursorPos;
	int					cursorLine;
	int					cvarMax;
	bool				wrap;
	bool				readonly;
	bool				numeric;
	hfStr				sourceFile;
	hfSliderWindow *	scroller;
	hfList<int>			breaks;
	float				sizeBias;
	int					textIndex;
	int					lastTextLength;
	bool				forceScroll;	
	hfWinBool			password;

	hfWinStr			cvarStr;
	hfCVar *			cvar;

	hfWinBool			liveUpdate;
	hfWinStr			cvarGroup;
};

#endif /* !__EDITWINDOW_H__ */

