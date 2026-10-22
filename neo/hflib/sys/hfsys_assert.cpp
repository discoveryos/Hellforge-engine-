#pragma hdrstop
#include "../hfprecompiled.h"

/*
================================================================================================
Contains the AssertMacro implementation.
================================================================================================
*/

hfCVar com_assertOutOfDebugger( "com_assertOutOfDebugger", "0", CVAR_BOOL, "by default, do not assert while not running under the debugger" );

struct skippedAssertion_t {
					skippedAssertion_t() :
						file( NULL ),
						line( -1 ) {
					}
	const char *	file;
	int				line;
};
static hfStaticList< skippedAssertion_t,20 > skippedAssertions;

/*
========================
AssertFailed
========================
*/
bool AssertFailed( const char * file, int line, const char * expression ) {
	// Set this to true to skip ALL assertions, including ones YOU CAUSE!
	static volatile bool skipAllAssertions = false;
	if ( skipAllAssertions ) {
		return false;
	}

	// Set this to true to skip ONLY this assertion
	static volatile bool skipThisAssertion = false;
	skipThisAssertion = false;

	for ( int i = 0; i < skippedAssertions.Num(); i++ ) {
		if ( skippedAssertions[i].file == file && skippedAssertions[i].line == line ) {
			skipThisAssertion = true;
			// Set breakpoint here to re-enable
			if ( !skipThisAssertion ) {
				skippedAssertions.RemoveIndexFast( i );
			}
			return false;
		}
	}

	hfLib::Warning( "ASSERTION FAILED! %s(%d): '%s'", file, line, expression );

	#if defined(_WIN32)
	if ( IsDebuggerPresent() || com_assertOutOfDebugger.GetBool() ) {
#else
	if ( com_assertOutOfDebugger.GetBool() ) {
#endif
			HF_DEBUG_BREAK();
	}

	if ( skipThisAssertion ) {
		skippedAssertion_t * skipped = skippedAssertions.Alloc();
		skipped->file = file;
		skipped->line = line;
	}

	return true;
}


