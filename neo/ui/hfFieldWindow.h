#ifndef __FIELDWINDOW_H
#define __FIELDWINDOW_H

#include "hfWindow.h"


class hfFieldWindow : public hfWindow {
public:
	hfFieldWindow(hfUserInterfaceLocal *gui);
	virtual ~hfFieldWindow();

	virtual void Draw(int time, float x, float y);
	
private:
	virtual bool ParseInternalVar(const char *name, hfTokenParser *src);
	void CommonInit();
	void CalcPaintOffset(int len);
	int cursorPos;
	int lastTextLength;
	int lastCursorPos;
	int paintOffset;
	bool showCursor;
	hfStr cursorVar;
};

#endif // __FIELDWINDOW_H

