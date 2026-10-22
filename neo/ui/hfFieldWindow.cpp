#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfDeviceContext.h"
#include "hfWindow.h"
#include "hfUserInterfaceLocal.h"
#include "hfFieldWindow.h"


void hfFieldWindow::CommonInit() {
	cursorPos = 0;
	lastTextLength = 0;
	lastCursorPos = 0;
	paintOffset = 0;
	showCursor = false;
}

hfFieldWindow::hfFieldWindow(hfUserInterfaceLocal *g) : hfWindow(g) {
	gui = g;
	CommonInit();
}

hfFieldWindow::~hfFieldWindow() {

}

bool hfFieldWindow::ParseInternalVar(const char *_name, hfTokenParser *src) {
	if (hfStr::Icmp(_name, "cursorvar") == 0) {
		ParseString(src, cursorVar);
		return true;
	}
	if (hfStr::Icmp(_name, "showcursor") == 0) {
		showCursor = src->ParseBool();
		return true;
	}
	return hfWindow::ParseInternalVar(_name, src);
}


void hfFieldWindow::CalcPaintOffset(int len) {
	lastCursorPos = cursorPos;
	lastTextLength = len;
	paintOffset = 0;
	int tw = dc->TextWidth(text, textScale, -1);
	if (tw < textRect.w) {
		return;
	}
	while (tw > textRect.w && len > 0) {
		tw = dc->TextWidth(text, textScale, --len);
		paintOffset++;
	}
}


void hfFieldWindow::Draw(int time, float x, float y) {
	float scale = textScale;
	int len = text.Length();
	cursorPos = gui->State().GetInt( cursorVar );
	if (len != lastTextLength || cursorPos != lastCursorPos) {
		CalcPaintOffset(len);
	}
	hfRectangle rect = textRect;
	if (paintOffset >= len) {
		paintOffset = 0;
	}
	if (cursorPos > len) {
		cursorPos = len;
	}
	dc->DrawText(&text[paintOffset], scale, 0, foreColor, rect, false, ((flags & WIN_FOCUS) || showCursor) ? cursorPos - paintOffset : -1);
}


