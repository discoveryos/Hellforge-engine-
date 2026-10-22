#ifndef __KEYINPUT_H__
#define __KEYINPUT_H__

struct keyBindings_t {
	hfStr keyboard;
	hfStr mouse;
	hfStr gamepad;
};

class hfSerializer;

// Converts from a USB HID code to a K_ code
int Key_CovertHIDCode( int hid );

class hfKeyInput {
public:
	static void				Init();
	static void				Shutdown();

	static void				ArgCompletion_KeyName( const hfCmdArgs &args, void(*callback)( const char *s ) );
	static void				PreliminaryKeyEvent( int keyNum, bool down );
	static bool				IsDown( int keyNum );
	static int				GetUsercmdAction( int keyNum );
	static bool				GetOverstrikeMode();
	static void				SetOverstrikeMode( bool state );
	static void				ClearStates();

	static keyNum_t			StringToKeyNum( const char * str );		// This is used by the "bind" command
	static const char *		KeyNumToString( keyNum_t keyNum );		// This is the inverse of StringToKeyNum, used for config files
	static const char *		LocalizedKeyName( keyNum_t keyNum );	// This returns text suitable to print on screen

	static void				SetBinding( int keyNum, const char *binding );
	static const char *		GetBinding( int keyNum );
	static bool				UnbindBinding( const char *bind );
	static int				NumBinds( const char *binding );
	static bool				ExecKeyBinding( int keyNum );
	static const char *		KeysFromBinding( const char *bind );
	static const char *		BindingFromKey( const char *key );
	static bool				KeyIsBoundTo( int keyNum, const char *binding );
	static void				WriteBindings( hfFile *f );
	static keyBindings_t	KeyBindingsFromBinding( const char * bind, bool firstOnly = false, bool localized = false );
};

#endif /* !__KEYINPUT_H__ */

