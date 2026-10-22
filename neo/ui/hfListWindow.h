#ifndef __LISTWINDOW_H
#define __LISTWINDOW_H

class hfSliderWindow;

enum {
	TAB_TYPE_TEXT = 0,
	TAB_TYPE_ICON = 1
};

struct hfTabRect {
	int x;
	int w;
	int align;
	int valign;
	int	type;
	hfVec2 iconSize;
	float iconVOffset;
};

class hfListWindow : public hfWindow {
public:
	hfListWindow(hfUserInterfaceLocal *gui);

	virtual const char*	HandleEvent(const sysEvent_t *event, bool *updateVisuals);
	virtual void		PostParse();
	virtual void		Draw(int time, float x, float y);
	virtual void		Activate(bool activate, hfStr &act);
	virtual void		HandleBuddyUpdate(hfWindow *buddy);
	virtual void		StateChanged( bool redraw = false );
	virtual size_t		Allocated(){return hfWindow::Allocated();};
	virtual hfWinVar*	GetWinVarByName(const char *_name, bool winLookup = false, drawWin_t** owner = NULL);

	void				UpdateList();
	
private:
	virtual bool		ParseInternalVar(const char *name, hfTokenParser *src);
	void				CommonInit();
	void				InitScroller( bool horizontal );
	void				SetCurrentSel( int sel );
	void				AddCurrentSel( int sel );
	int					GetCurrentSel();
	bool				IsSelected( int index );
	void				ClearSelection( int sel );

	hfList<hfTabRect, TAG_OLD_UI>	tabInfo;
	int					top;
	float				sizeBias;
	bool				horizontal;
	hfStr				tabStopStr;
	hfStr				tabAlignStr;
	hfStr				tabVAlignStr;
	hfStr				tabTypeStr;
	hfStr				tabIconSizeStr;
	hfStr				tabIconVOffsetStr;
	hfHashTable<const hfMaterial*> iconMaterials;						
	bool				multipleSel;

	hfStrList			listItems;
	hfSliderWindow*		scroller;
	hfList<int, TAG_OLD_UI>			currentSel;
	hfStr				listName;

	int					clickTime;

	int					typedTime;
	hfStr				typed;
};

#endif // __LISTWINDOW_H

