#ifndef __BINDWINDOW_H
#define __BINDWINDOW_H

class hfUserInterfaceLocal;
class hfBindWindow : public hfWindow {
public:
	hfBindWindow(hfUserInterfaceLocal *gui);
	virtual ~hfBindWindow();

	virtual const char *HandleEvent(const sysEvent_t *event, bool *updateVisuals);
	virtual void PostParse();
	virtual void Draw(int time, float x, float y);
	virtual size_t Allocated(){return hfWindow::Allocated();};
// 
//  
	virtual hfWinVar *GetWinVarByName(const char *_name, bool winLookup = false, drawWin_t** owner = NULL );
// 
	virtual void Activate( bool activate, hfStr &act );
	
private:
	void CommonInit();
	hfWinStr bindName;
	bool waitingOnKey;
};

#endif // __BINDWINDOW_H

