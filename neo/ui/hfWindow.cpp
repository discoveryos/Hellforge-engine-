#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfDeviceContext.h"
#include "hfWindow.h"
#include "hfUserInterfaceLocal.h"
#include "hfEditWindow.h"
#include "hfChoiceWindow.h"
#include "hfSliderWindow.h"
#include "hfBindWindow.h"
#include "hfListWindow.h"
#include "hfRenderWindow.h"
#include "hfFieldWindow.h"

#include "hfGameSSDWindow.h"
#include "hfGameBearShootWindow.h"
#include "hfGameBustOutWindow.h"

bool hfWindow::registerIsTemporary[MAX_EXPRESSION_REGISTERS];		// statics to assist during parsing
//float hfWindow::shaderRegisters[MAX_EXPRESSION_REGISTERS];
//wexpOp_t hfWindow::shaderOps[MAX_EXPRESSION_OPS];

hfCVar hfWindow::gui_debug( "gui_debug", "0", CVAR_GUI | CVAR_BOOL, "" );
hfCVar hfWindow::gui_edit( "gui_edit", "0", CVAR_GUI | CVAR_BOOL, "" );

hfCVar hud_titlesafe( "hud_titlesafe", "0.0", CVAR_GUI | CVAR_FLOAT, "fraction of the screen to leave around hud for titlesafe area" );

extern hfCVar r_skipGuiShaders;		// 1 = don't render any gui elements on surfaces

//  made RegisterVars a member of hfWindow
const hfRegEntry hfWindow::RegisterVars[] = {
	{ "forecolor", hfRegister::VEC4 },
	{ "hovercolor", hfRegister::VEC4 },
	{ "backcolor", hfRegister::VEC4 },
	{ "bordercolor", hfRegister::VEC4 },
	{ "rect", hfRegister::RECTANGLE },
	{ "matcolor", hfRegister::VEC4 },
	{ "scale", hfRegister::VEC2 },
	{ "translate", hfRegister::VEC2 },
	{ "rotate", hfRegister::FLOAT },
	{ "textscale", hfRegister::FLOAT },
	{ "visible", hfRegister::BOOL },
	{ "noevents", hfRegister::BOOL },
	{ "text", hfRegister::STRING },
	{ "background", hfRegister::STRING },
	{ "runscript", hfRegister::STRING },
	{ "varbackground", hfRegister::STRING },
	{ "cvar", hfRegister::STRING },
	{ "choices", hfRegister::STRING },
	{ "choiceVar", hfRegister::STRING },
	{ "bind", hfRegister::STRING },
	{ "modelRotate", hfRegister::VEC4 },
	{ "modelOrigin", hfRegister::VEC4 },
	{ "lightOrigin", hfRegister::VEC4 },
	{ "lightColor", hfRegister::VEC4 },
	{ "viewOffset", hfRegister::VEC4 },
	{ "hideCursor", hfRegister::BOOL}
};

const int hfWindow::NumRegisterVars = sizeof(RegisterVars) / sizeof(hfRegEntry);

const char *hfWindow::ScriptNames[] = {
	"onMouseEnter",
	"onMouseExit",
	"onAction",
	"onActivate",
	"onDeactivate",
	"onESC",
	"onEvent",
	"onTrigger",
	"onActionRelease",
	"onEnter",
	"onEnterRelease"
};

/*
================
hfWindow::CommonInit
================
*/
void hfWindow::CommonInit() {
	childID = 0;
	flags = 0;
	lastTimeRun = 0;
	origin.Zero();
	font = renderSystem->RegisterFont( "" );
	timeLine = -1;
	xOffset = yOffset = 0.0;
	cursor = 0;
	forceAspectWidth = 640;
	forceAspectHeight = 480;
	matScalex = 1;
	matScaley = 1;
	borderSize = 0;
	noTime = false;
	visible = true;
	textAlign = 0;
	textAlignx = 0;
	textAligny = 0;
	noEvents = false;
	rotate = 0;
	shear.Zero();
	textScale = 0.35f;
	backColor.Zero();
	foreColor = hfVec4(1, 1, 1, 1);
	hoverColor = hfVec4(1, 1, 1, 1);
	matColor = hfVec4(1, 1, 1, 1);
	borderColor.Zero();
	background = NULL;
	backGroundName = "";
	focusedChild = NULL;
	captureChild = NULL;
	overChild = NULL;
	parent = NULL;
	saveOps = NULL;
	saveRegs = NULL;
	timeLine = -1;
	textShadow = 0;
	hover = false;

	for (int i = 0; i < SCRIPT_COUNT; i++) {
		scripts[i] = NULL;
	}

	hideCursor = false;
}

/*
================
hfWindow::Size
================
*/
size_t hfWindow::Size() {
	int c = children.Num();
	int sz = 0;
	for (int i = 0; i < c; i++) {
		sz += children[i]->Size();
	}
	sz += sizeof(*this) + Allocated();
	return sz;
}

/*
================
hfWindow::Allocated
================
*/
size_t hfWindow::Allocated() {
	int i, c;
	int sz = name.Allocated();
	sz += text.Size();
	sz += backGroundName.Size();

	c = definedVars.Num();
	for (i = 0; i < c; i++) {
		sz += definedVars[i]->Size();
	}

	for (i = 0; i < SCRIPT_COUNT; i++) {
		if (scripts[i]) {
			sz += scripts[i]->Size();
		}
	}
	c = timeLineEvents.Num();
	for (i = 0; i < c; i++) {
		sz += timeLineEvents[i]->Size();
	}

	c = namedEvents.Num();
	for (i = 0; i < c; i++) {
		sz += namedEvents[i]->Size();
	}

	c = drawWindows.Num();
	for (i = 0; i < c; i++) {
		if (drawWindows[i].simp) {
			sz += drawWindows[i].simp->Size();
		}
	}

	return sz;
}

/*
================
hfWindow::hfWindow
================
*/
hfWindow::hfWindow(hfUserInterfaceLocal *ui) {
	gui = ui;
	CommonInit();
}
				  
/*
================
hfWindow::CleanUp
================
*/
void hfWindow::CleanUp() {
	int i, c = drawWindows.Num();
	for (i = 0; i < c; i++) {
		delete drawWindows[i].simp;
	}

	// ensure the register list gets cleaned up
	regList.Reset ( );
	
	// Cleanup the named events
	namedEvents.DeleteContents(true);

	drawWindows.Clear();
	children.DeleteContents(true);
	definedVars.DeleteContents(true);
	timeLineEvents.DeleteContents(true);
	for (i = 0; i < SCRIPT_COUNT; i++) {
		delete scripts[i];
	}
	CommonInit();
}

/*
================
hfWindow::~hfWindow
================
*/
hfWindow::~hfWindow() {
	CleanUp();
}

/*
================
hfWindow::Move
================
*/
void hfWindow::Move(float x, float y) {
	hfRectangle rct = rect;
	rct.x = x;
	rct.y = y;
	hfRegister *reg = RegList()->FindReg("rect");
	if (reg) {
		reg->Enable(false);
	}
	rect = rct;
}

/*
================
hfWindow::SetFont
================
*/
void hfWindow::SetFont() {
	dc->SetFont( font );
}

/*
================
hfWindow::GetMaxCharHeight
================
*/
float hfWindow::GetMaxCharHeight() {
	SetFont();
	return dc->MaxCharHeight(textScale);
}

/*
================
hfWindow::GetMaxCharWidth
================
*/
float hfWindow::GetMaxCharWidth() {
	SetFont();
	return dc->MaxCharWidth(textScale);
}

/*
================
hfWindow::Draw
================
*/
void hfWindow::Draw( int time, float x, float y ) {
	if ( text.Length() == 0 ) {
		return;
	}
	if ( textShadow ) {
		hfStr shadowText = text;
		hfRectangle shadowRect = textRect;

		shadowText.RemoveColors();
		shadowRect.x += textShadow;
		shadowRect.y += textShadow;

		dc->DrawText( shadowText, textScale, textAlign, colorBlack, shadowRect, !( flags & WIN_NOWRAP ), -1 );
	}
	dc->DrawText( text, textScale, textAlign, foreColor, textRect, !( flags & WIN_NOWRAP ), -1 );

	if ( gui_edit.GetBool() ) {
		dc->EnableClipping( false );
		dc->DrawText( va( "x: %i  y: %i", ( int )rect.x(), ( int )rect.y() ), 0.25, 0, dc->colorWhite, hfRectangle( rect.x(), rect.y() - 15, 100, 20 ), false );
		dc->DrawText( va( "w: %i  h: %i", ( int )rect.w(), ( int )rect.h() ), 0.25, 0, dc->colorWhite, hfRectangle( rect.x() + rect.w(), rect.w() + rect.h() + 5, 100, 20 ), false );
		dc->EnableClipping( true );
	}

}

/*
================
hfWindow::BringToTop
================
*/
void hfWindow::BringToTop(hfWindow *w) {
	
	if (w && !(w->flags & WIN_MODAL)) {
		return;
	}

	int c = children.Num();
	for (int i = 0; i < c; i++) {
		if (children[i] == w) {
			// this is it move from i - 1 to 0 to i to 1 then shove this one into 0
			for (int j = i+1; j < c; j++) {
				children[j-1] = children[j];
			}
			children[c-1] = w;
			break;
		}
	}
}

/*
================
hfWindow::Size
================
*/
void hfWindow::Size(float x, float y, float w, float h) {
	hfRectangle rct = rect;
	rct.x = x;
	rct.y = y;
	rct.w = w;
	rct.h = h;
	rect = rct;
	CalcClientRect(0,0);
}

/*
================
hfWindow::MouseEnter
================
*/
void hfWindow::MouseEnter() {
	
	if (noEvents) {
		return;
	}

	RunScript(ON_MOUSEENTER);
}

/*
================
hfWindow::MouseExit
================
*/
void hfWindow::MouseExit() {
	
	if (noEvents) {
		return;
	}

	RunScript(ON_MOUSEEXIT);
}

/*
================
hfWindow::GetChildWithOnAction
================
*/
hfWindow * hfWindow::GetChildWithOnAction( float xd, float yd ) {

	int c = children.Num();
	while ( c > 0 ) {
		hfWindow *child = children[--c];
		if ( child->visible && child->Contains(child->drawRect, gui->CursorX(), gui->CursorY() ) && !child->noEvents ) {
			child->hover = true;
			if ( child->cursor > 0 ) {
				return child;
			}
		}

		hfWindow * check = child->GetChildWithOnAction( xd, yd );
		if ( check != NULL && check != child ) {
			return check;
		}
	}

	return this;

}

/*
================
hfWindow::RouteMouseCoords
================
*/
const char *hfWindow::RouteMouseCoords(float xd, float yd) {
	hfStr str;
	if (GetCaptureChild()) {
		//FIXME: unkludge this whole mechanism
		return GetCaptureChild()->RouteMouseCoords(xd, yd);
	}
	
	if (xd == -2000 || yd == -2000) {
		return "";
	}

	hfWindow * child = GetChildWithOnAction( xd, yd );
	if ( overChild != child ) {
		if ( overChild ) {
			overChild->MouseExit();
			str = overChild->cmd;
			if (str.Length()) {
				gui->GetDesktop()->AddCommand(str);
				overChild->cmd = "";
			}
		}
		overChild = child;
		if ( overChild ) {
			overChild->MouseEnter();
			str = overChild->cmd;
			if (str.Length()) {
				gui->GetDesktop()->AddCommand(str);
				overChild->cmd = "";
			}

			dc->SetCursor( overChild->cursor );
		}
	}

	return "";
}

/*
================
hfWindow::Activate
================
*/
void hfWindow::Activate( bool activate,	hfStr &act ) {

	int n = (activate) ? ON_ACTIVATE : ON_DEACTIVATE;

	//  make sure win vars are updated before activation
	UpdateWinVars ( );

	RunScript(n);
	int c = children.Num();
	for (int i = 0; i < c; i++) {
		children[i]->Activate( activate, act );
	}

	if ( act.Length() ) {
		act += " ; ";
	}
}

/*
================
hfWindow::Trigger
================
*/
void hfWindow::Trigger() {
	RunScript( ON_TRIGGER );
	int c = children.Num();
	for ( int i = 0; i < c; i++ ) {
		children[i]->Trigger();
	}
	StateChanged( true );
}

/*
================
hfWindow::StateChanged
================
*/
void hfWindow::StateChanged( bool redraw ) {

	UpdateWinVars();

	if (expressionRegisters.Num() && ops.Num()) {
		EvalRegs();
	}

	int c = drawWindows.Num();
	for ( int i = 0; i < c; i++ ) {
		if ( drawWindows[i].win ) {
			drawWindows[i].win->StateChanged( redraw );
		} else {
			drawWindows[i].simp->StateChanged( redraw );
		}
	}
}

/*
================
hfWindow::SetCapture
================
*/
hfWindow *hfWindow::SetCapture(hfWindow *w) {
	// only one child can have the focus

	hfWindow *last = NULL;
	int c = children.Num();
	for (int i = 0; i < c; i++) {
		if ( children[i]->flags & WIN_CAPTURE ) {
			last = children[i];
			//last->flags &= ~WIN_CAPTURE;
			last->LoseCapture();
			break;
		}
	}

	w->flags |= WIN_CAPTURE;
	w->GainCapture();
	gui->GetDesktop()->captureChild = w;
	return last;
}

/*
================
hfWindow::AddUpdateVar
================
*/
void hfWindow::AddUpdateVar(hfWinVar *var) {
	updateVars.AddUnique(var);
}

/*
================
hfWindow::UpdateWinVars
================
*/
void hfWindow::UpdateWinVars() {
	int c = updateVars.Num();
	for (int i = 0; i < c; i++) {
		updateVars[i]->Update();
	}
}

/*
================
hfWindow::RunTimeEvents
================
*/
bool hfWindow::RunTimeEvents(int time) {

	if ( time == lastTimeRun ) {
		return false;
	}

	lastTimeRun = time;

	UpdateWinVars();

	if (expressionRegisters.Num() && ops.Num()) {
		EvalRegs();
	}

	if ( flags & WIN_INTRANSITION ) {
		Transition();
	}

	Time();

	// renamed ON_EVENT to ON_FRAME
	RunScript(ON_FRAME);

	int c = children.Num();
	for (int i = 0; i < c; i++) {
		children[i]->RunTimeEvents(time);
	}

	return true;
}

/*
================
hfWindow::RunNamedEvent
================
*/
void hfWindow::RunNamedEvent ( const char* eventName )
{
	int i;
	int c;

	// Find and run the event	
	c = namedEvents.Num( );
	for ( i = 0; i < c; i ++ ) {
		if ( namedEvents[i]->mName.Icmp( eventName ) ) {	
			continue;
		}

		UpdateWinVars();

		// Make sure we got all the current values for stuff
		if (expressionRegisters.Num() && ops.Num()) {
			EvalRegs(-1, true);
		}
		
		RunScriptList( namedEvents[i]->mEvent );
		
		break;
	}
	
	// Run the event in all the children as well
	c = children.Num();
	for ( i = 0; i < c; i++ ) {
		children[i]->RunNamedEvent ( eventName );
	}
}

/*
================
hfWindow::Contains
================
*/
bool hfWindow::Contains(const hfRectangle &sr, float x, float y) {
	hfRectangle r = sr;
	r.x += actualX - drawRect.x;
	r.y += actualY - drawRect.y;
	return r.Contains(x, y);
}

/*
================
hfWindow::Contains
================
*/
bool hfWindow::Contains(float x, float y) {
	hfRectangle r = drawRect;
	r.x = actualX;
	r.y = actualY;
	return r.Contains(x, y);
}

/*
================
hfWindow::AddCommand
================
*/
void hfWindow::AddCommand(const char *_cmd) {
	hfStr str = cmd;
	if (str.Length()) {
		str += " ; ";
		str += _cmd;
	} else {
		str = _cmd;
	}
	cmd = str;
}

/*
================
hfWindow::HandleEvent
================
*/
const char *hfWindow::HandleEvent(const sysEvent_t *event, bool *updateVisuals) {
	static bool actionDownRun;
	static bool actionUpRun;

	cmd = "";

	if ( flags & WIN_DESKTOP ) {
		actionDownRun = false;
		actionUpRun = false;
		if (expressionRegisters.Num() && ops.Num()) {
			EvalRegs();
		}
		RunTimeEvents(gui->GetTime());
		CalcRects(0,0);

		if ( overChild != NULL ) {
			dc->SetCursor( overChild->cursor );
		} else {
			dc->SetCursor( hfDeviceContext::CURSOR_ARROW );
		}
	}

	if (visible && !noEvents) {

		if (event->evType == SE_KEY) {
			EvalRegs(-1, true);
			if (updateVisuals) {
				*updateVisuals = true;
			}

			if (event->evValue == K_MOUSE1) {

				if (!event->evValue2 && GetCaptureChild()) {
					GetCaptureChild()->LoseCapture();
					gui->GetDesktop()->captureChild = NULL;
					return "";
				} 

				int c = children.Num();
				while (--c >= 0) {
					if (children[c]->visible && children[c]->Contains(children[c]->drawRect, gui->CursorX(), gui->CursorY()) && !(children[c]->noEvents)) {
						hfWindow *child = children[c];
						if (event->evValue2) {
							BringToTop(child);
							SetFocus(child);
							if (child->flags & WIN_HOLDCAPTURE) {
								SetCapture(child);
							}
						}
						if (child->Contains(child->clientRect, gui->CursorX(), gui->CursorY())) {
							//if ((gui_edit.GetBool() && (child->flags & WIN_SELECTED)) || (!gui_edit.GetBool() && (child->flags & WIN_MOVABLE))) {
							//	SetCapture(child);
							//}
							SetFocus(child);
							const char *childRet = child->HandleEvent(event, updateVisuals);
							if (childRet != NULL && *childRet != NULL) {
								return childRet;
							} 
							if (child->flags & WIN_MODAL) {
								return "";
							}
						} else {
							if (event->evValue2) {
								SetFocus(child);
								bool capture = true;
								if (capture && ((child->flags & WIN_MOVABLE) || gui_edit.GetBool())) {
									SetCapture(child);
								}
								return "";
							} else {
							}
						}
					}
				}
				if (event->evValue2 && !actionDownRun) {
					actionDownRun = RunScript( ON_ACTION );
				} else if (!actionUpRun) {
					actionUpRun = RunScript( ON_ACTIONRELEASE );
				}
			} else if (event->evValue == K_MOUSE2) {

				if (!event->evValue2 && GetCaptureChild()) {
					GetCaptureChild()->LoseCapture();
					gui->GetDesktop()->captureChild = NULL;
					return "";
				}

				int c = children.Num();
				while (--c >= 0) {
					if (children[c]->visible && children[c]->Contains(children[c]->drawRect, gui->CursorX(), gui->CursorY()) && !(children[c]->noEvents)) {
						hfWindow *child = children[c];
						if (event->evValue2) {
							BringToTop(child);
							SetFocus(child);
						}
						if (child->Contains(child->clientRect,gui->CursorX(), gui->CursorY()) || GetCaptureChild() == child) {
							if ((gui_edit.GetBool() && (child->flags & WIN_SELECTED)) || (!gui_edit.GetBool() && (child->flags & WIN_MOVABLE))) {
								SetCapture(child);
							}
							const char *childRet = child->HandleEvent(event, updateVisuals);
							if (childRet && *childRet) {
								return childRet;
							} 
							if (child->flags & WIN_MODAL) {
								return "";
							}
						}
					}
				}
			} else if (event->evValue == K_MOUSE3) {
				if (gui_edit.GetBool()) {
					int c = children.Num();
					for (int i = 0; i < c; i++) {
						if (children[i]->drawRect.Contains(gui->CursorX(), gui->CursorY())) {
							if (event->evValue2) {
								children[i]->flags ^= WIN_SELECTED;
								if (children[i]->flags & WIN_SELECTED) {
									flags &= ~WIN_SELECTED;
									return "childsel";
								}
							}
						}
					}
				}
			} else if (event->evValue == K_TAB && event->evValue2) {
				if (GetFocusedChild()) {
					const char *childRet = GetFocusedChild()->HandleEvent(event, updateVisuals);
					if (childRet && *childRet) {
						return childRet;
					}

					// If the window didn't handle the tab, then move the focus to the next window
					// or the previous window if shift is held down

					int direction = 1;
					if ( hfKeyInput::IsDown( K_LSHIFT ) || hfKeyInput::IsDown( K_RSHIFT ) ) {
						direction = -1;
					}

					hfWindow *currentFocus = GetFocusedChild();
					hfWindow *child = GetFocusedChild();
					hfWindow *parent = child->GetParent();
					while ( parent ) {
						bool foundFocus = false;
						bool recurse = false;
						int index = 0;
						if ( child ) {
							index = parent->GetChildIndex( child ) + direction;
						} else if ( direction < 0 ) {
							index = parent->GetChildCount() - 1;
						}
						while ( index < parent->GetChildCount() && index >= 0) {
							hfWindow *testWindow = parent->GetChild( index );
							if ( testWindow == currentFocus ) {
								// we managed to wrap around and get back to our starting window
								foundFocus = true;
								break;
							}
							if ( testWindow && !testWindow->noEvents && testWindow->visible ) {
								if ( testWindow->flags & WIN_CANFOCUS ) {
									SetFocus( testWindow );
									foundFocus = true;
									break;
								} else if ( testWindow->GetChildCount() > 0 ) {
									parent = testWindow;
									child = NULL;
									recurse = true;
									break;
								}
							}
							index += direction;
						}
						if ( foundFocus ) {
							// We found a child to focus on
							break;
						} else if ( recurse ) {
							// We found a child with children
							continue;
						} else {
							// We didn't find anything, so go back up to our parent
							child = parent;
							parent = child->GetParent();
							if ( parent == gui->GetDesktop() ) {
								// We got back to the desktop, so wrap around but don't actually go to the desktop
								parent = NULL;
								child = NULL;
							}
						}
					}
				}
			} else if ( ( event->evValue == K_ESCAPE || event->evValue == K_JOY9 ) && event->evValue2) {
				if (GetFocusedChild()) {
					const char *childRet = GetFocusedChild()->HandleEvent(event, updateVisuals);
					if (childRet && *childRet) {
						return childRet;
					}
				}
				RunScript( ON_ESC );
			} else if (event->evValue == K_ENTER ) {
				if (GetFocusedChild()) {
					const char *childRet = GetFocusedChild()->HandleEvent(event, updateVisuals);
					if (childRet && *childRet) {
						return childRet;
					}
				}
				if ( flags & WIN_WANTENTER ) {
					if ( event->evValue2 ) {
						RunScript( ON_ACTION );
					} else {
						RunScript( ON_ACTIONRELEASE );
					}
				}
			} else {
				if (GetFocusedChild()) {
					const char *childRet = GetFocusedChild()->HandleEvent(event, updateVisuals);
					if (childRet && *childRet) {
						return childRet;
					}
				}
			}

		} else if (event->evType == SE_MOUSE) {
			if (updateVisuals) {
				*updateVisuals = true;
			}
			const char *mouseRet = RouteMouseCoords(event->evValue, event->evValue2);
			if (mouseRet != NULL && *mouseRet != NULL) {
				return mouseRet;
			}
		} else if (event->evType == SE_NONE) {
		} else if (event->evType == SE_CHAR) {
			if (GetFocusedChild()) {
				const char *childRet = GetFocusedChild()->HandleEvent(event, updateVisuals);
				if (childRet && *childRet) {
					return childRet;
				}
			}
		}
	}

	gui->GetReturnCmd() = cmd;
	if ( gui->GetPendingCmd().Length() ) {
		gui->GetReturnCmd() += " ; ";
		gui->GetReturnCmd() += gui->GetPendingCmd();
		gui->GetPendingCmd().Clear();
	}
	cmd = "";
	return gui->GetReturnCmd();
}

/*
================
hfWindow::DebugDraw
================
*/
void hfWindow::DebugDraw(int time, float x, float y) {
	static char buff[16384] = { 0 };
	if (dc) {
		dc->EnableClipping(false);
		if (gui_debug.GetInteger() == 1) {
			dc->DrawRect(drawRect.x, drawRect.y, drawRect.w, drawRect.h, 1, hfDeviceContext::colorRed);
		} else if (gui_debug.GetInteger() == 2) {
			char out[1024];
			hfStr str;
			str = text.c_str();
			
			if (str.Length()) {
				sprintf(buff, "%s\n", str.c_str());
			}

			sprintf(out, "Rect: %0.1f, %0.1f, %0.1f, %0.1f\n", rect.x(), rect.y(), rect.w(), rect.h());
			strcat(buff, out);
			sprintf(out, "Draw Rect: %0.1f, %0.1f, %0.1f, %0.1f\n", drawRect.x, drawRect.y, drawRect.w, drawRect.h);
			strcat(buff, out);
			sprintf(out, "Client Rect: %0.1f, %0.1f, %0.1f, %0.1f\n", clientRect.x, clientRect.y, clientRect.w, clientRect.h);
			strcat(buff, out);
			sprintf(out, "Cursor: %0.1f : %0.1f\n", gui->CursorX(), gui->CursorY());
			strcat(buff, out);


			//hfRectangle tempRect = textRect;
			//tempRect.x += offsetX;
			//drawRect.y += offsetY;
			dc->DrawText(buff, textScale, textAlign, foreColor, textRect, true);
		} 
		dc->EnableClipping(true);
	}
}

/*
================
hfWindow::Transition
================
*/
void hfWindow::Transition() {
	int i, c = transitions.Num();
	bool clear = true;

	for ( i = 0; i < c; i++ ) {
		hfTransitionData *data = &transitions[i];
		hfWinRectangle *r = NULL;
		hfWinVec4 *v4 = dynamic_cast<hfWinVec4*>(data->data);
		hfWinFloat* val = NULL;
		if (v4 == NULL) {
			r = dynamic_cast<hfWinRectangle*>(data->data);
			if ( r == NULL ) {
				val = dynamic_cast<hfWinFloat*>(data->data);
			}
		}
		if ( data->interp.IsDone( gui->GetTime() ) && data->data) {
			if (v4) {
				*v4 = data->interp.GetEndValue();
			} else if ( val ) {
				*val = data->interp.GetEndValue()[0];
			} else if ( r != NULL ) {
				*r = data->interp.GetEndValue();
			}
		} else {
			clear = false;
			if (data->data) {
				if (v4) {
					*v4 = data->interp.GetCurrentValue( gui->GetTime() );
				} else if ( val ) {
					*val = data->interp.GetCurrentValue( gui->GetTime() )[0];
				} else if ( r != NULL ) {
					*r = data->interp.GetCurrentValue( gui->GetTime() );
				}
			} else {
				common->Warning("Invalid transitional data for window %s in gui %s", GetName(), gui->GetSourceFile());
			}
		}
	}

	if ( clear ) {
		transitions.SetNum( 0 );
		flags &= ~WIN_INTRANSITION;
	}
}

/*
================
hfWindow::Time
================
*/
void hfWindow::Time() {
	
	if ( noTime ) {
		return;
	}

	if ( timeLine == -1 ) {
		timeLine = gui->GetTime();
	}

	cmd = "";

	int c = timeLineEvents.Num();
	if ( c > 0 ) {
		for (int i = 0; i < c; i++) {
			if ( timeLineEvents[i]->pending && gui->GetTime() - timeLine >= timeLineEvents[i]->time ) {
				timeLineEvents[i]->pending = false;
				RunScriptList( timeLineEvents[i]->event );
			}
		}
	}

	if ( gui->Active() ) {
		if ( gui->GetPendingCmd().Length() > 0 ) {
			gui->GetPendingCmd() += ";";
		}
		gui->GetPendingCmd() += cmd;
	}
}

/*
================
hfWindow::EvalRegs
================
*/
float hfWindow::EvalRegs(int test, bool force) {
	static float regs[MAX_EXPRESSION_REGISTERS];
	static hfWindow *lastEval = NULL;

	if (!force && test >= 0 && test < MAX_EXPRESSION_REGISTERS && lastEval == this) {
		return regs[test];
	}

	lastEval = this;

	if (expressionRegisters.Num()) {
		regList.SetToRegs(regs);
		EvaluateRegisters(regs);
		regList.GetFromRegs(regs);
	}

	if (test >= 0 && test < MAX_EXPRESSION_REGISTERS) {
		return regs[test];
	}

	return 0.0;
}

/*
================
hfWindow::DrawBackground
================
*/
void hfWindow::DrawBackground(const hfRectangle &drawRect) {
	if ( backColor.w() ) {
		dc->DrawFilledRect(drawRect.x, drawRect.y, drawRect.w, drawRect.h, backColor);
	}

	if ( background && matColor.w() ) {
		float scalex, scaley;
		if ( flags & WIN_NATURALMAT ) {
			scalex = drawRect.w / background->GetImageWidth();
			scaley = drawRect.h / background->GetImageHeight();
		} else {
			scalex = matScalex;
			scaley = matScaley;
		}
		dc->DrawMaterial(drawRect.x, drawRect.y, drawRect.w, drawRect.h, background, matColor, scalex, scaley);
	}
}

/*
================
hfWindow::DrawBorderAndCaption
================
*/
void hfWindow::DrawBorderAndCaption(const hfRectangle &drawRect) {
	if ( flags & WIN_BORDER && borderSize && borderColor.w() ) {
		dc->DrawRect(drawRect.x, drawRect.y, drawRect.w, drawRect.h, borderSize, borderColor);
	}
}

/*
================
hfWindow::SetupTransforms
================
*/
void hfWindow::SetupTransforms(float x, float y) {
	static hfMat3 trans;
	static hfVec3 org;
	
	trans.Identity();
	org.Set( origin.x + x, origin.y + y, 0 );

	if ( rotate ) {
		static hfRotation rot;
		static hfVec3 vec(0, 0, 1);
		rot.Set( org, vec, rotate );
		trans = rot.ToMat3();
	}

	if ( shear.x || shear.y ) {
		static hfMat3 smat;
		smat.Identity();
		smat[0][1] = shear.x;
		smat[1][0] = shear.y;
		trans *= smat;
	}

	if ( !trans.IsIdentity() ) {
		dc->SetTransformInfo( org, trans );
	}
}

/*
================
hfWindow::CalcRects
================
*/
void hfWindow::CalcRects(float x, float y) {
	CalcClientRect(0, 0);
	drawRect.Offset(x, y);
	clientRect.Offset(x, y);
	actualX = drawRect.x;
	actualY = drawRect.y;
	int c = drawWindows.Num();
	for (int i = 0; i < c; i++) {
		if (drawWindows[i].win) {
			drawWindows[i].win->CalcRects(clientRect.x + xOffset, clientRect.y + yOffset);
		}
	}
	drawRect.Offset(-x, -y);
	clientRect.Offset(-x, -y);
}

/*
================
hfWindow::Redraw
================
*/
void hfWindow::Redraw(float x, float y, bool hud) {
	hfStr str;

	if (r_skipGuiShaders.GetInteger() == 1 || dc == NULL ) {
		return;
	}
	
	int time = gui->GetTime();

	if ( flags & WIN_DESKTOP && r_skipGuiShaders.GetInteger() != 3 ) {
		RunTimeEvents( time );
	}

	if ( r_skipGuiShaders.GetInteger() == 2 ) {
		return;
	}

	if ( flags & WIN_SHOWTIME ) {
		dc->DrawText(va(" %0.1f seconds\n%s", (float)(time - timeLine) / 1000, gui->State().GetString("name")), 0.35f, 0, dc->colorWhite, hfRectangle(100, 0, 80, 80), false);
	}

	if ( flags & WIN_SHOWCOORDS ) {
		dc->EnableClipping(false);
		sprintf(str, "x: %i y: %i  cursorx: %i cursory: %i", (int)rect.x(), (int)rect.y(), (int)gui->CursorX(), (int)gui->CursorY());
		dc->DrawText(str, 0.25f, 0, dc->colorWhite, hfRectangle(0, 0, 100, 20), false);
		dc->EnableClipping(true);
	}

	if (!visible) {
		return;
	}

	CalcClientRect(0, 0);

	SetFont();

	if ( hud ) {
		float tileSafeOffset = hud_titlesafe.GetFloat();
		float tileSafeScale = 1.0f / ( 1.0f - hud_titlesafe.GetFloat() * 2.0f );
		dc->SetSize( forceAspectWidth * tileSafeScale, forceAspectHeight * tileSafeScale );
		dc->SetOffset( forceAspectWidth * tileSafeOffset, forceAspectHeight * tileSafeOffset );
	} else {
		dc->SetSize( forceAspectWidth, forceAspectHeight );
		dc->SetOffset( 0.0f, 0.0f );
	}

	//FIXME: go to screen coord tracking
	drawRect.Offset(x, y);
	clientRect.Offset(x, y);
	textRect.Offset(x, y);
	actualX = drawRect.x;
	actualY = drawRect.y;

	hfVec3	oldOrg;
	hfMat3	oldTrans;
		
	dc->GetTransformInfo( oldOrg, oldTrans );

	SetupTransforms(x, y);
	DrawBackground(drawRect);
	DrawBorderAndCaption(drawRect);

	if ( !( flags & WIN_NOCLIP) ) {
		dc->PushClipRect(clientRect);
	} 

	if ( r_skipGuiShaders.GetInteger() < 5 ) {
		Draw(time, x, y);
	}

	if ( gui_debug.GetInteger() ) {
		DebugDraw(time, x, y);
	}

	int c = drawWindows.Num();
	for ( int i = 0; i < c; i++ ) {
		if ( drawWindows[i].win ) {
			drawWindows[i].win->Redraw( clientRect.x + xOffset, clientRect.y + yOffset, hud );
		} else {
			drawWindows[i].simp->Redraw( clientRect.x + xOffset, clientRect.y + yOffset );
		}
	}

	// Put transforms back to what they were before the children were processed
	dc->SetTransformInfo(oldOrg, oldTrans);

	if ( ! ( flags & WIN_NOCLIP ) ) {
		dc->PopClipRect();
	} 

	if (gui_edit.GetBool()  || (flags & WIN_DESKTOP && !( flags & WIN_NOCURSOR )  && !hideCursor && (gui->Active() || ( flags & WIN_MENUGUI ) ))) {
		dc->SetTransformInfo(vec3_origin, mat3_identity);
		gui->DrawCursor();
	}

	if (gui_debug.GetInteger() && flags & WIN_DESKTOP) {
		dc->EnableClipping(false);
		sprintf(str, "x: %1.f y: %1.f",  gui->CursorX(), gui->CursorY());
		dc->DrawText(str, 0.25, 0, dc->colorWhite, hfRectangle(0, 0, 100, 20), false);
		dc->DrawText(gui->GetSourceFile(), 0.25, 0, dc->colorWhite, hfRectangle(0, 20, 300, 20), false);
		dc->EnableClipping(true);
	}

	drawRect.Offset(-x, -y);
	clientRect.Offset(-x, -y);
	textRect.Offset(-x, -y);
}

/*
================
hfWindow::ArchiveToDictionary
================
*/
void hfWindow::ArchiveToDictionary(hfDict *dict, bool useNames) {
	//FIXME: rewrite without state
	int c = children.Num();
	for (int i = 0; i < c; i++) {
		children[i]->ArchiveToDictionary(dict);
	}
}

/*
================
hfWindow::InitFromDictionary
================
*/
void hfWindow::InitFromDictionary(hfDict *dict, bool byName) {
	//FIXME: rewrite without state
	int c = children.Num();
	for (int i = 0; i < c; i++) {
		children[i]->InitFromDictionary(dict);
	}
}

/*
================
hfWindow::CalcClientRect
================
*/
void hfWindow::CalcClientRect(float xofs, float yofs) {
	drawRect = rect;

	if ( flags & WIN_INVERTRECT ) {
		drawRect.x = rect.x() - rect.w();
		drawRect.y = rect.y() - rect.h();
	}
	
	if (flags & (WIN_HCENTER | WIN_VCENTER) && parent) {
		// in this case treat xofs and yofs as absolute top left coords
		// and ignore the original positioning
		if (flags & WIN_HCENTER) {
			drawRect.x = (parent->rect.w() - rect.w()) / 2;
		} else {
			drawRect.y = (parent->rect.h() - rect.h()) / 2;
		}
	}

	drawRect.x += xofs;
	drawRect.y += yofs;

	clientRect = drawRect;
	if (rect.h() > 0.0 && rect.w() > 0.0) {

		if (flags & WIN_BORDER && borderSize != 0.0) {
			clientRect.x += borderSize;
			clientRect.y += borderSize;
			clientRect.w -= borderSize;
			clientRect.h -= borderSize;
		}

		textRect = clientRect;
		textRect.x += 2.0;
	 	textRect.w -= 2.0;
		textRect.y += 2.0;
		textRect.h -= 2.0;

		textRect.x += textAlignx;
		textRect.y += textAligny;

	}
	origin.Set( rect.x() + (rect.w() / 2 ), rect.y() + ( rect.h() / 2 ) );

}

/*
================
hfWindow::SetupBackground
================
*/
void hfWindow::SetupBackground() {
	if (backGroundName.Length()) {
		background = declManager->FindMaterial(backGroundName);
		if ( background != NULL && !background->TestMaterialFlag( MF_DEFAULTED ) ) {
			background->SetSort(SS_GUI );
		}
	}
	backGroundName.SetMaterialPtr(&background);
}

/*
================
hfWindow::SetupFromState
================
*/
void hfWindow::SetupFromState() {
	hfStr str;
	background = NULL;

	SetupBackground();

	if (borderSize) {
		flags |= WIN_BORDER;
	}

	if (regList.FindReg("rotate") || regList.FindReg("shear")) {
		flags |= WIN_TRANSFORM;
	}
	
	CalcClientRect(0,0);
	if ( scripts[ ON_ACTION ] ) {
		cursor = hfDeviceContext::CURSOR_HAND;
		flags |= WIN_CANFOCUS;
	}
}

/*
================
hfWindow::Moved
================
*/
void hfWindow::Moved() {
}

/*
================
hfWindow::Sized
================
*/
void hfWindow::Sized() {
}

/*
================
hfWindow::GainFocus
================
*/
void hfWindow::GainFocus() {
}

/*
================
hfWindow::LoseFocus
================
*/
void hfWindow::LoseFocus() {
}

/*
================
hfWindow::GainCapture
================
*/
void hfWindow::GainCapture() {
}

/*
================
hfWindow::LoseCapture
================
*/
void hfWindow::LoseCapture() {
	flags &= ~WIN_CAPTURE;
}

/*
================
hfWindow::SetFlag
================
*/
void hfWindow::SetFlag(unsigned int f) {
	flags |= f;
}

/*
================
hfWindow::ClearFlag
================
*/
void hfWindow::ClearFlag(unsigned int f) {
	flags &= ~f;
}


/*
================
hfWindow::SetParent
================
*/
void hfWindow::SetParent(hfWindow *w) {
	parent = w;
}

/*
================
hfWindow::GetCaptureChild
================
*/
hfWindow *hfWindow::GetCaptureChild() {
	if (flags & WIN_DESKTOP) {
		return gui->GetDesktop()->captureChild;
	}
	return NULL;
}

/*
================
hfWindow::GetFocusedChild
================
*/
hfWindow *hfWindow::GetFocusedChild() {
	if (flags & WIN_DESKTOP) {
		return gui->GetDesktop()->focusedChild;
	}
	return NULL;
}


/*
================
hfWindow::SetFocus
================
*/
hfWindow *hfWindow::SetFocus(hfWindow *w, bool scripts) {
	// only one child can have the focus
	hfWindow *lastFocus = NULL;
	if (w->flags & WIN_CANFOCUS) {
		lastFocus = gui->GetDesktop()->focusedChild;
		if ( lastFocus ) {
			lastFocus->flags &= ~WIN_FOCUS;
			lastFocus->LoseFocus();
		}

		//  call on lose focus
		if ( scripts && lastFocus ) {
			// calling this broke all sorts of guis
			// lastFocus->RunScript(ON_MOUSEEXIT);
		}
		//  call on gain focus
		if ( scripts && w ) {
			// calling this broke all sorts of guis
			// w->RunScript(ON_MOUSEENTER);
		}

		w->flags |= WIN_FOCUS;
		w->GainFocus();
		gui->GetDesktop()->focusedChild = w;
	}

	return lastFocus;
}

/*
================
hfWindow::ParseScript
================
*/
bool hfWindow::ParseScript(hfTokenParser *src, hfGuiScriptList &list, int *timeParm, bool elseBlock ) {

	bool	ifElseBlock = false;

	hfToken token;

	// scripts start with { ( unless parm is true ) and have ; separated command lists.. commands are command,
	// arg.. basically we want everything between the { } as it will be interpreted at
	// run time
	
	if ( elseBlock ) {
		src->ReadToken ( &token );
	
		if ( !token.Icmp ( "if" ) ) {
			ifElseBlock = true;
		}
		
		src->UnreadToken ( &token );

		if ( !ifElseBlock && !src->ExpectTokenString( "{" ) ) {
			return false;
		}
	}
	else if ( !src->ExpectTokenString( "{" ) ) {
		return false;
	}

	int nest = 0;

	while (1) {
		if ( !src->ReadToken(&token) ) {
			src->Error( "Unexpected end of file" );
			return false;
		}

		if ( token == "{" ) {
			nest++;
		}

		if ( token == "}" ) {
			if (nest-- <= 0) {
				return true;
			}
		}

		hfGuiScript *gs = new (TAG_OLD_UI) hfGuiScript();
		if (token.Icmp("if") == 0) {
			gs->conditionReg = ParseExpression(src);
			gs->ifList = new (TAG_OLD_UI) hfGuiScriptList();
			ParseScript(src, *gs->ifList, NULL);
			if (src->ReadToken(&token)) {
				if (token == "else") {
					gs->elseList = new (TAG_OLD_UI) hfGuiScriptList();
					// pass true to indicate we are parsing an else condition
					ParseScript(src, *gs->elseList, NULL, true );
				} else {
					src->UnreadToken(&token);
				}
			}

			list.Append(gs);

			// if we are parsing an else if then return out so 
			// the initial "if" parser can handle the rest of the tokens
			if ( ifElseBlock ) {
				return true;
			}
			continue;
		} else {
			src->UnreadToken(&token);
		}

		// empty { } is not allowed
		if ( token == "{" ) {
			 src->Error ( "Unexpected {" );
			 delete gs;
			 return false;
		}

		gs->Parse(src);
		list.Append(gs);
	}

}

/*
================
hfWindow::SaveExpressionParseState
================
*/
void hfWindow::SaveExpressionParseState() {
	saveTemps = (bool*)Mem_Alloc(MAX_EXPRESSION_REGISTERS * sizeof(bool), TAG_CRAP);
	memcpy(saveTemps, registerIsTemporary, MAX_EXPRESSION_REGISTERS * sizeof(bool));
}

/*
================
hfWindow::RestoreExpressionParseState
================
*/
void hfWindow::RestoreExpressionParseState() {
	memcpy(registerIsTemporary, saveTemps, MAX_EXPRESSION_REGISTERS * sizeof(bool));
	Mem_Free(saveTemps);
}

/*
================
hfWindow::ParseScriptEntry
================
*/
bool hfWindow::ParseScriptEntry(const char *name, hfTokenParser *src) {
	for (int i = 0; i < SCRIPT_COUNT; i++) {
		if (hfStr::Icmp(name, ScriptNames[i]) == 0) {
			delete scripts[i];
			scripts[i] = new (TAG_OLD_UI) hfGuiScriptList;
			return ParseScript(src, *scripts[i]);
		}
	}
	return false;
}

/*
================
hfWindow::DisableRegister
================
*/
void hfWindow::DisableRegister(const char *_name) {
	hfRegister *reg = RegList()->FindReg(_name);
	if (reg) {
		reg->Enable(false);
	}
}

/*
================================
hfSort_TimeLine
================================
*/
class hfSort_TimeLine : public hfSort_Quick< hfTimeLineEvent *, hfSort_TimeLine > {
public:
	int Compare( hfTimeLineEvent * const & a, hfTimeLineEvent * const & b ) const {
		return a->time - b->time;
	}
};

/*
================
hfWindow::PostParse
================
*/
void hfWindow::PostParse() {
	// Sort timeline events
	hfSort_TimeLine sorter;
	timeLineEvents.SortWithTemplate( sorter );
}
	
/*
================
hfWindow::GetWinVarOffset
================
*/
int hfWindow::GetWinVarOffset( hfWinVar *wv, drawWin_t* owner) {
	int ret = -1;

	if ( wv == &rect ) {
		ret = (int)(intptr_t)&( ( hfWindow * ) 0 )->rect;
	}

	if ( wv == &backColor ) {
		ret = (int)(intptr_t)&( ( hfWindow * ) 0 )->backColor;
	}

	if ( wv == &matColor ) {
		ret = (int)(intptr_t)&( ( hfWindow * ) 0 )->matColor;
	}

	if ( wv == &foreColor ) {
		ret = (int)(intptr_t)&( ( hfWindow * ) 0 )->foreColor;
	}

	if ( wv == &hoverColor ) {
		ret = (int)(intptr_t)&( ( hfWindow * ) 0 )->hoverColor;
	}

	if ( wv == &borderColor ) {
		ret = (int)(intptr_t)&( ( hfWindow * ) 0 )->borderColor;
	}

	if ( wv == &textScale ) {
		ret = (int)(intptr_t)&( ( hfWindow * ) 0 )->textScale;
	}

	if ( wv == &rotate ) {
		ret = (int)(intptr_t)&( ( hfWindow * ) 0 )->rotate;
	}

	if ( ret != -1 ) {
		owner->win = this;
		return ret;
	}

	for ( int i = 0; i < drawWindows.Num(); i++ ) {
		if ( drawWindows[i].win ) {
			ret = drawWindows[i].win->GetWinVarOffset( wv, owner );
		} else {
			ret = drawWindows[i].simp->GetWinVarOffset( wv, owner );
		}
		if ( ret != -1 ) {
			break;
		}
	}

	return ret;
}

/*
================
hfWindow::GetWinVarByName
================
*/
hfWinVar *hfWindow::GetWinVarByName(const char *_name, bool fixup, drawWin_t** owner) {
	hfWinVar *retVar = NULL;

	if ( owner ) {
		*owner = NULL;
	}

	if (hfStr::Icmp(_name, "notime") == 0) {
		retVar = &noTime;
	}
	if (hfStr::Icmp(_name, "background") == 0) {
		retVar = &backGroundName;
	}
	if (hfStr::Icmp(_name, "visible") == 0) {
		retVar = &visible;
	}
	if (hfStr::Icmp(_name, "rect") == 0) {
		retVar = &rect;
	}
	if (hfStr::Icmp(_name, "backColor") == 0) {
		retVar = &backColor;
	}
	if (hfStr::Icmp(_name, "matColor") == 0) {
		retVar = &matColor;
	}
	if (hfStr::Icmp(_name, "foreColor") == 0) {
		retVar = &foreColor;
	}
	if (hfStr::Icmp(_name, "hoverColor") == 0) {
		retVar = &hoverColor;
	}
	if (hfStr::Icmp(_name, "borderColor") == 0) {
		retVar = &borderColor;
	}
	if (hfStr::Icmp(_name, "textScale") == 0) {
		retVar = &textScale;
	}
	if (hfStr::Icmp(_name, "rotate") == 0) {
		retVar = &rotate;
	}
	if (hfStr::Icmp(_name, "noEvents") == 0) {
		retVar = &noEvents;
	}
	if (hfStr::Icmp(_name, "text") == 0) {
		retVar = &text;
	}
	if (hfStr::Icmp(_name, "backGroundName") == 0) {
		retVar = &backGroundName;
	}
	if (hfStr::Icmp(_name, "hidecursor") == 0) {
		retVar = &hideCursor;
	}

	hfStr key = _name;
	bool guiVar = (key.Find(VAR_GUIPREFIX) >= 0);
	int c = definedVars.Num();
	for (int i = 0; i < c; i++) {
		if (hfStr::Icmp(_name, (guiVar) ? va("%s",definedVars[i]->GetName()) : definedVars[i]->GetName()) == 0) {
			retVar = definedVars[i];
			break;
		}
	}

	if (retVar) {
		if (fixup && *_name != '$') {
			DisableRegister(_name);
		}

		if ( owner && parent ) {
			*owner = parent->FindChildByName ( name );
		}

		return retVar;
	}

	int len = key.Length();
	if ( len > 5 && guiVar ) {
		hfWinVar *var = new (TAG_OLD_UI) hfWinStr;
		var->Init(_name, this);
		definedVars.Append(var);
		return var;
	} else if (fixup) {
		int n = key.Find("::");
		if (n > 0) {
			hfStr winName = key.Left(n);
			hfStr var = key.Right(key.Length() - n - 2);
			drawWin_t *win = GetGui()->GetDesktop()->FindChildByName(winName);
			if (win) {
				if (win->win) {
					return win->win->GetWinVarByName(var, false, owner);
				} else {
					if ( owner ) {
						*owner = win;
					}
					return win->simp->GetWinVarByName(var);
				}
			} 
		}
	}
	return NULL;
}

/*
================
hfWindow::ParseString
================
*/
void hfWindow::ParseString(hfTokenParser *src, hfStr &out) {
	hfToken tok;
	if (src->ReadToken(&tok)) {
		out = tok;
	}
}

/*
================
hfWindow::ParseVec4
================
*/
void hfWindow::ParseVec4(hfTokenParser *src, hfVec4 &out) {
	hfToken tok;
	src->ReadToken(&tok);
	out.x = atof(tok);
	src->ExpectTokenString(",");
	src->ReadToken(&tok);
	out.y = atof(tok);
	src->ExpectTokenString(",");
	src->ReadToken(&tok);
	out.z = atof(tok);
	src->ExpectTokenString(",");
	src->ReadToken(&tok);
	out.w = atof(tok);
}

/*
================
hfWindow::ParseInternalVar
================
*/
bool hfWindow::ParseInternalVar(const char *_name, hfTokenParser *src) {

	if (hfStr::Icmp(_name, "showtime") == 0) {
		if ( src->ParseBool() ) {
			flags |= WIN_SHOWTIME;
		}
		return true;
	}
	if (hfStr::Icmp(_name, "showcoords") == 0) {
		if ( src->ParseBool() ) {
			flags |= WIN_SHOWCOORDS;
		}
		return true;
	}
	if (hfStr::Icmp(_name, "forceaspectwidth") == 0) {
		forceAspectWidth = src->ParseFloat();
		return true;
	}
	if (hfStr::Icmp(_name, "forceaspectheight") == 0) {
		forceAspectHeight = src->ParseFloat();
		return true;
	}
	if (hfStr::Icmp(_name, "matscalex") == 0) {
		matScalex = src->ParseFloat();
		return true;
	}
	if (hfStr::Icmp(_name, "matscaley") == 0) {
		matScaley = src->ParseFloat();
		return true;
	}
	if (hfStr::Icmp(_name, "bordersize") == 0) {
		borderSize = src->ParseFloat();
		return true;
	}
	if (hfStr::Icmp(_name, "nowrap") == 0) {
		if ( src->ParseBool() ) {
			flags |= WIN_NOWRAP;
		}
		return true;
	}
	if (hfStr::Icmp(_name, "shadow") == 0) {
		textShadow = src->ParseInt();
		return true;
	}
	if (hfStr::Icmp(_name, "textalign") == 0) {
		textAlign = src->ParseInt();
		return true;
	}
	if (hfStr::Icmp(_name, "textalignx") == 0) {
		textAlignx = src->ParseFloat();
		return true;
	}
	if (hfStr::Icmp(_name, "textaligny") == 0) {
		textAligny = src->ParseFloat();
		return true;
	}
	if (hfStr::Icmp(_name, "shear") == 0) {
		shear.x = src->ParseFloat();
		hfToken tok;
		src->ReadToken( &tok );
		if ( tok.Icmp( "," ) ) {
			src->Error( "Expected comma in shear definiation" );
			return false;
		}
		shear.y = src->ParseFloat();
		return true;
	}
	if (hfStr::Icmp(_name, "wantenter") == 0) {
		if ( src->ParseBool() ) {
			flags |= WIN_WANTENTER;
		}
		return true;
	}
	if (hfStr::Icmp(_name, "naturalmatscale") == 0) {
		if ( src->ParseBool() ) {
			flags |= WIN_NATURALMAT;
		}
		return true;
	}
	if (hfStr::Icmp(_name, "noclip") == 0) {
		if ( src->ParseBool() ) {
			flags |= WIN_NOCLIP;
		}
		return true;
	}
	if (hfStr::Icmp(_name, "nocursor") == 0) {
		if ( src->ParseBool() ) {
			flags |= WIN_NOCURSOR;
		}
		return true;
	}
	if (hfStr::Icmp(_name, "menugui") == 0) {
		if ( src->ParseBool() ) {
			flags |= WIN_MENUGUI;
		}
		return true;
	}
	if (hfStr::Icmp(_name, "modal") == 0) {
		if ( src->ParseBool() ) {
			flags |= WIN_MODAL;
		}
		return true;
	}
	if (hfStr::Icmp(_name, "invertrect") == 0) {
		if ( src->ParseBool() ) {
			flags |= WIN_INVERTRECT;
		}
		return true;
	}
	if (hfStr::Icmp(_name, "name") == 0) {
		ParseString(src, name);
		return true;
	}
	if (hfStr::Icmp(_name, "play") == 0) {
		common->Warning( "play encountered during gui parse.. see Robert\n" );
		hfStr playStr;
		ParseString(src, playStr);
		return true;
	}
	if (hfStr::Icmp(_name, "comment") == 0) {
		ParseString(src, comment);
		return true;
	}
	if ( hfStr::Icmp( _name, "font" ) == 0 ) {
		hfStr fontName;
		ParseString( src, fontName );
		font = renderSystem->RegisterFont( fontName );
		return true;
	}
	return false;
}

/*
================
hfWindow::ParseRegEntry
================
*/
bool hfWindow::ParseRegEntry(const char *name, hfTokenParser *src) {
	hfStr work;
	work = name;
	work.ToLower();

	hfWinVar *var = GetWinVarByName(work, NULL);
	if ( var ) {
		for (int i = 0; i < NumRegisterVars; i++) {
			if (hfStr::Icmp(work, RegisterVars[i].name) == 0) {
				regList.AddReg(work, RegisterVars[i].type, src, this, var);
				return true;
			}
		}
	}

	// not predefined so just read the next token and add it to the state
	hfToken tok;
	hfVec4 v;	
	hfWinInt *vari;
	hfWinFloat *varf;
	hfWinStr *vars;
	if (src->ReadToken(&tok)) {
		if (var) {
			var->Set(tok);
			return true;
		}
		switch (tok.type) {
			case TT_NUMBER : 
				if (tok.subtype & TT_INTEGER) {
					vari = new (TAG_OLD_UI) hfWinInt();
					*vari = atoi(tok);
					vari->SetName(work);
					definedVars.Append(vari);
				} else if (tok.subtype & TT_FLOAT) {
					varf = new (TAG_OLD_UI) hfWinFloat();
					*varf = atof(tok);
					varf->SetName(work);
					definedVars.Append(varf);
				} else {
					vars = new (TAG_OLD_UI) hfWinStr();
					*vars = tok;
					vars->SetName(work);
					definedVars.Append(vars);
				}
				break;
			default :
				vars = new (TAG_OLD_UI) hfWinStr();
				*vars = tok;
				vars->SetName(work);
				definedVars.Append(vars);
				break;
		}
	}

	return true;
}

/*
================
hfWindow::SetInitialState
================
*/
void hfWindow::SetInitialState(const char *_name) {
	name = _name;
	matScalex = 1.0;
	matScaley = 1.0;
	forceAspectWidth = 640.0;
	forceAspectHeight = 480.0;
	noTime = false;
	visible = true;
	flags = 0;
}

/*
================
hfWindow::Parse
================
*/
bool hfWindow::Parse( hfTokenParser *src, bool rebuild) {
	hfToken token, token2, token3, token4, token5, token6, token7;
	hfStr work;

	if (rebuild) {
		CleanUp();
	}

	drawWin_t dwt;

	timeLineEvents.Clear();
	transitions.Clear();

	namedEvents.DeleteContents( true );

	src->ExpectTokenType( TT_NAME, 0, &token );

	SetInitialState(token);

	src->ExpectTokenString( "{" );
	src->ExpectAnyToken( &token );

	bool ret = true;

	while( token != "}" ) {
		// track what was parsed so we can maintain it for the guieditor
		src->SetMarker ( );

		if ( token == "windowDef" || token == "animationDef" ) {
			if (token == "animationDef") {
				visible = false;
				rect = hfRectangle(0,0,0,0);
			}
			src->ExpectTokenType( TT_NAME, 0, &token );
			token2 = token;
			src->UnreadToken(&token);
			drawWin_t *dw = FindChildByName(token2.c_str());
			if (dw != NULL && dw->win != NULL) {
				SaveExpressionParseState();
				dw->win->Parse(src, rebuild);
				RestoreExpressionParseState();
			} else {
				hfWindow *win = new (TAG_OLD_UI) hfWindow(gui);
				SaveExpressionParseState();
				win->Parse(src, rebuild);
				RestoreExpressionParseState();
				win->SetParent(this);
				dwt.simp = NULL;
				dwt.win = NULL;
				if (win->IsSimple()) {
					hfSimpleWindow *simple = new (TAG_OLD_UI) hfSimpleWindow(win);
					dwt.simp = simple;
					drawWindows.Append(dwt);
					delete win;
				} else {
					AddChild(win);
					SetFocus(win,false);
					dwt.win = win;
					drawWindows.Append(dwt);
				}
			}
		} 
		else if ( token == "editDef" ) {
			hfEditWindow *win = new (TAG_OLD_UI) hfEditWindow(gui);
		  	SaveExpressionParseState();
			win->Parse(src, rebuild);	
		  	RestoreExpressionParseState();
			AddChild(win);
			win->SetParent(this);
			dwt.simp = NULL;
			dwt.win = win;
			drawWindows.Append(dwt);
		}
		else if ( token == "choiceDef" ) {
			hfChoiceWindow *win = new (TAG_OLD_UI) hfChoiceWindow(gui);
		  	SaveExpressionParseState();
			win->Parse(src, rebuild);	
		  	RestoreExpressionParseState();
			AddChild(win);
			win->SetParent(this);
			dwt.simp = NULL;
			dwt.win = win;
			drawWindows.Append(dwt);
		}
		else if ( token == "sliderDef" ) {
			hfSliderWindow *win = new (TAG_OLD_UI) hfSliderWindow(gui);
		  	SaveExpressionParseState();
			win->Parse(src, rebuild);	
		  	RestoreExpressionParseState();
			AddChild(win);
			win->SetParent(this);
			dwt.simp = NULL;
			dwt.win = win;
			drawWindows.Append(dwt);
		}
		else if ( token == "bindDef" ) {
			hfBindWindow *win = new (TAG_OLD_UI) hfBindWindow(gui);
		  	SaveExpressionParseState();
			win->Parse(src, rebuild);	
		  	RestoreExpressionParseState();
			AddChild(win);
			win->SetParent(this);
			dwt.simp = NULL;
			dwt.win = win;
			drawWindows.Append(dwt);
		}
		else if ( token == "listDef" ) {
			hfListWindow *win = new (TAG_OLD_UI) hfListWindow(gui);
		  	SaveExpressionParseState();
			win->Parse(src, rebuild);	
		  	RestoreExpressionParseState();
			AddChild(win);
			win->SetParent(this);
			dwt.simp = NULL;
			dwt.win = win;
			drawWindows.Append(dwt);
		}
		else if ( token == "fieldDef" ) {
			hfFieldWindow *win = new (TAG_OLD_UI) hfFieldWindow(gui);
		  	SaveExpressionParseState();
			win->Parse(src, rebuild);	
		  	RestoreExpressionParseState();
			AddChild(win);
			win->SetParent(this);
			dwt.simp = NULL;
			dwt.win = win;
			drawWindows.Append(dwt);
		}
		else if ( token == "renderDef" ) {
			// D3 could render a 3D model in a subrect of a full screen
			// GUI for the main menus, but we have cut that ability so
			// we don't need to deal with offset viewports on all platforms.
			hfRenderWindow *win = new (TAG_OLD_UI) hfRenderWindow(gui);
		  	SaveExpressionParseState();
			win->Parse(src, rebuild);	
		  	RestoreExpressionParseState();
			AddChild(win);
			win->SetParent(this);
			dwt.simp = NULL;
			dwt.win = win;
			drawWindows.Append(dwt);
		}
		else if ( token == "gameSSDDef" ) {
			hfGameSSDWindow *win = new (TAG_OLD_UI) hfGameSSDWindow(gui);
			SaveExpressionParseState();
			win->Parse(src, rebuild);	
			RestoreExpressionParseState();
			AddChild(win);
			win->SetParent(this);
			dwt.simp = NULL;
			dwt.win = win;
			drawWindows.Append(dwt);
		}
		else if ( token == "gameBearShootDef" ) {
			hfGameBearShootWindow *win = new (TAG_OLD_UI) hfGameBearShootWindow(gui);
			SaveExpressionParseState();
			win->Parse(src, rebuild);	
			RestoreExpressionParseState();
			AddChild(win);
			win->SetParent(this);
			dwt.simp = NULL;
			dwt.win = win;
			drawWindows.Append(dwt);
		}
		else if ( token == "gameBustOutDef" ) {
			hfGameBustOutWindow *win = new (TAG_OLD_UI) hfGameBustOutWindow(gui);
			SaveExpressionParseState();
			win->Parse(src, rebuild);	
			RestoreExpressionParseState();
			AddChild(win);
			win->SetParent(this);
			dwt.simp = NULL;
			dwt.win = win;
			drawWindows.Append(dwt);
		}
// 
//  added new onEvent
		else if ( token == "onNamedEvent" ) {
			// Read the event name
			if ( !src->ReadToken(&token) ) {
				src->Error( "Expected event name" );
				return false;
			}

			rvNamedEvent* ev = new (TAG_OLD_UI) rvNamedEvent ( token );
			
			src->SetMarker ( );
			
			if ( !ParseScript ( src, *ev->mEvent ) ) {
				ret = false;
				break;
			}

			namedEvents.Append(ev);
		}
		else if ( token == "onTime" ) {
			hfTimeLineEvent *ev = new (TAG_OLD_UI) hfTimeLineEvent;

			if ( !src->ReadToken(&token) ) {
				src->Error( "Unexpected end of file" );
				return false;
			}
			ev->time = atoi(token.c_str());
			
			// reset the mark since we dont want it to include the time
			src->SetMarker ( );

			if (!ParseScript(src, *ev->event, &ev->time)) {
				ret = false;
				break;
			}

			// this is a timeline event
			ev->pending = true;
			timeLineEvents.Append(ev);
		}
		else if ( token == "definefloat" ) {
			src->ReadToken(&token);
			work = token;
			work.ToLower();
			hfWinFloat *varf = new (TAG_OLD_UI) hfWinFloat();
			varf->SetName(work);
			definedVars.Append(varf);

			// add the float to the editors wrapper dict
			// Set the marker after the float name
			src->SetMarker ( );

			// Read in the float 
			regList.AddReg(work, hfRegister::FLOAT, src, this, varf);
		}
		else if ( token == "definevec4" ) {
			src->ReadToken(&token);
			work = token;
			work.ToLower();
			hfWinVec4 *var = new (TAG_OLD_UI) hfWinVec4();
			var->SetName(work);

			// set the marker so we can determine what was parsed
			// set the marker after the vec4 name
			src->SetMarker ( );

			// FIXME: how about we add the var to the desktop instead of this window so it won't get deleted
			//        when this window is destoyed which even happens during parsing with simple windows ?
			//definedVars.Append(var);
			gui->GetDesktop()->definedVars.Append( var );
			gui->GetDesktop()->regList.AddReg( work, hfRegister::VEC4, src, gui->GetDesktop(), var );
		}
		else if ( token == "float" ) {
			src->ReadToken(&token);
			work = token;
			work.ToLower();
			hfWinFloat *varf = new (TAG_OLD_UI) hfWinFloat();
			varf->SetName(work);
			definedVars.Append(varf);

			// add the float to the editors wrapper dict
			// set the marker to after the float name
			src->SetMarker ( );

			// Parse the float
			regList.AddReg(work, hfRegister::FLOAT, src, this, varf);
		}
		else if (ParseScriptEntry(token, src)) {

		} else if (ParseInternalVar(token, src)) {

		}
		else {
			ParseRegEntry(token, src);
		} 
		if ( !src->ReadToken( &token ) ) {
			src->Error( "Unexpected end of file" );
			ret = false;
			break;
		}
	}

	if (ret) {
		EvalRegs(-1, true);
	}

	SetupFromState();
	PostParse();

	return ret;
}

/*
================
hfWindow::FindSimpleWinByName
================
*/
hfSimpleWindow *hfWindow::FindSimpleWinByName(const char *_name) {
	int c = drawWindows.Num();
	for (int i = 0; i < c; i++) {
		if (drawWindows[i].simp == NULL) {
			continue;
		}
		if ( hfStr::Icmp(drawWindows[i].simp->name, _name) == 0 ) {
			return drawWindows[i].simp;
		} 
	}
	return NULL;
}

/*
================
hfWindow::FindChildByName
================
*/
drawWin_t *hfWindow::FindChildByName(const char *_name) {
	static drawWin_t dw;
	if (hfStr::Icmp(name,_name) == 0) {
		dw.simp = NULL;
		dw.win = this;
		return &dw;
	}
	int c = drawWindows.Num();
	for (int i = 0; i < c; i++) {
		if (drawWindows[i].win) {
			if (hfStr::Icmp(drawWindows[i].win->name, _name) == 0) {
				return &drawWindows[i];
			}
			drawWin_t *win = drawWindows[i].win->FindChildByName(_name);
			if (win) {
				return win;
			}
		} else {
			if (hfStr::Icmp(drawWindows[i].simp->name, _name) == 0) {
				return &drawWindows[i];
			}
		}
	}
	return NULL;
}

/*
================
hfWindow::GetStrPtrByName
================
*/
hfStr* hfWindow::GetStrPtrByName(const char *_name) {
	return NULL;
}

/*
================
hfWindow::AddTransition
================
*/
void hfWindow::AddTransition(hfWinVar *dest, hfVec4 from, hfVec4 to, int time, float accelTime, float decelTime) {
	hfTransitionData data;
	data.data = dest;
	data.interp.Init(gui->GetTime(), accelTime * time, decelTime * time, time, from, to);
	transitions.Append(data);
}


/*
================
hfWindow::StartTransition
================
*/
void hfWindow::StartTransition() {
	flags |= WIN_INTRANSITION;
}

/*
================
hfWindow::ResetCinematics
================
*/
void hfWindow::ResetCinematics() {
	if ( background ) {
		background->ResetCinematicTime( gui->GetTime() );
	}
}

/*
================
hfWindow::ResetTime
================
*/
void hfWindow::ResetTime(int t) {

	timeLine = gui->GetTime() - t;

	int i, c = timeLineEvents.Num();
	for ( i = 0; i < c; i++ ) {
		if ( timeLineEvents[i]->time >= t ) {
			timeLineEvents[i]->pending = true;
		}
	}

	noTime = false;

	c = transitions.Num();
	for ( i = 0; i < c; i++ ) {
		hfTransitionData *data = &transitions[i];
		if ( data->interp.IsDone( gui->GetTime() ) && data->data ) {
			transitions.RemoveIndex( i );
			i--;
			c--;
		}
	}

}


/*
================
hfWindow::RunScriptList
================
*/
bool hfWindow::RunScriptList(hfGuiScriptList *src) {
	if (src == NULL) {
		return false;
	}
	src->Execute(this);
	return true;
}

/*
================
hfWindow::RunScript
================
*/
bool hfWindow::RunScript(int n) {
	if (n >= ON_MOUSEENTER && n < SCRIPT_COUNT) {
		return RunScriptList(scripts[n]);
	}
	return false;
}

/*
================
hfWindow::ExpressionConstant
================
*/
int hfWindow::ExpressionConstant(float f) {
	int		i;

	for ( i = WEXP_REG_NUM_PREDEFINED ; i < expressionRegisters.Num() ; i++ ) {
		if ( !registerIsTemporary[i] && expressionRegisters[i] == f ) {
			return i;
		}
	}
	if ( expressionRegisters.Num() == MAX_EXPRESSION_REGISTERS ) {
		common->Warning( "expressionConstant: gui %s hit MAX_EXPRESSION_REGISTERS", gui->GetSourceFile() );
		return 0;
	}

	int c = expressionRegisters.Num();
	if (i > c) {
		while (i > c) {
			expressionRegisters.Append(-9999999);
			i--;
		}
	}

	i = expressionRegisters.Append(f);
	registerIsTemporary[i] = false;
	return i;
}

/*
================
hfWindow::ExpressionTemporary
================
*/
int hfWindow::ExpressionTemporary() {
	if ( expressionRegisters.Num() == MAX_EXPRESSION_REGISTERS ) {
		common->Warning( "expressionTemporary: gui %s hit MAX_EXPRESSION_REGISTERS", gui->GetSourceFile());
		return 0;
	}
	int i = expressionRegisters.Num();
	registerIsTemporary[i] = true;
	i = expressionRegisters.Append(0);
	return i;
}

/*
================
hfWindow::ExpressionOp
================
*/
wexpOp_t *hfWindow::ExpressionOp() {
	if ( ops.Num() == MAX_EXPRESSION_OPS ) {
		common->Warning( "expressionOp: gui %s hit MAX_EXPRESSION_OPS", gui->GetSourceFile());
		return &ops[0];
	}
	wexpOp_t wop;
	memset(&wop, 0, sizeof(wexpOp_t));
	int i = ops.Append(wop);
	return &ops[i];
}

/*
================
hfWindow::EmitOp
================
*/

int hfWindow::EmitOp( int a, int b, wexpOpType_t opType, wexpOp_t **opp ) {
	wexpOp_t *op;
/*
	// optimize away identity operations
	if ( opType == WOP_TYPE_ADD ) {
		if ( !registerIsTemporary[a] && shaderRegisters[a] == 0 ) {
			return b;
		}
		if ( !registerIsTemporary[b] && shaderRegisters[b] == 0 ) {
			return a;
		}
		if ( !registerIsTemporary[a] && !registerIsTemporary[b] ) {
			return ExpressionConstant( shaderRegisters[a] + shaderRegisters[b] );
		}
	}
	if ( opType == WOP_TYPE_MULTIPLY ) {
		if ( !registerIsTemporary[a] && shaderRegisters[a] == 1 ) {
			return b;
		}
		if ( !registerIsTemporary[a] && shaderRegisters[a] == 0 ) {
			return a;
		}
		if ( !registerIsTemporary[b] && shaderRegisters[b] == 1 ) {
			return a;
		}
		if ( !registerIsTemporary[b] && shaderRegisters[b] == 0 ) {
			return b;
		}
		if ( !registerIsTemporary[a] && !registerIsTemporary[b] ) {
			return ExpressionConstant( shaderRegisters[a] * shaderRegisters[b] );
		}
	}
*/
	op = ExpressionOp();

	op->opType = opType;
	op->a = a;
	op->b = b;
	op->c = ExpressionTemporary();

	if (opp) {
		*opp = op;
	}
	return op->c;
}

/*
================
hfWindow::ParseEmitOp
================
*/
int hfWindow::ParseEmitOp( hfTokenParser *src, int a, wexpOpType_t opType, int priority, wexpOp_t **opp ) {
	int b = ParseExpressionPriority( src, priority );
	return EmitOp( a, b, opType, opp );  
}


/*
================
hfWindow::ParseTerm

Returns a register index
=================
*/
int hfWindow::ParseTerm( hfTokenParser *src,	hfWinVar *var, int component ) {
	hfToken token;
	int		a, b;

	src->ReadToken( &token );

	if ( token == "(" ) {
		a = ParseExpression( src );
		src->ExpectTokenString(")");
		return a;
	}

	if ( !token.Icmp( "time" ) ) {
		return WEXP_REG_TIME;
	}

	// parse negative numbers
	if ( token == "-" ) {
		src->ReadToken( &token );
		if ( token.type == TT_NUMBER || token == "." ) {
			return ExpressionConstant( -(float) token.GetFloatValue() );
		}
		src->Warning( "Bad negative number '%s'", token.c_str() );
		return 0;
	}

	if ( token.type == TT_NUMBER || token == "." || token == "-" ) {
		return ExpressionConstant( (float) token.GetFloatValue() );
	}

	// see if it is a table name
	const hfDeclTable *table = static_cast<const hfDeclTable *>( declManager->FindType( DECL_TABLE, token.c_str(), false ) );
	if ( table ) {
		a = table->Index();
		// parse a table expression
		src->ExpectTokenString("[");
		b = ParseExpression(src);
		src->ExpectTokenString("]");
		return EmitOp( a, b, WOP_TYPE_TABLE );
	}
	
	if (var == NULL) {
		var = GetWinVarByName(token, true);
	}
	if (var) {
		a = (int)(intptr_t)var;
		//assert(dynamic_cast<hfWinVec4*>(var));
		var->Init(token, this);
		b = component;
		if (dynamic_cast<hfWinVec4*>(var)) {
			if (src->ReadToken(&token)) {
				if (token == "[") {
					b = ParseExpression(src);
					src->ExpectTokenString("]");
				} else {
					src->UnreadToken(&token);
				}
			}
			return EmitOp(a, b, WOP_TYPE_VAR);
		} else if (dynamic_cast<hfWinFloat*>(var)) {
			return EmitOp(a, b, WOP_TYPE_VARF);
		} else if (dynamic_cast<hfWinInt*>(var)) {
			return EmitOp(a, b, WOP_TYPE_VARI);
		} else if (dynamic_cast<hfWinBool*>(var)) {
			return EmitOp(a, b, WOP_TYPE_VARB);
		} else if (dynamic_cast<hfWinStr*>(var)) {
			return EmitOp(a, b, WOP_TYPE_VARS);
		} else {
			src->Warning("Var expression not vec4, float or int '%s'", token.c_str());
		}
		return 0;
	} else {
		// ugly but used for post parsing to fixup named vars
		char *p = new (TAG_OLD_UI) char[token.Length()+1];
		strcpy(p, token);
		a = (int)(intptr_t)p;
		b = -2;
		return EmitOp(a, b, WOP_TYPE_VAR);
	}

}

/*
=================
hfWindow::ParseExpressionPriority

Returns a register index
=================
*/
#define	TOP_PRIORITY 4
int hfWindow::ParseExpressionPriority( hfTokenParser *src, int priority, hfWinVar *var, int component ) {
	hfToken token;
	int		a;

	if ( priority == 0 ) {
		return ParseTerm( src, var, component );
	}

	a = ParseExpressionPriority( src, priority - 1, var, component );

	if ( !src->ReadToken( &token ) ) {
		// we won't get EOF in a real file, but we can
		// when parsing from generated strings
		return a;
	}

	if ( priority == 1 && token == "*" ) {
		return ParseEmitOp( src, a, WOP_TYPE_MULTIPLY, priority );
	}
	if ( priority == 1 && token == "/" ) {
		return ParseEmitOp( src, a, WOP_TYPE_DIVIDE, priority );
	}
	if ( priority == 1 && token == "%" ) {	// implied truncate both to integer
		return ParseEmitOp( src, a, WOP_TYPE_MOD, priority );
	}
	if ( priority == 2 && token == "+" ) {
		return ParseEmitOp( src, a, WOP_TYPE_ADD, priority );
	}
	if ( priority == 2 && token == "-" ) {
		return ParseEmitOp( src, a, WOP_TYPE_SUBTRACT, priority );
	}
	if ( priority == 3 && token == ">" ) {
		return ParseEmitOp( src, a, WOP_TYPE_GT, priority );
	}
	if ( priority == 3 && token == ">=" ) {
		return ParseEmitOp( src, a, WOP_TYPE_GE, priority );
	}
	if ( priority == 3 && token == "<" ) {
		return ParseEmitOp( src, a, WOP_TYPE_LT, priority );
	}
	if ( priority == 3 && token == "<=" ) {
		return ParseEmitOp( src, a, WOP_TYPE_LE, priority );
	}
	if ( priority == 3 && token == "==" ) {
		return ParseEmitOp( src, a, WOP_TYPE_EQ, priority );
	}
	if ( priority == 3 && token == "!=" ) {
		return ParseEmitOp( src, a, WOP_TYPE_NE, priority );
	}
	if ( priority == 4 && token == "&&" ) {
		return ParseEmitOp( src, a, WOP_TYPE_AND, priority );
	}
	if ( priority == 4 && token == "||" ) {
		return ParseEmitOp( src, a, WOP_TYPE_OR, priority );
	}
	if ( priority == 4 && token == "?" ) {
		wexpOp_t *oop = NULL;
		int o = ParseEmitOp( src, a, WOP_TYPE_COND, priority, &oop );
		if ( !src->ReadToken( &token ) ) {
			return o;
		}
		if (token == ":") {
			a = ParseExpressionPriority( src, priority - 1, var );
			oop->d = a;
		}
		return o;
	}

	// assume that anything else terminates the expression
	// not too robust error checking...

	src->UnreadToken( &token );

	return a;
}

/*
================
hfWindow::ParseExpression

Returns a register index
================
*/
int hfWindow::ParseExpression(hfTokenParser *src, hfWinVar *var, int component) {
	return ParseExpressionPriority( src, TOP_PRIORITY, var );
}

/*
================
hfWindow::ParseBracedExpression
================
*/
void hfWindow::ParseBracedExpression(hfTokenParser *src) {
	src->ExpectTokenString("{");
	ParseExpression(src);
	src->ExpectTokenString("}");
}

/*
===============
hfWindow::EvaluateRegisters

Parameters are taken from the localSpace and the renderView,
then all expressions are evaluated, leaving the shader registers
set to their apropriate values.
===============
*/
void hfWindow::EvaluateRegisters(float *registers) {
	int		i, b;
	wexpOp_t	*op;
	hfVec4 v;

	int erc = expressionRegisters.Num();
	int oc = ops.Num();
	// copy the constants
	for ( i = WEXP_REG_NUM_PREDEFINED ; i < erc ; i++ ) {
		registers[i] = expressionRegisters[i];
	}

	// copy the local and global parameters
	registers[WEXP_REG_TIME] = gui->GetTime();

	for ( i = 0 ; i < oc ; i++ ) {
		op = &ops[i];
		if (op->b == -2) {
			continue;
		}
		switch( op->opType ) {
		case WOP_TYPE_ADD:
			registers[op->c] = registers[op->a] + registers[op->b];
			break;
		case WOP_TYPE_SUBTRACT:
			registers[op->c] = registers[op->a] - registers[op->b];
			break;
		case WOP_TYPE_MULTIPLY:
			registers[op->c] = registers[op->a] * registers[op->b];
			break;
		case WOP_TYPE_DIVIDE:
			if ( registers[op->b] == 0.0f ) {
				common->Warning( "Divide by zero in window '%s' in %s", GetName(), gui->GetSourceFile() );
				registers[op->c] = registers[op->a];
			} else {
				registers[op->c] = registers[op->a] / registers[op->b];
			}
			break;
		case WOP_TYPE_MOD:
			b = (int)registers[op->b];
			b = b != 0 ? b : 1;
			registers[op->c] = (int)registers[op->a] % b;
			break;
		case WOP_TYPE_TABLE:
			{
				const hfDeclTable *table = static_cast<const hfDeclTable *>( declManager->DeclByIndex( DECL_TABLE, op->a ) );
				registers[op->c] = table->TableLookup( registers[op->b] );
			}
			break;
		case WOP_TYPE_GT:
			registers[op->c] = registers[ op->a ] > registers[op->b];
			break;
		case WOP_TYPE_GE:
			registers[op->c] = registers[ op->a ] >= registers[op->b];
			break;
		case WOP_TYPE_LT:
			registers[op->c] = registers[ op->a ] < registers[op->b];
			break;
		case WOP_TYPE_LE:
			registers[op->c] = registers[ op->a ] <= registers[op->b];
			break;
		case WOP_TYPE_EQ:
			registers[op->c] = registers[ op->a ] == registers[op->b];
			break;
		case WOP_TYPE_NE:
			registers[op->c] = registers[ op->a ] != registers[op->b];
			break;
		case WOP_TYPE_COND:
			registers[op->c] = (registers[ op->a ]) ? registers[op->b] : registers[op->d];
			break;
		case WOP_TYPE_AND:
			registers[op->c] = registers[ op->a ] && registers[op->b];
			break;
		case WOP_TYPE_OR:
			registers[op->c] = registers[ op->a ] || registers[op->b];
			break;
		case WOP_TYPE_VAR:
			if ( !op->a ) {
				registers[op->c] = 0.0f;
				break;
			}
			if ( op->b >= 0 && registers[op->b] >= 0 && registers[op->b] < 4 ) {
				// grabs vector components
				hfWinVec4 *var = (hfWinVec4 *)( op->a );
				registers[op->c] = ((hfVec4&)var)[registers[op->b]];
			} else {
				registers[op->c] = ((hfWinVar*)(op->a))->x();
			}
			break;
		case WOP_TYPE_VARS:
			if (op->a) {
				hfWinStr *var = (hfWinStr*)(op->a);
				registers[op->c] = atof(var->c_str());
			} else {
				registers[op->c] = 0;
			}
			break;
		case WOP_TYPE_VARF:
			if (op->a) {
				hfWinFloat *var = (hfWinFloat*)(op->a);
				registers[op->c] = *var;
			} else {
				registers[op->c] = 0;
			}
			break;
		case WOP_TYPE_VARI:
			if (op->a) {
				hfWinInt *var = (hfWinInt*)(op->a);
				registers[op->c] = *var;
			} else {
				registers[op->c] = 0;
			}
			break;
		case WOP_TYPE_VARB:
			if (op->a) {
				hfWinBool *var = (hfWinBool*)(op->a);
				registers[op->c] = *var;
			} else {
				registers[op->c] = 0;
			}
			break;
		default:
			common->FatalError( "R_EvaluateExpression: bad opcode" );
		}
	}

}

/*
================
hfWindow::ReadFromDemoFile
================
*/
void hfWindow::ReadFromDemoFile( class hfDemoFile *f, bool rebuild ) {

	// should never hit unless we re-enable WRITE_GUIS
#ifndef WRITE_GUIS
	assert( false );
#else

	if (rebuild) {
		CommonInit();
	}
	
	f->SetLog(true, "window1");
	backGroundName = f->ReadHashString();
	f->SetLog(true, backGroundName);
	if ( backGroundName[0] ) {
		background = declManager->FindMaterial(backGroundName);
	} else {
		background = NULL;
	}
	f->ReadUnsignedChar( cursor );
	f->ReadUnsignedInt( flags );
	f->ReadInt( timeLine );
	f->ReadInt( lastTimeRun );
	hfRectangle rct = rect;
	f->ReadFloat( rct.x );
	f->ReadFloat( rct.y );
	f->ReadFloat( rct.w );
	f->ReadFloat( rct.h );
	f->ReadFloat( drawRect.x );
	f->ReadFloat( drawRect.y );
	f->ReadFloat( drawRect.w );
	f->ReadFloat( drawRect.h );
	f->ReadFloat( clientRect.x );
	f->ReadFloat( clientRect.y );
	f->ReadFloat( clientRect.w );
	f->ReadFloat( clientRect.h );
	f->ReadFloat( textRect.x );
	f->ReadFloat( textRect.y );
	f->ReadFloat( textRect.w );
	f->ReadFloat( textRect.h );
	f->ReadFloat( xOffset);
	f->ReadFloat( yOffset);
	int i, c;

	hfStr work;
	if (rebuild) {
		f->SetLog(true, (work + "-scripts"));
		for (i = 0; i < SCRIPT_COUNT; i++) {
			bool b;
			f->ReadBool( b );
			if (b) {
				delete scripts[i];
				scripts[i] = new (TAG_OLD_UI) hfGuiScriptList;
				scripts[i]->ReadFromDemoFile(f);
			}
		}

		f->SetLog(true, (work + "-timelines"));
		f->ReadInt( c );
		for (i = 0; i < c; i++) {
			hfTimeLineEvent *tl = new (TAG_OLD_UI) hfTimeLineEvent;
			f->ReadInt( tl->time );
			f->ReadBool( tl->pending );
			tl->event->ReadFromDemoFile(f);
			if (rebuild) {
				timeLineEvents.Append(tl);
			} else {
				assert(i < timeLineEvents.Num());
				timeLineEvents[i]->time = tl->time;
				timeLineEvents[i]->pending = tl->pending;
			}
		}
	}

	f->SetLog(true, (work + "-transitions"));
	f->ReadInt( c );
	for (i = 0; i < c; i++) {
		hfTransitionData td;
		td.data = NULL;
		f->ReadInt ( td.offset );

		float startTime, accelTime, linearTime, decelTime;
		hfVec4 startValue, endValue;	   
		f->ReadFloat( startTime );
		f->ReadFloat( accelTime );
		f->ReadFloat( linearTime );
		f->ReadFloat( decelTime );
		f->ReadVec4( startValue );
		f->ReadVec4( endValue );
		td.interp.Init( startTime, accelTime, decelTime, accelTime + linearTime + decelTime, startValue, endValue );
		
		// read this for correct data padding with the win32 savegames
		// the extrapolate is correctly initialized through the above Init call
		int extrapolationType;
		float duration;
		hfVec4 baseSpeed, speed;
		float currentTime;
		hfVec4 currentValue;
		f->ReadInt( extrapolationType );
		f->ReadFloat( startTime );
		f->ReadFloat( duration );
		f->ReadVec4( startValue );
		f->ReadVec4( baseSpeed );
		f->ReadVec4( speed );
		f->ReadFloat( currentTime );
		f->ReadVec4( currentValue );

		transitions.Append(td);
	}

	f->SetLog(true, (work + "-regstuff"));
	if (rebuild) {
		f->ReadInt( c );
		for (i = 0; i < c; i++) {
			wexpOp_t w;
			f->ReadInt( (int&)w.opType );
			f->ReadInt( w.a );
			f->ReadInt( w.b );
			f->ReadInt( w.c );
			f->ReadInt( w.d );
			ops.Append(w);
		}

		f->ReadInt( c );
		for (i = 0; i < c; i++) {
			float ff;
			f->ReadFloat( ff );
			expressionRegisters.Append(ff);
		}
	
		regList.ReadFromDemoFile(f);

	}
	f->SetLog(true, (work + "-children"));
	f->ReadInt( c );
	for (i = 0; i < c; i++) {
		if (rebuild) {
			hfWindow *win = new (TAG_OLD_UI) hfWindow( gui );
			win->ReadFromDemoFile(f);
			AddChild(win);
		} else {
			for (int j = 0; j < c; j++) {
				if (children[j]->childID == i) {
					children[j]->ReadFromDemoFile(f,rebuild);
					break;
				} else {
					continue;
				}
			}
		}
	}
#endif /* WRITE_GUIS */
}

/*
================
hfWindow::WriteToDemoFile
================
*/
void hfWindow::WriteToDemoFile( class hfDemoFile *f ) {
	// should never hit unless we re-enable WRITE_GUIS
#ifndef WRITE_GUIS
	assert( false );
#else

	f->SetLog(true, "window");
	f->WriteHashString(backGroundName);
	f->SetLog(true, backGroundName);
	f->WriteUnsignedChar( cursor );
	f->WriteUnsignedInt( flags );
	f->WriteInt( timeLine );
	f->WriteInt( lastTimeRun );
	hfRectangle rct = rect;
	f->WriteFloat( rct.x );
	f->WriteFloat( rct.y );
	f->WriteFloat( rct.w );
	f->WriteFloat( rct.h );
	f->WriteFloat( drawRect.x );
	f->WriteFloat( drawRect.y );
	f->WriteFloat( drawRect.w );
	f->WriteFloat( drawRect.h );
	f->WriteFloat( clientRect.x );
	f->WriteFloat( clientRect.y );
	f->WriteFloat( clientRect.w );
	f->WriteFloat( clientRect.h );
	f->WriteFloat( textRect.x );
	f->WriteFloat( textRect.y );
	f->WriteFloat( textRect.w );
	f->WriteFloat( textRect.h );
	f->WriteFloat( xOffset );
	f->WriteFloat( yOffset );
	hfStr work;
	f->SetLog(true, work);

 	int i, c;

	f->SetLog(true, (work + "-transitions"));
	c = transitions.Num();
	f->WriteInt( c );
	for (i = 0; i < c; i++) {
		f->WriteInt( 0 );
		f->WriteInt( transitions[i].offset );
		
		f->WriteFloat( transitions[i].interp.GetStartTime() );
		f->WriteFloat( transitions[i].interp.GetAccelTime() );
		f->WriteFloat( transitions[i].interp.GetLinearTime() );
		f->WriteFloat( transitions[i].interp.GetDecelTime() );
		f->WriteVec4( transitions[i].interp.GetStartValue() );
		f->WriteVec4( transitions[i].interp.GetEndValue() );

		// write to keep win32 render demo format compatiblity - we don't actually read them back anymore
		f->WriteInt( transitions[i].interp.GetExtrapolate()->GetExtrapolationType() );
		f->WriteFloat( transitions[i].interp.GetExtrapolate()->GetStartTime() );
		f->WriteFloat( transitions[i].interp.GetExtrapolate()->GetDuration() );
		f->WriteVec4( transitions[i].interp.GetExtrapolate()->GetStartValue() );
		f->WriteVec4( transitions[i].interp.GetExtrapolate()->GetBaseSpeed() );
		f->WriteVec4( transitions[i].interp.GetExtrapolate()->GetSpeed() );
		f->WriteFloat( transitions[i].interp.GetExtrapolate()->GetCurrentTime() );
		f->WriteVec4( transitions[i].interp.GetExtrapolate()->GetCurrentValue() );
	}

	f->SetLog(true, (work + "-regstuff"));

	f->SetLog(true, (work + "-children"));
	c = children.Num();
	f->WriteInt( c );
	for (i = 0; i < c; i++) {
		for (int j = 0; j < c; j++) {
			if (children[j]->childID == i) {
				children[j]->WriteToDemoFile(f);
				break;
			} else {
				continue;
			}
		}
	}
#endif /* WRITE_GUIS */
}

/*
===============
hfWindow::WriteString
===============
*/
void hfWindow::WriteSaveGameString( const char *string, hfFile *savefile ) {
	int len = strlen( string );

	savefile->Write( &len, sizeof( len ) );
	savefile->Write( string, len );
}

/*
===============
hfWindow::WriteSaveGameTransition
===============
*/
void hfWindow::WriteSaveGameTransition( hfTransitionData &trans, hfFile *savefile ) {
	drawWin_t dw, *fdw;
	hfStr winName("");
	dw.simp = NULL;
	dw.win = NULL;
	int offset = gui->GetDesktop()->GetWinVarOffset( trans.data, &dw );
	if ( dw.win || dw.simp ) {
		winName = ( dw.win ) ? dw.win->GetName() : dw.simp->name.c_str();
	}
	fdw = gui->GetDesktop()->FindChildByName( winName );
	if ( offset != -1 && fdw != NULL && ( fdw->win != NULL || fdw->simp != NULL ) ) {
		savefile->Write( &offset, sizeof( offset ) );
		WriteSaveGameString( winName, savefile );
		savefile->Write( &trans.interp, sizeof( trans.interp ) );
	} else {
		offset = -1;
		savefile->Write( &offset, sizeof( offset ) );
	}
}

/*
===============
hfWindow::ReadSaveGameTransition
===============
*/
void hfWindow::ReadSaveGameTransition( hfTransitionData &trans, hfFile *savefile ) {
	int offset;

	savefile->Read( &offset, sizeof( offset ) );
	if ( offset != -1 ) {
		hfStr winName;
		ReadSaveGameString( winName, savefile );
		savefile->Read( &trans.interp, sizeof( trans.interp ) );
		trans.data = NULL;
		trans.offset = offset;
		if ( winName.Length() ) {
			hfWinStr *strVar = new (TAG_OLD_UI) hfWinStr();
			strVar->Set( winName );
			trans.data = dynamic_cast< hfWinVar* >( strVar );
		}
	}
}

/*
===============
hfWindow::WriteToSaveGame
===============
*/
void hfWindow::WriteToSaveGame( hfFile *savefile ) {
	int i;

	WriteSaveGameString( cmd, savefile );

	savefile->Write( &actualX, sizeof( actualX ) );
	savefile->Write( &actualY, sizeof( actualY ) );
	savefile->Write( &childID, sizeof( childID ) );
	savefile->Write( &flags, sizeof( flags ) );
	savefile->Write( &lastTimeRun, sizeof( lastTimeRun ) );
	savefile->Write( &drawRect, sizeof( drawRect ) );
	savefile->Write( &clientRect, sizeof( clientRect ) );
	savefile->Write( &origin, sizeof( origin ) );
	savefile->Write( &timeLine, sizeof( timeLine ) );
	savefile->Write( &xOffset, sizeof( xOffset ) );
	savefile->Write( &yOffset, sizeof( yOffset ) );
	savefile->Write( &cursor, sizeof( cursor ) );
	savefile->Write( &forceAspectWidth, sizeof( forceAspectWidth ) );
	savefile->Write( &forceAspectHeight, sizeof( forceAspectHeight ) );
	savefile->Write( &matScalex, sizeof( matScalex ) );
	savefile->Write( &matScaley, sizeof( matScaley ) );
	savefile->Write( &borderSize, sizeof( borderSize ) );
	savefile->Write( &textAlign, sizeof( textAlign ) );
	savefile->Write( &textAlignx, sizeof( textAlignx ) );
	savefile->Write( &textAligny, sizeof( textAligny ) );
	savefile->Write( &textShadow, sizeof( textShadow ) );
	savefile->Write( &shear, sizeof( shear ) );

	savefile->WriteString( font->GetName() );

	WriteSaveGameString( name, savefile );
	WriteSaveGameString( comment, savefile );

	// WinVars
	noTime.WriteToSaveGame( savefile );
	visible.WriteToSaveGame( savefile );
	rect.WriteToSaveGame( savefile );
	backColor.WriteToSaveGame( savefile );
	matColor.WriteToSaveGame( savefile );
	foreColor.WriteToSaveGame( savefile );
	hoverColor.WriteToSaveGame( savefile );
	borderColor.WriteToSaveGame( savefile );
	textScale.WriteToSaveGame( savefile );
	noEvents.WriteToSaveGame( savefile );
	rotate.WriteToSaveGame( savefile );
	text.WriteToSaveGame( savefile );
	backGroundName.WriteToSaveGame( savefile );
	hideCursor.WriteToSaveGame(savefile);

	// Defined Vars
	for ( i = 0; i < definedVars.Num(); i++ ) {
		definedVars[i]->WriteToSaveGame( savefile );
	}

	savefile->Write( &textRect, sizeof( textRect ) );

	// Window pointers saved as the child ID of the window
	int winID;

	winID = focusedChild ? focusedChild->childID : -1 ;
	savefile->Write( &winID, sizeof( winID ) );

	winID = captureChild ? captureChild->childID : -1 ;
	savefile->Write( &winID, sizeof( winID ) );

	winID = overChild ? overChild->childID : -1 ;
	savefile->Write( &winID, sizeof( winID ) );


	// Scripts
	for ( i = 0; i < SCRIPT_COUNT; i++ ) {
		if ( scripts[i] ) {
			scripts[i]->WriteToSaveGame( savefile );
		}
	}

	// TimeLine Events
	for ( i = 0; i < timeLineEvents.Num(); i++ ) {
		if ( timeLineEvents[i] ) {
			savefile->Write( &timeLineEvents[i]->pending, sizeof( timeLineEvents[i]->pending ) );
			savefile->Write( &timeLineEvents[i]->time, sizeof( timeLineEvents[i]->time ) );
			if ( timeLineEvents[i]->event ) {
				timeLineEvents[i]->event->WriteToSaveGame( savefile );
			}
		}
	}

	// Transitions
	int num = transitions.Num();

	savefile->Write( &num, sizeof( num ) );
	for ( i = 0; i < transitions.Num(); i++ ) {
		WriteSaveGameTransition( transitions[ i ], savefile );
	}


	// Named Events
	for ( i = 0; i < namedEvents.Num(); i++ ) {
		if ( namedEvents[i] ) {
			WriteSaveGameString( namedEvents[i]->mName, savefile );
			if ( namedEvents[i]->mEvent ) {
				namedEvents[i]->mEvent->WriteToSaveGame( savefile );
			}
		}
	}

	// regList
	regList.WriteToSaveGame( savefile );

	if ( background ) {
		savefile->WriteInt( background->GetCinematicStartTime() );
	} else {
		savefile->WriteInt( -1 );
	}

	// Save children
	for ( i = 0; i < drawWindows.Num(); i++ ) {
		drawWin_t	window = drawWindows[i];

		if ( window.simp ) {
			window.simp->WriteToSaveGame( savefile );
		} else if ( window.win ) {
			window.win->WriteToSaveGame( savefile );
		}
	}
}

/*
===============
hfWindow::ReadSaveGameString
===============
*/
void hfWindow::ReadSaveGameString( hfStr &string, hfFile *savefile ) {
	int len;

	savefile->Read( &len, sizeof( len ) );
	if ( len < 0 ) {
		common->Warning( "hfWindow::ReadSaveGameString: invalid length" );
	}

	string.Fill( ' ', len );
	savefile->Read( &string[0], len );
}

/*
===============
hfWindow::ReadFromSaveGame
===============
*/
void hfWindow::ReadFromSaveGame( hfFile *savefile ) {
	int i;

	transitions.Clear();

	ReadSaveGameString( cmd, savefile );

	savefile->Read( &actualX, sizeof( actualX ) );
	savefile->Read( &actualY, sizeof( actualY ) );
	savefile->Read( &childID, sizeof( childID ) );
	savefile->Read( &flags, sizeof( flags ) );
	savefile->Read( &lastTimeRun, sizeof( lastTimeRun ) );
	savefile->Read( &drawRect, sizeof( drawRect ) );
	savefile->Read( &clientRect, sizeof( clientRect ) );
	savefile->Read( &origin, sizeof( origin ) );
/*	if ( savefile->GetFileVersion() < BUILD_NUMBER_8TH_ANNIVERSARY_1 ) {
		unsigned char fontNum;
		savefile->Read( &fontNum, sizeof( fontNum ) );
		font = renderSystem->RegisterFont( "" );
	}*/
	savefile->Read( &timeLine, sizeof( timeLine ) );
	savefile->Read( &xOffset, sizeof( xOffset ) );
	savefile->Read( &yOffset, sizeof( yOffset ) );
	savefile->Read( &cursor, sizeof( cursor ) );
	savefile->Read( &forceAspectWidth, sizeof( forceAspectWidth ) );
	savefile->Read( &forceAspectHeight, sizeof( forceAspectHeight ) );
	savefile->Read( &matScalex, sizeof( matScalex ) );
	savefile->Read( &matScaley, sizeof( matScaley ) );
	savefile->Read( &borderSize, sizeof( borderSize ) );
	savefile->Read( &textAlign, sizeof( textAlign ) );
	savefile->Read( &textAlignx, sizeof( textAlignx ) );
	savefile->Read( &textAligny, sizeof( textAligny ) );
	savefile->Read( &textShadow, sizeof( textShadow ) );
	savefile->Read( &shear, sizeof( shear ) );

//	if ( savefile->GetFileVersion() >= BUILD_NUMBER_8TH_ANNIVERSARY_1 ) {
		hfStr fontName;
		savefile->ReadString( fontName );
		font = renderSystem->RegisterFont( fontName );
//	} 

	ReadSaveGameString( name, savefile );
	ReadSaveGameString( comment, savefile );

	// WinVars
	noTime.ReadFromSaveGame( savefile );
	visible.ReadFromSaveGame( savefile );
	rect.ReadFromSaveGame( savefile );
	backColor.ReadFromSaveGame( savefile );
	matColor.ReadFromSaveGame( savefile );
	foreColor.ReadFromSaveGame( savefile );
	hoverColor.ReadFromSaveGame( savefile );
	borderColor.ReadFromSaveGame( savefile );
	textScale.ReadFromSaveGame( savefile );
	noEvents.ReadFromSaveGame( savefile );
	rotate.ReadFromSaveGame( savefile );
	text.ReadFromSaveGame( savefile );
	backGroundName.ReadFromSaveGame( savefile );
	hideCursor.ReadFromSaveGame(savefile);

	// Defined Vars
	for ( i = 0; i < definedVars.Num(); i++ ) {
		definedVars[i]->ReadFromSaveGame( savefile );
	}

	savefile->Read( &textRect, sizeof( textRect ) );

	// Window pointers saved as the child ID of the window
	int winID = -1;

	savefile->Read( &winID, sizeof( winID ) );
	for ( i = 0; i < children.Num(); i++ ) {
		if ( children[i]->childID == winID ) {
			focusedChild = children[i];
		}
	}
	savefile->Read( &winID, sizeof( winID ) );
	for ( i = 0; i < children.Num(); i++ ) {
		if ( children[i]->childID == winID ) {
			captureChild = children[i];
		}
	}
	savefile->Read( &winID, sizeof( winID ) );
	for ( i = 0; i < children.Num(); i++ ) {
		if ( children[i]->childID == winID ) {
			overChild = children[i];
		}
	}
	
	// Scripts
	for ( i = 0; i < SCRIPT_COUNT; i++ ) {
		if ( scripts[i] ) {
			scripts[i]->ReadFromSaveGame( savefile );
		}
	}

	// TimeLine Events
	for ( i = 0; i < timeLineEvents.Num(); i++ ) {
		if ( timeLineEvents[i] ) {
			savefile->Read( &timeLineEvents[i]->pending, sizeof( timeLineEvents[i]->pending ) );
			savefile->Read( &timeLineEvents[i]->time, sizeof( timeLineEvents[i]->time ) );
			if ( timeLineEvents[i]->event ) {
				timeLineEvents[i]->event->ReadFromSaveGame( savefile );
			}
		}
	}


	// Transitions
	int num;
	savefile->Read( &num, sizeof( num ) );
	for ( i = 0; i < num; i++ ) {
		hfTransitionData trans;
		trans.data = NULL;
		ReadSaveGameTransition( trans, savefile );
		if ( trans.data ) {
			transitions.Append( trans );
		}
	}


	// Named Events
	for ( i = 0; i < namedEvents.Num(); i++ ) {
		if ( namedEvents[i] ) {
			ReadSaveGameString( namedEvents[i]->mName, savefile );
			if ( namedEvents[i]->mEvent ) {
				namedEvents[i]->mEvent->ReadFromSaveGame( savefile );
			}
		}
	}

	// regList
	regList.ReadFromSaveGame( savefile );

	int cinematicStartTime = 0;
	savefile->ReadInt( cinematicStartTime );
	if ( background ) {
		background->ResetCinematicTime( cinematicStartTime );
	}

	// Read children
	for ( i = 0; i < drawWindows.Num(); i++ ) {
		drawWin_t	window = drawWindows[i];

		if ( window.simp ) {
			window.simp->ReadFromSaveGame( savefile );
		} else if ( window.win ) {
			window.win->ReadFromSaveGame( savefile );
		}
	}

	if ( flags & WIN_DESKTOP ) {
		FixupTransitions();
	}
}

/*
===============
hfWindow::NumTransitions
===============
*/
int hfWindow::NumTransitions() {
	int c = transitions.Num();
	for ( int i = 0; i < children.Num(); i++ ) {
		c += children[i]->NumTransitions();
	}
	return c;
}


/*
===============
hfWindow::FixupTransitions
===============
*/
void hfWindow::FixupTransitions() {
	int i, c = transitions.Num();
	for ( i = 0; i < c; i++ ) {
		drawWin_t *dw = gui->GetDesktop()->FindChildByName( ( ( hfWinStr* )transitions[i].data )->c_str() );
		delete transitions[i].data;
		transitions[i].data = NULL;
		if ( dw != NULL && ( dw->win != NULL || dw->simp != NULL ) ){
			if ( dw->win != NULL ) {
				if ( transitions[i].offset == (int)(intptr_t)&( ( hfWindow * ) 0 )->rect ) {
					transitions[i].data = &dw->win->rect;
				} else if ( transitions[i].offset == (int)(intptr_t)&( ( hfWindow * ) 0 )->backColor ) {
					transitions[i].data = &dw->win->backColor;
				} else if ( transitions[i].offset == (int)(intptr_t)&( ( hfWindow * ) 0 )->matColor ) {
					transitions[i].data = &dw->win->matColor;
				} else if ( transitions[i].offset == (int)(intptr_t)&( ( hfWindow * ) 0 )->foreColor ) {
					transitions[i].data = &dw->win->foreColor;
				} else if ( transitions[i].offset == (int)(intptr_t)&( ( hfWindow * ) 0 )->borderColor ) {
					transitions[i].data = &dw->win->borderColor;
				} else if ( transitions[i].offset == (int)(intptr_t)&( ( hfWindow * ) 0 )->textScale ) {
					transitions[i].data = &dw->win->textScale;
				} else if ( transitions[i].offset == (int)(intptr_t)&( ( hfWindow * ) 0 )->rotate ) {
					transitions[i].data = &dw->win->rotate;
				}
			} else {
				if ( transitions[i].offset == (int)(intptr_t)&( ( hfSimpleWindow * ) 0 )->rect ) {
					transitions[i].data = &dw->simp->rect;
				} else if ( transitions[i].offset == (int)(intptr_t)&( ( hfSimpleWindow * ) 0 )->backColor ) {
					transitions[i].data = &dw->simp->backColor;
				} else if ( transitions[i].offset == (int)(intptr_t)&( ( hfSimpleWindow * ) 0 )->matColor ) {
					transitions[i].data = &dw->simp->matColor;
				} else if ( transitions[i].offset == (int)(intptr_t)&( ( hfSimpleWindow * ) 0 )->foreColor ) {
					transitions[i].data = &dw->simp->foreColor;
				} else if ( transitions[i].offset == (int)(intptr_t)&( ( hfSimpleWindow * ) 0 )->borderColor ) {
					transitions[i].data = &dw->simp->borderColor;
				} else if ( transitions[i].offset == (int)(intptr_t)&( ( hfSimpleWindow * ) 0 )->textScale ) {
					transitions[i].data = &dw->simp->textScale;
				} else if ( transitions[i].offset == (int)(intptr_t)&( ( hfSimpleWindow * ) 0 )->rotate ) {
					transitions[i].data = &dw->simp->rotate;
				}
			}
		}
		if ( transitions[i].data == NULL ) {
			transitions.RemoveIndex( i );
			i--;
			c--;
		}
	}
	for ( c = 0; c < children.Num(); c++ ) {
		children[c]->FixupTransitions();
	}
}


/*
===============
hfWindow::AddChild
===============
*/
void hfWindow::AddChild(hfWindow *win) {
	win->childID = children.Append(win);
}

/*
================
hfWindow::FixupParms
================
*/
void hfWindow::FixupParms() {
	int i;
	int c = children.Num();
	for (i = 0; i < c; i++) {
		children[i]->FixupParms();
	}
	for (i = 0; i < SCRIPT_COUNT; i++) {
		if (scripts[i]) {
			scripts[i]->FixupParms(this);
		}
	}

	c = timeLineEvents.Num();
	for (i = 0; i < c; i++) {
		timeLineEvents[i]->event->FixupParms(this);
	}

	c = namedEvents.Num();
	for (i = 0; i < c; i++) {
		namedEvents[i]->mEvent->FixupParms(this);
	}

	c = ops.Num();
	for (i = 0; i < c; i++) {
		if (ops[i].b == -2) {
			// need to fix this up
			const char *p = (const char*)(ops[i].a);
			hfWinVar *var = GetWinVarByName(p, true);
			delete []p;
			ops[i].a = (int)(intptr_t)var;
			ops[i].b = -1;
		}
	}
	
	
	if (flags & WIN_DESKTOP) {
		CalcRects(0,0);
	}

}

/*
================
hfWindow::IsSimple
================
*/
bool hfWindow::IsSimple() {

	if (ops.Num()) {
		return false;
	}
	if (flags & (WIN_HCENTER | WIN_VCENTER)) {
		return false;
	}
	if (children.Num() || drawWindows.Num()) {
		return false;
	}
	for (int i = 0; i < SCRIPT_COUNT; i++) {
		if (scripts[i]) {
			return false;
		}
	}
	if (timeLineEvents.Num()) {
		return false;
	}

	if ( namedEvents.Num() ) {
		return false;
	}

	return true;
}

/*
================
hfWindow::ContainsStateVars
================
*/
bool hfWindow::ContainsStateVars() {
	if ( updateVars.Num() ) {
		return true;
	}
	int c = children.Num();
	for (int i = 0; i < c; i++) {
		if ( children[i]->ContainsStateVars() ) {
			return true;
		}
	}
	return false;
}

/*
================
hfWindow::Interactive
================
*/
bool hfWindow::Interactive() {
	if ( scripts[ ON_ACTION ] ) {
		return true;
	}
	int c = children.Num();
	for (int i = 0; i < c; i++) {
		if (children[i]->Interactive()) {
			return true;
		}
	}
	return false;
}

/*
================
hfWindow::SetChildWinVarVal
================
*/
void hfWindow::SetChildWinVarVal(const char *name, const char *var, const char *val) {
	drawWin_t *dw = FindChildByName(name);
	hfWinVar *wv = NULL;
	if (dw != NULL && dw->simp != NULL) {
		wv = dw->simp->GetWinVarByName(var);
	} else if (dw != NULL && dw->win != NULL) {
		wv = dw->win->GetWinVarByName(var);
	}
	if (wv) {
		wv->Set(val);
		wv->SetEval(false);
	}
}


/*
================
hfWindow::FindChildByPoint

Finds the window under the given point
================
*/
hfWindow* hfWindow::FindChildByPoint ( float x, float y, hfWindow** below ) {
	int c = children.Num();

	// If we are looking for a window below this one then
	// the next window should be good, but this one wasnt it
	if ( *below == this ) {
		*below = NULL;
		return NULL;
	}

	if ( !Contains ( drawRect, x, y ) ) {
		return NULL;
	}
		
	for (int i = c - 1; i >= 0 ; i-- ) {
		hfWindow* found = children[i]->FindChildByPoint ( x, y, below );
		if ( found ) {
			if ( *below ) {
				continue;
			}
			
			return found;
		}									
	}

	return this;
}

/*
================
hfWindow::FindChildByPoint
================
*/
hfWindow* hfWindow::FindChildByPoint ( float x, float y, hfWindow* below )
{
	return FindChildByPoint ( x, y, &below );
}

/*
================
hfWindow::GetChildCount

Returns the number of children
================
*/
int hfWindow::GetChildCount ()
{
	return drawWindows.Num ( );
}

/*
================
hfWindow::GetChild

Returns the child window at the given index
================
*/
hfWindow* hfWindow::GetChild ( int index )
{
	return drawWindows[index].win;
}

/*
================
hfWindow::GetChildIndex

Returns the index of the given child window
================
*/
int hfWindow::GetChildIndex ( hfWindow* window ) {
	int find;
	for ( find = 0; find < drawWindows.Num(); find ++ ) {
		if ( drawWindows[find].win == window ) {
			return find;
		}
	}
	return -1;
}

/*
================
hfWindow::RemoveChild

Removes the child from the list of children.   Note that the child window being
removed must still be deallocated by the caller
================
*/
void hfWindow::RemoveChild ( hfWindow *win ) {
	int find;

	// Remove the child window
	children.Remove ( win );
	
	for ( find = 0; find < drawWindows.Num(); find ++ )
	{
		if ( drawWindows[find].win == win )
		{
			drawWindows.RemoveIndex ( find );
			break;
		}
	}
}

/*
================
hfWindow::InsertChild

Inserts the given window as a child into the given location in the zorder.
================
*/
bool hfWindow::InsertChild ( hfWindow *win, hfWindow* before )
{
	AddChild ( win );
		
	win->parent = this;

	drawWin_t dwt;
	dwt.simp = NULL;
	dwt.win = win;

	// If not inserting before anything then just add it at the end
	if ( before ) {		
		int index;
		index = GetChildIndex ( before );
		if ( index != -1 ) {
			drawWindows.Insert ( dwt, index );
			return true;
		}
	}
	
	drawWindows.Append ( dwt );
	return true;
}

/*
================
hfWindow::ScreenToClient
================
*/
void hfWindow::ScreenToClient ( hfRectangle* r ) {
	int		  x;
	int		  y;
	hfWindow* p;
	
	for ( p = this, x = 0, y = 0; p; p = p->parent ) {
		x += p->rect.x();
		y += p->rect.y();
	}
	
	r->x -= x;
	r->y -= y;
}

/*
================
hfWindow::ClientToScreen
================
*/
void hfWindow::ClientToScreen ( hfRectangle* r ) {
	int		  x;
	int		  y;
	hfWindow* p;
	
	for ( p = this, x = 0, y = 0; p; p = p->parent ) {
		x += p->rect.x();
		y += p->rect.y();
	}
	
	r->x += x;
	r->y += y;	
}

/*
================
hfWindow::SetDefaults

Set the window do a default window with no text, no background and 
default colors, etc..
================
*/
void hfWindow::SetDefaults () {	
	forceAspectWidth = 640.0f;
	forceAspectHeight = 480.0f;
	matScalex = 1;
	matScaley = 1;
	borderSize = 0;
	noTime = false;
	visible = true;
	textAlign = 0;
	textAlignx = 0;
	textAligny = 0;
	noEvents = false;
	rotate = 0;
	shear.Zero();
	textScale = 0.35f;
	backColor.Zero();
	foreColor = hfVec4(1, 1, 1, 1);
	hoverColor = hfVec4(1, 1, 1, 1);
	matColor = hfVec4(1, 1, 1, 1);
	borderColor.Zero();
	text = "";	

	background = NULL;
	backGroundName = "";
}

/*
================
hfWindow::UpdateFromDictionary

The editor only has a dictionary to work with so the easiest way to push the
values of the dictionary onto the window is for the window to interpret the 
dictionary as if were a file being parsed.
================
*/
bool hfWindow::UpdateFromDictionary ( hfDict& dict ) {
	const hfKeyValue*	kv;
	int					i;
	
	SetDefaults ( );
	
	// Clear all registers since they will get recreated
	regList.Reset ( );
	expressionRegisters.Clear ( );
	ops.Clear ( );
	
	for ( i = 0; i < dict.GetNumKeyVals(); i ++ ) {
		kv = dict.GetKeyVal ( i );

		// Special case name
		if ( !kv->GetKey().Icmp ( "name" ) ) {
			name = kv->GetValue();
			continue;
		}

		hfParser src( kv->GetValue().c_str(), kv->GetValue().Length(), "",
					  LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );
		hfTokenParser src2;
		src2.LoadFromParser( src, "temp" );
		src2.StartParsing( "temp" );
		if ( !ParseInternalVar ( kv->GetKey(), &src2 ) ) {
			// Kill the old register since the parse reg entry will add a new one
			if ( !ParseRegEntry ( kv->GetKey(), &src2 ) ) {
				continue;
			}
		}
	}
			
	EvalRegs(-1, true);

	SetupFromState();
	PostParse();
	
	return true;
}

