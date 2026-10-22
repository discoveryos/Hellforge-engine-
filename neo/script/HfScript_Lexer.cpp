/*
===========================================================================

Hellforge Power 1 Engine - hfscript lexer

Python/Ruby-flavored tokenizer: whitespace (indentation) defines block
structure, hash marks (and C-style block comments) start comments,
strings may use single or double quotes.

===========================================================================
*/
#include "../hflib/hfprecompiled.h"
#include "HfScript_Internal.h"

/*
========================
ColumnToIndex
========================
*/
int HfScriptLexer::ColumnToIndex( int column ) {
	return ( column + 3 ) & ~3;
}

/*
========================
SkipSpace

Skips spaces/tabs on the current line.  Returns false at end of buffer.
========================
*/
bool HfScriptLexer::SkipSpace() {
	while ( cursor < end ) {
		if ( *cursor == ' ' || *cursor == '\t' ) {
			lineIndent = ColumnToIndex( lineIndent + 1 );
			cursor++;
		} else {
			break;
		}
	}
	return cursor < end;
}

/*
========================
ReadComment

Consumes a comment to the end of the line.  Returns true if a comment
was consumed.
========================
*/
bool HfScriptLexer::ReadComment() {
	if ( cursor + 1 >= end ) {
		return false;
	}
	if ( *cursor == '#' ) {
		cursor++;
		while ( cursor < end && *cursor != '\n' ) {
			cursor++;
		}
		return true;
	}
	if ( *cursor == '/' && cursor + 1 < end ) {
		if ( *( cursor + 1 ) == '/' ) {
			cursor += 2;
			while ( cursor < end && *cursor != '\n' ) {
				cursor++;
			}
			return true;
		}
		if ( *( cursor + 1 ) == '*' ) {
			cursor += 2;
			while ( cursor + 1 < end && !( *cursor == '*' && *( cursor + 1 ) == '/' ) ) {
				if ( *cursor == '\n' ) {
					line++;
				}
				cursor++;
			}
			if ( cursor + 1 >= end ) {
				error = "unterminated /* comment";
				return true;
			}
			cursor += 2;
			return true;
		}
	}
	return false;
}

/*
========================
Init
========================
*/
void HfScriptLexer::Init( const char * text ) {
	src = text;
	cursor = text;
	end = text + strlen( text );
	line = 1;
	lineIndent = 0;
	atLineStart = true;
	parenDepth = 0;
	indentCount = 0;
	indentStack[0] = 0;
	pendingIndents = 0;
	pendingDedents = 0;
	pendingNewline = false;
	error = "";
}

/*
========================
EmitIndents
========================
*/
void HfScriptLexer::EmitIndents( HfScriptToken & out ) {
	if ( pendingNewline ) {
		pendingNewline = false;
		out.type = HS_TK_NEWLINE;
		out.line = line;
		return;
	}
	if ( pendingDedents > 0 ) {
		pendingDedents--;
		out.type = HS_TK_DEDENT;
		out.line = line;
		return;
	}
	if ( pendingIndents > 0 ) {
		pendingIndents--;
		out.type = HS_TK_INDENT;
		out.line = line;
		return;
	}
}

/*
========================
LexString
========================
*/
bool HfScriptLexer::LexString( HfScriptToken & out ) {
	const char quote = *cursor;
	cursor++;
	hfStr str;
	bool closed = false;

	while ( cursor < end ) {
		char c = *cursor;
		if ( c == quote ) {
			cursor++;
			closed = true;
			break;
		}
		if ( c == '\\' ) {
			cursor++;
			if ( cursor >= end ) {
				break;
			}
			char e = *cursor;
			switch ( e ) {
				case 'n': str += '\n'; break;
				case 't': str += '\t'; break;
				case 'r': str += '\r'; break;
				case '0': str += '\0'; break;
				case '\\': str += '\\'; break;
				case '\'': str += '\''; break;
				case '"': str += '"'; break;
				case 'x': {
					if ( cursor + 2 < end && isxdigit( cursor[1] ) && isxdigit( cursor[2] ) ) {
						char hex[3] = { cursor[1], cursor[2], 0 };
						str += (char)strtol( hex, NULL, 16 );
						cursor += 2;
					}
					break;
				}
				default:
					str += e;
					break;
			}
			cursor++;
			continue;
		}
		if ( c == '\n' ) {
			line++;
		}
		str += c;
		cursor++;
	}

	if ( !closed ) {
		error = "unterminated string literal";
		return false;
	}

	out.type = HS_TK_STRING;
	out.text = str;
	out.line = line;
	return true;
}

/*
========================
LexNumber
========================
*/
bool HfScriptLexer::LexNumber( HfScriptToken & out ) {
	const char * start = cursor;
	bool isFloat = false;
	bool isHex = false;

	if ( cursor + 1 < end && *cursor == '0' && ( cursor[1] == 'x' || cursor[1] == 'X' ) ) {
		isHex = true;
		cursor += 2;
		while ( cursor < end && isxdigit( (unsigned char)*cursor ) ) {
			cursor++;
		}
	} else {
		if ( *cursor == '.' ) {
			isFloat = true;
		}
		while ( cursor < end && ( isdigit( (unsigned char)*cursor ) || *cursor == '.' ) ) {
			if ( *cursor == '.' && !isFloat ) {
				isFloat = true;
			}
			cursor++;
		}
		if ( !isFloat && cursor < end && ( *cursor == 'e' || *cursor == 'E' ) ) {
			const char * save = cursor;
			cursor++;
			if ( cursor < end && ( *cursor == '+' || *cursor == '-' ) ) {
				cursor++;
			}
			if ( cursor < end && isdigit( (unsigned char)*cursor ) ) {
				isFloat = true;
				while ( cursor < end && isdigit( (unsigned char)*cursor ) ) {
					cursor++;
				}
			} else {
				cursor = save;
			}
		}
	}

	if ( cursor == start || ( isHex && cursor == start + 2 ) ) {
		error = "malformed number";
		return false;
	}

	hfStr numText;
	for ( const char * p = start; p < cursor; p++ ) {
		numText += *p;
	}
	if ( isHex ) {
		out.type = HS_TK_INT;
		out.value = HfScriptValue::Int( (int64)strtoll( numText.c_str() + 2, NULL, 16 ) );
	} else if ( isFloat ) {
		out.type = HS_TK_FLOAT;
		out.value = HfScriptValue::Float( atof( numText.c_str() ) );
	} else {
		out.type = HS_TK_INT;
		out.value = HfScriptValue::Int( (int64)strtoll( numText.c_str(), NULL, 10 ) );
	}
	out.line = line;
	return true;
}

/*
========================
LexIdent
========================
*/
bool HfScriptLexer::LexIdent( HfScriptToken & out ) {
	const char * start = cursor;
	while ( cursor < end && ( isalnum( (unsigned char)*cursor ) || *cursor == '_' ) ) {
		cursor++;
	}
	out.type = HS_TK_IDENT;
	{
		hfStr identText;
		for ( const char * p = start; p < cursor; p++ ) {
			identText += *p;
		}
		out.text = identText;
	}
	out.line = line;
	return true;
}

/*
========================
LexOp
========================
*/
bool HfScriptLexer::LexOp( HfScriptToken & out ) {
	const char twoCharOps[][ 3 ] = {
		"<=", ">=", "==", "!=", "&&", "||", "+=", "-=", "*=", "/=", "%="
	};
	char c = *cursor;

	if ( c == '(' || c == '[' ) {
		parenDepth++;
	} else if ( c == ')' || c == ']' ) {
		if ( parenDepth > 0 ) {
			parenDepth--;
		}
	}

	if ( cursor + 1 < end ) {
		char two[ 3 ] = { c, cursor[ 1 ], 0 };
		for ( int i = 0; i < (int)( sizeof( twoCharOps ) / sizeof( twoCharOps[ 0 ] ) ); i++ ) {
			if ( two[ 0 ] == twoCharOps[ i ][ 0 ] && two[ 1 ] == twoCharOps[ i ][ 1 ] ) {
				cursor += 2;
				out.type = HS_TK_OP;
				out.text = two;
				out.line = line;
				return true;
			}
		}
	}

	cursor++;
	out.type = HS_TK_OP;
	{
		char s[ 2 ] = { c, 0 };
		out.text = s;
	}
	out.line = line;
	return true;
}

/*
========================
NextToken
========================
*/
bool HfScriptLexer::NextToken( HfScriptToken & out ) {
	out.type = HS_TK_EOF;
	out.text = "";

	for ( ;; ) {
		// At the start of a line: resolve blank/comment lines and indentation.
		if ( atLineStart ) {
			lineIndent = 0;
			if ( !SkipSpace() ) {
				// end of buffer
				goto atEof;
			}
			if ( *cursor == '\n' ) {
				cursor++;
				line++;
				continue;						// blank line: no token
			}
			if ( ReadComment() ) {
				continue;						// comment-only line: no token
			}
			atLineStart = false;

			// Compare the new indent to the stack top.
			int current = indentStack[ indentCount ];
			if ( lineIndent > current ) {
				if ( indentCount >= 63 ) {
					error = "too many indentation levels";
					return false;
				}
				indentStack[ ++indentCount ] = lineIndent;
				// The INDENT token is delivered before the line's first token.
				// Negative delimiter trick is not needed; emit eagerly:
				// buffer it so the parser always sees INDENT exactly once.
				pendingIndents = 1;
			} else {
				while ( indentCount > 0 && lineIndent < indentStack[ indentCount ] ) {
					indentCount--;
					pendingDedents++;
				}
				if ( lineIndent != indentStack[ indentCount ] ) {
					error = "inconsistent indentation";
					return false;
				}
			}
		}

		// Drain pending structural tokens first.
		if ( pendingNewline ) {
			pendingNewline = false;
			out.type = HS_TK_NEWLINE;
			out.line = line;
			return true;
		}
		if ( pendingDedents > 0 ) {
			pendingDedents--;
			out.type = HS_TK_DEDENT;
			out.line = line;
			return true;
		}
		if ( pendingIndents > 0 ) {
			pendingIndents--;
			out.type = HS_TK_INDENT;
			out.line = line;
			return true;
		}

		if ( cursor >= end ) {
			goto atEof;
		}

		// Tolerate inline spaces (already past line start).
		if ( *cursor == ' ' || *cursor == '\t' ) {
			cursor++;
			continue;
		}
		if ( ReadComment() ) {
			continue;
		}
		if ( *cursor == '\n' ) {
			cursor++;
			line++;
			atLineStart = true;
			if ( parenDepth > 0 ) {
				continue;					// inside parens: no NEWLINE
			}
			pendingNewline = true;
			continue;
		}
		if ( *cursor == '\r' ) {
			cursor++;
			continue;
		}

		// Real token.
		out.indent = lineIndent;
		if ( *cursor == '"' || *cursor == '\'' ) {
			return LexString( out );
		}
		if ( isdigit( (unsigned char)*cursor ) || ( *cursor == '.' && cursor + 1 < end && isdigit( (unsigned char)cursor[ 1 ] ) ) ) {
			return LexNumber( out );
		}
		if ( isalpha( (unsigned char)*cursor ) || *cursor == '_' ) {
			return LexIdent( out );
		}
		return LexOp( out );
	}

atEof:
	// Flush a trailing NEWLINE and all pending DEDENTs before reporting EOF.
	if ( pendingNewline ) {
		pendingNewline = false;
		out.type = HS_TK_NEWLINE;
		out.line = line;
		return true;
	}
	if ( pendingDedents > 0 ) {
		pendingDedents--;
		out.type = HS_TK_DEDENT;
		out.line = line;
		return true;
	}
	if ( indentCount > 0 ) {
		// The source ended inside an open block. Synthesize one DEDENT per
		// call so the parser sees the block close cleanly.
		indentCount--;
		pendingDedents = indentCount;		// emit the remainder next calls
		out.type = HS_TK_DEDENT;
		out.line = line;
		return true;
	}
	out.type = HS_TK_EOF;
	out.line = line;
	return true;
}

/*
========================
HfScript_Tokenize
========================
*/
bool HfScript_Tokenize( const char * text, hfList<HfScriptToken> & out, hfStr & error ) {
	HfScriptLexer lexer;
	lexer.Init( text );

	for ( ;; ) {
		HfScriptToken tok;
		if ( !lexer.NextToken( tok ) ) {
			error = lexer.GetError();
			return false;
		}
		if ( tok.type == HS_TK_EOF ) {
			out.Append( tok );				// keep the EOF sentinel so Cur() clamps at EOF
			break;
		}
		if ( tok.type == HS_TK_ERROR ) {
			error = "lex error";
			return false;
		}
		out.Append( tok );
	}
	return true;
}