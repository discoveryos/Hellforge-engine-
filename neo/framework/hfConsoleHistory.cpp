#pragma hdrstop
#include "../hflib/hfprecompiled.h"
#include "hfConsoleHistory.h"

hfConsoleHistory consoleHistory;

const char * HISTORY_FILE_NAME = "consoleHistory.txt";

/*
========================
hfConsoleHistory::AddToHistory
========================
*/
void hfConsoleHistory::AddToHistory( const char *line, bool writeHistoryFile ) {
	// empty lines never modify history
	if ( line == NULL ) {
		return;
	}
	const char *s;
	for ( s = line; *s != '\0'; s++ ) {
		if ( *s > ' ' ) {
			break;
		}
	}
	if ( *s == '\0' ) {
		return;
	}

	// repeating the last command doesn't add to the list
	if ( historyLines[( numHistory - 1 ) & ( COMMAND_HISTORY - 1 )].Icmp( line ) == 0 ) {
		if ( historyLines[returnLine & ( COMMAND_HISTORY - 1 )].Icmp( line ) == 0 ) {
			// the command was retrieved from the history, so
			// move the up point down so that up arrow will retrieve the same
			// command again.
			upPoint = returnLine;
		} else {
			// the command was typed again, so leave the up/down points alone
		}
		return;
	}
	// If we had previously retrieved older history commands with
	// up arrow, the up/down point will be reset to the end where
	// this command is added.
	upPoint = numHistory;
	returnLine = numHistory;
	downPoint = numHistory + 1;
//	//mem.PushHeap();	// go to the system heap, not the current map heap
	historyLines[numHistory & ( COMMAND_HISTORY - 1 )] = line;
//	//mem.PopHeap();
	numHistory++;

	// write the history file to disk
	if ( writeHistoryFile ) {
		hfFile *f = fileSystem->OpenFileWrite( HISTORY_FILE_NAME );
		if ( f != NULL ) {
			for ( int i = numHistory - COMMAND_HISTORY; i < numHistory; i++ ) {
				if ( i < 0 ) {
					continue;
				}
				f->Printf( "%s\n", historyLines[i & ( COMMAND_HISTORY - 1 )].c_str() );
			}
			delete f;
		}
	}
}

/*
========================
hfConsoleHistory::RetrieveFromHistory
========================
*/
hfStr hfConsoleHistory::RetrieveFromHistory( bool backward ) {
	// if there are no commands in the history
	if ( numHistory == 0 ) {
		return hfStr( "" );
	}
	// move the history point
	if ( backward ) {
		if ( upPoint < numHistory - COMMAND_HISTORY || upPoint < 0 ) {
			return hfStr( "" );
		}
		returnLine = upPoint;
		downPoint = upPoint + 1;
		upPoint--;
	} else {
		if ( downPoint >= numHistory ) {
			return hfStr( "" );
		}
		returnLine = downPoint;
		upPoint = downPoint - 1;
		downPoint++;
	}
	return historyLines[returnLine & ( COMMAND_HISTORY - 1 )];
}

/*
========================
hfConsoleHistory::LoadHistoryFile
========================
*/
void hfConsoleHistory::LoadHistoryFile() {
	hfLexer lex;
	if ( lex.LoadFile( HISTORY_FILE_NAME, false ) ) {
		while( 1 ) {
			hfStr	line;
			lex.ParseCompleteLine( line );
			if ( line.IsEmpty() ) {
				break;
			}
			line.StripTrailingWhitespace();	// remove the \n
			AddToHistory( line, false );
		}
	}
}

/*
========================
hfConsoleHistory::PrintHistory
========================
*/
void hfConsoleHistory::PrintHistory() {
	for ( int i = numHistory - COMMAND_HISTORY; i < numHistory; i++ ) {
		if ( i < 0 ) {
			continue;
		}
		hfLib::Printf( "%c%c%c%4i: %s\n", 
			i == upPoint ? 'U' : ' ',
			i == downPoint ? 'D' : ' ',
			i == returnLine ? 'R' : ' ',
			i, historyLines[i & ( COMMAND_HISTORY - 1 )].c_str() );
	}
}

/*
========================
hfConsoleHistory::ClearHistory
========================
*/
void hfConsoleHistory::ClearHistory() {
	upPoint = 0;
	downPoint = 0;
	returnLine = 0;
	numHistory = 0;
}

/*
========================
history
========================
*/
CONSOLE_COMMAND_SHIP( history, "Displays the console command history", 0 ) {
	consoleHistory.PrintHistory();
}

/*
========================
clearHistory
========================
*/
CONSOLE_COMMAND_SHIP( clearHistory, "Clears the console history", 0 ) {
	consoleHistory.ClearHistory();
}

