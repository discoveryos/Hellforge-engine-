/*
===========================================================================

Hellforge Power 1 Engine - hfscript internal lexer/parser plumbing

hfscript is an indentation-structured, dynamically typed scripting
language in the spirit of Python and Ruby.

===========================================================================
*/
#ifndef __HFSCRIPT_INTERNAL_H__
#define __HFSCRIPT_INTERNAL_H__

#include "../hflib/hfprecompiled.h"
#include "HfScript.h"

/*
================================================
Tokens

The lexer performs Python-style indentation handling: NEWLINE is emitted
at the end of each logical line, and INDENT / DEDENT tokens delimit
block structure so the parser never deals with whitespace directly.
================================================
*/
enum hfScriptTokenType_t {
	HS_TK_EOF,
	HS_TK_ERROR,
	HS_TK_NEWLINE,
	HS_TK_INDENT,
	HS_TK_DEDENT,
	HS_TK_IDENT,
	HS_TK_INT,
	HS_TK_FLOAT,
	HS_TK_STRING,
	HS_TK_OP,						// any punctuation / operator
};

struct HfScriptToken {
	int				type;
	hfStr			text;			// identifier / operator text, or unescaped string body
	HfScriptValue	value;			// literal value for int/float (string body lives in .text)
	int				line;
	int				indent;			// column of the token's line start

					HfScriptToken() : type( HS_TK_EOF ), line( 1 ), indent( 0 ) {}
	bool			Is( const char * t ) const { return type == HS_TK_OP && text == t; }
	bool			IsIdent( const char * t ) const { return type == HS_TK_IDENT && text == t; }
};

/*
================================================
Lexer
================================================
*/
class HfScriptLexer {
public:
	void			Init( const char * text );
	void			SavePos();
	void			RestorePos();
	bool			NextToken( HfScriptToken & out );
	const char *	GetError() const { return error.c_str(); }
	int				Line() const { return line; }

private:
	const char *	src;
	const char *	cursor;
	const char *	end;
	int				line;

	// indentation tracking
	int				indentStack[ 64 ];
	int				indentCount;
	int				atLineStart;
	int				lineIndent;
	int				pendingIndents;			// indents emitted before the next token
	int				pendingDedents;
	bool			pendingNewline;

	// parenthetical depth: no NEWLINE/indent handling inside ( ... ) or [ ... ]
	int				parenDepth;

	// saved position for the two-pass compile
	const char *	savedCursor;
	int				savedLine;
	int				savedLineIndent;
	int				savedPendingNewline;

	hfStr			error;

	bool			SkipSpace();
	bool			ReadComment();
	int				ColumnToIndex( int column );
	void			EmitIndents( HfScriptToken & out );
	bool			LexString( HfScriptToken & out );
	bool			LexNumber( HfScriptToken & out );
	bool			LexIdent( HfScriptToken & out );
	bool			LexOp( HfScriptToken & out );
};

// Tokenize an entire buffer into a token list (the parser works from this list,
// which also lets the compiler make two passes over function bodies cheaply).
bool				HfScript_Tokenize( const char * text, hfList<HfScriptToken> & out, hfStr & error );

#endif /* !__HFSCRIPT_INTERNAL_H__ */