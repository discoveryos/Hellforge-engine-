/*
** LINUX_INPUT.CPP
**
** X11 input handling: keyboard, mouse, and the event pump.
*/

#pragma hdrstop
#include "../../hflib/hfprecompiled.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include "hflinux_local.h"
#include "../../renderer/hftr_local.h"

hfCVar in_joystick( "in_joystick", "0", CVAR_ARCHIVE | CVAR_BOOL, "enable joystick support" );

hfCVar linux_allowAltTab( "linux_allowAltTab", "0", CVAR_ARCHIVE | CVAR_BOOL, "disables the system behavior of Alt-Tab" );
hfCVar linux_username( "linux_username", "owner", CVAR_ARCHIVE | CVAR_NOCHEAT, "set the default user name" );

static bool		ini_import = false;
static bool		keyboard_present = false;

// True when the mouse is inside the window and any button is down
static bool		mouseGrabbed = false;
static int		mouseLastX = 0;
static int		mouseLastY = 0;

// 256 key-state buffers to fake buffered events like DirectInput does
static byte		toggleFetch[2][ 512 ];
static int		diFetch = 0;

struct polledKeyEvent_t {
	int		key;
	bool	state;
};

static const int MAX_POLLED_KEY_EVENTS = 64;
static polledKeyEvent_t		polledKeyEvents[MAX_POLLED_KEY_EVENTS];
static int					polledKeyEventsCount = 0;

struct polledMouseEvent_t {
	int		ofs;	// event type
	int		data;
};
static polledMouseEvent_t	polledMouseEvents[MAX_MOUSE_EVENTS];
static int					polledMouseEventsCount = 0;

/*
================
Sys_Xatoid
================
*/
static int Sys_Xatoid( KeySym key ) {
	// map the X keysyms to the keyNum_t enum
	int k = K_NONE;
	switch ( key ) {
	case XK_Escape: k = K_ESCAPE; break;
	case XK_1: k = K_1; break;
	case XK_2: k = K_2; break;
	case XK_3: k = K_3; break;
	case XK_4: k = K_4; break;
	case XK_5: k = K_5; break;
	case XK_6: k = K_6; break;
	case XK_7: k = K_7; break;
	case XK_8: k = K_8; break;
	case XK_9: k = K_9; break;
	case XK_0: k = K_0; break;
	case XK_minus: k = K_MINUS; break;
	case XK_equal: k = K_EQUALS; break;
	case XK_BackSpace: k = K_BACKSPACE; break;
	case XK_Tab: k = K_TAB; break;
	case XK_q: k = K_Q; break;
	case XK_w: k = K_W; break;
	case XK_e: k = K_E; break;
	case XK_r: k = K_R; break;
	case XK_t: k = K_T; break;
	case XK_y: k = K_Y; break;
	case XK_u: k = K_U; break;
	case XK_i: k = K_I; break;
	case XK_o: k = K_O; break;
	case XK_p: k = K_P; break;
	case XK_bracketleft: k = K_LBRACKET; break;
	case XK_bracketright: k = K_RBRACKET; break;
	case XK_Return: k = K_ENTER; break;
	case XK_Control_L: k = K_LCTRL; break;
	case XK_a: k = K_A; break;
	case XK_s: k = K_S; break;
	case XK_d: k = K_D; break;
	case XK_f: k = K_F; break;
	case XK_g: k = K_G; break;
	case XK_h: k = K_H; break;
	case XK_j: k = K_J; break;
	case XK_k: k = K_K; break;
	case XK_l: k = K_L; break;
	case XK_semicolon: k = K_SEMICOLON; break;
	case XK_apostrophe: k = K_APOSTROPHE; break;
	case XK_grave: k = K_GRAVE; break;
	case XK_Shift_L: k = K_LSHIFT; break;
	case XK_backslash: k = K_BACKSLASH; break;
	case XK_z: k = K_Z; break;
	case XK_x: k = K_X; break;
	case XK_c: k = K_C; break;
	case XK_v: k = K_V; break;
	case XK_b: k = K_B; break;
	case XK_n: k = K_N; break;
	case XK_m: k = K_M; break;
	case XK_comma: k = K_COMMA; break;
	case XK_period: k = K_PERIOD; break;
	case XK_slash: k = K_SLASH; break;
	case XK_Shift_R: k = K_RSHIFT; break;
	case XK_KP_Multiply: k = K_KP_STAR; break;
	case XK_Alt_L: k = K_LALT; break;
	case XK_space: k = K_SPACE; break;
	case XK_Caps_Lock: k = K_CAPSLOCK; break;
	case XK_F1: k = K_F1; break;
	case XK_F2: k = K_F2; break;
	case XK_F3: k = K_F3; break;
	case XK_F4: k = K_F4; break;
	case XK_F5: k = K_F5; break;
	case XK_F6: k = K_F6; break;
	case XK_F7: k = K_F7; break;
	case XK_F8: k = K_F8; break;
	case XK_F9: k = K_F9; break;
	case XK_F10: k = K_F10; break;
	case XK_Num_Lock: k = K_NUMLOCK; break;
	case XK_Scroll_Lock: k = K_SCROLL; break;
	case XK_KP_7: k = K_KP_7; break;
	case XK_KP_8: k = K_KP_8; break;
	case XK_KP_9: k = K_KP_9; break;
	case XK_KP_Subtract: k = K_KP_MINUS; break;
	case XK_KP_4: k = K_KP_4; break;
	case XK_KP_5: k = K_KP_5; break;
	case XK_KP_6: k = K_KP_6; break;
	case XK_KP_Add: k = K_KP_PLUS; break;
	case XK_KP_1: k = K_KP_1; break;
	case XK_KP_2: k = K_KP_2; break;
	case XK_KP_3: k = K_KP_3; break;
	case XK_KP_0: k = K_KP_0; break;
	case XK_KP_Decimal: k = K_KP_DOT; break;
	case XK_F11: k = K_F11; break;
	case XK_F12: k = K_F12; break;
	case XK_Control_R: k = K_RCTRL; break;
	case XK_Print: k = K_PRINTSCREEN; break;
	case XK_Alt_R: k = K_RALT; break;
	case XK_Pause: k = K_PAUSE; break;
	case XK_Home: k = K_HOME; break;
	case XK_Up: k = K_UPARROW; break;
	case XK_Prior: k = K_PGUP; break;
	case XK_Left: k = K_LEFTARROW; break;
	case XK_Right: k = K_RIGHTARROW; break;
	case XK_End: k = K_END; break;
	case XK_Down: k = K_DOWNARROW; break;
	case XK_Next: k = K_PGDN; break;
	case XK_Insert: k = K_INS; break;
	case XK_Delete: k = K_DEL; break;
	case XK_Super_L: k = K_LWIN; break;
	case XK_Super_R: k = K_RWIN; break;
	case XK_Menu: k = K_APPS; break;
	}
	return k;
}

/*
================
Sys_GenerateEvents

Called in the main loop, pumps X11 events into the event queue
================
*/
void Sys_GenerateEvents() {
	if ( x_display == NULL ) {
		return;
	}

	// handle window close requests by generating a quit event
	while ( XPending( x_display ) ) {
		XEvent ev;
		XNextEvent( x_display, &ev );

		switch ( ev.type ) {
		case ClientMessage: {
			Atom wm_delete = XInternAtom( x_display, "WM_DELETE_WINDOW", False );
			if ( (Atom)ev.xclient.data.l[0] == wm_delete ) {
				Sys_QueEvent( SE_QUIT, 0, 0, 0, NULL, 0 );
			}
			break;
		}
		case KeyPress:
		case KeyRelease: {
			KeySym key = XLookupKeysym( &ev.xkey, 0 );
			int k = Sys_Xatoid( key );
			if ( k != K_NONE ) {
				bool state = ( ev.type == KeyPress );
				Sys_QueEvent( SE_KEY, k, state, 0, NULL, 0 );
				// also track the raw state for the polling API
				if ( polledKeyEventsCount < MAX_POLLED_KEY_EVENTS ) {
					polledKeyEvents[polledKeyEventsCount].key = k;
					polledKeyEvents[polledKeyEventsCount].state = state;
					polledKeyEventsCount++;
				}
				toggleFetch[diFetch][k & 0x1FF] = state;
			}
			break;
		}
		case ButtonPress:
		case ButtonRelease: {
			bool state = ( ev.type == ButtonPress );
			int key = K_MOUSE1 + ( ev.xbutton.button - 1 );
			if ( ev.xbutton.button == 4 ) {		// wheel up
				key = K_MWHEELUP;
				sysEvent_t e;
				Sys_QueEvent( SE_KEY, key, true, 0, NULL, 0 );
				Sys_QueEvent( SE_KEY, key, false, 0, NULL, 0 );
				break;
			}
			if ( ev.xbutton.button == 5 ) {		// wheel down
				key = K_MWHEELDOWN;
				Sys_QueEvent( SE_KEY, key, true, 0, NULL, 0 );
				Sys_QueEvent( SE_KEY, key, false, 0, NULL, 0 );
				break;
			}
			if ( ev.xbutton.button >= 1 && ev.xbutton.button <= 8 ) {
				Sys_QueEvent( SE_KEY, key, state, 0, NULL, 0 );
				if ( polledKeyEventsCount < MAX_POLLED_KEY_EVENTS ) {
					polledKeyEvents[polledKeyEventsCount].key = key;
					polledKeyEvents[polledKeyEventsCount].state = state;
					polledKeyEventsCount++;
				}
			}
			break;
		}
		case MotionNotify: {
			int dx = ev.xmotion.x - mouseLastX;
			int dy = ev.xmotion.y - mouseLastY;
			mouseLastX = ev.xmotion.x;
			mouseLastY = ev.xmotion.y;

			if ( mouseGrabbed ) {
				Sys_QueEvent( SE_MOUSE, dx, dy, 0, NULL, 0 );
				if ( polledMouseEventsCount < MAX_MOUSE_EVENTS ) {
					polledMouseEvents[polledMouseEventsCount].ofs = M_DELTAX;
					polledMouseEvents[polledMouseEventsCount].data = dx;
					polledMouseEventsCount++;
					if ( polledMouseEventsCount < MAX_MOUSE_EVENTS ) {
						polledMouseEvents[polledMouseEventsCount].ofs = M_DELTAY;
						polledMouseEvents[polledMouseEventsCount].data = dy;
						polledMouseEventsCount++;
					}
				}
			}
			break;
		}
		case FocusIn:
			linux.activeApp = true;
			linux.mouseReleased = false;
			break;
		case FocusOut:
			linux.activeApp = false;
			linux.mouseReleased = false;
			break;
		case ConfigureNotify: {
			if ( ev.xconfigure.width != glConfig.nativeScreenWidth ||
				ev.xconfigure.height != glConfig.nativeScreenHeight ) {
				glConfig.nativeScreenWidth = ev.xconfigure.width;
				glConfig.nativeScreenHeight = ev.xconfigure.height;
			}
			break;
		}
		default:
			break;
		}
	}
}

/*
================
Sys_InitInput
================
*/
void Sys_InitInput() {
	common->Printf( "\n------- Input Initialization -------\n" );
	keyboard_present = true;
	common->Printf( "Keyboardet present.\n" );
	common->Printf( "------------------------------------\n" );
}

/*
================
Sys_ShutdownInput
================
*/
void Sys_ShutdownInput() {
	Sys_GrabMouseCursor( false );
}

/*
================
Sys_GrabMouseCursor

Grabs or releases the mouse cursor
================
*/
void Sys_GrabMouseCursor( bool grabIt ) {
	if ( x_display == NULL || linux.wnd == 0 ) {
		return;
	}

	mouseGrabbed = grabIt;

	if ( grabIt ) {
		// hide the cursor
		Cursor invisibleCursor = XCreateFontCursor( x_display, XC_left_ptr );
		XDefineCursor( x_display, linux.wnd, invisibleCursor );

		XGrabPointer( x_display, linux.wnd, True, PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
			GrabModeAsync, GrabModeAsync, linux.wnd, None, CurrentTime );
		XWarpPointer( x_display, None, linux.wnd, 0, 0, 0, 0,
			glConfig.nativeScreenWidth / 2, glConfig.nativeScreenHeight / 2 );
		mouseLastX = glConfig.nativeScreenWidth / 2;
		mouseLastY = glConfig.nativeScreenHeight / 2;
	} else {
		XUngrabPointer( x_display, CurrentTime );
		XUndefineCursor( x_display, linux.wnd );
	}
	XFlush( x_display );

	linux.mouseReleased = !grabIt;
}

/*
================
Sys_PollKeyboardInputEvents
================
*/
int Sys_PollKeyboardInputEvents() {
	if ( !keyboard_present ) {
		return 0;
	}
	return polledKeyEventsCount;
}

/*
================
Sys_ReturnKeyboardInputEvent
================
*/
int Sys_ReturnKeyboardInputEvent( const int n, int &ch, bool &state ) {
	if ( n < 0 || n >= MAX_POLLED_KEY_EVENTS ) {
		return 0;
	}
	ch = polledKeyEvents[n].key;
	state = polledKeyEvents[n].state;
	return ch;
}

void Sys_EndKeyboardInputEvents() {
	polledKeyEventsCount = 0;
}

/*
================
Sys_PollMouseInputEvents
================
*/
int Sys_PollMouseInputEvents( int mouseEvents[MAX_MOUSE_EVENTS][2] ) {
	if ( !mouseGrabbed ) {
		polledMouseEventsCount = 0;
		return 0;
	}

	int count = 0;
	for ( int i = 0; i < polledMouseEventsCount && count < MAX_MOUSE_EVENTS; i++ ) {
		mouseEvents[count][0] = polledMouseEvents[i].ofs;
		mouseEvents[count][1] = polledMouseEvents[i].data;
		count++;
	}
	polledMouseEventsCount = 0;
	return count;
}

/*
================
Joystick (not implemented on Linux for now)
================
*/
void Sys_SetRumble( int device, int low, int hi ) {
}

int Sys_PollJoystickInputEvents( int deviceNum ) {
	return 0;
}

int Sys_ReturnJoystickInputEvent( const int n, int &action, int &value ) {
	return 0;
}

void Sys_EndJoystickInputEvents() {
}
