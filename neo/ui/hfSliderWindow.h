#ifndef __SLIDERWINDOW_H__
#define __SLIDERWINDOW_H__

class hfUserInterfaceLocal;

class hfSliderWindow : public hfWindow {
public:
						hfSliderWindow(hfUserInterfaceLocal *gui);
	virtual				~hfSliderWindow();

	void				InitWithDefaults(const char *_name, const hfRectangle &rect, const hfVec4 &foreColor, const hfVec4 &matColor, const char *_background, const char *thumbShader, bool _vertical, bool _scrollbar);

	void				SetRange(float _low, float _high, float _step);
	float				GetLow() { return low; }
	float				GetHigh() { return high; }

	void				SetValue(float _value);
	float				GetValue() { return value; };

	virtual size_t		Allocated(){return hfWindow::Allocated();};
	virtual hfWinVar *	GetWinVarByName(const char *_name, bool winLookup = false, drawWin_t** owner = NULL);
	virtual const char *HandleEvent(const sysEvent_t *event, bool *updateVisuals);
	virtual void		PostParse();
	virtual void		Draw(int time, float x, float y);
	virtual void		DrawBackground(const hfRectangle &drawRect);
	virtual const char *RouteMouseCoords(float xd, float yd);
	virtual void		Activate(bool activate, hfStr &act);
	virtual void		SetBuddy(hfWindow *buddy);

	void				RunNamedEvent( const char* eventName );
	
private:
	virtual bool		ParseInternalVar(const char *name, hfTokenParser *src);
	void				CommonInit();
	void				InitCvar();
						// true: read the updated cvar from cvar system
						// false: write to the cvar system
						// force == true overrides liveUpdate 0
	void				UpdateCvar( bool read, bool force = false );
	
	hfWinFloat			value;
	float				low;
	float				high;
	float				thumbWidth;
	float				thumbHeight;
	float				stepSize;
	float				lastValue;
	hfRectangle			thumbRect;
	const hfMaterial *	thumbMat;
	bool				vertical;
	bool				verticalFlip;
	bool				scrollbar;
	hfWindow *			buddyWin;
	hfStr				thumbShader;
	
	hfWinStr			cvarStr;
	hfCVar *			cvar;
	bool				cvar_init;
	hfWinBool			liveUpdate;
	hfWinStr			cvarGroup;	
};

#endif /* !__SLIDERWINDOW_H__ */


