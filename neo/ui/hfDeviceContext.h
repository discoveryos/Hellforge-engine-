#ifndef __DEVICECONTEXT_H__
#define __DEVICECONTEXT_H__

// device context support for gui stuff
//

#include "hfRectangle.h"
#include "../renderer/hfFont.h"

const int VIRTUAL_WIDTH = 640;
const int VIRTUAL_HEIGHT = 480;
const int BLINK_DIVISOR = 200;

class hfDeviceContext {
public:
	hfDeviceContext();
	~hfDeviceContext() { }
	
	void				Init();
	void				Shutdown();
	bool				Initialized() { return initialized; }
	void				EnableLocalization();

	void				GetTransformInfo(hfVec3& origin, hfMat3& mat );

	void				SetTransformInfo(const hfVec3 &origin, const hfMat3 &mat);
	void				DrawMaterial(float x, float y, float w, float h, const hfMaterial *mat, const hfVec4 &color, float scalex = 1.0, float scaley = 1.0);
	void				DrawRect(float x, float y, float width, float height, float size, const hfVec4 &color);
	void				DrawFilledRect(float x, float y, float width, float height, const hfVec4 &color);
	int					DrawText(const char *text, float textScale, int textAlign, hfVec4 color, hfRectangle rectDraw, bool wrap, int cursor = -1, bool calcOnly = false, hfList<int> *breaks = NULL, int limit = 0 );
	void				DrawMaterialRect( float x, float y, float w, float h, float size, const hfMaterial *mat, const hfVec4 &color);
	void				DrawStretchPic(float x, float y, float w, float h, float s0, float t0, float s1, float t1, const hfMaterial *mat);
	void				DrawMaterialRotated(float x, float y, float w, float h, const hfMaterial *mat, const hfVec4 &color, float scalex = 1.0, float scaley = 1.0, float angle = 0.0f);
	void				DrawStretchPicRotated(float x, float y, float w, float h, float s0, float t0, float s1, float t1, const hfMaterial *mat, float angle = 0.0f);
	void				DrawWinding( hfWinding & w, const hfMaterial * mat );

	int					CharWidth( const char c, float scale );
	int					TextWidth(const char *text, float scale, int limit);
	int					TextHeight(const char *text, float scale, int limit);
	int					MaxCharHeight(float scale);
	int					MaxCharWidth(float scale);

	hfRegion *			GetTextRegion(const char *text, float textScale, hfRectangle rectDraw, float xStart, float yStart);

	void				SetSize( float width, float height );
	void				SetOffset( float x, float y );

	const hfMaterial *	GetScrollBarImage(int index);

	void				DrawCursor(float *x, float *y, float size);
	void				SetCursor(int n);

	// clipping rects
	virtual bool		ClippedCoords(float *x, float *y, float *w, float *h, float *s1, float *t1, float *s2, float *t2);
	virtual void		PushClipRect(hfRectangle r);
	virtual void		PopClipRect();
	virtual void		EnableClipping(bool b);

	void				SetFont( hfFont * font ) { activeFont = font; }

	void				SetOverStrike(bool b) { overStrikeMode = b; }

	bool				GetOverStrike() { return overStrikeMode; }

	void				DrawEditCursor(float x, float y, float scale);

	enum {
		CURSOR_ARROW,
		CURSOR_HAND,
		CURSOR_HAND_JOY1,
		CURSOR_HAND_JOY2,
		CURSOR_HAND_JOY3,
		CURSOR_HAND_JOY4,
		CURSOR_COUNT
	};

	enum {
		ALIGN_LEFT,
		ALIGN_CENTER,
		ALIGN_RIGHT
	};

	enum {
		SCROLLBAR_HBACK,
		SCROLLBAR_VBACK,
		SCROLLBAR_THUMB,
		SCROLLBAR_RIGHT,
		SCROLLBAR_LEFT,
		SCROLLBAR_UP,
		SCROLLBAR_DOWN,
		SCROLLBAR_COUNT
	};

	static hfVec4 colorPurple;
	static hfVec4 colorOrange;
	static hfVec4 colorYellow;
	static hfVec4 colorGreen;
	static hfVec4 colorBlue;
	static hfVec4 colorRed;
	static hfVec4 colorWhite;
	static hfVec4 colorBlack;
	static hfVec4 colorNone;

protected:
	virtual int			DrawText( float x, float y, float scale, hfVec4 color, const char *text, float adjust, int limit, int style, int cursor = -1);
	void				PaintChar( float x, float y, const scaledGlyphInfo_t & glyphInfo );
	void				Clear();

	const hfMaterial *	cursorImages[CURSOR_COUNT];
	const hfMaterial *	scrollBarImages[SCROLLBAR_COUNT];
	const hfMaterial *	whiteImage;
	hfFont *			activeFont;

	float				xScale;
	float				yScale;
	float				xOffset;
	float				yOffset;

	int					cursor;

	hfList<hfRectangle>	clipRects;

	bool				enableClipping;

	bool				overStrikeMode;

	hfMat3				mat;
	bool				matIsIdentity;
	hfVec3				origin;
	bool				initialized;
};

class hfDeviceContextOptimized : public hfDeviceContext {

	virtual bool		ClippedCoords(float *x, float *y, float *w, float *h, float *s1, float *t1, float *s2, float *t2);
	virtual void		PushClipRect(hfRectangle r);
	virtual void		PopClipRect();
	virtual void		EnableClipping(bool b);

	virtual int			DrawText( float x, float y, float scale, hfVec4 color, const char *text, float adjust, int limit, int style, int cursor = -1);

	float				clipX1;
	float				clipX2;
	float				clipY1;
	float				clipY2;
};

#endif /* !__DEVICECONTEXT_H__ */

