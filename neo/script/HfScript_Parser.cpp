/*
===========================================================================

Hellforge Power 1 Engine - hfscript parser / bytecode compiler

Two passes are made over the token stream:

  1. collect function signatures (def files found at indent level 0), so
     forward references (calls to functions defined later) resolve
  2. compile function bodies and top-level statements to bytecode

This file is intentionally free of engine dependencies so it can be
linked by standalone tools; engine builtins live in HfScript_Engine.cpp.

===========================================================================
*/
#include "../hflib/hfprecompiled.h"
#include "../hflib/hfLangDict.h"
#include "HfScript_Internal.h"
#include "HfScript_English.h"

#define HFSCRIPT_TRACE 1
#if defined(HFSCRIPT_TRACE)
#define HFS_TCK(x) do {} while(0)
#else
#define HFS_TCK(x)
#endif

/*
========================
Compiler
========================
*/
struct HfLoopInfo {
	hfList<int>		breakOps;
	hfList<int>		continueOps;
};

struct HfExternalRef {
	hfStr		name;
	int			op;				// index of the HOP_CALL to patch
};

class HfScriptCompiler {
public:
	bool		Compile( HfScriptProgram * program, const char * source, const char * sourceName );

private:
	//---- token stream ----
	hfList<HfScriptToken>	tokens;
	int						pos;
	int						lastLine;

	//---- output ----
	HfScriptProgram *		prog;
	bool					failed;
	bool					ruby;				// Ruby-flavored dialect (def/if/while/for without ':', elsif, trailing 'end')

	//---- forward-reference signatures (pass 1) ----
	struct Sig {
		hfStr	name;
		int		numParams;
	};
	hfList<Sig>				signatures;

	//---- current function ----
	int						curFunction;	// index into prog->functions, -1 at top level
	hfList<hfStr>			localNames;		// params first, then locals
	hfList<hfStr>			globalRefs;		// names listed in "global x" statements
	hfList<HfLoopInfo>		loopStack;
	hfList<HfExternalRef>	externalRefs;
	int						firstEntryOp;

	//---- token helpers ----
	HfScriptToken & Cur( int ahead = 0 );
	bool			AtEnd();
	void			SkipNewlines();
	bool			MatchOp( const char * s );
	bool			MatchIdent( const char * s );
	void			ExpectOp( const char * s );
	void			ExpectIdent();
	void			Error( const char * msg );

	//---- emission ----
	int				Here();
	void			NoteLine( int line );
	int				Emit( int op, int a = 0, int b = 0 );
	void			PatchJump( int opIndex, int target );
	void			EmitConst( const HfScriptValue & v );

	int				CInt( int64 i );
	int				CFloat( float f );
	int				CStr( const char * s );

	//---- symbol tables ----
	int				FindGlobal( const char * name );
	int				DeclareGlobal( const char * name );
	int				FindLocal( const char * name );
	int				DeclareLocal( const char * name );
	int				FindFunction( const char * name );
	bool			IsGlobalName( const char * name );

	enum lvalRef_t { LV_LOCAL, LV_GLOBAL, LV_NONE };
	struct lval_t	{ int tag; int index; };
	lval_t			ResolveLValue( const char * name, bool forWrite );
	void			EmitLoad( const lval_t & lv );
	void			EmitStore( const lval_t & lv );

	//---- statements ----
	void			CompileSuite();					// : NEWLINE INDENT ... DEDENT  or  : stmt
	bool			ParseStatement();
	void			ParseDef();
	void			ParseIf();
	void			ParseWhile();
	void			ParseFor();
	void			ParseSimpleStatement();

	//---- expressions ----
	void			ParseExpression();				// assignment or expression
	void			ParseAssignment( bool compound );
	void			ParseOr();
	void			ParseAnd();
	void			ParseEquality();
	void			ParseRelational();
	void			ParseAdditive();
	void			ParseMultiplicative();
	void			ParseUnary();
	void			ParsePostfix();
	void			ParsePrimary();

	//---- two-pass local scans ----
	void			ScanFuncLocals( int bodyPos );
	void			SkipToColon();

	//---- diagnostics ----
	void			SetError( const char * msg );
};

/*
========================
token helpers
========================
*/
HfScriptToken & HfScriptCompiler::Cur( int ahead ) {
	int i = pos + ahead;
	if ( i >= tokens.Num() ) {
		i = tokens.Num() - 1;
	}
	return tokens[ i ];
}

bool HfScriptCompiler::AtEnd() {
	return Cur().type == HS_TK_EOF;
}

void HfScriptCompiler::SkipNewlines() {
	int guard = 0;
	while ( Cur().type == HS_TK_NEWLINE ) {
		pos++;
		if ( ++guard > 100000 ) {
			break;
		}
	}
}

bool HfScriptCompiler::MatchOp( const char * s ) {
	if ( Cur().Is( s ) ) {
		pos++;
		return true;
	}
	return false;
}

bool HfScriptCompiler::MatchIdent( const char * s ) {
	if ( Cur().IsIdent( s ) ) {
		pos++;
		return true;
	}
	return false;
}

void HfScriptCompiler::ExpectOp( const char * s ) {
	if ( !Cur().Is( s ) ) {
		hfStr msg;
		msg = "expected \"";
		msg += s;
		msg += "\", found \"";
		msg += Cur().text;
		msg += "\"";
		Error( msg.c_str() );
		return;
	}
	pos++;
}

void HfScriptCompiler::ExpectIdent() {
	if ( Cur().type != HS_TK_IDENT ) {
		Error( "expected identifier" );
		return;
	}
	pos++;
}

void HfScriptCompiler::Error( const char * msg ) {
	if ( failed ) {
		return;
	}
	failed = true;
	SetError( msg );
}

void HfScriptCompiler::SetError( const char * msg ) {
	prog->error = msg;
	prog->errorLine = Cur().line;
}

/*
========================
emission
========================
*/
int HfScriptCompiler::Here() {
	return prog->numOps;
}

void HfScriptCompiler::NoteLine( int line ) {
	if ( failed ) {
		return;
	}
	if ( line == lastLine ) {
		return;
	}
	lastLine = line;
	Emit( HOP_DEBUG_LINE, line, 0 );
}

int HfScriptCompiler::Emit( int op, int a, int b ) {
	if ( failed ) {
		return -1;
	}
	int index = prog->numOps;
	if ( prog->numOps >= prog->opAlloc ) {
		prog->AllocOps( prog->opAlloc > 0 ? prog->opAlloc * 2 : 64 );
	}
	prog->ops[ index ].op = op;
	prog->ops[ index ].a = a;
	prog->ops[ index ].b = b;
	prog->numOps++;
	return index;
}

void HfScriptCompiler::PatchJump( int opIndex, int target ) {
	if ( opIndex >= 0 && opIndex < prog->numOps ) {
		prog->ops[ opIndex ].a = target;
	}
}

void HfScriptCompiler::EmitConst( const HfScriptValue & v ) {
	Emit( HOP_CONST, prog->AddConstant( v ), 0 );
}

int HfScriptCompiler::CInt( int64 i ) {
	return prog->AddConstant( HfScriptValue::Int( i ) );
}

int HfScriptCompiler::CFloat( float f ) {
	return prog->AddConstant( HfScriptValue::Float( f ) );
}

int HfScriptCompiler::CStr( const char * s ) {
	int index = prog->PoolString( s );
	return prog->AddConstant( HfScriptValue::Str( prog->stringPool + index ) );
}

/*
========================
symbol tables
========================
*/
int HfScriptCompiler::FindGlobal( const char * name ) {
	for ( int i = 0; i < prog->numGlobals; i++ ) {
		if ( prog->globals[ i ].name == name ) {
			return prog->globals[ i ].index;
		}
	}
	return -1;
}

int HfScriptCompiler::DeclareGlobal( const char * name ) {
	int index = FindGlobal( name );
	if ( index >= 0 ) {
		return index;
	}
	return prog->AddGlobal( name );
}

int HfScriptCompiler::FindLocal( const char * name ) {
	for ( int i = 0; i < localNames.Num(); i++ ) {
		if ( localNames[ i ] == name ) {
			return i;
		}
	}
	return -1;
}

int HfScriptCompiler::DeclareLocal( const char * name ) {
	int index = FindLocal( name );
	if ( index >= 0 ) {
		return index;
	}
	localNames.Append( name );
	index = localNames.Num() - 1;
	if ( curFunction >= 0 && index >= prog->functions[ curFunction ].totalLocals ) {
		prog->functions[ curFunction ].totalLocals = index + 1;
	}
	return index;
}

int HfScriptCompiler::FindFunction( const char * name ) {
	for ( int i = 0; i < prog->numFunctions; i++ ) {
		if ( prog->functions[ i ].name == name ) {
			return i;
		}
	}
	return -1;
}

bool HfScriptCompiler::IsGlobalName( const char * name ) {
	if ( FindGlobal( name ) >= 0 ) {
		return true;
	}
	for ( int i = 0; i < globalRefs.Num(); i++ ) {
		if ( globalRefs[ i ] == name ) {
			return true;
		}
	}
	return false;
}

HfScriptCompiler::lval_t HfScriptCompiler::ResolveLValue( const char * name, bool forWrite ) {
	lval_t lv;
	if ( curFunction < 0 ) {
		// top level: everything is a global
		lv.tag = LV_GLOBAL;
		lv.index = DeclareGlobal( name );
		return lv;
	}
	if ( IsGlobalName( name ) ) {
		lv.tag = LV_GLOBAL;
		lv.index = DeclareGlobal( name );
		return lv;
	}
	if ( forWrite ) {
		// assignment creates a local unless the name is a known global
		lv.tag = LV_LOCAL;
		lv.index = DeclareLocal( name );
		return lv;
	}
	if ( FindLocal( name ) >= 0 ) {
		lv.tag = LV_LOCAL;
		lv.index = FindLocal( name );
		return lv;
	}
	// unknown read inside a function: fall back to a block-scoped local
	lv.tag = LV_LOCAL;
	lv.index = DeclareLocal( name );
	return lv;
}

void HfScriptCompiler::EmitLoad( const lval_t & lv ) {
	if ( lv.tag == LV_GLOBAL ) {
		Emit( HOP_GLOBAL, lv.index, 0 );
	} else {
		Emit( HOP_LOCAL, lv.index, 0 );
	}
}

void HfScriptCompiler::EmitStore( const lval_t & lv ) {
	if ( lv.tag == LV_GLOBAL ) {
		Emit( HOP_GLOBAL_SET, lv.index, 0 );
	} else {
		Emit( HOP_LOCAL_SET, lv.index, 0 );
	}
}

/*
========================
statements
========================
*/

// Compile the suite after a compound-statement header.  Handles:
//		: statement
//		: NEWLINE INDENT statements DEDENT
// In the Ruby dialect the ':' is optional and indentation still drives the
// block (the lexer emits INDENT/DEDENT from the indentation alone).
void HfScriptCompiler::CompileSuite() {
	if ( failed ) {
		return;
	}
	if ( !ruby ) {
		ExpectOp( ":" );
	} else if ( Cur().Is( ":" ) ) {
		pos++;
	}
	if ( Cur().type == HS_TK_NEWLINE ) {
		pos++;
		if ( Cur().type != HS_TK_INDENT ) {
			Error( "expected an indented block" );
			return;
		}
		pos++;
		for ( ;; ) {
			SkipNewlines();
			if ( Cur().type == HS_TK_DEDENT ) {
				pos++;
				return;
			}
			if ( Cur().type == HS_TK_EOF ) {
				Error( "unexpected end of file inside a block" );
				return;
			}
			if ( !ParseStatement() ) {
				return;
			}
		}
	}
	// single statement on the same line
	SkipNewlines();
	if ( Cur().type == HS_TK_DEDENT || Cur().type == HS_TK_EOF ) {
		return;
	}
	ParseSimpleStatement();
}

bool HfScriptCompiler::ParseStatement() {
	if ( failed ) {
		return false;
	}
	if ( MatchOp( ";" ) ) {
		return true;
	}
	if ( Cur().IsIdent( "def" ) ) {
		ParseDef();
		return !failed;
	}
	if ( Cur().IsIdent( "if" ) ) {
		ParseIf();
		return !failed;
	}
	if ( Cur().IsIdent( "while" ) ) {
		ParseWhile();
		return !failed;
	}
	if ( Cur().IsIdent( "for" ) ) {
		ParseFor();
		return !failed;
	}
	// Ruby closes blocks with an explicit 'end' keyword; indentation already
	// produced the DEDENT, so just consume the marker.
	if ( ruby && Cur().IsIdent( "end" ) ) {
		pos++;
		return true;
	}
	if ( Cur().type == HS_TK_DEDENT || Cur().type == HS_TK_EOF ) {
		return false;
	}
	ParseSimpleStatement();
	return !failed;
}

void HfScriptCompiler::ParseDef() {
	NoteLine( Cur().line );
	MatchIdent( "def" );
	if ( curFunction >= 0 ) {
		Error( "nested functions are not supported" );
		return;
	}
	if ( Cur().type != HS_TK_IDENT ) {
		Error( "expected function name after \"def\"" );
		return;
	}
	hfStr name = Cur().text;
	pos++;
	ExpectOp( "(" );

	// create the real function entry, matching the pass-1 signature
	curFunction = prog->AddFunction( name.c_str(), 0, 0 );

	// parameters become the first locals
	int numParams = 0;
	localNames.Clear();
	globalRefs.Clear();
	(void)curFunction;
	if ( !Cur().Is( ")" ) ) {
		for ( ;; ) {
			if ( Cur().type != HS_TK_IDENT ) {
				Error( "expected parameter name" );
				return;
			}
			hfStr param = Cur().text;
			pos++;
			DeclareLocal( param.c_str() );
			numParams++;
			if ( !MatchOp( "," ) ) {
				break;
			}
		}
	}
	ExpectOp( ")" );

	// note the position just before the ':' so CompileSuite can consume it
	// during the body pass; ScanFuncLocals needs the position after the
	// header (in the Ruby dialect there is no ':' and pos stays put).
	int colonPos = pos;
	if ( ruby ) {
		if ( Cur().Is( ":" ) ) {
			pos++;						// optional in Ruby
		}
	} else {
		if ( !Cur().Is( ":" ) ) {
			Error( "expected \":\" after function signature" );
			return;
		}
		pos++;							// consume ':'
	}

	prog->functions[ curFunction ].numParams = numParams;
	prog->functions[ curFunction ].line = Cur().line;

	// scan the body for locals before compiling it
	ScanFuncLocals( pos );

	// Function bodies are emitted inline, between the top-level statements
	// that surround the def.  The linear top-level op stream must never fall
	// through into a body, so emit an unconditional jump over it.
	int skipJump = -1;
	if ( !failed ) {
		skipJump = Emit( HOP_JUMP, 0, 0 );
	}

	// seed params + predeclared locals into the function layout; the
	// layout was sized during the scan, so just adopt it
	prog->functions[ curFunction ].totalLocals = localNames.Num();
	prog->functions[ curFunction ].opStart = Here();

	// rewind to the ':' and let CompileSuite consume it and the block
	pos = colonPos;
	int bodyLine = Cur().line;
	CompileSuite();
	prog->functions[ curFunction ].opEnd = Here();

	// implicit "return nil" so void calls still yield a value
	NoteLine( bodyLine );
	Emit( HOP_NIL, 0, 0 );
	Emit( HOP_RETURN, 0, 1 );
	PatchJump( skipJump, Here() );

	curFunction = -1;
	localNames.Clear();
	globalRefs.Clear();
}

void HfScriptCompiler::ParseIf() {
	NoteLine( Cur().line );
	pos++;											// consume 'if'
	ParseExpression();
	int jf = Emit( HOP_JUMP_FALSE, 0, 0 );
	CompileSuite();

	hfList<int> endPatches;
	bool sawElse = false;

	for ( ;; ) {
		if ( Cur().IsIdent( "elif" ) || ( ruby && Cur().IsIdent( "elsif" ) ) ) {
			int j = Emit( HOP_JUMP, 0, 0 );
			endPatches.Append( j );
			PatchJump( jf, Here() );
			pos++;
			NoteLine( Cur().line );
			ParseExpression();
			jf = Emit( HOP_JUMP_FALSE, 0, 0 );
			CompileSuite();
			continue;
		}
		if ( Cur().IsIdent( "else" ) ) {
			int j = Emit( HOP_JUMP, 0, 0 );
			endPatches.Append( j );
			PatchJump( jf, Here() );
			pos++;
			NoteLine( Cur().line );
			CompileSuite();
			sawElse = true;
			break;
		}
		break;
	}
	// Only the trailing JUMP_FALSE needs to fall through to the end when no
	// else branch consumed it.  (The old check tested the *next* token, which
	// after an else block is whatever statement follows - so it re-patched the
	// already-resolved else jump and made the else unreachable.)
	if ( !sawElse ) {
		PatchJump( jf, Here() );
	}
	int end = Here();
	for ( int i = 0; i < endPatches.Num(); i++ ) {
		PatchJump( endPatches[ i ], end );
	}
}

void HfScriptCompiler::ParseWhile() {
	NoteLine( Cur().line );
	pos++;											// consume 'while'
	int loopStart = Here();
	ParseExpression();
	int jf = Emit( HOP_JUMP_FALSE, 0, 0 );

	HfLoopInfo li;
	loopStack.Append( li );
	CompileSuite();

	// continue jumps back to the condition
	for ( int i = 0; i < loopStack[ loopStack.Num() - 1 ].continueOps.Num(); i++ ) {
		PatchJump( loopStack[ loopStack.Num() - 1 ].continueOps[ i ], loopStart );
	}
	// loop back to the condition
	Emit( HOP_JUMP, loopStart, 0 );
	int end = Here();
	PatchJump( jf, end );
	// break jumps past the loop
	for ( int i = 0; i < loopStack[ loopStack.Num() - 1 ].breakOps.Num(); i++ ) {
		PatchJump( loopStack[ loopStack.Num() - 1 ].breakOps[ i ], end );
	}
	loopStack.RemoveIndex( loopStack.Num() - 1 );
}

void HfScriptCompiler::ParseFor() {
	NoteLine( Cur().line );
	pos++;											// consume 'for'
	if ( Cur().type != HS_TK_IDENT ) {
		Error( "expected loop variable" );
		return;
	}
	hfStr loopVar = Cur().text;
	pos++;
	if ( !MatchIdent( "in" ) ) {
		Error( "expected \"in\"" );
		return;
	}
	if ( !MatchIdent( "range" ) ) {
		Error( "expected range(...) as the loop source" );
		return;
	}
	ExpectOp( "(" );

	// one or two arguments
	ParseExpression();								// a (or n)
	bool hasSecond = false;
	if ( MatchOp( "," ) ) {
		hasSecond = true;
		ParseExpression();							// b
	}
	ExpectOp( ")" );

	// reserve the loop variable + two hidden locals: counter, limit
	int lCounter = DeclareLocal( loopVar.c_str() );
	int lLimit = DeclareLocal( "__limit" );
	DeclareLocal( "__dummy" );

	if ( !hasSecond ) {
		// n is on the stack; counter starts at 0
		Emit( HOP_CONST, CInt( 0 ), 0 );
		Emit( HOP_LOCAL_SET, lCounter, 0 );
		Emit( HOP_POP, 0, 0 );
		Emit( HOP_LOCAL_SET, lLimit, 0 );			// n -> limit
		Emit( HOP_POP, 0, 0 );
	} else {
		// stack holds [a, b] with b on top; a -> counter, b -> limit
		Emit( HOP_LOCAL_SET, lLimit, 0 );			// b -> limit
		Emit( HOP_POP, 0, 0 );
		Emit( HOP_LOCAL_SET, lCounter, 0 );			// a -> counter
		Emit( HOP_POP, 0, 0 );
	}

	int loopStart = Here();
	Emit( HOP_LOCAL, lCounter, 0 );
	Emit( HOP_LOCAL, lLimit, 0 );
	Emit( HOP_CMP, 0, HSC_LT );
	int jf = Emit( HOP_JUMP_FALSE, 0, 0 );

	NoteLine( Cur().line );
	HfLoopInfo li;
	loopStack.Append( li );
	CompileSuite();

	// increment section: counter = counter + 1
	int incStart = Emit( HOP_LOCAL, lCounter, 0 );
	Emit( HOP_CONST, CInt( 1 ), 0 );
	Emit( HOP_ADD, 0, 0 );
	Emit( HOP_LOCAL_SET, lCounter, 0 );
	Emit( HOP_POP, 0, 0 );
	Emit( HOP_JUMP, loopStart, 0 );
	int end = Here();

	PatchJump( jf, end );
	for ( int i = 0; i < loopStack[ loopStack.Num() - 1 ].continueOps.Num(); i++ ) {
		PatchJump( loopStack[ loopStack.Num() - 1 ].continueOps[ i ], incStart );
	}
	for ( int i = 0; i < loopStack[ loopStack.Num() - 1 ].breakOps.Num(); i++ ) {
		PatchJump( loopStack[ loopStack.Num() - 1 ].breakOps[ i ], end );
	}
	loopStack.RemoveIndex( loopStack.Num() - 1 );
}

void HfScriptCompiler::ParseSimpleStatement() {
	NoteLine( Cur().line );

	if ( MatchIdent( "global" ) ) {
		// "global x" inside a function: name resolves to a global from here on
		if ( curFunction < 0 ) {
			Error( "\"global\" is only valid inside a function" );
			return;
		}
		if ( Cur().type != HS_TK_IDENT ) {
			Error( "expected variable name after \"global\"" );
			return;
		}
		globalRefs.Append( Cur().text );
		pos++;
		return;
	}
	if ( Cur().IsIdent( "return" ) ) {
		pos++;
		if ( curFunction < 0 ) {
			Error( "\"return\" outside of a function" );
			return;
		}
		if ( Cur().type == HS_TK_NEWLINE || Cur().type == HS_TK_DEDENT || Cur().type == HS_TK_EOF || Cur().Is( ";" ) ) {
			Emit( HOP_NIL, 0, 0 );
		} else {
			ParseExpression();
		}
		NoteLine( Cur().line );
		Emit( HOP_RETURN, 0, 1 );
		return;
	}
	if ( Cur().IsIdent( "break" ) ) {
		pos++;
		if ( loopStack.Num() == 0 ) {
			Error( "\"break\" outside of a loop" );
			return;
		}
		int j = Emit( HOP_JUMP, 0, 0 );
		loopStack[ loopStack.Num() - 1 ].breakOps.Append( j );
		return;
	}
	if ( Cur().IsIdent( "continue" ) ) {
		pos++;
		if ( loopStack.Num() == 0 ) {
			Error( "\"continue\" outside of a loop" );
			return;
		}
		int j = Emit( HOP_JUMP, 0, 0 );
		loopStack[ loopStack.Num() - 1 ].continueOps.Append( j );
		return;
	}
	if ( Cur().IsIdent( "pass" ) ) {
		pos++;
		return;
	}

	// assignment or expression statement; the value is discarded
	ParseExpression();
	if ( !failed ) {
		Emit( HOP_POP, 0, 0 );
	}
}

/*
========================
expressions
========================
*/
void HfScriptCompiler::ParseExpression() {
	if ( failed ) {
		return;
	}
	// assignment:  name = expr   or   name op= expr
	if ( Cur().type == HS_TK_IDENT ) {
		const HfScriptToken & nxt = Cur( 1 );
		if ( nxt.Is( "=" ) || nxt.Is( "+=" ) || nxt.Is( "-=" ) || nxt.Is( "*=" ) || nxt.Is( "/=" ) || nxt.Is( "%=" ) ) {
			ParseAssignment( nxt.text != "=" );
			return;
		}
	}
	ParseOr();
}

void HfScriptCompiler::ParseAssignment( bool compound ) {
	hfStr name = Cur().text;
	pos++;											// name
	hfStr op = Cur().text;
	pos++;											// op

	lval_t lv = ResolveLValue( name.c_str(), true );
	if ( lv.tag == LV_NONE ) {
		return;
	}

	if ( compound ) {
		// current value, then RHS, then combine, then store
		EmitLoad( lv );
		ParseOr();
		int binOp = HOP_ADD;
		if ( op == "-=" ) { binOp = HOP_SUB; }
		else if ( op == "*=" ) { binOp = HOP_MUL; }
		else if ( op == "/=" ) { binOp = HOP_DIV; }
		else if ( op == "%=" ) { binOp = HOP_MOD; }
		Emit( binOp, 0, 0 );
	} else {
		ParseOr();
	}
	EmitStore( lv );
}

void HfScriptCompiler::ParseOr() {
	ParseAnd();
	while ( !failed && ( Cur().Is( "or" ) || Cur().Is( "||" ) ) ) {
		pos++;
		ParseAnd();
		Emit( HOP_OR, 0, 0 );
	}
}

void HfScriptCompiler::ParseAnd() {
	ParseEquality();
	while ( !failed && ( Cur().Is( "and" ) || Cur().Is( "&&" ) ) ) {
		pos++;
		ParseEquality();
		Emit( HOP_AND, 0, 0 );
	}
}

void HfScriptCompiler::ParseEquality() {
	ParseRelational();
	for ( ;; ) {
		if ( Cur().Is( "==" ) ) {
			pos++;
			ParseRelational();
			Emit( HOP_CMP, 0, HSC_EQ );
			continue;
		}
		if ( Cur().Is( "!=" ) ) {
			pos++;
			ParseRelational();
			Emit( HOP_CMP, 0, HSC_NE );
			continue;
		}
		break;
	}
}

void HfScriptCompiler::ParseRelational() {
	ParseAdditive();
	for ( ;; ) {
		if ( Cur().Is( "<" ) ) {
			pos++;
			ParseAdditive();
			Emit( HOP_CMP, 0, HSC_LT );
			continue;
		}
		if ( Cur().Is( "<=" ) ) {
			pos++;
			ParseAdditive();
			Emit( HOP_CMP, 0, HSC_LE );
			continue;
		}
		if ( Cur().Is( ">" ) ) {
			pos++;
			ParseAdditive();
			Emit( HOP_CMP, 0, HSC_GT );
			continue;
		}
		if ( Cur().Is( ">=" ) ) {
			pos++;
			ParseAdditive();
			Emit( HOP_CMP, 0, HSC_GE );
			continue;
		}
		break;
	}
}

void HfScriptCompiler::ParseAdditive() {
	ParseMultiplicative();
	for ( ;; ) {
		if ( Cur().Is( "+" ) ) {
			pos++;
			ParseMultiplicative();
			Emit( HOP_ADD, 0, 0 );
			continue;
		}
		if ( Cur().Is( "-" ) ) {
			pos++;
			ParseMultiplicative();
			Emit( HOP_SUB, 0, 0 );
			continue;
		}
		break;
	}
}

void HfScriptCompiler::ParseMultiplicative() {
	ParseUnary();
	for ( ;; ) {
		if ( Cur().Is( "*" ) ) {
			pos++;
			ParseUnary();
			Emit( HOP_MUL, 0, 0 );
			continue;
		}
		if ( Cur().Is( "/" ) ) {
			pos++;
			ParseUnary();
			Emit( HOP_DIV, 0, 0 );
			continue;
		}
		if ( Cur().Is( "%" ) ) {
			pos++;
			ParseUnary();
			Emit( HOP_MOD, 0, 0 );
			continue;
		}
		break;
	}
}

void HfScriptCompiler::ParseUnary() {
	if ( Cur().Is( "-" ) ) {
		pos++;
		ParseUnary();
		Emit( HOP_NEG, 0, 0 );
		return;
	}
	if ( Cur().IsIdent( "not" ) || Cur().Is( "!" ) ) {
		pos++;
		ParseUnary();
		Emit( HOP_NOT, 0, 0 );
		return;
	}
	ParsePostfix();
}

void HfScriptCompiler::ParsePostfix() {
	ParsePrimary();
	// function calls
	for ( ;; ) {
		if ( !Cur().Is( "(" ) ) {
			break;
		}
		break;										// call handled in ParsePrimary
	}
}

void HfScriptCompiler::ParsePrimary() {
	if ( Cur().type == HS_TK_INT || Cur().type == HS_TK_FLOAT ) {
		EmitConst( Cur().value );
		pos++;
		return;
	}
	if ( Cur().type == HS_TK_STRING ) {
		// string literals starting with "#str_" are localized at compile time
		const char * text = Cur().text.c_str();
		const char * prefix = hfLangDict::KEY_PREFIX;
		int prefixLen = hfLangDict::KEY_PREFIX_LEN;
		bool localized = false;
		if ( prefixLen == 3 && text[ 0 ] == prefix[ 0 ] && text[ 1 ] == prefix[ 1 ] && text[ 2 ] == prefix[ 2 ] ) {
			localized = true;
		} else if ( prefixLen > 3 ) {
			int i;
			for ( i = 0; i < prefixLen && text[ i ] != 0; i++ ) {
				if ( text[ i ] != prefix[ i ] ) {
					break;
				}
			}
			localized = ( i == prefixLen );
		}
		if ( localized ) {
			text = hfLocalization::GetString( text );
		}
		Emit( HOP_CONST, CStr( text ), 0 );
		pos++;
		return;
	}
	if ( Cur().Is( "(" ) ) {
		pos++;
		ParseExpression();
		ExpectOp( ")" );
		return;
	}
	if ( Cur().type == HS_TK_IDENT ) {
		if ( Cur().IsIdent( "true" ) ) {
			Emit( HOP_TRUE, 0, 0 );
			pos++;
			return;
		}
		if ( Cur().IsIdent( "false" ) ) {
			Emit( HOP_FALSE, 0, 0 );
			pos++;
			return;
		}
		if ( Cur().IsIdent( "nil" ) ) {
			Emit( HOP_NIL, 0, 0 );
			pos++;
			return;
		}
		hfStr name = Cur().text;
		if ( Cur( 1 ).Is( "(" ) ) {
			pos++;									// name
			pos++;									// (
			int argc = 0;
			if ( !Cur().Is( ")" ) ) {
				ParseExpression();
				argc++;
				while ( MatchOp( "," ) && !failed ) {
					ParseExpression();
					argc++;
				}
			}
			ExpectOp( ")" );
			int hostIndex = HfScript_FindHostFunction( name.c_str() );
			if ( hostIndex >= 0 ) {
				Emit( HOP_CALL_HOST, hostIndex, argc );
				return;
			}
			int funcIndex = FindFunction( name.c_str() );
			if ( funcIndex >= 0 ) {
				Emit( HOP_CALL, funcIndex, argc );
				return;
			}
			// forward reference: resolve after all defs are compiled
			int op = Emit( HOP_CALL, -1, argc );
			HfExternalRef ref;
			ref.name = name;
			ref.op = op;
			externalRefs.Append( ref );
			return;
		}
		// variable reference
		pos++;
		lval_t lv = ResolveLValue( name.c_str(), false );
		if ( lv.tag == LV_NONE ) {
			return;
		}
		EmitLoad( lv );
		return;
	}
	hfStr msg = "unexpected token \"";
	msg += Cur().text;
	msg += "\"";
	Error( msg.c_str() );
}

/*
========================
two-pass local scan

Walks the tokens of a function body and predeclares local variables so a
local can be assigned after it is read, and global names are honoured.
========================
*/
void HfScriptCompiler::ScanFuncLocals( int bodyPos ) {
	pos = bodyPos;
	SkipNewlines();
	if ( Cur().type != HS_TK_INDENT ) {
		return;										// single-line body: nothing to scan
	}
	pos++;
	int depth = 1;
	for ( ;; ) {
		SkipNewlines();
		if ( Cur().type == HS_TK_DEDENT ) {
			pos++;
			depth--;
			if ( depth <= 0 ) {
				break;
			}
			continue;
		}
		if ( Cur().type == HS_TK_EOF ) {
			break;
		}

		if ( Cur().IsIdent( "global" ) ) {
			pos++;
			if ( Cur().type == HS_TK_IDENT ) {
				globalRefs.Append( Cur().text );
			}
		} else if ( Cur().IsIdent( "for" ) ) {
			pos++;
			if ( Cur().type == HS_TK_IDENT ) {
				const char * loopVar = Cur().text.c_str();
				if ( !IsGlobalName( loopVar ) && FindLocal( loopVar ) < 0 ) {
					DeclareLocal( loopVar );
				}
				pos++;
			}
			SkipToColon();
		} else if ( Cur().IsIdent( "if" ) || Cur().IsIdent( "while" ) ) {
			SkipToColon();
		} else if ( Cur().type == HS_TK_IDENT ) {
			// a name followed by '=' assigns; everything else is ignored
			if ( Cur( 1 ).Is( "=" ) || Cur( 1 ).Is( "+=" ) || Cur( 1 ).Is( "-=" ) || Cur( 1 ).Is( "*=" ) || Cur( 1 ).Is( "/=" ) || Cur( 1 ).Is( "%=" ) ) {
				const char * name = Cur().text.c_str();
				if ( !IsGlobalName( name ) && FindLocal( name ) < 0 ) {
					DeclareLocal( name );
				}
			}
			pos++;
		} else {
			pos++;
		}
	}
}

void HfScriptCompiler::SkipToColon() {
	int paren = 0;
	for ( ;; ) {
		if ( Cur().type == HS_TK_NEWLINE || Cur().type == HS_TK_DEDENT || Cur().type == HS_TK_EOF ) {
			return;
		}
		if ( Cur().Is( "(" ) ) {
			paren++;
		} else if ( Cur().Is( ")" ) ) {
			if ( paren > 0 ) {
				paren--;
			}
		} else if ( Cur().Is( ":" ) && paren == 0 ) {
			pos++;
			return;
		}
		pos++;
	}
}

/*
========================
HfScriptCompiler::Compile
========================
*/
bool HfScriptCompiler::Compile( HfScriptProgram * program, const char * source, const char * sourceName ) {
	prog = program;
	program->Reset();
	program->sourceName = sourceName;
	program->error = "";
	program->errorLine = 0;

	failed = false;
	pos = 0;
	lastLine = -1;
	curFunction = -1;
	firstEntryOp = -1;
	bool english = false;
	{
		// Ruby-flavored dialect selected by the .rb source tag.  Kept local so
		// the parser stays free of engine dependencies.
		ruby = false;
		if ( sourceName != NULL ) {
			int n = (int)strlen( sourceName );
			if ( n >= 3 && sourceName[ n - 3 ] == '.' && ( sourceName[ n - 2 ] == 'r' || sourceName[ n - 2 ] == 'R' ) && ( sourceName[ n - 1 ] == 'b' || sourceName[ n - 1 ] == 'B' ) ) {
				ruby = true;
			}
			// English-flavored dialect selected by the .eng source tag: the
			// token stream is rewritten into canonical hfscript and then run
			// through the Ruby-style block rules (colons/ends optional).
			if ( n >= 4 && sourceName[ n - 4 ] == '.' && ( sourceName[ n - 3 ] == 'e' || sourceName[ n - 3 ] == 'E' ) && ( sourceName[ n - 2 ] == 'n' || sourceName[ n - 2 ] == 'N' ) && ( sourceName[ n - 1 ] == 'g' || sourceName[ n - 1 ] == 'G' ) ) {
				english = true;
				ruby = true;
			}
		}
	}
	externalRefs.Clear();
	loopStack.Clear();
	localNames.Clear();
	globalRefs.Clear();

	hfStr lexError;
	// Source is a C string; NUL terminates the buffer, find the true length
	int length = (int)strlen( source );
	(void)length;
	HFS_TCK( "compiling" );
	if ( !HfScript_Tokenize( source, tokens, lexError ) ) {
		HFS_TCK( "tokenize FAILED" );
		program->error = lexError;
		program->errorLine = 1;
		return false;
	}
	if ( english ) {
		hfList<HfScriptToken> rewritten;
		if ( !HfScript_TranslateEnglish( tokens, rewritten, lexError ) ) {
			program->error = lexError;
			program->errorLine = 1;
			return false;
		}
		tokens = rewritten;
	}
	HFS_TCK( "tokenized" );
	if ( tokens.Num() == 0 ) {
		program->error = "empty source";
		return false;
	}

	// pass 1: collect function signatures (forward references); the real
	// function entries are created later in ParseDef, in the same order.
	signatures.Clear();
	for ( pos = 0; pos < tokens.Num() && !failed; ) {
		SkipNewlines();
		if ( Cur().IsIdent( "def" ) ) {
			// lightweight signature scan:  def name ( params ) :
			pos++;
			Sig sig;
			sig.numParams = 0;
			if ( Cur().type == HS_TK_IDENT ) {
				sig.name = Cur().text;
				pos++;
			}
			if ( Cur().Is( "(" ) ) {
				pos++;
				int depth = 1;
				while ( depth > 0 && pos < tokens.Num() ) {
					if ( Cur().Is( "(" ) ) {
						depth++;
					} else if ( Cur().Is( ")" ) ) {
						depth--;
					} else if ( Cur().type == HS_TK_IDENT && depth == 1 ) {
						sig.numParams++;
					}
					pos++;
				}
			}
			signatures.Append( sig );
			continue;
		}
		// skip an entire top-level statement
		for ( ;; ) {
			if ( pos >= tokens.Num() ) {
				break;
			}
			if ( Cur().type == HS_TK_NEWLINE ) {
				pos++;
				break;
			}
			pos++;
		}
	}

	HFS_TCK( "pass1 done" );
	if ( failed ) {
		return false;
	}

	// pass 2: compile everything
	pos = 0;
	if ( tokens.Num() > 0 ) {
		for ( ;; ) {
			if ( failed ) {
				break;
			}
			SkipNewlines();
			if ( Cur().type == HS_TK_EOF ) {
				break;
			}
			if ( firstEntryOp < 0 && !Cur().IsIdent( "def" ) ) {
				firstEntryOp = Here();
			}
			if ( !ParseStatement() ) {
				break;
			}
			SkipNewlines();
		}
	}

	HFS_TCK( "pass2 done" );
	// forward reference resolution
	for ( int i = 0; i < externalRefs.Num(); i++ ) {
		int funcIndex = FindFunction( externalRefs[ i ].name.c_str() );
		if ( funcIndex < 0 ) {
			prog->error = "call to undefined function \"";
			prog->error += externalRefs[ i ].name;
			prog->error += "\"";
			prog->errorLine = prog->ops[ externalRefs[ i ].op ].a;
			prog->ops[ externalRefs[ i ].op ].a = 0;
			prog->valid = false;
			return false;
		}
		prog->ops[ externalRefs[ i ].op ].a = funcIndex;
	}

	if ( failed ) {
		prog->valid = false;
		return false;
	}

	if ( firstEntryOp < 0 ) {
		firstEntryOp = Here();
	}
	Emit( HOP_END, 0, 0 );

	prog->entryOp = firstEntryOp;
	prog->entryFunction = FindFunction( "main" );
	prog->valid = true;
	return true;
}

/*
========================
HfScript_Compile
========================
*/
bool HfScript_Compile( HfScriptProgram * program, const char * source, const char * sourceName ) {
	if ( program == NULL || source == NULL ) {
		return false;
	}
	HfScriptCompiler compiler;
	return compiler.Compile( program, source, sourceName );
}