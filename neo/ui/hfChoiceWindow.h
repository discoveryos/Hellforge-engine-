#ifndef __CHOICEWINDOW_H
#define __CHOICEWINDOW_H

#include "hfWindow.h"

class hfUserInterfaceLocal;
class hfChoiceWindow : public hfWindow {
public:
						hfChoiceWindow(hfUserInterfaceLocal *gui);
	virtual				~hfChoiceWindow();

	virtual const char	*HandleEvent(const sysEvent_t *event, bool *updateVisuals);
	virtual void 		PostParse();
	virtual void 		Draw(int time, float x, float y);
	virtual void		Activate( bool activate, hfStr &act );
	virtual size_t		Allocated(){return hfWindow::Allocated();}; 
  
	virtual hfWinVar	*GetWinVarByName(const char *_name, bool winLookup = false, drawWin_t** owner = NULL);

	void				RunNamedEvent( const char* eventName );
	
private:
	virtual bool		ParseInternalVar(const char *name, hfTokenParser *src);
	void				CommonInit();
	void				UpdateChoice();
	void				ValidateChoice();
	
	void				InitVars();
						// true: read the updated cvar from cvar system, gui from dict
						// false: write to the cvar system, to the gui dict
						// force == true overrides liveUpdate 0
	void				UpdateVars( bool read, bool force = false );

	void				UpdateChoicesAndVals();
	
	int					currentChoice;
	int					choiceType;
	hfStr				latchedChoices;
	hfWinStr			choicesStr;
	hfStr				latchedVals;
	hfWinStr			choiceVals;
	hfStrList			choices;
	hfStrList			values;

	hfWinStr			guiStr;
	hfWinStr			cvarStr;
	hfCVar *			cvar;
	hfMultiWinVar		updateStr;

	hfWinBool			liveUpdate;
	hfWinStr			updateGroup;
};

#endif // __CHOICEWINDOW_H

