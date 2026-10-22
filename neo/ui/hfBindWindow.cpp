#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfDeviceContext.h"
#include "hfWindow.h"
#include "hfUserInterfaceLocal.h"
#include "hfBindWindow.h"


void hfBindWindow::CommonInit() {
	bindName = "";
	waitingOnKey = false;
}

hfBindWindow::hfBindWindow(hfUserInterfaceLocal *g) : hfWindow(g) {
	gui = g;
	CommonInit();
}

hfBindWindow::~hfBindWindow() {

}


const char *hfBindWindow::HandleEvent(const sysEvent_t *event, bool *updateVisuals) {
	static char ret[ 256 ];
	
	if (!(event->evType == SE_KEY && event->evValue2)) {
		return "";
	}

	int key = event->evValue;

	if (waitingOnKey) {
		waitingOnKey = false;
		if (key == K_ESCAPE) {
			hfStr::snPrintf( ret, sizeof( ret ), "clearbind \"%s\"", bindName.GetName());
		} else {
			hfStr::snPrintf( ret, sizeof( ret ), "bind %i \"%s\"", key, bindName.GetName());
		}
		return ret;
	} else {
		if (key == K_MOUSE1) {
			waitingOnKey = true;
			gui->SetBindHandler(this);
			return "";
		}
	}

	return "";
}

hfWinVar *hfBindWindow::GetWinVarByName(const char *_name, bool fixup, drawWin_t** owner) {

	if (hfStr::Icmp(_name, "bind") == 0) {
		return &bindName;
	}

	return hfWindow::GetWinVarByName(_name, fixup,owner);
}

void hfBindWindow::PostParse() {
	hfWindow::PostParse();
	bindName.SetGuiInfo( gui->GetStateDict(), bindName );
	bindName.Update();
	//bindName = state.GetString("bind");
	flags |= (WIN_HOLDCAPTURE | WIN_CANFOCUS);
}

void hfBindWindow::Draw(int time, float x, float y) {
	hfVec4 color = foreColor;

	hfStr str;
	if ( waitingOnKey ) {
		str = hfLocalization::GetString( "#str_07000" );
	} else if ( bindName.Length() ) {
		str = bindName.c_str();
	} else {
		str = hfLocalization::GetString( "#str_07001" );
	}

	if ( waitingOnKey || ( hover && !noEvents && Contains(gui->CursorX(), gui->CursorY()) ) ) {
		color = hoverColor;
	} else {
		hover = false;
	}

	dc->DrawText(str, textScale, textAlign, color, textRect, false, -1);
}

void hfBindWindow::Activate( bool activate, hfStr &act ) {
	hfWindow::Activate( activate, act );
	bindName.Update();
}

