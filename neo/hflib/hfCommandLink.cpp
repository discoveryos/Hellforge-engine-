#pragma hdrstop
#include "hfprecompiled.h"

/*
========================
CommandLinks

The command system is not required for hfLib, but we want to be able
to use the CONSOLE_COMMAND() macro inside idlib, so these must be here.
========================
*/
hfCommandLink *CommandLinks( hfCommandLink *cl ) {
	static hfCommandLink *commandLinks = NULL;
	if ( cl != NULL ) {
		commandLinks = cl;
	}
	return commandLinks;
}


hfCommandLink *commandLinks = NULL;

hfCommandLink::hfCommandLink( const char *cmdName, cmdFunction_t function, 
	const char *description, argCompletion_t argCompletion ) {
		next = CommandLinks();
		CommandLinks( this );
		cmdName_ = cmdName;
		function_ = function;
		description_ = description;
		argCompletion_ = argCompletion;
}


