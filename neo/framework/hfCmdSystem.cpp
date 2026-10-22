#include "../hflib/hfprecompiled.h"
#pragma hdrstop

#ifdef HF_RETAIL
hfCVar net_allowCheats( "net_allowCheats", "0", CVAR_BOOL | CVAR_ROM, "Allow cheats in multiplayer" );
#else
hfCVar net_allowCheats( "net_allowCheats", "0", CVAR_BOOL | CVAR_NOCHEAT, "Allow cheats in multiplayer" );
#endif

/*
===============================================================================

	hfCmdSystemLocal

===============================================================================
*/

typedef struct commandDef_s {
	struct commandDef_s *	next;
	char *					name;
	cmdFunction_t			function;
	argCompletion_t			argCompletion;
	int						flags;
	char *					description;
} commandDef_t;

/*
================================================
hfCmdSystemLocal 
================================================
*/
class hfCmdSystemLocal : public hfCmdSystem {
public:
	virtual void			Init();
	virtual void			Shutdown();

	virtual void			AddCommand( const char *cmdName, cmdFunction_t function, int flags, const char *description, argCompletion_t argCompletion = NULL );
	virtual void			RemoveCommand( const char *cmdName );
	virtual void			RemoveFlaggedCommands( int flags );

	virtual void			CommandCompletion( void(*callback)( const char *s ) );
	virtual void			ArgCompletion( const char *cmdString, void(*callback)( const char *s ) );
	virtual void			ExecuteCommandText( const char * text );
	virtual void			AppendCommandText( const char * text );

	virtual void			BufferCommandText( cmdExecution_t exec, const char *text );
	virtual void			ExecuteCommandBuffer();

	virtual void			ArgCompletion_FolderExtension( const hfCmdArgs &args, void(*callback)( const char *s ), const char *folder, bool stripFolder, ... );
	virtual void			ArgCompletion_DeclName( const hfCmdArgs &args, void(*callback)( const char *s ), int type );

	virtual void			BufferCommandArgs( cmdExecution_t exec, const hfCmdArgs &args );

	virtual void			SetupReloadEngine( const hfCmdArgs &args );
	virtual bool			PostReloadEngine();

	void					SetWait( int numFrames ) { wait = numFrames; }
	commandDef_t *			GetCommands() const { return commands; }

private:
	static const int		MAX_CMD_BUFFER = 0x10000;

	commandDef_t *			commands;

	int						wait;
	int						textLength;
	byte					textBuf[MAX_CMD_BUFFER];

	hfStr					completionString;
	hfStrList				completionParms;

	// piggybacks on the text buffer, avoids tokenize again and screwing it up
	hfList<hfCmdArgs>		tokenizedCmds;

	// a command stored to be executed after a reloadEngine and all associated commands have been processed
	hfCmdArgs				postReload;

private:	
	void					ExecuteTokenizedString( const hfCmdArgs &args );
	void					InsertCommandText( const char *text );

	static void				ListByFlags( const hfCmdArgs &args, cmdFlags_t flags );
	static void				List_f( const hfCmdArgs &args );
	static void				SystemList_f( const hfCmdArgs &args );
	static void				RendererList_f( const hfCmdArgs &args );
	static void				SoundList_f( const hfCmdArgs &args );
	static void				GameList_f( const hfCmdArgs &args );
	static void				ToolList_f( const hfCmdArgs &args );
	static void				Exec_f( const hfCmdArgs &args );
	static void				Vstr_f( const hfCmdArgs &args );
	static void				Echo_f( const hfCmdArgs &args );
	static void				Parse_f( const hfCmdArgs &args );
	static void				Wait_f( const hfCmdArgs &args );
	static void				PrintMemInfo_f( const hfCmdArgs &args );
};

hfCmdSystemLocal			cmdSystemLocal;
hfCmdSystem *				cmdSystem = &cmdSystemLocal;

/*
================================================
hfSort_CommandDef 
================================================
*/
class hfSort_CommandDef : public hfSort_Quick< commandDef_t, hfSort_CommandDef > {
public:
	int Compare( const commandDef_t & a, const commandDef_t & b ) const { return hfStr::Icmp( a.name, b.name ); }
};

/*
============
hfCmdSystemLocal::ListByFlags
============
*/
void hfCmdSystemLocal::ListByFlags( const hfCmdArgs &args, cmdFlags_t flags ) {
	int i;
	hfStr match;
	const commandDef_t *cmd;
	hfList<const commandDef_t *> cmdList;

	if ( args.Argc() > 1 ) {
		match = args.Args( 1, -1 );
		match.Replace( " ", "" );
	} else {
		match = "";
	}

	for ( cmd = cmdSystemLocal.GetCommands(); cmd; cmd = cmd->next ) {
		if ( !( cmd->flags & flags ) ) {
			continue;
		}
		if ( match.Length() && hfStr( cmd->name ).Filter( match, false ) == 0 ) {
			continue;
		}

		cmdList.Append( cmd );
	}

	//cmdList.SortWithTemplate( hfSort_CommandDef() );

	for ( i = 0; i < cmdList.Num(); i++ ) {
		cmd = cmdList[i];

		common->Printf( "  %-21s %s\n", cmd->name, cmd->description );
	}

	common->Printf( "%i commands\n", cmdList.Num() );
}

/*
============
hfCmdSystemLocal::List_f
============
*/
void hfCmdSystemLocal::List_f( const hfCmdArgs &args ) {
	hfCmdSystemLocal::ListByFlags( args, CMD_FL_ALL );
}

/*
============
hfCmdSystemLocal::SystemList_f
============
*/
void hfCmdSystemLocal::SystemList_f( const hfCmdArgs &args ) {
	hfCmdSystemLocal::ListByFlags( args, CMD_FL_SYSTEM );
}

/*
============
hfCmdSystemLocal::RendererList_f
============
*/
void hfCmdSystemLocal::RendererList_f( const hfCmdArgs &args ) {
	hfCmdSystemLocal::ListByFlags( args, CMD_FL_RENDERER );
}

/*
============
hfCmdSystemLocal::SoundList_f
============
*/
void hfCmdSystemLocal::SoundList_f( const hfCmdArgs &args ) {
	hfCmdSystemLocal::ListByFlags( args, CMD_FL_SOUND );
}

/*
============
hfCmdSystemLocal::GameList_f
============
*/
void hfCmdSystemLocal::GameList_f( const hfCmdArgs &args ) {
	hfCmdSystemLocal::ListByFlags( args, CMD_FL_GAME );
}

/*
============
hfCmdSystemLocal::ToolList_f
============
*/
void hfCmdSystemLocal::ToolList_f( const hfCmdArgs &args ) {
	hfCmdSystemLocal::ListByFlags( args, CMD_FL_TOOL );
}

/*
===============
hfCmdSystemLocal::Exec_f
===============
*/
void hfCmdSystemLocal::Exec_f( const hfCmdArgs &args ) {
	char *	f;
	int		len;
	hfStr	filename;

	if ( args.Argc () != 2 ) {
		common->Printf( "exec <filename> : execute a script file\n" );
		return;
	}

	filename = args.Argv(1);
	filename.DefaultFileExtension( ".cfg" );
	len = fileSystem->ReadFile( filename, reinterpret_cast<void **>(&f), NULL );
	if ( !f ) {
		common->Printf( "couldn't exec %s\n", args.Argv(1) );
		return;
	}
	common->Printf( "execing %s\n", args.Argv(1) );
	
	cmdSystemLocal.BufferCommandText( CMD_EXEC_INSERT, f );

	fileSystem->FreeFile( f );
}

/*
===============
hfCmdSystemLocal::Vstr_f

Inserts the current value of a cvar as command text
===============
*/
void hfCmdSystemLocal::Vstr_f( const hfCmdArgs &args ) {
	const char *v;

	if ( args.Argc () != 2 ) {
		common->Printf( "vstr <variablename> : execute a variable command\n" );
		return;
	}

	v = cvarSystem->GetCVarString( args.Argv( 1 ) );

	cmdSystemLocal.BufferCommandText( CMD_EXEC_APPEND, va( "%s\n", v ) );
}

/*
===============
hfCmdSystemLocal::Echo_f

Just prints the rest of the line to the console
===============
*/
void hfCmdSystemLocal::Echo_f( const hfCmdArgs &args ) {
	int		i;
	
	for ( i = 1; i < args.Argc(); i++ ) {
		common->Printf( "%s ", args.Argv( i ) );
	}
	common->Printf( "\n" );
}

/*
============
hfCmdSystemLocal::Wait_f

Causes execution of the remainder of the command buffer to be delayed until next frame.
============
*/
void hfCmdSystemLocal::Wait_f( const hfCmdArgs &args ) {
	if ( args.Argc() == 2 ) {
		cmdSystemLocal.SetWait( atoi( args.Argv( 1 ) ) );
	} else {
		cmdSystemLocal.SetWait( 1 );
	}
}

/*
============
hfCmdSystemLocal::Parse_f

This just prints out how the rest of the line was parsed, as a debugging tool.
============
*/
void hfCmdSystemLocal::Parse_f( const hfCmdArgs &args ) {
	int		i;

	for ( i = 0; i < args.Argc(); i++ ) {
		common->Printf( "%i: %s\n", i, args.Argv(i) );
	}
}

/*
============
hfCmdSystemLocal::Init
============
*/
void hfCmdSystemLocal::Init() {

	AddCommand( "listCmds", List_f, CMD_FL_SYSTEM, "lists commands" );
	AddCommand( "listSystemCmds", SystemList_f, CMD_FL_SYSTEM, "lists system commands" );
	AddCommand( "listRendererCmds", RendererList_f, CMD_FL_SYSTEM, "lists renderer commands" );
	AddCommand( "listSoundCmds", SoundList_f, CMD_FL_SYSTEM, "lists sound commands" );
	AddCommand( "listGameCmds", GameList_f, CMD_FL_SYSTEM, "lists game commands" );
	AddCommand( "listToolCmds", ToolList_f, CMD_FL_SYSTEM, "lists tool commands" );
	AddCommand( "exec", Exec_f, CMD_FL_SYSTEM, "executes a config file", ArgCompletion_ConfigName );
	AddCommand( "vstr", Vstr_f, CMD_FL_SYSTEM, "inserts the current value of a cvar as command text" );
	AddCommand( "echo", Echo_f, CMD_FL_SYSTEM, "prints text" );
	AddCommand( "parse", Parse_f, CMD_FL_SYSTEM, "prints tokenized string" );
	AddCommand( "wait", Wait_f, CMD_FL_SYSTEM, "delays remaining buffered commands one or more frames" );

	// link in all the commands declared with static hfCommandLink variables or CONSOLE_COMMAND macros
	for ( hfCommandLink * link = CommandLinks(); link != NULL; link = link->next ) {
		AddCommand( link->cmdName_, link->function_, CMD_FL_SYSTEM, link->description_, link->argCompletion_ );
	}

	completionString = "*";

	textLength = 0;
}

/*
============
hfCmdSystemLocal::Shutdown
============
*/
void hfCmdSystemLocal::Shutdown() {
	commandDef_t *cmd;

	for ( cmd = commands; cmd; cmd = commands ) {
		commands = commands->next;
		Mem_Free( cmd->name );
		Mem_Free( cmd->description );
		delete cmd;
	}

	completionString.Clear();
	completionParms.Clear();
	tokenizedCmds.Clear();
	postReload.Clear();
}

/*
============
hfCmdSystemLocal::AddCommand
============
*/
void hfCmdSystemLocal::AddCommand( const char *cmdName, cmdFunction_t function, int flags, const char *description, argCompletion_t argCompletion ) {
	commandDef_t *cmd;
	
	// fail if the command already exists
	for ( cmd = commands; cmd; cmd = cmd->next ) {
		if ( hfStr::Cmp( cmdName, cmd->name ) == 0 ) {
			if ( function != cmd->function ) {
				common->Printf( "hfCmdSystemLocal::AddCommand: %s already defined\n", cmdName );
			}
			return;
		}
	}

	cmd = new (TAG_SYSTEM) commandDef_t;
	cmd->name = Mem_CopyString( cmdName );
	cmd->function = function;
	cmd->argCompletion = argCompletion;
	cmd->flags = flags;
	cmd->description = Mem_CopyString( description );
	cmd->next = commands;
	commands = cmd;
}

/*
============
hfCmdSystemLocal::RemoveCommand
============
*/
void hfCmdSystemLocal::RemoveCommand( const char *cmdName ) {
	commandDef_t *cmd, **last;

	for ( last = &commands, cmd = *last; cmd; cmd = *last ) {
		if ( hfStr::Cmp( cmdName, cmd->name ) == 0 ) {
			*last = cmd->next;
			Mem_Free( cmd->name );
			Mem_Free( cmd->description );
			delete cmd;
			return;
		}
		last = &cmd->next;
	}
}

/*
============
hfCmdSystemLocal::RemoveFlaggedCommands
============
*/
void hfCmdSystemLocal::RemoveFlaggedCommands( int flags ) {
	commandDef_t *cmd, **last;

	for ( last = &commands, cmd = *last; cmd; cmd = *last ) {
		if ( cmd->flags & flags ) {
			*last = cmd->next;
			Mem_Free( cmd->name );
			Mem_Free( cmd->description );
			delete cmd;
			continue;
		}
		last = &cmd->next;
	}
}

/*
============
hfCmdSystemLocal::CommandCompletion
============
*/
void hfCmdSystemLocal::CommandCompletion( void(*callback)( const char *s ) ) {
	commandDef_t *cmd;
	
	for ( cmd = commands; cmd; cmd = cmd->next ) {
		callback( cmd->name );
	}
}

/*
============
hfCmdSystemLocal::ArgCompletion
============
*/
void hfCmdSystemLocal::ArgCompletion( const char *cmdString, void(*callback)( const char *s ) ) {
	commandDef_t *cmd;
	hfCmdArgs args;

	args.TokenizeString( cmdString, false );

	for ( cmd = commands; cmd; cmd = cmd->next ) {
		if ( !cmd->argCompletion ) {
			continue;
		}
		if ( hfStr::Icmp( args.Argv( 0 ), cmd->name ) == 0 ) {
			cmd->argCompletion( args, callback );
			break;
		}
	}
}

/*
============
hfCmdSystemLocal::ExecuteTokenizedString
============
*/
void hfCmdSystemLocal::ExecuteTokenizedString( const hfCmdArgs &args ) {	
	commandDef_t *cmd, **prev;
	
	// execute the command line
	if ( !args.Argc() ) {
		return;		// no tokens
	}

	// check registered command functions	
	for ( prev = &commands; *prev; prev = &cmd->next ) {
		cmd = *prev;
		if ( hfStr::Icmp( args.Argv( 0 ), cmd->name ) == 0 ) {
			// rearrange the links so that the command will be
			// near the head of the list next time it is used
			*prev = cmd->next;
			cmd->next = commands;
			commands = cmd;

			if ( ( cmd->flags & (CMD_FL_CHEAT|CMD_FL_TOOL) ) && common->IsMultiplayer() && !net_allowCheats.GetBool() ) {
				common->Printf( "Command '%s' not valid in multiplayer mode.\n", cmd->name );
				return;
			}
			// perform the action
			if ( !cmd->function ) {
				break;
			} else {
				cmd->function( args );
			}
			return;
		}
	}
	
	// check cvars
	if ( cvarSystem->Command( args ) ) {
		return;
	}

	common->Printf( "Unknown command '%s'\n", args.Argv( 0 ) );
}

/*
============
hfCmdSystemLocal::ExecuteCommandText

Tokenizes, then executes.
============
*/
void hfCmdSystemLocal::ExecuteCommandText( const char *text ) {	
	ExecuteTokenizedString( hfCmdArgs( text, false ) );
}

/*
============
hfCmdSystemLocal::InsertCommandText

Adds command text immediately after the current command
Adds a \n to the text
============
*/
void hfCmdSystemLocal::InsertCommandText( const char *text ) {
	int		len;
	int		i;

	len = strlen( text ) + 1;
	if ( len + textLength > (int)sizeof( textBuf ) ) {
		common->Printf( "hfCmdSystemLocal::InsertText: buffer overflow\n" );
		return;
	}

	// move the existing command text
	for ( i = textLength - 1; i >= 0; i-- ) {
		textBuf[ i + len ] = textBuf[ i ];
	}

	// copy the new text in
	memcpy( textBuf, text, len - 1 );

	// add a \n
	textBuf[ len - 1 ] = '\n';

	textLength += len;
}

/*
============
hfCmdSystemLocal::AppendCommandText

Adds command text at the end of the buffer, does NOT add a final \n
============
*/
void hfCmdSystemLocal::AppendCommandText( const char *text ) {
	int l;
	
	l = strlen( text );

	if ( textLength + l >= (int)sizeof( textBuf ) ) {
		common->Printf( "hfCmdSystemLocal::AppendText: buffer overflow\n" );
		return;
	}
	memcpy( textBuf + textLength, text, l );
	textLength += l;
}

/*
============
hfCmdSystemLocal::BufferCommandText
============
*/
void hfCmdSystemLocal::BufferCommandText( cmdExecution_t exec, const char *text ) {
	switch( exec ) {
		case CMD_EXEC_NOW: {
			ExecuteCommandText( text );
			break;
		}
		case CMD_EXEC_INSERT: {
			InsertCommandText( text );
			break;
		}
		case CMD_EXEC_APPEND: {
			AppendCommandText( text );
			break;
		}
		default: {
			common->FatalError( "hfCmdSystemLocal::BufferCommandText: bad exec type" );
		}
	}
}

/*
============
hfCmdSystemLocal::BufferCommandArgs
============
*/
void hfCmdSystemLocal::BufferCommandArgs( cmdExecution_t exec, const hfCmdArgs &args ) {
	switch ( exec ) {
		case CMD_EXEC_NOW: {
			ExecuteTokenizedString( args );
			break;
		}
		case CMD_EXEC_APPEND: {
			AppendCommandText( "_execTokenized\n" );
			tokenizedCmds.Append( args );
			break;
		}
		default: {
			common->FatalError( "hfCmdSystemLocal::BufferCommandArgs: bad exec type" );
		}
	}
}

/*
============
hfCmdSystemLocal::ExecuteCommandBuffer
============
*/
void hfCmdSystemLocal::ExecuteCommandBuffer() {
	int			i;
	char *		text;
	int			quotes;
	hfCmdArgs	args;

	while( textLength ) {

		if ( wait )	{
			// skip out while text still remains in buffer, leaving it for next frame
			wait--;
			break;
		}

		// find a \n or ; line break
		text = (char *)textBuf;

		quotes = 0;
		for ( i = 0; i < textLength; i++ ) {
			if ( text[i] == '"' ) {
				quotes++;
			}
			if ( !( quotes & 1 ) &&  text[i] == ';' ) {
				break;	// don't break if inside a quoted string
			}
			if ( text[i] == '\n' || text[i] == '\r' ) {
				break;
			}
		}
			
		text[i] = 0;

		if ( !hfStr::Cmp( text, "_execTokenized" ) ) {
			args = tokenizedCmds[ 0 ];
			tokenizedCmds.RemoveIndex( 0 );
		} else {
			args.TokenizeString( text, false );
		}

		// delete the text from the command buffer and move remaining commands down
		// this is necessary because commands (exec) can insert data at the
		// beginning of the text buffer

		if ( i == textLength ) {
			textLength = 0;
		} else {
			i++;
			textLength -= i;
			memmove( text, text+i, textLength );
		}

		// execute the command line that we have already tokenized
		ExecuteTokenizedString( args );
	}
}

/*
============
hfCmdSystemLocal::ArgCompletion_FolderExtension
============
*/
void hfCmdSystemLocal::ArgCompletion_FolderExtension( const hfCmdArgs &args, void(*callback)( const char *s ), const char *folder, bool stripFolder, ... ) {
	int i;
	hfStr string;
	const char *extension;
	va_list argPtr;

	string = args.Argv( 0 );
	string += " ";
	string += args.Argv( 1 );

	if ( string.Icmp( completionString ) != 0 ) {
		hfStr parm, path;
		hfFileList *names;

		completionString = string;
		completionParms.Clear();

		parm = args.Argv( 1 );
		parm.ExtractFilePath( path );
		if ( stripFolder || path.Length() == 0 ) {
			path = folder + path;
		}
		path.StripTrailing( '/' );

		// list folders
		names = fileSystem->ListFiles( path, "/", true, true );
		for ( i = 0; i < names->GetNumFiles(); i++ ) {
			hfStr name = names->GetFile( i );
			if ( stripFolder ) {
				name.Strip( folder );
			} else {
				name.Strip( "/" );
			}
			name = args.Argv( 0 ) + ( " " + name ) + "/";
			completionParms.Append( name );
		}
		fileSystem->FreeFileList( names );

		// list files
		va_start( argPtr, stripFolder );
		for ( extension = va_arg( argPtr, const char * ); extension; extension = va_arg( argPtr, const char * ) ) {
			names = fileSystem->ListFiles( path, extension, true, true );
			for ( i = 0; i < names->GetNumFiles(); i++ ) {
				hfStr name = names->GetFile( i );
				if ( stripFolder ) {
					name.Strip( folder );
				} else {
					name.Strip( "/" );
				}
				name = args.Argv( 0 ) + ( " " + name );
				completionParms.Append( name );
			}
			fileSystem->FreeFileList( names );
		}
		va_end( argPtr );
	}
	for ( i = 0; i < completionParms.Num(); i++ ) {
		callback( completionParms[i] );
	}
}

/*
============
hfCmdSystemLocal::ArgCompletion_DeclName
============
*/
void hfCmdSystemLocal::ArgCompletion_DeclName( const hfCmdArgs &args, void(*callback)( const char *s ), int type ) {
	int i, num;

	if ( declManager == NULL ) {
		return;
	}
	num = declManager->GetNumDecls( (declType_t)type );
	for ( i = 0; i < num; i++ ) {
		callback( hfStr( args.Argv( 0 ) ) + " " + declManager->DeclByIndex( (declType_t)type, i , false )->GetName() );
	}
}

/*
============
hfCmdSystemLocal::SetupReloadEngine
============
*/
void hfCmdSystemLocal::SetupReloadEngine( const hfCmdArgs &args ) {
	BufferCommandText( CMD_EXEC_APPEND, "reloadEngine\n" );
	postReload = args;
}

/*
============
hfCmdSystemLocal::PostReloadEngine
============
*/
bool hfCmdSystemLocal::PostReloadEngine() {
	if ( !postReload.Argc() ) {
		return false;
	}
	BufferCommandArgs( CMD_EXEC_APPEND, postReload );
	postReload.Clear();
	return true;
}

