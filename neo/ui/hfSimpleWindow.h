#ifndef __SIMPLEWIN_H__
#define __SIMPLEWIN_H__

class hfUserInterfaceLocal;
class hfDeviceContext;
class hfSimpleWindow;

typedef struct {
	hfWindow *win;
	hfSimpleWindow *simp;
} drawWin_t;

class hfSimpleWindow {
	friend class hfWindow;
public:
					hfSimpleWindow(hfWindow* win);
	virtual			~hfSimpleWindow();
	void			Redraw(float x, float y);
	void			StateChanged( bool redraw );

	hfStr			name;

	hfWinVar *		GetWinVarByName(const char *_name);
	int				GetWinVarOffset( hfWinVar *wv, drawWin_t* owner);
	size_t			Size();

	hfWindow*		GetParent () { return mParent; }

	virtual void	WriteToSaveGame( hfFile *savefile );
	virtual void	ReadFromSaveGame( hfFile *savefile );

protected:
	void 			CalcClientRect(float xofs, float yofs);
	void 			SetupTransforms(float x, float y);
	void 			DrawBackground(const hfRectangle &drawRect);
	void 			DrawBorderAndCaption(const hfRectangle &drawRect);

	hfUserInterfaceLocal *gui;
	int 			flags;
	hfRectangle 	drawRect;			// overall rect
	hfRectangle 	clientRect;			// client area
	hfRectangle 	textRect;
	hfVec2			origin;
	class hfFont *	font;
	float 			matScalex;
	float 			matScaley;
	float 			borderSize;
	int 			textAlign;
	float 			textAlignx;
	float 			textAligny;
	int				textShadow;

	hfWinStr		text;
	hfWinBool		visible;
	hfWinRectangle	rect;				// overall rect
	hfWinVec4		backColor;
	hfWinVec4		matColor;
	hfWinVec4		foreColor;
	hfWinVec4		borderColor;
	hfWinFloat		textScale;
	hfWinFloat		rotate;
	hfWinVec2		shear;
	hfWinBackground	backGroundName;

	const hfMaterial* background;
	
	hfWindow *		mParent;

	hfWinBool	hideCursor;
};

#endif /* !__SIMPLEWIN_H__ */

