/*
===========================================================================

Hellforge Power 1 Engine - hfscript "English" dialect

The English front end is a token-level rewrite that lowers a friendlier,
more natural-language spelling onto the canonical hfscript token stream
before the normal Python/Ruby compiler runs.  Nothing here changes the
bytecode, VM, or semantics: a .eng script is just sugar that becomes a
.hfs program.

The dialect keeps Python-style indentation (it is the easiest block
delimiter to read) but drops the punctuation colons and spells control
flow the way a person would say it:

	set speed to 5            ->  speed = 5
	increase speed by 1       ->  speed += 1
	show speed                ->  print(speed)
	if speed is greater than 3
	    ...
	otherwise
	    ...
	repeat 10 times
	    ...
	define update with dt
	    give back dt * 2
	stop / skip / do nothing  ->  break / continue / pass

Comparison and arithmetic can be written as words:

	is greater than   ->  >        is at least   ->  >=
	is less than      ->  <        is at most    ->  <=
	is equal to       ->  ==       is not equal to ->  !=
	is (alone)        ->  ==       equals        ->  ==
	plus/minus/times/divided by/modulo -> + - * / %

Keywords are reserved in this dialect (as in every real language), so a
.eng script should not use "set", "show", "is", etc. as variable names.
Statements may end with an optional "then"/":" which is simply discarded.

===========================================================================
*/
#ifndef __HFSCRIPT_ENGLISH_H__
#define __HFSCRIPT_ENGLISH_H__

#include "../hflib/hfprecompiled.h"
#include "HfScript_Internal.h"

namespace hfeng {

static HfScriptToken MakeTok( int type, const char * text, int line ) {
	HfScriptToken t;
	t.type = type;
	t.text = text;
	t.line = line;
	t.indent = 0;
	return t;
}

static HfScriptToken MakeIdent( const char * text, int line ) {
	return MakeTok( HS_TK_IDENT, text, line );
}

static HfScriptToken MakeOp( const char * text, int line ) {
	return MakeTok( HS_TK_OP, text, line );
}

static bool IsStmtEnd( int type ) {
	return type == HS_TK_NEWLINE || type == HS_TK_DEDENT || type == HS_TK_EOF;
}

// Rewrite an English token stream into canonical hfscript tokens.
// Returns false (and sets error) on an English-level syntax problem.
static bool Translate( const hfList<HfScriptToken> & in, hfList<HfScriptToken> & out, hfStr & error ) {
	out.Clear();
	const int n = in.Num();
	int i = 0;

	while ( i < n ) {
		const HfScriptToken & tok = in[ i ];

		if ( tok.type == HS_TK_IDENT ) {
			int line = tok.line;

			// ---- statement keywords -------------------------------------
			if ( tok.text == "give" && i + 1 < n && in[ i + 1 ].IsIdent( "back" ) ) {
				out.Append( MakeIdent( "return", line ) );
				i += 2;
				continue;
			}
			if ( tok.text == "do" && i + 1 < n && in[ i + 1 ].IsIdent( "nothing" ) ) {
				out.Append( MakeIdent( "pass", line ) );
				i += 2;
				continue;
			}
			if ( tok.text == "set" && i + 2 < n && in[ i + 1 ].type == HS_TK_IDENT && in[ i + 2 ].IsIdent( "to" ) ) {
				out.Append( MakeIdent( in[ i + 1 ].text.c_str(), line ) );
				out.Append( MakeOp( "=", line ) );
				i += 3;
				continue;
			}
			if ( ( tok.text == "change" || tok.text == "increase" || tok.text == "decrease" )
				&& i + 2 < n && in[ i + 1 ].type == HS_TK_IDENT && in[ i + 2 ].IsIdent( "by" ) ) {
				const char * op = tok.text == "change" ? "+=" : ( tok.text == "increase" ? "+=" : "-=" );
				out.Append( MakeIdent( in[ i + 1 ].text.c_str(), line ) );
				out.Append( MakeOp( op, line ) );
				i += 3;
				continue;
			}
			if ( tok.text == "otherwise" ) {
				out.Append( MakeIdent( "else", line ) );
				i += 1;
				continue;
			}
			if ( tok.text == "else" && i + 1 < n && in[ i + 1 ].IsIdent( "if" ) ) {
				out.Append( MakeIdent( "elif", line ) );
				i += 2;
				continue;
			}
			if ( tok.text == "then" ) {
				i += 1;								// decorative separator
				continue;
			}
			if ( tok.text == "stop" ) {
				out.Append( MakeIdent( "break", line ) );
				i += 1;
				continue;
			}
			if ( tok.text == "skip" ) {
				out.Append( MakeIdent( "continue", line ) );
				i += 1;
				continue;
			}
			if ( tok.text == "repeat" ) {
				// repeat EXPR times  ->  for __repeat in range( EXPR )
				int j = i + 1;
				int depth = 0;
				while ( j < n ) {
					if ( IsStmtEnd( in[ j ].type ) ) {
						break;
					}
					if ( in[ j ].Is( "(" ) ) {
						depth++;
					} else if ( in[ j ].Is( ")" ) ) {
						if ( depth > 0 ) {
							depth--;
						}
					} else if ( depth == 0 && in[ j ].IsIdent( "times" ) ) {
						break;
					}
					j++;
				}
				if ( j >= n || !in[ j ].IsIdent( "times" ) ) {
					error = "expected \"times\" after a repeat count";
					return false;
				}
				if ( j == i + 1 ) {
					error = "\"repeat\" needs a count before \"times\"";
					return false;
				}
				out.Append( MakeIdent( "for", line ) );
				out.Append( MakeIdent( "__repeat", line ) );
				out.Append( MakeIdent( "in", line ) );
				out.Append( MakeIdent( "range", line ) );
				out.Append( MakeOp( "(", line ) );
				for ( int k = i + 1; k < j; k++ ) {
					out.Append( in[ k ] );
				}
				out.Append( MakeOp( ")", line ) );
				i = j + 1;
				continue;
			}
			if ( tok.text == "define" ) {
				// define NAME [with p and q]   ->   def NAME ( p , q )
				if ( i + 1 >= n || in[ i + 1 ].type != HS_TK_IDENT ) {
					error = "expected a function name after \"define\"";
					return false;
				}
				out.Append( MakeIdent( "def", line ) );
				out.Append( MakeIdent( in[ i + 1 ].text.c_str(), in[ i + 1 ].line ) );
				i += 2;
				if ( i < n && in[ i ].Is( "(" ) ) {
					int depth = 0;
					while ( i < n ) {
						out.Append( in[ i ] );
						if ( in[ i ].Is( "(" ) ) {
							depth++;
						} else if ( in[ i ].Is( ")" ) ) {
							depth--;
							if ( depth == 0 ) {
								i++;
								break;
							}
						}
						i++;
					}
					continue;
				}
				out.Append( MakeOp( "(", line ) );
				if ( i < n && in[ i ].IsIdent( "with" ) ) {
					i++;
					for ( ;; ) {
						if ( i >= n || in[ i ].type != HS_TK_IDENT ) {
							error = "expected a parameter name after \"with\"";
							return false;
						}
						out.Append( MakeIdent( in[ i ].text.c_str(), in[ i ].line ) );
						i++;
						if ( i < n && in[ i ].Is( "," ) ) {
							out.Append( MakeOp( ",", line ) );
							i++;
							continue;
						}
						if ( i < n && in[ i ].IsIdent( "and" ) ) {
							out.Append( MakeOp( ",", line ) );
							i++;
							continue;
						}
						break;
					}
				}
				out.Append( MakeOp( ")", line ) );
				continue;
			}
			if ( tok.text == "show" ) {
				// show EXPR   ->   print( EXPR )
				if ( i + 1 >= n || IsStmtEnd( in[ i + 1 ].type ) ) {
					error = "expected an expression after \"show\"";
					return false;
				}
				out.Append( MakeIdent( "print", line ) );
				out.Append( MakeOp( "(", line ) );
				i++;
				while ( i < n && !IsStmtEnd( in[ i ].type ) ) {
					out.Append( in[ i ] );
					i++;
				}
				out.Append( MakeOp( ")", line ) );
				continue;
			}

			// ---- word operators -----------------------------------------
			if ( tok.text == "is" ) {
				int used = 1;
				const char * op = "==";
				if ( i + 3 < n && in[ i + 1 ].IsIdent( "not" ) && in[ i + 2 ].IsIdent( "equal" ) && in[ i + 3 ].IsIdent( "to" ) ) {
					op = "!="; used = 4;
				} else if ( i + 2 < n && in[ i + 1 ].IsIdent( "equal" ) && in[ i + 2 ].IsIdent( "to" ) ) {
					op = "=="; used = 3;
				} else if ( i + 2 < n && in[ i + 1 ].IsIdent( "greater" ) && in[ i + 2 ].IsIdent( "than" ) ) {
					op = ">"; used = 3;
				} else if ( i + 2 < n && in[ i + 1 ].IsIdent( "less" ) && in[ i + 2 ].IsIdent( "than" ) ) {
					op = "<"; used = 3;
				} else if ( i + 2 < n && in[ i + 1 ].IsIdent( "at" ) && in[ i + 2 ].IsIdent( "least" ) ) {
					op = ">="; used = 3;
				} else if ( i + 2 < n && in[ i + 1 ].IsIdent( "at" ) && in[ i + 2 ].IsIdent( "most" ) ) {
					op = "<="; used = 3;
				} else if ( i + 1 < n && in[ i + 1 ].IsIdent( "not" ) ) {
					op = "!="; used = 2;
				}
				out.Append( MakeOp( op, line ) );
				i += used;
				continue;
			}
			if ( tok.text == "equals" ) {
				out.Append( MakeOp( "==", line ) );
				i += 1;
				continue;
			}
			if ( tok.text == "plus" ) {
				out.Append( MakeOp( "+", line ) ); i += 1; continue;
			}
			if ( tok.text == "minus" ) {
				out.Append( MakeOp( "-", line ) ); i += 1; continue;
			}
			if ( tok.text == "times" ) {
				out.Append( MakeOp( "*", line ) ); i += 1; continue;
			}
			if ( tok.text == "modulo" ) {
				out.Append( MakeOp( "%", line ) ); i += 1; continue;
			}
			if ( tok.text == "divided" && i + 1 < n && in[ i + 1 ].IsIdent( "by" ) ) {
				out.Append( MakeOp( "/", line ) );
				i += 2;
				continue;
			}
			if ( tok.text == "multiplied" && i + 1 < n && in[ i + 1 ].IsIdent( "by" ) ) {
				out.Append( MakeOp( "*", line ) );
				i += 2;
				continue;
			}
		}

		out.Append( tok );
		i++;
	}

	return true;
}

} // namespace hfeng

// Public entry used by the compiler.
static bool HfScript_TranslateEnglish( const hfList<HfScriptToken> & in, hfList<HfScriptToken> & out, hfStr & error ) {
	return hfeng::Translate( in, out, error );
}

#endif /* !__HFSCRIPT_ENGLISH_H__ */
