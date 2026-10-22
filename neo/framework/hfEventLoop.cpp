#include "../hflib/hfprecompiled.h"
#pragma hdrstop

hfCVar hfEventLoop::com_journal( "com_journal", "0", CVAR_INIT|CVAR_SYSTEM, "1 = record journal, 2 = play back journal", 0, 2, hfCmdSystem::ArgCompletion_Integer<0,2> );

hfEventLoop eventLoopLocal;
hfEventLoop *eventLoop = &eventLoopLocal;


/*
=================
hfEventLoop::hfEventLoop
=================
*/
hfEventLoop::hfEventLoop() {
	com_journalFile = NULL;
	com_journalDataFile = NULL;
	initialTimeOffset = 0;
}

/*
=================
hfEventLoop::~hfEventLoop
=================
*/
hfEventLoop::~hfEventLoop() {
}

/*
=================
hfEventLoop::GetRealEvent
=================
*/
sysEvent_t	hfEventLoop::GetRealEvent() {
	int			r;
	sysEvent_t	ev;

	// either get an event from the system or the journal file
	if ( com_journal.GetInteger() == 2 ) {
		r = com_journalFile->Read( &ev, sizeof(ev) );
		if ( r != sizeof(ev) ) {
			common->FatalError( "Error reading from journal file" );
		}
		if ( ev.evPtrLength ) {
			ev.evPtr = Mem_ClearedAlloc( ev.evPtrLength, TAG_EVENTS );
			r = com_journalFile->Read( ev.evPtr, ev.evPtrLength );
			if ( r != ev.evPtrLength ) {
				common->FatalError( "Error reading from journal file" );
			}
		}
	} else {
		ev = Sys_GetEvent();

		// write the journal value out if needed
		if ( com_journal.GetInteger() == 1 ) {
			r = com_journalFile->Write( &ev, sizeof(ev) );
			if ( r != sizeof(ev) ) {
				common->FatalError( "Error writing to journal file" );
			}
			if ( ev.evPtrLength ) {
				r = com_journalFile->Write( ev.evPtr, ev.evPtrLength );
				if ( r != ev.evPtrLength ) {
					common->FatalError( "Error writing to journal file" );
				}
			}
		}
	}

	return ev;
}

/*
=================
hfEventLoop::PushEvent
=================
*/
void hfEventLoop::PushEvent( sysEvent_t *event ) {
	sysEvent_t		*ev;
	static			bool printedWarning;

	ev = &com_pushedEvents[ com_pushedEventsHead & (MAX_PUSHED_EVENTS-1) ];

	if ( com_pushedEventsHead - com_pushedEventsTail >= MAX_PUSHED_EVENTS ) {

		// don't print the warning constantly, or it can give time for more...
		if ( !printedWarning ) {
			printedWarning = true;
			common->Printf( "WARNING: Com_PushEvent overflow\n" );
		}

		if ( ev->evPtr ) {
			Mem_Free( ev->evPtr );
		}
		com_pushedEventsTail++;
	} else {
		printedWarning = false;
	}

	*ev = *event;
	com_pushedEventsHead++;
}

/*
=================
hfEventLoop::GetEvent
=================
*/
sysEvent_t hfEventLoop::GetEvent() {
	if ( com_pushedEventsHead > com_pushedEventsTail ) {
		com_pushedEventsTail++;
		return com_pushedEvents[ (com_pushedEventsTail-1) & (MAX_PUSHED_EVENTS-1) ];
	}
	return GetRealEvent();
}

/*
=================
hfEventLoop::ProcessEvent
=================
*/
void hfEventLoop::ProcessEvent( sysEvent_t ev ) {
	// track key up / down states
	if ( ev.evType == SE_KEY ) {
		hfKeyInput::PreliminaryKeyEvent( ev.evValue, ( ev.evValue2 != 0 ) );
	}

	if ( ev.evType == SE_CONSOLE ) {
		// from a text console outside the game window
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, (char *)ev.evPtr );
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "\n" );
	} else {
		common->ProcessEvent( &ev );
	}

	// free any block data
	if ( ev.evPtr ) {
		Mem_Free( ev.evPtr );
	}
}

/*
===============
hfEventLoop::RunEventLoop
===============
*/
int hfEventLoop::RunEventLoop( bool commandExecution ) {
	sysEvent_t	ev;

	while ( 1 ) {

		if ( commandExecution ) {
			// execute any bound commands before processing another event
			cmdSystem->ExecuteCommandBuffer();
		}

		ev = GetEvent();

		// if no more events are available
		if ( ev.evType == SE_NONE ) {
			return 0;
		}
		ProcessEvent( ev );
	}

	return 0;	// never reached
}

/*
=============
hfEventLoop::Init
=============
*/
void hfEventLoop::Init() {

	initialTimeOffset = Sys_Milliseconds();

	common->StartupVariable( "journal" );

	if ( com_journal.GetInteger() == 1 ) {
		common->Printf( "Journaling events\n" );
		com_journalFile = fileSystem->OpenFileWrite( "journal.dat" );
		com_journalDataFile = fileSystem->OpenFileWrite( "journaldata.dat" );
	} else if ( com_journal.GetInteger() == 2 ) {
		common->Printf( "Replaying journaled events\n" );
		com_journalFile = fileSystem->OpenFileRead( "journal.dat" );
		com_journalDataFile = fileSystem->OpenFileRead( "journaldata.dat" );
	}

	if ( !com_journalFile || !com_journalDataFile ) {
		com_journal.SetInteger( 0 );
		com_journalFile = 0;
		com_journalDataFile = 0;
		common->Printf( "Couldn't open journal files\n" );
	}
}

/*
=============
hfEventLoop::Shutdown
=============
*/
void hfEventLoop::Shutdown() {
	if ( com_journalFile ) {
		fileSystem->CloseFile( com_journalFile );
		com_journalFile = NULL;
	}
	if ( com_journalDataFile ) {
		fileSystem->CloseFile( com_journalDataFile );
		com_journalDataFile = NULL;
	}
}

/*
================
hfEventLoop::Milliseconds

Can be used for profiling, but will be journaled accurately
================
*/
int hfEventLoop::Milliseconds() {
#if 1	// FIXME!
	return Sys_Milliseconds() - initialTimeOffset;
#else
	sysEvent_t	ev;

	// get events and push them until we get a null event with the current time
	do {

		ev = Com_GetRealEvent();
		if ( ev.evType != SE_NONE ) {
			Com_PushEvent( &ev );
		}
	} while ( ev.evType != SE_NONE );
	
	return ev.evTime;
#endif
}

/*
================
hfEventLoop::JournalLevel
================
*/
int hfEventLoop::JournalLevel() const {
	return com_journal.GetInteger();
}

