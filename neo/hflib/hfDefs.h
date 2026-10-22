#ifndef HFDEFS_H
#define HFDEFS_H 1
#include "hfConfig.h"
#include "hfTypes.h"
#define HF_ARRAY_LEN(a) ( sizeof(a) / sizeof((a)[0]) )
#define HF_MAX(a,b) ((a)>(b)?(a):(b))
#define HF_MIN(a,b) ((a)<(b)?(a):(b))

/*
===============================================================================
	Color codes embedded in strings. The engine writes them directly into
	text (typically for pass/fail reporting), so they expand to literals.

	The digit maps directly onto the color table in hfStr.cpp:
	  ^0 black  ^1 red   ^2 green ^3 yellow
	  ^4 blue   ^5 cyan  ^6 orange ^7 white
	  ^8 gray   ^9 default
===============================================================================
*/
#define S_COLOR_ESCAPE	'^'
#define S_COLOR_BLACK	"^0"
#define S_COLOR_RED		"^1"
#define S_COLOR_GREEN	"^2"
#define S_COLOR_YELLOW	"^3"
#define S_COLOR_BLUE	"^4"
#define S_COLOR_CYAN	"^5"
#define S_COLOR_ORANGE	"^6"
#define S_COLOR_WHITE	"^7"
#define S_COLOR_GRAY	"^8"
#define S_COLOR_DEFAULT	"^9"

// The console passes the single character following the escape around, so
// the console color codes are those characters rather than table indices.
#define C_COLOR_ESCAPE	'^'
#define C_COLOR_BLACK	'0'
#define C_COLOR_RED		'1'
#define C_COLOR_GREEN	'2'
#define C_COLOR_YELLOW	'3'
#define C_COLOR_BLUE	'4'
#define C_COLOR_CYAN	'5'
#define C_COLOR_ORANGE	'6'
#define C_COLOR_WHITE	'7'
#define C_COLOR_GRAY	'8'
#define C_COLOR_DEFAULT	'9'

#define S_COLOR_BLACK	"^0"

/*
===============================================================================
	ASSERT_ENUM_STRING( value, expected ) - compile time assertion that an enum
	value matches an expected constant, while also expanding to a comma
	separated array element. Used to build name tables that must stay in sync
	with their enum.
===============================================================================
*/
#define ASSERT_ENUM_STRING( x, y ) ( ( ( x ) == ( y ) ) ? "" : __FILE__ ":" #x " != " #y, "" )
#endif
