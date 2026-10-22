#include "hfprecompiled.h"
#pragma hdrstop

//#define DEBUG_EVAL
#define MAX_DEFINEPARMS				128
#define DEFINEHASHSIZE				2048

#define TOKEN_FL_RECURSIVE_DEFINE	1

define_t * hfParser::globaldefines;

/*
================
hfParser::SetBaseFolder
================
*/
void hfParser::SetBaseFolder( const char *path) {
	hfLexer::SetBaseFolder(path);
}

/*
================
hfParser::AddGlobalDefine
================
*/
int hfParser::AddGlobalDefine( const char *string ) {
	define_t *define;

	define = hfParser::DefineFromString(string);
	if (!define) {
		return false;
	}
	define->next = globaldefines;
	globaldefines = define;
	return true;
}

/*
================
hfParser::RemoveGlobalDefine
================
*/
int hfParser::RemoveGlobalDefine( const char *name ) {
	define_t *d, *prev;

	for ( prev = NULL, d = hfParser::globaldefines; d; prev = d, d = d->next ) {
		if ( !strcmp( d->name, name ) ) {
			break;
		}
	}
	if ( d ) {
		if ( prev ) {
			prev->next = d->next;
		}
		else {
			hfParser::globaldefines = d->next;
		}
		hfParser::FreeDefine( d );
		return true;
	}
	return false;
}

/*
================
hfParser::RemoveAllGlobalDefines
================
*/
void hfParser::RemoveAllGlobalDefines() {
	define_t *define;

	for ( define = globaldefines; define; define = globaldefines ) {
		globaldefines = globaldefines->next;
		hfParser::FreeDefine(define);
	}
}


/*
===============================================================================

hfParser

===============================================================================
*/

/*
================
hfParser::PrintDefine
================
*/
void hfParser::PrintDefine( define_t *define ) {
	hfLib::common->Printf("define->name = %s\n", define->name);
	hfLib::common->Printf("define->flags = %d\n", define->flags);
	hfLib::common->Printf("define->builtin = %d\n", define->builtin);
	hfLib::common->Printf("define->numparms = %d\n", define->numparms);
}

/*
================
PC_PrintDefineHashTable
================
* /
static void PC_PrintDefineHashTable(define_t **definehash) {
	int i;
	define_t *d;

	for (i = 0; i < DEFINEHASHSIZE; i++) {
		Log_Write("%4d:", i);
		for (d = definehash[i]; d; d = d->hashnext) {
			Log_Write(" %s", d->name);
		}
		Log_Write("\n");
	}
}
*/

/*
================
PC_NameHash
================
*/
HF_INLINE int PC_NameHash( const char *name ) {
	int hash, i;

	hash = 0;
	for ( i = 0; name[i] != '\0'; i++ ) {
		hash += name[i] * (119 + i);
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20)) & (DEFINEHASHSIZE-1);
	return hash;
}

/*
================
hfParser::AddDefineToHash
================
*/
void hfParser::AddDefineToHash( define_t *define, define_t **definehash ) {
	int hash;

	hash = PC_NameHash(define->name);
	define->hashnext = definehash[hash];
	definehash[hash] = define;
}

/*
================
FindHashedDefine
================
*/
define_t *hfParser::FindHashedDefine( define_t **definehash, const char *name ) {
	define_t *d;
	int hash;

	hash = PC_NameHash(name);
	for ( d = definehash[hash]; d; d = d->hashnext ) {
		if ( !strcmp(d->name, name) ) {
			return d;
		}
	}
	return NULL;
}

/*
================
hfParser::FindDefine
================
*/
define_t *hfParser::FindDefine( define_t *defines, const char *name ) {
	define_t *d;

	for ( d = defines; d; d = d->next ) {
		if ( !strcmp(d->name, name) ) {
			return d;
		}
	}
	return NULL;
}

/*
================
hfParser::FindDefineParm
================
*/
int hfParser::FindDefineParm( define_t *define, const char *name ) {
	hfToken *p;
	int i;

	i = 0;
	for ( p = define->parms; p; p = p->next ) {
		if ( (*p) == name ) {
			return i;
		}
		i++;
	}
	return -1;
}

/*
================
hfParser::CopyDefine
================
*/
define_t *hfParser::CopyDefine( define_t *define ) {
	define_t *newdefine;
	hfToken *token, *newtoken, *lasttoken;

	newdefine = (define_t *) Mem_Alloc(sizeof(define_t) + strlen(define->name) + 1, TAG_IDLIB_PARSER);
	//copy the define name
	newdefine->name = (char *) newdefine + sizeof(define_t);
	strcpy(newdefine->name, define->name);
	newdefine->flags = define->flags;
	newdefine->builtin = define->builtin;
	newdefine->numparms = define->numparms;
	//the define is not linked
	newdefine->next = NULL;
	newdefine->hashnext = NULL;
	//copy the define tokens
	newdefine->tokens = NULL;
	for (lasttoken = NULL, token = define->tokens; token; token = token->next) {
		newtoken = new (TAG_IDLIB_PARSER) hfToken(token);
		newtoken->next = NULL;
		if (lasttoken) lasttoken->next = newtoken;
		else newdefine->tokens = newtoken;
		lasttoken = newtoken;
	}
	//copy the define parameters
	newdefine->parms = NULL;
	for (lasttoken = NULL, token = define->parms; token; token = token->next) {
		newtoken = new (TAG_IDLIB_PARSER) hfToken(token);
		newtoken->next = NULL;
		if (lasttoken) lasttoken->next = newtoken;
		else newdefine->parms = newtoken;
		lasttoken = newtoken;
	}
	return newdefine;
}

/*
================
hfParser::FreeDefine
================
*/
void hfParser::FreeDefine( define_t *define ) {
	hfToken *t, *next;

	//free the define parameters
	for (t = define->parms; t; t = next) {
		next = t->next;
		delete t;
	}
	//free the define tokens
	for (t = define->tokens; t; t = next) {
		next = t->next;
		delete t;
	}
	//free the define
	Mem_Free( define );
}

/*
================
hfParser::DefineFromString
================
*/
define_t *hfParser::DefineFromString( const char *string ) {
	hfParser src;
	define_t *def;

	if ( !src.LoadMemory(string, strlen(string), "*defineString") ) {
		return NULL;
	}
	// create a define from the source
	if ( !src.Directive_define() ) {
		src.FreeSource();
		return NULL;
	}
	def = src.CopyFirstDefine();
	src.FreeSource();
	//if the define was created succesfully
	return def;
}

/*
================
hfParser::Error
================
*/
void hfParser::Error( const char *str, ... ) const {
	char text[MAX_STRING_CHARS];
	va_list ap;

	va_start(ap, str);
	vsprintf(text, str, ap);
	va_end(ap);
	if ( hfParser::scriptstack ) {
		hfParser::scriptstack->Error( text );
	}
}

/*
================
hfParser::Warning
================
*/
void hfParser::Warning( const char *str, ... ) const {
	char text[MAX_STRING_CHARS];
	va_list ap;

	va_start(ap, str);
	vsprintf(text, str, ap);
	va_end(ap);
	if ( hfParser::scriptstack ) {
		hfParser::scriptstack->Warning( text );
	}
}

/*
================
hfParser::PushIndent
================
*/
void hfParser::PushIndent( int type, int skip ) {
	indent_t *indent;

	indent = (indent_t *) Mem_Alloc(sizeof(indent_t), TAG_IDLIB_PARSER);
	indent->type = type;
	indent->script = hfParser::scriptstack;
	indent->skip = (skip != 0);
	hfParser::skip += indent->skip;
	indent->next = hfParser::indentstack;
	hfParser::indentstack = indent;
}

/*
================
hfParser::PopIndent
================
*/
void hfParser::PopIndent( int *type, int *skip ) {
	indent_t *indent;

	*type = 0;
	*skip = 0;

	indent = hfParser::indentstack;
	if (!indent) return;

	// must be an indent from the current script
	if (hfParser::indentstack->script != hfParser::scriptstack) {
		return;
	}

	*type = indent->type;
	*skip = indent->skip;
	hfParser::indentstack = hfParser::indentstack->next;
	hfParser::skip -= indent->skip;
	Mem_Free( indent );
}

/*
================
hfParser::PushScript
================
*/
void hfParser::PushScript( hfLexer *script ) {
	hfLexer *s;

	for ( s = hfParser::scriptstack; s; s = s->next ) {
		if ( !hfStr::Icmp(s->GetFileName(), script->GetFileName()) ) {
			hfParser::Warning( "'%s' recursively included", script->GetFileName() );
			return;
		}
	}
	//push the script on the script stack
	script->next = hfParser::scriptstack;
	hfParser::scriptstack = script;
}

/*
================
hfParser::ReadSourceToken
================
*/
int hfParser::ReadSourceToken( hfToken *token ) {
	hfToken *t;
	hfLexer *script;
	int type, skip, changedScript;

	if ( !hfParser::scriptstack ) {
		hfLib::common->FatalError( "hfParser::ReadSourceToken: not loaded" );
		return false;
	}
	changedScript = 0;
	// if there's no token already available
	while( !hfParser::tokens ) {
		// if there's a token to read from the script
		if ( hfParser::scriptstack->ReadToken( token ) ) {
			token->linesCrossed += changedScript;

			// set the marker based on the start of the token read in
			if ( !marker_p ) {
				marker_p = token->whiteSpaceEnd_p;
			}
			return true;
		}
		// if at the end of the script
		if ( hfParser::scriptstack->EndOfFile() ) {
			// remove all indents of the script
			while( hfParser::indentstack && hfParser::indentstack->script == hfParser::scriptstack ) {
				hfParser::Warning( "missing #endif" );
				hfParser::PopIndent( &type, &skip );
			}
			changedScript = 1;
		}
		// if this was the initial script
		if ( !hfParser::scriptstack->next ) {
			return false;
		}
		// remove the script and return to the previous one
		script = hfParser::scriptstack;
		hfParser::scriptstack = hfParser::scriptstack->next;
		delete script;
	}
	// copy the already available token
	*token = *hfParser::tokens;
	// remove the token from the source
	t = hfParser::tokens;
	assert( hfParser::tokens != NULL );
	hfParser::tokens = hfParser::tokens->next;
	delete t;
	return true;
}

/*
================
hfParser::UnreadSourceToken
================
*/
int hfParser::UnreadSourceToken( hfToken *token ) {
	hfToken *t;

	t = new (TAG_IDLIB_PARSER) hfToken(token);
	t->next = hfParser::tokens;
	hfParser::tokens = t;
	return true;
}

/*
================
hfParser::ReadDefineParms
================
*/
int hfParser::ReadDefineParms( define_t *define, hfToken **parms, int maxparms ) {
	define_t *newdefine;
	hfToken token, *t, *last;
	int i, done, lastcomma, numparms, indent;

	if ( !hfParser::ReadSourceToken( &token ) ) {
		hfParser::Error( "define '%s' missing parameters", define->name );
		return false;
	}

	if ( define->numparms > maxparms ) {
		hfParser::Error( "define with more than %d parameters", maxparms );
		return false;
	}

	for ( i = 0; i < define->numparms; i++ ) {
		parms[i] = NULL;
	}
	// if no leading "("
	if ( token != "(" ) {
		hfParser::UnreadSourceToken( &token );
		hfParser::Error( "define '%s' missing parameters", define->name );
		return false;
	}
	// read the define parameters
	for ( done = 0, numparms = 0, indent = 1; !done; ) {
		if ( numparms >= maxparms ) {
			hfParser::Error( "define '%s' with too many parameters", define->name );
			return false;
		}
		parms[numparms] = NULL;
		lastcomma = 1;
		last = NULL;
		while( !done ) {

			if ( !hfParser::ReadSourceToken( &token ) ) {
				hfParser::Error( "define '%s' incomplete", define->name );
				return false;
			}

			if ( token == "," ) {
				if ( indent <= 1 ) {
					if ( lastcomma ) {
						hfParser::Warning( "too many comma's" );
					}
					if ( numparms >= define->numparms ) {
						hfParser::Warning( "too many define parameters" );
					}
					lastcomma = 1;
					break;
				}
			}
			else if ( token == "(" ) {
				indent++;
			}
			else if ( token == ")" ) {
				indent--;
				if ( indent <= 0 ) {
					if ( !parms[define->numparms-1] ) {
						hfParser::Warning( "too few define parameters" );
					}
					done = 1;
					break;
				}
			}
			else if ( token.type == TT_NAME ) {
				newdefine = FindHashedDefine( hfParser::definehash, token.c_str() );
				if ( newdefine ) {
					if ( !hfParser::ExpandDefineIntoSource( &token, newdefine ) ) {
						return false;
					}
					continue;
				}
			}

			lastcomma = 0;

			if ( numparms < define->numparms ) {

				t = new (TAG_IDLIB_PARSER) hfToken( token );
				t->next = NULL;
				if (last) last->next = t;
				else parms[numparms] = t;
				last = t;
			}
		}
		numparms++;
	}
	return true;
}

/*
================
hfParser::StringizeTokens
================
*/
int hfParser::StringizeTokens( hfToken *tokens, hfToken *token ) {
	hfToken *t;

	token->type = TT_STRING;
	token->whiteSpaceStart_p = NULL;
	token->whiteSpaceEnd_p = NULL;
	(*token) = "";
	for ( t = tokens; t; t = t->next ) {
		token->Append( t->c_str() );
	}
	return true;
}

/*
================
hfParser::MergeTokens
================
*/
int hfParser::MergeTokens( hfToken *t1, hfToken *t2 ) {
	// merging of a name with a name or number
	if ( t1->type == TT_NAME && (t2->type == TT_NAME || (t2->type == TT_NUMBER && !(t2->subtype & TT_FLOAT))) ) {
		t1->Append( t2->c_str() );
		return true;
	}
	// merging of two strings
	if (t1->type == TT_STRING && t2->type == TT_STRING) {
		t1->Append( t2->c_str() );
		return true;
	}
	// merging of two numbers
	if ( t1->type == TT_NUMBER && t2->type == TT_NUMBER &&
			!(t1->subtype & (TT_HEX|TT_BINARY)) && !(t2->subtype & (TT_HEX|TT_BINARY)) &&
			(!(t1->subtype & TT_FLOAT) || !(t2->subtype & TT_FLOAT)) ) {
		t1->Append( t2->c_str() );
		return true;
	}

	return false;
}

/*
================
hfParser::AddBuiltinDefines
================
*/
void hfParser::AddBuiltinDefines() {
	int i;
	define_t *define;
	struct builtin
	{
		char *string;
		int id;
	} builtin[] = {
		{ "__LINE__",	BUILTIN_LINE }, 
		{ "__FILE__",	BUILTIN_FILE },
		{ "__DATE__",	BUILTIN_DATE },
		{ "__TIME__",	BUILTIN_TIME },
		{ "__STDC__", BUILTIN_STDC },
		{ NULL, 0 }
	};

	for (i = 0; builtin[i].string; i++) {
		define = (define_t *) Mem_Alloc(sizeof(define_t) + strlen(builtin[i].string) + 1, TAG_IDLIB_PARSER);
		define->name = (char *) define + sizeof(define_t);
		strcpy(define->name, builtin[i].string);
		define->flags = DEFINE_FIXED;
		define->builtin = builtin[i].id;
		define->numparms = 0;
		define->parms = NULL;
		define->tokens = NULL;
		// add the define to the source
		AddDefineToHash(define, hfParser::definehash);
	}
}

/*
================
hfParser::CopyFirstDefine
================
*/
define_t *hfParser::CopyFirstDefine() {
	int i;

	for ( i = 0; i < DEFINEHASHSIZE; i++ ) {
		if ( hfParser::definehash[i] ) {
			return CopyDefine(hfParser::definehash[i]);
		}
	}
	return NULL;
}

static hfStr PreProcessorDate() {
	time_t t = time(NULL);
	char *curtime = ctime(&t);
	if ( hfStr::Length( curtime ) < 24 ) {
		return hfStr( "*** BAD CURTIME ***" );
	}
	hfStr	str = "\"";
	// skip DAY, extract MMM DD
	for ( int i = 4 ; i < 10 ; i++ ) { 
		str.Append( curtime[i] );
	}
	// skip time, extract space+YYYY
	for ( int i = 19 ; i < 24 ; i++ ) {
		str.Append( curtime[i] );
	}
	str.Append( "\"" );
	return str;
}

static hfStr PreProcessorTime() {
	time_t t = time(NULL);
	char *curtime = ctime(&t);
	if ( hfStr::Length( curtime ) < 24 ) {
		return hfStr( "*** BAD CURTIME ***" );
	}

	hfStr	str = "\"";
	for ( int i = 11 ; i < 19 ; i++ ) {
		str.Append( curtime[i] );
	}
	str.Append( "\"" );
	return str;
}

CONSOLE_COMMAND( TestPreprocessorMacros, "check analyze warning", 0 ) {
	hfLib::Printf( "%s : %s\n", __DATE__, PreProcessorDate().c_str() );
	hfLib::Printf( "%s : %s\n", __TIME__, PreProcessorTime().c_str() );
}

/*
================
hfParser::ExpandBuiltinDefine
================
*/
int hfParser::ExpandBuiltinDefine( hfToken *deftoken, define_t *define, hfToken **firsttoken, hfToken **lasttoken ) {
	hfToken *token;
	char buf[MAX_STRING_CHARS];

	token = new (TAG_IDLIB_PARSER) hfToken(deftoken);
	switch( define->builtin ) {
		case BUILTIN_LINE: {
			sprintf( buf, "%d", deftoken->line );
			(*token) = buf;
			token->intvalue = deftoken->line;
			token->floatvalue = deftoken->line;
			token->type = TT_NUMBER;
			token->subtype = TT_DECIMAL | TT_INTEGER | TT_VALUESVALID;
			token->line = deftoken->line;
			token->linesCrossed = deftoken->linesCrossed;
			token->flags = 0;
			*firsttoken = token;
			*lasttoken = token;
			break;
		}
		case BUILTIN_FILE: {
			(*token) = hfParser::scriptstack->GetFileName();
			token->type = TT_NAME;
			token->subtype = token->Length();
			token->line = deftoken->line;
			token->linesCrossed = deftoken->linesCrossed;
			token->flags = 0;
			*firsttoken = token;
			*lasttoken = token;
			break;
		}
		case BUILTIN_DATE: {
			*token = PreProcessorDate();
			token->type = TT_STRING;
			token->subtype = token->Length();
			token->line = deftoken->line;
			token->linesCrossed = deftoken->linesCrossed;
			token->flags = 0;
			*firsttoken = token;
			*lasttoken = token;
			break;
		}
		case BUILTIN_TIME: {
			*token = PreProcessorTime();
			token->type = TT_STRING;
			token->subtype = token->Length();
			token->line = deftoken->line;
			token->linesCrossed = deftoken->linesCrossed;
			token->flags = 0;
			*firsttoken = token;
			*lasttoken = token;
			break;
		}
		case BUILTIN_STDC: {
			hfParser::Warning( "__STDC__ not supported\n" );
			*firsttoken = NULL;
			*lasttoken = NULL;
			break;
		}
		default: {
			*firsttoken = NULL;
			*lasttoken = NULL;
			break;
		}
	}
	return true;
}

/*
================
hfParser::ExpandDefine
================
*/
int hfParser::ExpandDefine( hfToken *deftoken, define_t *define, hfToken **firsttoken, hfToken **lasttoken ) {
	hfToken *parms[MAX_DEFINEPARMS], *dt, *pt, *t;
	hfToken *t1, *t2, *first, *last, *nextpt, token;
	int parmnum, i;

	// if it is a builtin define
	if ( define->builtin ) {
		return hfParser::ExpandBuiltinDefine( deftoken, define, firsttoken, lasttoken );
	}
	// if the define has parameters
	if ( define->numparms ) {
		if ( !hfParser::ReadDefineParms( define, parms, MAX_DEFINEPARMS ) ) {
			return false;
		}
#ifdef DEBUG_EVAL
		for ( i = 0; i < define->numparms; i++ ) {
			Log_Write("define parms %d:", i);
			for ( pt = parms[i]; pt; pt = pt->next ) {
				Log_Write( "%s", pt->c_str() );
			}
		}
#endif //DEBUG_EVAL
	}
	// empty list at first
	first = NULL;
	last = NULL;
	// create a list with tokens of the expanded define
	for ( dt = define->tokens; dt; dt = dt->next ) {
		parmnum = -1;
		// if the token is a name, it could be a define parameter
		if ( dt->type == TT_NAME ) {
			parmnum = FindDefineParm( define, dt->c_str() );
		}
		// if it is a define parameter
		if ( parmnum >= 0 ) {
			for ( pt = parms[parmnum]; pt; pt = pt->next ) {
				t = new (TAG_IDLIB_PARSER) hfToken(pt);
				//add the token to the list
				t->next = NULL;
				if (last) last->next = t;
				else first = t;
				last = t;
			}
		}
		else {
			// if stringizing operator
			if ( (*dt) == "#" ) {
				// the stringizing operator must be followed by a define parameter
				if ( dt->next ) {
					parmnum = FindDefineParm( define, dt->next->c_str() );
				}
				else {
					parmnum = -1;
				}

				if ( parmnum >= 0 ) {
					// step over the stringizing operator
					dt = dt->next;
					// stringize the define parameter tokens
					if ( !hfParser::StringizeTokens( parms[parmnum], &token ) ) {
						hfParser::Error( "can't stringize tokens" );
						return false;
					}
					t = new (TAG_IDLIB_PARSER) hfToken(token);
					t->line = deftoken->line;
				}
				else {
					hfParser::Warning( "stringizing operator without define parameter" );
					continue;
				}
			}
			else {
				t = new (TAG_IDLIB_PARSER) hfToken(dt);
				t->line = deftoken->line;
			}
			// add the token to the list
			t->next = NULL;
// the token being read from the define list should use the line number of
// the original file, not the header file			
			t->line = deftoken->line;

			if ( last ) last->next = t;
			else first = t;
			last = t;
		}
	}
	// check for the merging operator
	for ( t = first; t; ) {
		if ( t->next ) {
			// if the merging operator
			if ( (*t->next) == "##" ) {
				t1 = t;
				t2 = t->next->next;
				if ( t2 ) {
					if ( !hfParser::MergeTokens( t1, t2 ) ) {
						hfParser::Error( "can't merge '%s' with '%s'", t1->c_str(), t2->c_str() );
						return false;
					}
					delete t1->next;
					t1->next = t2->next;
					if ( t2 == last ) last = t1;
					delete t2;
					continue;
				}
			}
		}
		t = t->next;
	}
	// store the first and last token of the list
	*firsttoken = first;
	*lasttoken = last;
	// free all the parameter tokens
	for ( i = 0; i < define->numparms; i++ ) {
		for ( pt = parms[i]; pt; pt = nextpt ) {
			nextpt = pt->next;
			delete pt;
		}
	}

	return true;
}

/*
================
hfParser::ExpandDefineIntoSource
================
*/
int hfParser::ExpandDefineIntoSource( hfToken *deftoken, define_t *define ) {
	hfToken *firsttoken, *lasttoken;

	if ( !hfParser::ExpandDefine( deftoken, define, &firsttoken, &lasttoken ) ) {
		return false;
	}
	// if the define is not empty
	if ( firsttoken && lasttoken ) {
		firsttoken->linesCrossed += deftoken->linesCrossed;
		lasttoken->next = hfParser::tokens;
		hfParser::tokens = firsttoken;
	}
	return true;
}

/*
================
hfParser::ReadLine

reads a token from the current line, continues reading on the next
line only if a backslash '\' is found
================
*/
int hfParser::ReadLine( hfToken *token ) {
	int crossline;

	crossline = 0;
	do {
		if (!hfParser::ReadSourceToken( token )) {
			return false;
		}
		
		if (token->linesCrossed > crossline) {
			hfParser::UnreadSourceToken( token );
			return false;
		}
		crossline = 1;
	} while( (*token) == "\\" );
	return true;
}

/*
================
hfParser::Directive_include
================
*/
int hfParser::Directive_include() {
	hfLexer *script;
	hfToken token;
	hfStr path;

	if ( !hfParser::ReadSourceToken( &token ) ) {
		hfParser::Error( "#include without file name" );
		return false;
	}
	if ( token.linesCrossed > 0 ) {
		hfParser::Error( "#include without file name" );
		return false;
	}
	if ( token.type == TT_STRING ) {
		script = new (TAG_IDLIB_PARSER) hfLexer;
		// try relative to the current file
		path = scriptstack->GetFileName();
		path.StripFilename();
		path += "/";
		path += token;
		if ( !script->LoadFile( path, OSPath ) ) {
			// try absolute path
			path = token;
			if ( !script->LoadFile( path, OSPath ) ) {
				// try from the include path
				path = includepath + token;
				if ( !script->LoadFile( path, OSPath ) ) {
					delete script;
					script = NULL;
				}
			}
		}
	}
	else if ( token.type == TT_PUNCTUATION && token == "<" ) {
		path = hfParser::includepath;
		while( hfParser::ReadSourceToken( &token ) ) {
			if ( token.linesCrossed > 0 ) {
				hfParser::UnreadSourceToken( &token );
				break;
			}
			if ( token.type == TT_PUNCTUATION && token == ">" ) {
				break;
			}
			path += token;
		}
		if ( token != ">" ) {
			hfParser::Warning( "#include missing trailing >" );
		}
		if ( !path.Length() ) {
			hfParser::Error( "#include without file name between < >" );
			return false;
		}
		if ( hfParser::flags & LEXFL_NOBASEINCLUDES ) {
			return true;
		}
		script = new (TAG_IDLIB_PARSER) hfLexer;
		if ( !script->LoadFile( includepath + path, OSPath ) ) {
			delete script;
			script = NULL;
		}
	}
	else {
		hfParser::Error( "#include without file name" );
		return false;
	}
	if (!script) {
		hfParser::Error( "file '%s' not found", path.c_str() );
		return false;
	}
	script->SetFlags( hfParser::flags );
	script->SetPunctuations( hfParser::punctuations );
	hfParser::PushScript( script );
	return true;
}

/*
================
hfParser::Directive_undef
================
*/
int hfParser::Directive_undef() {
	hfToken token;
	define_t *define, *lastdefine;
	int hash;

	//
	if (!hfParser::ReadLine( &token )) {
		hfParser::Error( "undef without name" );
		return false;
	}
	if (token.type != TT_NAME) {
		hfParser::UnreadSourceToken( &token );
		hfParser::Error( "expected name but found '%s'", token.c_str() );
		return false;
	}

	hash = PC_NameHash( token.c_str() );
	for (lastdefine = NULL, define = hfParser::definehash[hash]; define; define = define->hashnext) {
		if (!strcmp(define->name, token.c_str()))
		{
			if (define->flags & DEFINE_FIXED) {
				hfParser::Warning( "can't undef '%s'", token.c_str() );
			}
			else {
				if (lastdefine) {
					lastdefine->hashnext = define->hashnext;
				}
				else {
					hfParser::definehash[hash] = define->hashnext;
				}
				FreeDefine(define);
			}
			break;
		}
		lastdefine = define;
	}
	return true;
}

/*
================
hfParser::Directive_define
================
*/
int hfParser::Directive_define() {
	hfToken token, *t, *last;
	define_t *define;

	if (!hfParser::ReadLine( &token )) {
		hfParser::Error( "#define without name" );
		return false;
	}
	if (token.type != TT_NAME) {
		hfParser::UnreadSourceToken( &token );
		hfParser::Error( "expected name after #define, found '%s'", token.c_str() );
		return false;
	}
	// check if the define already exists
	define = FindHashedDefine(hfParser::definehash, token.c_str());
	if (define) {
		if (define->flags & DEFINE_FIXED) {
			hfParser::Error( "can't redefine '%s'", token.c_str() );
			return false;
		}
		hfParser::Warning( "redefinition of '%s'", token.c_str() );
		// unread the define name before executing the #undef directive
		hfParser::UnreadSourceToken( &token );
		if (!hfParser::Directive_undef())
			return false;
		// if the define was not removed (define->flags & DEFINE_FIXED)
		define = FindHashedDefine(hfParser::definehash, token.c_str());
	}
	// allocate define
	define = (define_t *) Mem_ClearedAlloc(sizeof(define_t) + token.Length() + 1, TAG_IDLIB_PARSER);
	define->name = (char *) define + sizeof(define_t);
	strcpy(define->name, token.c_str());
	// add the define to the source
	AddDefineToHash(define, hfParser::definehash);
	// if nothing is defined, just return
	if ( !hfParser::ReadLine( &token ) ) {
		return true;
	}
	// if it is a define with parameters
	if ( token.WhiteSpaceBeforeToken() == 0 && token == "(" ) {
		// read the define parameters
		last = NULL;
		if ( !hfParser::CheckTokenString(")") ) {
			while(1) {
				if ( !hfParser::ReadLine( &token ) ) {
					hfParser::Error( "expected define parameter" );
					return false;
				}
				// if it isn't a name
				if (token.type != TT_NAME) {
					hfParser::Error( "invalid define parameter" );
					return false;
				}

				if (FindDefineParm(define, token.c_str()) >= 0) {
					hfParser::Error( "two the same define parameters" );
					return false;
				}
				// add the define parm
				t = new (TAG_IDLIB_PARSER) hfToken(token);
				t->ClearTokenWhiteSpace();
				t->next = NULL;
				if (last) last->next = t;
				else define->parms = t;
				last = t;
				define->numparms++;
				// read next token
				if (!hfParser::ReadLine( &token )) {
					hfParser::Error( "define parameters not terminated" );
					return false;
				}

				if ( token == ")" ) {
					break;
				}
				// then it must be a comma
				if ( token != "," ) {
					hfParser::Error( "define not terminated" );
					return false;
				}
			}
		}
		if ( !hfParser::ReadLine( &token ) ) {
			return true;
		}
	}
	// read the defined stuff
	last = NULL;
	do
	{
		t = new (TAG_IDLIB_PARSER) hfToken(token);
		if ( t->type == TT_NAME && !strcmp( t->c_str(), define->name ) ) {
			t->flags |= TOKEN_FL_RECURSIVE_DEFINE;
			hfParser::Warning( "recursive define (removed recursion)" );
		}
		t->ClearTokenWhiteSpace();
		t->next = NULL;
		if ( last ) last->next = t;
		else define->tokens = t;
		last = t;
	} while( hfParser::ReadLine( &token ) );

	if ( last ) {
		// check for merge operators at the beginning or end
		if ( (*define->tokens) == "##" || (*last) == "##" ) {
			hfParser::Error( "define with misplaced ##" );
			return false;
		}
	}
	return true;
}

/*
================
hfParser::AddDefine
================
*/
int hfParser::AddDefine( const char *string ) {
	define_t *define;

	define = DefineFromString( string );
	if (!define) {
		return false;
	}
	AddDefineToHash(define, hfParser::definehash);
	return true;
}

/*
================
hfParser::AddGlobalDefinesToSource
================
*/
void hfParser::AddGlobalDefinesToSource() {
	define_t *define, *newdefine;

	for (define = globaldefines; define; define = define->next) {
		newdefine = CopyDefine( define );
		AddDefineToHash(newdefine, hfParser::definehash);
	}
}

/*
================
hfParser::Directive_if_def
================
*/
int hfParser::Directive_if_def( int type ) {
	hfToken token;
	define_t *d;
	int skip;

	if ( !hfParser::ReadLine( &token ) ) {
		hfParser::Error( "#ifdef without name" );
		return false;
	}
	if (token.type != TT_NAME) {
		hfParser::UnreadSourceToken( &token );
		hfParser::Error( "expected name after #ifdef, found '%s'", token.c_str() );
		return false;
	}
	d = FindHashedDefine(hfParser::definehash, token.c_str());
	skip = (type == INDENT_IFDEF) == (d == NULL);
	hfParser::PushIndent( type, skip );
	return true;
}

/*
================
hfParser::Directive_ifdef
================
*/
int hfParser::Directive_ifdef() {
	return hfParser::Directive_if_def( INDENT_IFDEF );
}

/*
================
hfParser::Directive_ifndef
================
*/
int hfParser::Directive_ifndef() {
	return hfParser::Directive_if_def( INDENT_IFNDEF );
}

/*
================
hfParser::Directive_else
================
*/
int hfParser::Directive_else() {
	int type, skip;

	hfParser::PopIndent( &type, &skip );
	if (!type) {
		hfParser::Error( "misplaced #else" );
		return false;
	}
	if (type == INDENT_ELSE) {
		hfParser::Error( "#else after #else" );
		return false;
	}
	hfParser::PushIndent( INDENT_ELSE, !skip );
	return true;
}

/*
================
hfParser::Directive_endif
================
*/
int hfParser::Directive_endif() {
	int type, skip;

	hfParser::PopIndent( &type, &skip );
	if (!type) {
		hfParser::Error( "misplaced #endif" );
		return false;
	}
	return true;
}

/*
================
hfParser::EvaluateTokens
================
*/
typedef struct operator_s
{
	int op;
	int priority;
	int parentheses;
	struct operator_s *prev, *next;
} operator_t;

typedef struct value_s
{
	signed long int intvalue;
	double floatvalue;
	int parentheses;
	struct value_s *prev, *next;
} value_t;

int PC_OperatorPriority(int op) {
	switch(op) {
		case P_MUL: return 15;
		case P_DIV: return 15;
		case P_MOD: return 15;
		case P_ADD: return 14;
		case P_SUB: return 14;

		case P_LOGIC_AND: return 7;
		case P_LOGIC_OR: return 6;
		case P_LOGIC_GEQ: return 12;
		case P_LOGIC_LEQ: return 12;
		case P_LOGIC_EQ: return 11;
		case P_LOGIC_UNEQ: return 11;

		case P_LOGIC_NOT: return 16;
		case P_LOGIC_GREATER: return 12;
		case P_LOGIC_LESS: return 12;

		case P_RSHIFT: return 13;
		case P_LSHIFT: return 13;

		case P_BIN_AND: return 10;
		case P_BIN_OR: return 8;
		case P_BIN_XOR: return 9;
		case P_BIN_NOT: return 16;

		case P_COLON: return 5;
		case P_QUESTIONMARK: return 5;
	}
	return false;
}

//#define AllocValue()			GetClearedMemory(sizeof(value_t));
//#define FreeValue(val)		FreeMemory(val)
//#define AllocOperator(op)		op = (operator_t *) GetClearedMemory(sizeof(operator_t));
//#define FreeOperator(op)		FreeMemory(op);

#define MAX_VALUES		64
#define MAX_OPERATORS	64

#define AllocValue(val)									\
	if ( numvalues >= MAX_VALUES ) {					\
		hfParser::Error( "out of value space\n" );		\
		error = 1;										\
		break;											\
	}													\
	else {												\
		val = &value_heap[numvalues++];					\
	}

#define FreeValue(val)

#define AllocOperator(op)								\
	if ( numoperators >= MAX_OPERATORS ) {				\
		hfParser::Error( "out of operator space\n" );	\
		error = 1;										\
		break;											\
	}													\
	else {												\
		op = &operator_heap[numoperators++];			\
	}

#define FreeOperator(op)

int hfParser::EvaluateTokens( hfToken *tokens, signed long int *intvalue, double *floatvalue, int integer ) {
	operator_t *o, *firstoperator, *lastoperator;
	value_t *v, *firstvalue, *lastvalue, *v1, *v2;
	hfToken *t;
	int brace = 0;
	int parentheses = 0;
	int error = 0;
	int lastwasvalue = 0;
	int negativevalue = 0;
	int questmarkintvalue = 0;
	double questmarkfloatvalue = 0;
	int gotquestmarkvalue = false;
	int lastoperatortype = 0;
	//
	operator_t operator_heap[MAX_OPERATORS];
	int numoperators = 0;
	value_t value_heap[MAX_VALUES];
	int numvalues = 0;

	firstoperator = lastoperator = NULL;
	firstvalue = lastvalue = NULL;
	if (intvalue) *intvalue = 0;
	if (floatvalue) *floatvalue = 0;
	for ( t = tokens; t; t = t->next ) {
		switch( t->type ) {
			case TT_NAME:
			{
				if ( lastwasvalue || negativevalue ) {
					hfParser::Error( "syntax error in #if/#elif" );
					error = 1;
					break;
				}
				if ( (*t) != "defined" ) {
					hfParser::Error( "undefined name '%s' in #if/#elif", t->c_str() );
					error = 1;
					break;
				}
				t = t->next;
				if ( (*t) == "(" ) {
					brace = true;
					t = t->next;
				}
				if (!t || t->type != TT_NAME) {
					hfParser::Error( "defined() without name in #if/#elif" );
					error = 1;
					break;
				}
				//v = (value_t *) GetClearedMemory(sizeof(value_t));
				AllocValue(v);
				if (FindHashedDefine(hfParser::definehash, t->c_str())) {
					v->intvalue = 1;
					v->floatvalue = 1;
				}
				else {
					v->intvalue = 0;
					v->floatvalue = 0;
				}
				v->parentheses = parentheses;
				v->next = NULL;
				v->prev = lastvalue;
				if (lastvalue) lastvalue->next = v;
				else firstvalue = v;
				lastvalue = v;
				if (brace) {
					t = t->next;
					if (!t || (*t) != ")" ) {
						hfParser::Error( "defined missing ) in #if/#elif" );
						error = 1;
						break;
					}
				}
				brace = false;
				// defined() creates a value
				lastwasvalue = 1;
				break;
			}
			case TT_NUMBER:
			{
				if (lastwasvalue) {
					hfParser::Error( "syntax error in #if/#elif" );
					error = 1;
					break;
				}
				//v = (value_t *) GetClearedMemory(sizeof(value_t));
				AllocValue(v);
				if (negativevalue) {
					v->intvalue = - t->GetIntValue();
					v->floatvalue = - t->GetFloatValue();
				}
				else {
					v->intvalue = t->GetIntValue();
					v->floatvalue = t->GetFloatValue();
				}
				v->parentheses = parentheses;
				v->next = NULL;
				v->prev = lastvalue;
				if (lastvalue) lastvalue->next = v;
				else firstvalue = v;
				lastvalue = v;
				//last token was a value
				lastwasvalue = 1;
				//
				negativevalue = 0;
				break;
			}
			case TT_PUNCTUATION:
			{
				if (negativevalue) {
					hfParser::Error( "misplaced minus sign in #if/#elif" );
					error = 1;
					break;
				}
				if (t->subtype == P_PARENTHESESOPEN) {
					parentheses++;
					break;
				}
				else if (t->subtype == P_PARENTHESESCLOSE) {
					parentheses--;
					if (parentheses < 0) {
						hfParser::Error( "too many ) in #if/#elsif" );
						error = 1;
					}
					break;
				}
				//check for invalid operators on floating point values
				if ( !integer ) {
					if (t->subtype == P_BIN_NOT || t->subtype == P_MOD ||
						t->subtype == P_RSHIFT || t->subtype == P_LSHIFT ||
						t->subtype == P_BIN_AND || t->subtype == P_BIN_OR ||
						t->subtype == P_BIN_XOR) {
						hfParser::Error( "illigal operator '%s' on floating point operands\n", t->c_str() );
						error = 1;
						break;
					}
				}
				switch( t->subtype ) {
					case P_LOGIC_NOT:
					case P_BIN_NOT:
					{
						if (lastwasvalue) {
							hfParser::Error( "! or ~ after value in #if/#elif" );
							error = 1;
							break;
						}
						break;
					}
					case P_INC:
					case P_DEC:
					{
						hfParser::Error( "++ or -- used in #if/#elif" );
						break;
					}
					case P_SUB:
					{
						if (!lastwasvalue) {
							negativevalue = 1;
							break;
						}
					}
					
					case P_MUL:
					case P_DIV:
					case P_MOD:
					case P_ADD:

					case P_LOGIC_AND:
					case P_LOGIC_OR:
					case P_LOGIC_GEQ:
					case P_LOGIC_LEQ:
					case P_LOGIC_EQ:
					case P_LOGIC_UNEQ:

					case P_LOGIC_GREATER:
					case P_LOGIC_LESS:

					case P_RSHIFT:
					case P_LSHIFT:

					case P_BIN_AND:
					case P_BIN_OR:
					case P_BIN_XOR:

					case P_COLON:
					case P_QUESTIONMARK:
					{
						if (!lastwasvalue) {
							hfParser::Error( "operator '%s' after operator in #if/#elif", t->c_str() );
							error = 1;
							break;
						}
						break;
					}
					default:
					{
						hfParser::Error( "invalid operator '%s' in #if/#elif", t->c_str() );
						error = 1;
						break;
					}
				}
				if (!error && !negativevalue) {
					//o = (operator_t *) GetClearedMemory(sizeof(operator_t));
					AllocOperator(o);
					o->op = t->subtype;
					o->priority = PC_OperatorPriority(t->subtype);
					o->parentheses = parentheses;
					o->next = NULL;
					o->prev = lastoperator;
					if (lastoperator) lastoperator->next = o;
					else firstoperator = o;
					lastoperator = o;
					lastwasvalue = 0;
				}
				break;
			}
			default:
			{
				hfParser::Error( "unknown '%s' in #if/#elif", t->c_str() );
				error = 1;
				break;
			}
		}
		if (error) {
			break;
		}
	}
	if (!error) {
		if (!lastwasvalue) {
			hfParser::Error( "trailing operator in #if/#elif" );
			error = 1;
		}
		else if (parentheses) {
			hfParser::Error( "too many ( in #if/#elif" );
			error = 1;
		}
	}
	//
	gotquestmarkvalue = false;
	questmarkintvalue = 0;
	questmarkfloatvalue = 0;
	//while there are operators
	while( !error && firstoperator ) {
		v = firstvalue;
		for (o = firstoperator; o->next; o = o->next) {
			//if the current operator is nested deeper in parentheses
			//than the next operator
			if (o->parentheses > o->next->parentheses) {
				break;
			}
			//if the current and next operator are nested equally deep in parentheses
			if (o->parentheses == o->next->parentheses) {
				//if the priority of the current operator is equal or higher
				//than the priority of the next operator
				if (o->priority >= o->next->priority) {
					break;
				}
			}
			//if the arity of the operator isn't equal to 1
			if (o->op != P_LOGIC_NOT && o->op != P_BIN_NOT) {
				v = v->next;
			}
			//if there's no value or no next value
			if (!v) {
				hfParser::Error( "mising values in #if/#elif" );
				error = 1;
				break;
			}
		}
		if (error) {
			break;
		}
		v1 = v;
		v2 = v->next;
#ifdef DEBUG_EVAL
		if (integer) {
			Log_Write("operator %s, value1 = %d", hfParser::scriptstack->getPunctuationFromId(o->op), v1->intvalue);
			if (v2) Log_Write("value2 = %d", v2->intvalue);
		}
		else {
			Log_Write("operator %s, value1 = %f", hfParser::scriptstack->getPunctuationFromId(o->op), v1->floatvalue);
			if (v2) Log_Write("value2 = %f", v2->floatvalue);
		}
#endif //DEBUG_EVAL
		switch(o->op) {
			case P_LOGIC_NOT:		v1->intvalue = !v1->intvalue;
									v1->floatvalue = !v1->floatvalue; break;
			case P_BIN_NOT:			v1->intvalue = ~v1->intvalue;
									break;
			case P_MUL:				v1->intvalue *= v2->intvalue;
									v1->floatvalue *= v2->floatvalue; break;
			case P_DIV:				if (!v2->intvalue || !v2->floatvalue)
									{
										hfParser::Error( "divide by zero in #if/#elif\n" );
										error = 1;
										break;
									}
									v1->intvalue /= v2->intvalue;
									v1->floatvalue /= v2->floatvalue; break;
			case P_MOD:				if (!v2->intvalue)
									{
										hfParser::Error( "divide by zero in #if/#elif\n" );
										error = 1;
										break;
									}
									v1->intvalue %= v2->intvalue; break;
			case P_ADD:				v1->intvalue += v2->intvalue;
									v1->floatvalue += v2->floatvalue; break;
			case P_SUB:				v1->intvalue -= v2->intvalue;
									v1->floatvalue -= v2->floatvalue; break;
			case P_LOGIC_AND:		v1->intvalue = v1->intvalue && v2->intvalue;
									v1->floatvalue = v1->floatvalue && v2->floatvalue; break;
			case P_LOGIC_OR:		v1->intvalue = v1->intvalue || v2->intvalue;
									v1->floatvalue = v1->floatvalue || v2->floatvalue; break;
			case P_LOGIC_GEQ:		v1->intvalue = v1->intvalue >= v2->intvalue;
									v1->floatvalue = v1->floatvalue >= v2->floatvalue; break;
			case P_LOGIC_LEQ:		v1->intvalue = v1->intvalue <= v2->intvalue;
									v1->floatvalue = v1->floatvalue <= v2->floatvalue; break;
			case P_LOGIC_EQ:		v1->intvalue = v1->intvalue == v2->intvalue;
									v1->floatvalue = v1->floatvalue == v2->floatvalue; break;
			case P_LOGIC_UNEQ:		v1->intvalue = v1->intvalue != v2->intvalue;
									v1->floatvalue = v1->floatvalue != v2->floatvalue; break;
			case P_LOGIC_GREATER:	v1->intvalue = v1->intvalue > v2->intvalue;
									v1->floatvalue = v1->floatvalue > v2->floatvalue; break;
			case P_LOGIC_LESS:		v1->intvalue = v1->intvalue < v2->intvalue;
									v1->floatvalue = v1->floatvalue < v2->floatvalue; break;
			case P_RSHIFT:			v1->intvalue >>= v2->intvalue;
									break;
			case P_LSHIFT:			v1->intvalue <<= v2->intvalue;
									break;
			case P_BIN_AND:			v1->intvalue &= v2->intvalue;
									break;
			case P_BIN_OR:			v1->intvalue |= v2->intvalue;
									break;
			case P_BIN_XOR:			v1->intvalue ^= v2->intvalue;
									break;
			case P_COLON:
			{
				if (!gotquestmarkvalue) {
					hfParser::Error( ": without ? in #if/#elif" );
					error = 1;
					break;
				}
				if (integer) {
					if (!questmarkintvalue)
						v1->intvalue = v2->intvalue;
				}
				else {
					if (!questmarkfloatvalue)
						v1->floatvalue = v2->floatvalue;
				}
				gotquestmarkvalue = false;
				break;
			}
			case P_QUESTIONMARK:
			{
				if (gotquestmarkvalue) {
					hfParser::Error( "? after ? in #if/#elif" );
					error = 1;
					break;
				}
				questmarkintvalue = v1->intvalue;
				questmarkfloatvalue = v1->floatvalue;
				gotquestmarkvalue = true;
				break;
			}
		}
#ifdef DEBUG_EVAL
		if (integer) Log_Write("result value = %d", v1->intvalue);
		else Log_Write("result value = %f", v1->floatvalue);
#endif //DEBUG_EVAL
		if (error)
			break;
		lastoperatortype = o->op;
		//if not an operator with arity 1
		if (o->op != P_LOGIC_NOT && o->op != P_BIN_NOT) {
			//remove the second value if not question mark operator
			if (o->op != P_QUESTIONMARK) {
				v = v->next;
			}
			//
			if (v->prev) v->prev->next = v->next;
			else firstvalue = v->next;
			if (v->next) v->next->prev = v->prev;
			else lastvalue = v->prev;
			//FreeMemory(v);
			FreeValue(v);
		}
		//remove the operator
		if (o->prev) o->prev->next = o->next;
		else firstoperator = o->next;
		if (o->next) o->next->prev = o->prev;
		else lastoperator = o->prev;
		//FreeMemory(o);
		FreeOperator(o);
	}
	if (firstvalue) {
		if (intvalue) *intvalue = firstvalue->intvalue;
		if (floatvalue) *floatvalue = firstvalue->floatvalue;
	}
	for (o = firstoperator; o; o = lastoperator) {
		lastoperator = o->next;
		//FreeMemory(o);
		FreeOperator(o);
	}
	for (v = firstvalue; v; v = lastvalue) {
		lastvalue = v->next;
		//FreeMemory(v);
		FreeValue(v);
	}
	if (!error) {
		return true;
	}
	if (intvalue) {
		*intvalue = 0;
	}
	if (floatvalue) {
		*floatvalue = 0;
	}
	return false;
}

/*
================
hfParser::Evaluate
================
*/
int hfParser::Evaluate( signed long int *intvalue, double *floatvalue, int integer ) {
	hfToken token, *firsttoken, *lasttoken;
	hfToken *t, *nexttoken;
	define_t *define;
	int defined = false;

	if (intvalue) {
		*intvalue = 0;
	}
	if (floatvalue) {
		*floatvalue = 0;
	}
	//
	if ( !hfParser::ReadLine( &token ) ) {
		hfParser::Error( "no value after #if/#elif" );
		return false;
	}
	firsttoken = NULL;
	lasttoken = NULL;
	do {
		//if the token is a name
		if (token.type == TT_NAME) {
			if (defined) {
				defined = false;
				t = new (TAG_IDLIB_PARSER) hfToken(token);
				t->next = NULL;
				if (lasttoken) lasttoken->next = t;
				else firsttoken = t;
				lasttoken = t;
			}
			else if ( token == "defined" ) {
				defined = true;
				t = new (TAG_IDLIB_PARSER) hfToken(token);
				t->next = NULL;
				if (lasttoken) lasttoken->next = t;
				else firsttoken = t;
				lasttoken = t;
			}
			else {
				//then it must be a define
				define = FindHashedDefine(hfParser::definehash, token.c_str());
				if (!define) {
					hfParser::Error( "can't Evaluate '%s', not defined", token.c_str() );
					return false;
				}
				if ( !hfParser::ExpandDefineIntoSource( &token, define ) ) {
					return false;
				}
			}
		}
		//if the token is a number or a punctuation
		else if (token.type == TT_NUMBER || token.type == TT_PUNCTUATION) {
			t = new (TAG_IDLIB_PARSER) hfToken(token);
			t->next = NULL;
			if (lasttoken) lasttoken->next = t;
			else firsttoken = t;
			lasttoken = t;
		}
		else {
			hfParser::Error( "can't Evaluate '%s'", token.c_str() );
			return false;
		}
	} while(hfParser::ReadLine( &token ));
	//
	if ( !hfParser::EvaluateTokens( firsttoken, intvalue, floatvalue, integer ) ) {
		return false;
	}
	//
#ifdef DEBUG_EVAL
	Log_Write("eval:");
#endif //DEBUG_EVAL
	for (t = firsttoken; t; t = nexttoken) {
#ifdef DEBUG_EVAL
		Log_Write(" %s", t->c_str());
#endif //DEBUG_EVAL
		nexttoken = t->next;
		delete t;
	} //end for
#ifdef DEBUG_EVAL
	if (integer) Log_Write("eval result: %d", *intvalue);
	else Log_Write("eval result: %f", *floatvalue);
#endif //DEBUG_EVAL
	//
	return true;
}

/*
================
hfParser::DollarEvaluate
================
*/
int hfParser::DollarEvaluate( signed long int *intvalue, double *floatvalue, int integer) {
	int indent, defined = false;
	hfToken token, *firsttoken, *lasttoken;
	hfToken *t, *nexttoken;
	define_t *define;

	if (intvalue) {
		*intvalue = 0;
	}
	if (floatvalue) {
		*floatvalue = 0;
	}
	//
	if ( !hfParser::ReadSourceToken( &token ) ) {
		hfParser::Error( "no leading ( after $evalint/$evalfloat" );
		return false;
	}
	if ( !hfParser::ReadSourceToken( &token ) ) {
		hfParser::Error( "nothing to Evaluate" );
		return false;
	}
	indent = 1;
	firsttoken = NULL;
	lasttoken = NULL;
	do {
		//if the token is a name
		if (token.type == TT_NAME) {
			if (defined) {
				defined = false;
				t = new (TAG_IDLIB_PARSER) hfToken(token);
				t->next = NULL;
				if (lasttoken) lasttoken->next = t;
				else firsttoken = t;
				lasttoken = t;
			}
			else if ( token == "defined" ) {
				defined = true;
				t = new (TAG_IDLIB_PARSER) hfToken(token);
				t->next = NULL;
				if (lasttoken) lasttoken->next = t;
				else firsttoken = t;
				lasttoken = t;
			}
			else {
				//then it must be a define
				define = FindHashedDefine(hfParser::definehash, token.c_str());
				if (!define) {
					hfParser::Warning( "can't Evaluate '%s', not defined", token.c_str() );
					return false;
				}
				if ( !hfParser::ExpandDefineIntoSource( &token, define ) ) {
					return false;
				}
			}
		}
		//if the token is a number or a punctuation
		else if (token.type == TT_NUMBER || token.type == TT_PUNCTUATION) {
			if ( token[0] == '(' ) indent++;
			else if ( token[0] == ')' ) indent--;
			if (indent <= 0) {
				break;
			}
			t = new (TAG_IDLIB_PARSER) hfToken(token);
			t->next = NULL;
			if (lasttoken) lasttoken->next = t;
			else firsttoken = t;
			lasttoken = t;
		}
		else {
			hfParser::Error( "can't Evaluate '%s'", token.c_str() );
			return false;
		}
	} while(hfParser::ReadSourceToken( &token ));
	//
	if (!hfParser::EvaluateTokens( firsttoken, intvalue, floatvalue, integer)) {
		return false;
	}
	//
#ifdef DEBUG_EVAL
	Log_Write("$eval:");
#endif //DEBUG_EVAL
	for (t = firsttoken; t; t = nexttoken) {
#ifdef DEBUG_EVAL
		Log_Write(" %s", t->c_str());
#endif //DEBUG_EVAL
		nexttoken = t->next;
		delete t;
	} //end for
#ifdef DEBUG_EVAL
	if (integer) Log_Write("$eval result: %d", *intvalue);
	else Log_Write("$eval result: %f", *floatvalue);
#endif //DEBUG_EVAL
	//
	return true;
}

/*
================
hfParser::Directive_elif
================
*/
int hfParser::Directive_elif() {
	signed long int value;
	int type, skip;

	hfParser::PopIndent( &type, &skip );
	if (!type || type == INDENT_ELSE) {
		hfParser::Error( "misplaced #elif" );
		return false;
	}
	if ( !hfParser::Evaluate( &value, NULL, true ) ) {
		return false;
	}
	skip = (value == 0);
	hfParser::PushIndent( INDENT_ELIF, skip );
	return true;
}

/*
================
hfParser::Directive_if
================
*/
int hfParser::Directive_if() {
	signed long int value;
	int skip;

	if ( !hfParser::Evaluate( &value, NULL, true ) ) {
		return false;
	}
	skip = (value == 0);
	hfParser::PushIndent( INDENT_IF, skip );
	return true;
}

/*
================
hfParser::Directive_line
================
*/
int hfParser::Directive_line() {
	hfToken token;

	hfParser::Error( "#line directive not supported" );
	while( hfParser::ReadLine( &token ) ) {
	}
	return true;
}

/*
================
hfParser::Directive_error
================
*/
int hfParser::Directive_error() {
	hfToken token;

	if ( !hfParser::ReadLine( &token) || token.type != TT_STRING ) {
		hfParser::Error( "#error without string" );
		return false;
	}
	hfParser::Error( "#error: %s", token.c_str() );
	return true;
}

/*
================
hfParser::Directive_warning
================
*/
int hfParser::Directive_warning() {
	hfToken token;

	if ( !hfParser::ReadLine( &token) || token.type != TT_STRING ) {
		hfParser::Warning( "#warning without string" );
		return false;
	}
	hfParser::Warning( "#warning: %s", token.c_str() );
	return true;
}

/*
================
hfParser::Directive_pragma
================
*/
int hfParser::Directive_pragma() {
	hfToken token;

	hfParser::Warning( "#pragma directive not supported" );
	while( hfParser::ReadLine( &token ) ) {
	}
	return true;
}

/*
================
hfParser::UnreadSignToken
================
*/
void hfParser::UnreadSignToken() {
	hfToken token;

	token.line = hfParser::scriptstack->GetLineNum();
	token.whiteSpaceStart_p = NULL;
	token.whiteSpaceEnd_p = NULL;
	token.linesCrossed = 0;
	token.flags = 0;
	token = "-";
	token.type = TT_PUNCTUATION;
	token.subtype = P_SUB;
	hfParser::UnreadSourceToken( &token );
}

/*
================
hfParser::Directive_eval
================
*/
int hfParser::Directive_eval() {
	signed long int value;
	hfToken token;
	char buf[128];

	if ( !hfParser::Evaluate( &value, NULL, true ) ) {
		return false;
	}

	token.line = hfParser::scriptstack->GetLineNum();
	token.whiteSpaceStart_p = NULL;
	token.whiteSpaceEnd_p = NULL;
	token.linesCrossed = 0;
	token.flags = 0;
	sprintf(buf, "%d", abs(value));
	token = buf;
	token.type = TT_NUMBER;
	token.subtype = TT_INTEGER|TT_LONG|TT_DECIMAL;
	hfParser::UnreadSourceToken( &token );
	if ( value < 0 ) {
		hfParser::UnreadSignToken();
	}
	return true;
}

/*
================
hfParser::Directive_evalfloat
================
*/
int hfParser::Directive_evalfloat() {
	double value;
	hfToken token;
	char buf[128];

	if ( !hfParser::Evaluate( NULL, &value, false ) ) {
		return false;
	}

	token.line = hfParser::scriptstack->GetLineNum();
	token.whiteSpaceStart_p = NULL;
	token.whiteSpaceEnd_p = NULL;
	token.linesCrossed = 0;
	token.flags = 0;
	sprintf(buf, "%1.2f", hfMath::Fabs(value));
	token = buf;
	token.type = TT_NUMBER;
	token.subtype = TT_FLOAT|TT_LONG|TT_DECIMAL;
	hfParser::UnreadSourceToken( &token );
	if (value < 0) {
		hfParser::UnreadSignToken();
	}
	return true;
}

/*
================
hfParser::ReadDirective
================
*/
int hfParser::ReadDirective() {
	hfToken token;

	//read the directive name
	if ( !hfParser::ReadSourceToken( &token ) ) {
		hfParser::Error( "found '#' without name" );
		return false;
	}
	//directive name must be on the same line
	if (token.linesCrossed > 0) {
		hfParser::UnreadSourceToken( &token );
		hfParser::Error( "found '#' at end of line" );
		return false;
	}
	//if if is a name
	if (token.type == TT_NAME) {
		if ( token == "if" ) {
			return hfParser::Directive_if();
		}
		else if ( token == "ifdef" ) {
			return hfParser::Directive_ifdef();
		}
		else if ( token == "ifndef" ) {
			return hfParser::Directive_ifndef();
		}
		else if ( token == "elif" ) {
			return hfParser::Directive_elif();
		}
		else if ( token == "else" ) {
			return hfParser::Directive_else();
		}
		else if ( token == "endif" ) {
			return hfParser::Directive_endif();
		}
		else if (hfParser::skip > 0) {
			// skip the rest of the line
			while( hfParser::ReadLine( &token ) ) {
			}
			return true;
		}
		else {
			if ( token == "include" ) {
				return hfParser::Directive_include();
			}
			else if ( token == "define" ) {
				return hfParser::Directive_define();
			}
			else if ( token == "undef" ) {
				return hfParser::Directive_undef();
			}
			else if ( token == "line" ) {
				return hfParser::Directive_line();
			}
			else if ( token == "error" ) {
				return hfParser::Directive_error();
			}
			else if ( token == "warning" ) {
				return hfParser::Directive_warning();
			}
			else if ( token == "pragma" ) {
				return hfParser::Directive_pragma();
			}
			else if ( token == "eval" ) {
				return hfParser::Directive_eval();
			}
			else if ( token == "evalfloat" ) {
				return hfParser::Directive_evalfloat();
			}
		}
	}
	hfParser::Error( "unknown precompiler directive '%s'", token.c_str() );
	return false;
}

/*
================
hfParser::DollarDirective_evalint
================
*/
int hfParser::DollarDirective_evalint() {
	signed long int value;
	hfToken token;
	char buf[128];

	if ( !hfParser::DollarEvaluate( &value, NULL, true ) ) {
		return false;
	}

	token.line = hfParser::scriptstack->GetLineNum();
	token.whiteSpaceStart_p = NULL;
	token.whiteSpaceEnd_p = NULL;
	token.linesCrossed = 0;
	token.flags = 0;
	sprintf( buf, "%d", abs( value ) );
	token = buf;
	token.type = TT_NUMBER;
	token.subtype = TT_INTEGER | TT_LONG | TT_DECIMAL | TT_VALUESVALID;
	token.intvalue = abs( value );
	token.floatvalue = abs( value );
	hfParser::UnreadSourceToken( &token );
	if ( value < 0 ) {
		hfParser::UnreadSignToken();
	}
	return true;
}

/*
================
hfParser::DollarDirective_evalfloat
================
*/
int hfParser::DollarDirective_evalfloat() {
	double value;
	hfToken token;
	char buf[128];

	if ( !hfParser::DollarEvaluate( NULL, &value, false ) ) {
		return false;
	}

	token.line = hfParser::scriptstack->GetLineNum();
	token.whiteSpaceStart_p = NULL;
	token.whiteSpaceEnd_p = NULL;
	token.linesCrossed = 0;
	token.flags = 0;
	sprintf( buf, "%1.2f", fabs( value ) );
	token = buf;
	token.type = TT_NUMBER;
	token.subtype = TT_FLOAT | TT_LONG | TT_DECIMAL | TT_VALUESVALID;
	token.intvalue = (unsigned long) fabs( value );
	token.floatvalue = fabs( value );
	hfParser::UnreadSourceToken( &token );
	if ( value < 0 ) {
		hfParser::UnreadSignToken();
	}
	return true;
}

/*
================
hfParser::ReadDollarDirective
================
*/
int hfParser::ReadDollarDirective() {
	hfToken token;

	// read the directive name
	if ( !hfParser::ReadSourceToken( &token ) ) {
		hfParser::Error( "found '$' without name" );
		return false;
	}
	// directive name must be on the same line
	if ( token.linesCrossed > 0 ) {
		hfParser::UnreadSourceToken( &token );
		hfParser::Error( "found '$' at end of line" );
		return false;
	}
	// if if is a name
	if (token.type == TT_NAME) {
		if ( token == "evalint" ) {
			return hfParser::DollarDirective_evalint();
		}
		else if ( token == "evalfloat" ) {
			return hfParser::DollarDirective_evalfloat();
		}
	}
	hfParser::UnreadSourceToken( &token );
	return false;
}

/*
================
hfParser::ReadToken
================
*/
int hfParser::ReadToken( hfToken *token ) {
	define_t *define;

	while(1) {
		if ( !hfParser::ReadSourceToken( token ) ) {
			return false;
		}
		// check for precompiler directives
		if ( token->type == TT_PUNCTUATION && (*token)[0] == '#' && (*token)[1] == '\0' ) {
			// read the precompiler directive
			if ( !hfParser::ReadDirective() ) {
				return false;
			}
			continue;
		}
		// if skipping source because of conditional compilation
		if ( hfParser::skip ) {
			continue;
		}
		// recursively concatenate strings that are behind each other still resolving defines
		if ( token->type == TT_STRING && !(hfParser::scriptstack->GetFlags() & LEXFL_NOSTRINGCONCAT) ) {
			hfToken newtoken;
			if ( hfParser::ReadToken( &newtoken ) ) {
				if ( newtoken.type == TT_STRING ) {
					token->Append( newtoken.c_str() );
				}
				else {
					hfParser::UnreadSourceToken( &newtoken );
				}
			}
		}
		//
		if ( !(hfParser::scriptstack->GetFlags() & LEXFL_NODOLLARPRECOMPILE) ) {
			// check for special precompiler directives
			if ( token->type == TT_PUNCTUATION && (*token)[0] == '$' && (*token)[1] == '\0' ) {
				// read the precompiler directive
				if ( hfParser::ReadDollarDirective() ) {
					continue;
				}
			}
		}
		// if the token is a name
		if ( token->type == TT_NAME && !( token->flags & TOKEN_FL_RECURSIVE_DEFINE ) ) {
			// check if the name is a define macro
			define = FindHashedDefine( hfParser::definehash, token->c_str() );
			// if it is a define macro
			if ( define ) {
				// expand the defined macro
				if ( !hfParser::ExpandDefineIntoSource( token, define ) ) {
					return false;
				}
				continue;
			}
		}
		// found a token
		return true;
	}
}

/*
================
hfParser::ExpectTokenString
================
*/
int hfParser::ExpectTokenString( const char *string ) {
	hfToken token;

	if ( !hfParser::ReadToken( &token ) ) {
		hfParser::Error( "couldn't find expected '%s'", string );
		return false;
	}

	if ( token != string ) {
		hfParser::Error( "expected '%s' but found '%s'", string, token.c_str() );
		return false;
	}
	return true;
}

/*
================
hfParser::ExpectTokenType
================
*/
int hfParser::ExpectTokenType( int type, int subtype, hfToken *token ) {
	hfStr str;

	if ( !hfParser::ReadToken( token ) ) {
		hfParser::Error( "couldn't read expected token" );
		return 0;
	}

	if ( token->type != type ) {
		switch( type ) {
			case TT_STRING: str = "string"; break;
			case TT_LITERAL: str = "literal"; break;
			case TT_NUMBER: str = "number"; break;
			case TT_NAME: str = "name"; break;
			case TT_PUNCTUATION: str = "punctuation"; break;
			default: str = "unknown type"; break;
		}
		hfParser::Error( "expected a %s but found '%s'", str.c_str(), token->c_str() );
		return 0;
	}
	if ( token->type == TT_NUMBER ) {
		if ( (token->subtype & subtype) != subtype ) {
			str.Clear();
			if ( subtype & TT_DECIMAL ) str = "decimal ";
			if ( subtype & TT_HEX ) str = "hex ";
			if ( subtype & TT_OCTAL ) str = "octal ";
			if ( subtype & TT_BINARY ) str = "binary ";
			if ( subtype & TT_UNSIGNED ) str += "unsigned ";
			if ( subtype & TT_LONG ) str += "long ";
			if ( subtype & TT_FLOAT ) str += "float ";
			if ( subtype & TT_INTEGER ) str += "integer ";
			str.StripTrailing( ' ' );
			hfParser::Error( "expected %s but found '%s'", str.c_str(), token->c_str() );
			return 0;
		}
	}
	else if ( token->type == TT_PUNCTUATION ) {
		if ( subtype < 0 ) {
			hfParser::Error( "BUG: wrong punctuation subtype" );
			return 0;
		}
		if ( token->subtype != subtype ) {
			hfParser::Error( "expected '%s' but found '%s'", scriptstack->GetPunctuationFromId( subtype ), token->c_str() );
			return 0;
		}
	}
	return 1;
}

/*
================
hfParser::ExpectAnyToken
================
*/
int hfParser::ExpectAnyToken( hfToken *token ) {
	if (!hfParser::ReadToken( token )) {
		hfParser::Error( "couldn't read expected token" );
		return false;
	}
	else {
		return true;
	}
}

/*
================
hfParser::CheckTokenString
================
*/
int hfParser::CheckTokenString( const char *string ) {
	hfToken tok;

	if ( !ReadToken( &tok ) ) {
		return false;
	}
	//if the token is available
	if ( tok == string ) {
		return true;
	}

	UnreadSourceToken( &tok );
	return false;
}

/*
================
hfParser::CheckTokenType
================
*/
int hfParser::CheckTokenType( int type, int subtype, hfToken *token ) {
	hfToken tok;

	if ( !ReadToken( &tok ) ) {
		return false;
	}
	//if the type matches
	if (tok.type == type && (tok.subtype & subtype) == subtype) {
		*token = tok;
		return true;
	}

	UnreadSourceToken( &tok );
	return false;
}

/*
================
hfParser::PeekTokenString
================
*/
int hfParser::PeekTokenString( const char *string ) {
	hfToken tok;

	if ( !ReadToken( &tok ) ) {
		return false;
	}

	UnreadSourceToken( &tok );

	// if the token is available
	if ( tok == string ) {
		return true;
	}
	return false;
}

/*
================
hfParser::PeekTokenType
================
*/
int hfParser::PeekTokenType( int type, int subtype, hfToken *token ) {
	hfToken tok;

	if ( !ReadToken( &tok ) ) {
		return false;
	}

	UnreadSourceToken( &tok );

	// if the type matches
	if ( tok.type == type && ( tok.subtype & subtype ) == subtype ) {
		*token = tok;
		return true;
	}
	return false;
}

/*
================
hfParser::SkipUntilString
================
*/
int hfParser::SkipUntilString( const char *string ) {
	hfToken token;

	while(hfParser::ReadToken( &token )) {
		if ( token == string ) {
			return true;
		}
	}
	return false;
}

/*
================
hfParser::SkipRestOfLine
================
*/
int hfParser::SkipRestOfLine() {
	hfToken token;

	while(hfParser::ReadToken( &token )) {
		if ( token.linesCrossed ) {
			hfParser::UnreadSourceToken( &token );
			return true;
		}
	}
	return false;
}

/*
=================
hfParser::SkipBracedSection

Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
int hfParser::SkipBracedSection( bool parseFirstBrace ) {
	hfToken token;
	int depth;

	depth = parseFirstBrace ? 0 : 1;
	do {
		if ( !ReadToken( &token ) ) {
			return false;
		}
		if( token.type == TT_PUNCTUATION ) {
			if( token == "{" ) {
				depth++;
			} else if ( token == "}" ) {
				depth--;
			}
		}
	} while( depth );
	return true;
}

/*
=================
hfParser::ParseBracedSectionExact

The next token should be an open brace.
Parses until a matching close brace is found.
Maintains the exact formating of the braced section

  FIXME: what about precompilation ?
=================
*/
const char *hfParser::ParseBracedSectionExact( hfStr &out, int tabs ) {
	return scriptstack->ParseBracedSectionExact( out, tabs );
}


/*
========================
hfParser::ParseBracedSection

The next token should be an open brace. Parses until a matching close brace is found. Internal 
brace depths are properly skipped.
========================
*/
const char* hfParser::ParseBracedSection( hfStr& out, int tabs, bool parseFirstBrace, char intro, char outro ) {
	hfToken token;
	int i, depth;
	bool doTabs;

	char temp[ 2 ] = { 0, 0 };
	*temp = intro;

	out.Empty();
	if ( parseFirstBrace ) {
		if ( !ExpectTokenString( temp ) ) {
			return out.c_str();
		}
		out = temp;
	}
	depth = 1;
	doTabs = ( tabs >= 0 );
	do {
		if ( !ReadToken( &token ) ) {
			Error( "missing closing brace" );
			return out.c_str();
		}

		// if the token is on a new line
		for ( i = 0; i < token.linesCrossed; i++ ) {
			out += "\r\n";
		}

		if ( doTabs && token.linesCrossed ) {
			i = tabs;
			if ( token[ 0 ] == outro && i > 0 ) {
				i--;
			}
			while( i-- > 0 ) {
				out += "\t";
			}
		}
		if ( token.type == TT_STRING ) {
			out += "\"" + token + "\"";
		} else if ( token.type == TT_LITERAL ) {
			out += "\'" + token + "\'";
		} else {
			if ( token[ 0 ] == intro ) {
				depth++;
				if ( doTabs ) {
					tabs++;
				}
			} else if ( token[ 0 ] == outro ) {
				depth--;
				if ( doTabs ) {
					tabs--;
				}
			}
			out += token;
		}
		out += " ";
	} while( depth );

	return out.c_str();
}

/*
=================
hfParser::ParseRestOfLine

  parse the rest of the line
=================
*/
const char *hfParser::ParseRestOfLine( hfStr &out ) {
	hfToken token;

	out.Empty();
	while(hfParser::ReadToken( &token )) {
		if ( token.linesCrossed ) {
			hfParser::UnreadSourceToken( &token );
			break;
		}
		if ( out.Length() ) {
			out += " ";
		}
		out += token;
	}
	return out.c_str();
}

/*
================
hfParser::UnreadToken
================
*/
void hfParser::UnreadToken( hfToken *token ) {
	hfParser::UnreadSourceToken( token );
}

/*
================
hfParser::ReadTokenOnLine
================
*/
int hfParser::ReadTokenOnLine( hfToken *token ) {
	hfToken tok;

	if (!hfParser::ReadToken( &tok )) {
		return false;
	}
	// if no lines were crossed before this token
	if ( !tok.linesCrossed ) {
		*token = tok;
		return true;
	}
	//
	hfParser::UnreadSourceToken( &tok );
	return false;
}

/*
================
hfParser::ParseInt
================
*/
int hfParser::ParseInt() {
	hfToken token;

	if ( !hfParser::ReadToken( &token ) ) {
		hfParser::Error( "couldn't read expected integer" );
		return 0;
	}
	if ( token.type == TT_PUNCTUATION && token == "-" ) {
		hfParser::ExpectTokenType( TT_NUMBER, TT_INTEGER, &token );
		return -((signed int) token.GetIntValue());
	}
	else if ( token.type != TT_NUMBER || token.subtype == TT_FLOAT ) {
		hfParser::Error( "expected integer value, found '%s'", token.c_str() );
	}
	return token.GetIntValue();
}

/*
================
hfParser::ParseBool
================
*/
bool hfParser::ParseBool() {
	hfToken token;

	if ( !hfParser::ExpectTokenType( TT_NUMBER, 0, &token ) ) {
		hfParser::Error( "couldn't read expected boolean" );
		return false;
	}
	return ( token.GetIntValue() != 0 );
}

/*
================
hfParser::ParseFloat
================
*/
float hfParser::ParseFloat() {
	hfToken token;

	if ( !hfParser::ReadToken( &token ) ) {
		hfParser::Error( "couldn't read expected floating point number" );
		return 0.0f;
	}
	if ( token.type == TT_PUNCTUATION && token == "-" ) {
		hfParser::ExpectTokenType( TT_NUMBER, 0, &token );
		return -token.GetFloatValue();
	}
	else if ( token.type != TT_NUMBER ) {
		hfParser::Error( "expected float value, found '%s'", token.c_str() );
	}
	return token.GetFloatValue();
}

/*
================
hfParser::Parse1DMatrix
================
*/
int hfParser::Parse1DMatrix( int x, float *m ) {
	int i;

	if ( !hfParser::ExpectTokenString( "(" ) ) {
		return false;
	}

	for ( i = 0; i < x; i++ ) {
		m[i] = hfParser::ParseFloat();
	}

	if ( !hfParser::ExpectTokenString( ")" ) ) {
		return false;
	}
	return true;
}

/*
================
hfParser::Parse2DMatrix
================
*/
int hfParser::Parse2DMatrix( int y, int x, float *m ) {
	int i;

	if ( !hfParser::ExpectTokenString( "(" ) ) {
		return false;
	}

	for ( i = 0; i < y; i++ ) {
		if ( !hfParser::Parse1DMatrix( x, m + i * x ) ) {
			return false;
		}
	}

	if ( !hfParser::ExpectTokenString( ")" ) ) {
		return false;
	}
	return true;
}

/*
================
hfParser::Parse3DMatrix
================
*/
int hfParser::Parse3DMatrix( int z, int y, int x, float *m ) {
	int i;

	if ( !hfParser::ExpectTokenString( "(" ) ) {
		return false;
	}

	for ( i = 0 ; i < z; i++ ) {
		if ( !hfParser::Parse2DMatrix( y, x, m + i * x*y ) ) {
			return false;
		}
	}

	if ( !hfParser::ExpectTokenString( ")" ) ) {
		return false;
	}
	return true;
}

/*
================
hfParser::GetLastWhiteSpace
================
*/
int hfParser::GetLastWhiteSpace( hfStr &whiteSpace ) const {
	if ( scriptstack ) {
		scriptstack->GetLastWhiteSpace( whiteSpace );
	} else {
		whiteSpace.Clear();
	}
	return whiteSpace.Length();
}

/*
================
hfParser::SetMarker
================
*/
void hfParser::SetMarker() {
	marker_p = NULL;
}

/*
================
hfParser::GetStringFromMarker

  FIXME: this is very bad code, the script isn't even garrenteed to still be around
================
*/
void hfParser::GetStringFromMarker( hfStr& out, bool clean ) {
	char*	p;
	char	save;

	if ( marker_p == NULL ) {
		marker_p = scriptstack->buffer;
	}
		
	if ( tokens ) {
		p = (char*)tokens->whiteSpaceStart_p;
	} else {
		p = (char*)scriptstack->script_p;
	}
	
	// Set the end character to NULL to give us a complete string
	save = *p;
	*p = 0;
	
	// If cleaning then reparse
	if ( clean ) {	
		hfParser temp( marker_p, strlen( marker_p ), "temp", flags );
		hfToken token;
		while ( temp.ReadToken ( &token ) ) {
			out += token;
		}
	} else {
		out = marker_p;
	}
	
	// restore the character we set to NULL
	*p = save;		
}

/*
================
hfParser::SetIncludePath
================
*/
void hfParser::SetIncludePath( const char *path ) {
	hfParser::includepath = path;
	// add trailing path seperator
	if (hfParser::includepath[hfParser::includepath.Length()-1] != '\\' &&
		hfParser::includepath[hfParser::includepath.Length()-1] != '/') {
		hfParser::includepath += PATHSEPARATOR_STR;
	}
}

/*
================
hfParser::SetPunctuations
================
*/
void hfParser::SetPunctuations( const punctuation_t *p ) {
	hfParser::punctuations = p;
}

/*
================
hfParser::SetFlags
================
*/
void hfParser::SetFlags( int flags ) {
	hfLexer *s;

	hfParser::flags = flags;
	for ( s = hfParser::scriptstack; s; s = s->next ) {
		s->SetFlags( flags );
	}
}

/*
================
hfParser::GetFlags
================
*/
int hfParser::GetFlags() const {
	return hfParser::flags;
}

/*
================
hfParser::LoadFile
================
*/
int hfParser::LoadFile( const char *filename, bool OSPath ) {
	hfLexer *script;

	if ( hfParser::loaded ) {
		hfLib::common->FatalError("hfParser::loadFile: another source already loaded");
		return false;
	}
	script = new (TAG_IDLIB_PARSER) hfLexer( filename, 0, OSPath );
	if ( !script->IsLoaded() ) {
		delete script;
		return false;
	}
	script->SetFlags( hfParser::flags );
	script->SetPunctuations( hfParser::punctuations );
	script->next = NULL;
	hfParser::OSPath = OSPath;
	hfParser::filename = filename;
	hfParser::scriptstack = script;
	hfParser::tokens = NULL;
	hfParser::indentstack = NULL;
	hfParser::skip = 0;
	hfParser::loaded = true;

	if ( !hfParser::definehash ) {
		hfParser::defines = NULL;
		hfParser::definehash = (define_t **) Mem_ClearedAlloc( DEFINEHASHSIZE * sizeof(define_t *), TAG_IDLIB_PARSER );
		hfParser::AddGlobalDefinesToSource();
	}
	return true;
}

/*
================
hfParser::LoadMemory
================
*/
int hfParser::LoadMemory(const char *ptr, int length, const char *name ) {
	hfLexer *script;

	if ( hfParser::loaded ) {
		hfLib::common->FatalError("hfParser::loadMemory: another source already loaded");
		return false;
	}
	script = new (TAG_IDLIB_PARSER) hfLexer( ptr, length, name );
	if ( !script->IsLoaded() ) {
		delete script;
		return false;
	}
	script->SetFlags( hfParser::flags );
	script->SetPunctuations( hfParser::punctuations );
	script->next = NULL;
	hfParser::filename = name;
	hfParser::scriptstack = script;
	hfParser::tokens = NULL;
	hfParser::indentstack = NULL;
	hfParser::skip = 0;
	hfParser::loaded = true;

	if ( !hfParser::definehash ) {
		hfParser::defines = NULL;
		hfParser::definehash = (define_t **) Mem_ClearedAlloc( DEFINEHASHSIZE * sizeof(define_t *), TAG_IDLIB_PARSER );
		hfParser::AddGlobalDefinesToSource();
	}
	return true;
}

/*
================
hfParser::FreeSource
================
*/
void hfParser::FreeSource( bool keepDefines ) {
	hfLexer *script;
	hfToken *token;
	define_t *define;
	indent_t *indent;
	int i;

	// free all the scripts
	while( scriptstack ) {
		script = scriptstack;
		scriptstack = scriptstack->next;
		delete script;
	}
	// free all the tokens
	while( tokens ) {
		token = tokens;
		tokens = tokens->next;
		delete token;
	}
	// free all indents
	while( indentstack ) {
		indent = indentstack;
		indentstack = indentstack->next;
		Mem_Free( indent );
	}
	if ( !keepDefines ) {
		// free hash table
		if ( definehash ) {
			// free defines
			for ( i = 0; i < DEFINEHASHSIZE; i++ ) {
				while( definehash[i] ) {
					define = definehash[i];
					definehash[i] = definehash[i]->hashnext;
					FreeDefine(define);
				}
			}
			defines = NULL;
			Mem_Free( hfParser::definehash );
			definehash = NULL;
		}
	}
	loaded = false;
}

/*
================
hfParser::GetPunctuationFromId
================
*/
const char *hfParser::GetPunctuationFromId( int id ) {
	int i;

	if ( !hfParser::punctuations ) {
		hfLexer lex;
		return lex.GetPunctuationFromId( id );
	}

	for (i = 0; hfParser::punctuations[i].p; i++) {
		if ( hfParser::punctuations[i].n == id ) {
			return hfParser::punctuations[i].p;
		}
	}
	return "unkown punctuation";
}

/*
================
hfParser::GetPunctuationId
================
*/
int hfParser::GetPunctuationId( const char *p ) {
	int i;

	if ( !hfParser::punctuations ) {
		hfLexer lex;
		return lex.GetPunctuationId( p );
	}

	for (i = 0; hfParser::punctuations[i].p; i++) {
		if ( !strcmp(hfParser::punctuations[i].p, p) ) {
			return hfParser::punctuations[i].n;
		}
	}
	return 0;
}

/*
================
hfParser::hfParser
================
*/
hfParser::hfParser() {
	this->loaded = false;
	this->OSPath = false;
	this->punctuations = 0;
	this->flags = 0;
	this->scriptstack = NULL;
	this->indentstack = NULL;
	this->definehash = NULL;
	this->defines = NULL;
	this->tokens = NULL;
	this->marker_p = NULL;
}

/*
================
hfParser::hfParser
================
*/
hfParser::hfParser( int flags ) {
	this->loaded = false;
	this->OSPath = false;
	this->punctuations = 0;
	this->flags = flags;
	this->scriptstack = NULL;
	this->indentstack = NULL;
	this->definehash = NULL;
	this->defines = NULL;
	this->tokens = NULL;
	this->marker_p = NULL;
}

/*
================
hfParser::hfParser
================
*/
hfParser::hfParser( const char *filename, int flags, bool OSPath ) {
	this->loaded = false;
	this->OSPath = true;
	this->punctuations = 0;
	this->flags = flags;
	this->scriptstack = NULL;
	this->indentstack = NULL;
	this->definehash = NULL;
	this->defines = NULL;
	this->tokens = NULL;
	this->marker_p = NULL;
	LoadFile( filename, OSPath );
}

/*
================
hfParser::hfParser
================
*/
hfParser::hfParser( const char *ptr, int length, const char *name, int flags ) {
	this->loaded = false;
	this->OSPath = false;
	this->punctuations = 0;
	this->flags = flags;
	this->scriptstack = NULL;
	this->indentstack = NULL;
	this->definehash = NULL;
	this->defines = NULL;
	this->tokens = NULL;
	this->marker_p = NULL;
	LoadMemory( ptr, length, name );
}

/*
================
hfParser::~hfParser
================
*/
hfParser::~hfParser() {
	hfParser::FreeSource( false );
}

/*
========================
hfParser::EndOfFile
========================
*/
bool hfParser::EndOfFile() {
	if ( scriptstack != NULL ) {
		return (bool) scriptstack->EndOfFile();
	}
	return true;
}


