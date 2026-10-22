#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfDeviceContext.h"
#include "hfWindow.h"
#include "hfUserInterfaceLocal.h"
#include "hfSliderWindow.h"

/*
============
hfSliderWindow::CommonInit
============
*/
void hfSliderWindow::CommonInit() {
	value = 0.0;
	low = 0.0;
	high = 100.0;
	stepSize = 1.0;
	thumbMat = declManager->FindMaterial("_default");
	buddyWin = NULL;

	cvar = NULL;
	cvar_init = false;
	liveUpdate = true;

	vertical = false;
	scrollbar = false;

	verticalFlip = false;
}

hfSliderWindow::hfSliderWindow(hfUserInterfaceLocal *g) : hfWindow(g) {
	gui = g;
	CommonInit();
}

hfSliderWindow::~hfSliderWindow() {

}

bool hfSliderWindow::ParseInternalVar(const char *_name, hfTokenParser *src) {
	if (hfStr::Icmp(_name, "stepsize") == 0 || hfStr::Icmp(_name, "step") == 0) {
		stepSize = src->ParseFloat();
		return true;
	}
	if (hfStr::Icmp(_name, "low") == 0) {
		low = src->ParseFloat();
		return true;
	}
	if (hfStr::Icmp(_name, "high") == 0) {
		high = src->ParseFloat();
		return true;
	}
	if (hfStr::Icmp(_name, "vertical") == 0) {
		vertical = src->ParseBool();
		return true;
	}
	if (hfStr::Icmp(_name, "verticalflip") == 0) {
		verticalFlip = src->ParseBool();
		return true;
	}
	if (hfStr::Icmp(_name, "scrollbar") == 0) {
		scrollbar = src->ParseBool();
		return true;
	}
	if (hfStr::Icmp(_name, "thumbshader") == 0) {
		ParseString(src, thumbShader);
		declManager->FindMaterial(thumbShader);
		return true;
	}
	return hfWindow::ParseInternalVar(_name, src);
}

hfWinVar *hfSliderWindow::GetWinVarByName(const char *_name, bool fixup, drawWin_t** owner) {
 
	if (hfStr::Icmp(_name, "value") == 0) {
		return &value;
	}
	if (hfStr::Icmp(_name, "cvar") == 0) {
		return &cvarStr;
	}
	if ( hfStr::Icmp( _name, "liveUpdate" ) == 0 ) {
		return &liveUpdate;
	}
	if ( hfStr::Icmp( _name, "cvarGroup" ) == 0 ) {
		return &cvarGroup;
	}
	
	return hfWindow::GetWinVarByName(_name, fixup, owner);
}

const char *hfSliderWindow::HandleEvent(const sysEvent_t *event, bool *updateVisuals) {

	if (!(event->evType == SE_KEY && event->evValue2)) {
		return "";
	}

	int key = event->evValue;

	if ( event->evValue2 && key == K_MOUSE1 ) {
		SetCapture(this);
		RouteMouseCoords(0.0f, 0.0f);
		return "";
	} 

	if ( key == K_RIGHTARROW || key == K_KP_6 || ( key == K_MOUSE2 && gui->CursorY() > thumbRect.y ) )  {
		value = value + stepSize;
	}

	if ( key == K_LEFTARROW || key == K_KP_4 || ( key == K_MOUSE2 && gui->CursorY() < thumbRect.y ) ) {
		value = value - stepSize;
	}

	if (buddyWin) {
		buddyWin->HandleBuddyUpdate(this);
	} else {
		gui->SetStateFloat( cvarStr, value );
		UpdateCvar( false );
	}

	return "";
}


void hfSliderWindow::SetBuddy(hfWindow *buddy) {
	buddyWin = buddy;
}

void hfSliderWindow::PostParse() {
	hfWindow::PostParse();
	value = 0.0;
	thumbMat = declManager->FindMaterial(thumbShader);
	thumbMat->SetSort( SS_GUI );
	thumbWidth = thumbMat->GetImageWidth();
	thumbHeight = thumbMat->GetImageHeight();
	//vertical = state.GetBool("vertical");
	//scrollbar = state.GetBool("scrollbar");
	flags |= (WIN_HOLDCAPTURE | WIN_CANFOCUS);
	InitCvar();
}

void hfSliderWindow::InitWithDefaults(const char *_name, const hfRectangle &_rect, const hfVec4 &_foreColor, const hfVec4 &_matColor, const char *_background, const char *thumbShader, bool _vertical, bool _scrollbar) {
	SetInitialState(_name);
	rect = _rect;
	foreColor = _foreColor;
	matColor = _matColor;
	thumbMat = declManager->FindMaterial(thumbShader);
	thumbMat->SetSort( SS_GUI );
	thumbWidth = thumbMat->GetImageWidth();
	thumbHeight = thumbMat->GetImageHeight();
	background = declManager->FindMaterial(_background);
	background->SetSort( SS_GUI );
	vertical = _vertical;
	scrollbar = _scrollbar;
	flags |= WIN_HOLDCAPTURE;
}

void hfSliderWindow::SetRange(float _low, float _high, float _step) {
	low = _low;
	high = _high;
	stepSize = _step;
}

void hfSliderWindow::SetValue(float _value) {
	value = _value;
}

void hfSliderWindow::Draw(int time, float x, float y) {
	hfVec4 color = foreColor;

	if ( !cvar && !buddyWin ) {
		return;
	}

	if ( !thumbWidth || !thumbHeight ) {
		thumbWidth = thumbMat->GetImageWidth();
		thumbHeight = thumbMat->GetImageHeight();
	}

	UpdateCvar( true );
	if ( value > high ) {
		value = high;
	} else if ( value < low ) {
		value = low;
	}

	float range = high - low;

	if ( range <= 0.0f ) {
		return;
	}

	float thumbPos = (range) ? (value - low) / range : 0.0;
	if (vertical) {
		if ( verticalFlip ) {
			thumbPos = 1.f - thumbPos;
		}
		thumbPos *= drawRect.h - thumbHeight;
		thumbPos += drawRect.y;
		thumbRect.y = thumbPos;
		thumbRect.x = drawRect.x;
	} else {
		thumbPos *= drawRect.w - thumbWidth;
		thumbPos += drawRect.x;
		thumbRect.x = thumbPos;
		thumbRect.y = drawRect.y;
	}
	thumbRect.w = thumbWidth;
	thumbRect.h = thumbHeight;

	if ( hover && !noEvents && Contains(gui->CursorX(), gui->CursorY()) ) {
		color = hoverColor;
	} else {
		hover = false;
	}
	if ( flags & WIN_CAPTURE ) {
		color = hoverColor;
		hover = true;
	}

	dc->DrawMaterial(thumbRect.x, thumbRect.y, thumbRect.w, thumbRect.h, thumbMat, color);
	if ( flags & WIN_FOCUS ) {
		dc->DrawRect(thumbRect.x+1.0f, thumbRect.y+1.0f, thumbRect.w-2.0f, thumbRect.h-2.0f, 1.0f, color);
	}
}


void hfSliderWindow::DrawBackground(const hfRectangle &_drawRect) {
	if ( !cvar && !buddyWin ) {
		return;
	}

	if ( high - low <= 0.0f ) {
		return;
	}

	hfRectangle r = _drawRect;
	if (!scrollbar) {
		if ( vertical ) {
			r.y += thumbHeight / 2.f;
			r.h -= thumbHeight;
		} else {
			r.x += thumbWidth / 2.0;
			r.w -= thumbWidth;
		}
	}
	hfWindow::DrawBackground(r);
}

const char *hfSliderWindow::RouteMouseCoords(float xd, float yd) {
	float pct;

	if (!(flags & WIN_CAPTURE)) {
		return "";
	}

	hfRectangle r = drawRect;
	r.x = actualX;
	r.y = actualY;
	r.x += thumbWidth / 2.0;
	r.w -= thumbWidth;
	if (vertical) {
		r.y += thumbHeight / 2;
		r.h -= thumbHeight;
		if (gui->CursorY() >= r.y && gui->CursorY() <= r.Bottom()) {
			pct = (gui->CursorY() - r.y) / r.h;
			if ( verticalFlip ) {
				pct = 1.f - pct;
			}
			value = low + (high - low) * pct;
		} else if (gui->CursorY() < r.y) {
			if ( verticalFlip ) {
				value = high;
			} else {
				value = low;
			}
		} else {
			if ( verticalFlip ) {
				value = low;
			} else {
				value = high;
			}
		}
	} else {
		r.x += thumbWidth / 2;
		r.w -= thumbWidth;
		if (gui->CursorX() >= r.x && gui->CursorX() <= r.Right()) {
			pct = (gui->CursorX() - r.x) / r.w;
			value = low + (high - low) * pct;
		} else if (gui->CursorX() < r.x) {
			value = low;
		} else {
			value = high;
		}
	}

	if (buddyWin) {
		buddyWin->HandleBuddyUpdate(this);
	} else {
		gui->SetStateFloat( cvarStr, value );
	}
	UpdateCvar( false );

	return "";
}


void hfSliderWindow::Activate(bool activate, hfStr &act) {
	hfWindow::Activate(activate, act);
	if ( activate ) {
		UpdateCvar( true, true );
	}
}

/*
============
hfSliderWindow::InitCvar
============
*/
void hfSliderWindow::InitCvar( ) {
	if ( cvarStr[0] == '\0' ) {
		if ( !buddyWin ) {
			common->Warning( "hfSliderWindow::InitCvar: gui '%s' window '%s' has an empty cvar string", gui->GetSourceFile(), name.c_str() );
		}
		cvar_init = true;
		cvar = NULL;
		return;
	}

	cvar = cvarSystem->Find( cvarStr );
	if ( !cvar ) {
		common->Warning( "hfSliderWindow::InitCvar: gui '%s' window '%s' references undefined cvar '%s'", gui->GetSourceFile(), name.c_str(), cvarStr.c_str() );
		cvar_init = true;
		return;
	}
}

/*
============
hfSliderWindow::UpdateCvar
============
*/
void hfSliderWindow::UpdateCvar( bool read, bool force ) {
	if ( buddyWin || !cvar ) {
		return;
	}
	if ( force || liveUpdate ) {
		value = cvar->GetFloat();
		if ( value != gui->State().GetFloat( cvarStr ) ) {
			if ( read ) {
				gui->SetStateFloat( cvarStr, value );
			} else {
				value = gui->State().GetFloat( cvarStr );
				cvar->SetFloat( value );
			}
		}
	}
}

/*
============
hfSliderWindow::RunNamedEvent
============
*/
void hfSliderWindow::RunNamedEvent( const char* eventName ) {
	hfStr event, group;
	
	if ( !hfStr::Cmpn( eventName, "cvar read ", 10 ) ) {
		event = eventName;
		group = event.Mid( 10, event.Length() - 10 );
		if ( !group.Cmp( cvarGroup ) ) {
			UpdateCvar( true, true );
		}
	} else if ( !hfStr::Cmpn( eventName, "cvar write ", 11 ) ) {
		event = eventName;
		group = event.Mid( 11, event.Length() - 11 );
		if ( !group.Cmp( cvarGroup ) ) {
			UpdateCvar( false, true );
		}
	}
}


