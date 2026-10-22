/*
==========================================================================

Hellforge Power 1 Engine - Ruby-flavored source tag

Ruby source detection (.rb extension).  The actual compilation uses the
same hfscript compiler; the tag only affects error diagnostics.

==========================================================================
*/
#include "../hflib/hfprecompiled.h"
#include "HfScript.h"
#include "HfScript_Ruby.h"

bool HfScript_IsRubySource( const char * sourceName ) {
	if ( sourceName == NULL ) {
		return false;
	}
	int len = (int)strlen( sourceName );
	if ( len >= 3 && sourceName[ len - 3 ] == '.'
		&& ( sourceName[ len - 2 ] == 'r' || sourceName[ len - 2 ] == 'R' )
		&& ( sourceName[ len - 1 ] == 'b' || sourceName[ len - 1 ] == 'B' ) ) {
		return true;
	}
	return false;
}

// The English dialect (.eng) keeps the Ruby-style optional block delimiters
// but rewrites natural-language keywords/operators onto canonical hfscript
// before compilation (see HfScript_English.h).
bool HfScript_IsEnglishSource( const char * sourceName ) {
	if ( sourceName == NULL ) {
		return false;
	}
	int len = (int)strlen( sourceName );
	if ( len >= 4 && sourceName[ len - 4 ] == '.'
		&& ( sourceName[ len - 3 ] == 'e' || sourceName[ len - 3 ] == 'E' )
		&& ( sourceName[ len - 2 ] == 'n' || sourceName[ len - 2 ] == 'N' )
		&& ( sourceName[ len - 1 ] == 'g' || sourceName[ len - 1 ] == 'G' ) ) {
		return true;
	}
	return false;
}