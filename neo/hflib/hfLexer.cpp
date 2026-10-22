#include "hfprecompiled.h"
#pragma hdrstop

#define PUNCTABLE

//longer punctuations first
punctuation_t default_punctuations[] = {
	//binary operators
	{">>=",P_RSHIFT_ASSIGN},
	{"<<=",P_LSHIFT_ASSIGN},
	//
	{"...",P_PARMS},
	//define merge operator
	{"##",P_PRECOMPMERGE},				// pre-compiler
	//logic operators
	{"&&",P_LOGIC_AND},					// pre-compiler
	{"||",P_LOGIC_OR},					// pre-compiler
	{">=",P_LOGIC_GEQ},					// pre-compiler
	{"<=",P_LOGIC_LEQ},					// pre-compiler
	{"==",P_LOGIC_EQ},					// pre-compiler
	{"!=",P_LOGIC_UNEQ},				// pre-compiler
	//arithmatic operators
	{"*=",P_MUL_ASSIGN},
	{"/=",P_DIV_ASSIGN},
	{"%=",P_MOD_ASSIGN},
	{"+=",P_ADD_ASSIGN},
	{"-=",P_SUB_ASSIGN},
	{"++",P_INC},
	{"--",P_DEC},
	//binary operators
	{"&=",P_BIN_AND_ASSIGN},
	{"|=",P_BIN_OR_ASSIGN},
	{"^=",P_BIN_XOR_ASSIGN},
	{">>",P_RSHIFT},					// pre-compiler
	{"<<",P_LSHIFT},					// pre-compiler
	//reference operators
	{"->",P_POINTERREF},
	//C++
	{"::",P_CPP1},
	{".*",P_CPP2},
	//arithmatic operators
	{"*",P_MUL},						// pre-compiler
	{"/",P_DIV},						// pre-compiler
	{"%",P_MOD},						// pre-compiler
	{"+",P_ADD},						// pre-compiler
	{"-",P_SUB},						// pre-compiler
	{"=",P_ASSIGN},
	//binary operators
	{"&",P_BIN_AND},					// pre-compiler
	{"|",P_BIN_OR},						// pre-compiler
	{"^",P_BIN_XOR},					// pre-compiler
	{"~",P_BIN_NOT},					// pre-compiler
	//logic operators
	{"!",P_LOGIC_NOT},					// pre-compiler
	{">",P_LOGIC_GREATER},				// pre-compiler
	{"<",P_LOGIC_LESS},					// pre-compiler
	//reference operator
	{".",P_REF},
	//seperators
	{",",P_COMMA},						// pre-compiler
	{";",P_SEMICOLON},
	//label indication
	{":",P_COLON},						// pre-compiler
	//if statement
	{"?",P_QUESTIONMARK},				// pre-compiler
	//embracements
	{"(",P_PARENTHESESOPEN},			// pre-compiler
	{")",P_PARENTHESESCLOSE},			// pre-compiler
	{"{",P_BRACEOPEN},					// pre-compiler
	{"}",P_BRACECLOSE},					// pre-compiler
	{"[",P_SQBRACKETOPEN},
	{"]",P_SQBRACKETCLOSE},
	//
	{"\\",P_BACKSLASH},
	//precompiler operator
	{"#",P_PRECOMP},					// pre-compiler
	{"$",P_DOLLAR},
	{NULL, 0}
};

int default_punctuationtable[256];
int default_nextpunctuation[sizeof(default_punctuations) / sizeof(punctuation_t)];
int default_setup;

char hfLexer::baseFolder[ 256 ];

/*
================
hfLexer::CreatePunctuationTable
================
*/
void hfLexer::CreatePunctuationTable( const punctuation_t *punctuations ) {
	int i, n, lastp;
	const punctuation_t *p, *newp;

	//get memory for the table
	if ( punctuations == default_punctuations ) {
		hfLexer::punctuationtable = default_punctuationtable;
		hfLexer::nextpunctuation = default_nextpunctuation;
		if ( default_setup ) {
			return;
		}
		default_setup = true;
		i = sizeof(default_punctuations) / sizeof(punctuation_t);
	}
	else {
		if ( !hfLexer::punctuationtable || hfLexer::punctuationtable == default_punctuationtable ) {
			hfLexer::punctuationtable = (int *) Mem_Alloc(256 * sizeof(int), TAG_IDLIB_LEXER);
		}
		if ( hfLexer::nextpunctuation && hfLexer::nextpunctuation != default_nextpunctuation ) {
			Mem_Free( hfLexer::nextpunctuation );
		}
		for (i = 0; punctuations[i].p; i++) {
		}
		hfLexer::nextpunctuation = (int *) Mem_Alloc(i * sizeof(int), TAG_IDLIB_LEXER);
	}
	memset(hfLexer::punctuationtable, 0xFF, 256 * sizeof(int));
	memset(hfLexer::nextpunctuation, 0xFF, i * sizeof(int));
	//add the punctuations in the list to the punctuation table
	for (i = 0; punctuations[i].p; i++) {
		newp = &punctuations[i];
		lastp = -1;
		//sort the punctuations in this table entry on length (longer punctuations first)
		for (n = hfLexer::punctuationtable[(unsigned int) newp->p[0]]; n >= 0; n = hfLexer::nextpunctuation[n] ) {
			p = &punctuations[n];
			if (strlen(p->p) < strlen(newp->p)) {
				hfLexer::nextpunctuation[i] = n;
				if (lastp >= 0) {
					hfLexer::nextpunctuation[lastp] = i;
				}
				else {
					hfLexer::punctuationtable[(unsigned int) newp->p[0]] = i;
				}
				break;
			}
			lastp = n;
		}
		if (n < 0) {
			hfLexer::nextpunctuation[i] = -1;
			if (lastp >= 0) {
				hfLexer::nextpunctuation[lastp] = i;
			}
			else {
				hfLexer::punctuationtable[(unsigned int) newp->p[0]] = i;
			}
		}
	}
}

/*
================
hfLexer::GetPunctuationFromId
================
*/
const char *hfLexer::GetPunctuationFromId( int id ) {
	int i;

	for (i = 0; hfLexer::punctuations[i].p; i++) {
		if ( hfLexer::punctuations[i].n == id ) {
			return hfLexer::punctuations[i].p;
		}
	}
	return "unkown punctuation";
}

/*
================
hfLexer::GetPunctuationId
================
*/
int hfLexer::GetPunctuationId( const char *p ) {
	int i;

	for (i = 0; hfLexer::punctuations[i].p; i++) {
		if ( !strcmp(hfLexer::punctuations[i].p, p) ) {
			return hfLexer::punctuations[i].n;
		}
	}
	return 0;
}

/*
================
hfLexer::Error
================
*/
void hfLexer::Error( const char *str, ... ) {
	char text[MAX_STRING_CHARS];
	va_list ap;

	hadError = true;

	if ( hfLexer::flags & LEXFL_NOERRORS ) {
		return;
	}

	va_start(ap, str);
	vsprintf(text, str, ap);
	va_end(ap);

	if ( hfLexer::flags & LEXFL_NOFATALERRORS ) {
		hfLib::common->Warning( "file %s, line %d: %s", hfLexer::filename.c_str(), hfLexer::line, text );
	} else {
		hfLib::common->Error( "file %s, line %d: %s", hfLexer::filename.c_str(), hfLexer::line, text );
	}
}

/*
================
hfLexer::Warning
================
*/
void hfLexer::Warning( const char *str, ... ) {
	char text[MAX_STRING_CHARS];
	va_list ap;

	if ( hfLexer::flags & LEXFL_NOWARNINGS ) {
		return;
	}

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );
	hfLib::common->Warning( "file %s, line %d: %s", hfLexer::filename.c_str(), hfLexer::line, text );
}

/*
================
hfLexer::SetPunctuations
================
*/
void hfLexer::SetPunctuations( const punctuation_t *p ) {
#ifdef PUNCTABLE
	if (p) {
		hfLexer::CreatePunctuationTable( p );
	}
	else {
		hfLexer::CreatePunctuationTable( default_punctuations );
	}
#endif //PUNCTABLE
	if (p) {
		hfLexer::punctuations = p;
	}
	else {
		hfLexer::punctuations = default_punctuations;
	}
}

/*
================
hfLexer::ReadWhiteSpace

Reads spaces, tabs, C-like comments etc.
When a newline character is found the scripts line counter is increased.
================
*/
int hfLexer::ReadWhiteSpace() {
	while(1) {
		// skip white space
		while(*hfLexer::script_p <= ' ') {
			if (!*hfLexer::script_p) {
				return 0;
			}
			if (*hfLexer::script_p == '\n') {
				hfLexer::line++;
			}
			hfLexer::script_p++;
		}
		// skip comments
		if (*hfLexer::script_p == '/') {
			// comments //
			if (*(hfLexer::script_p+1) == '/') {
				hfLexer::script_p++;
				do {
					hfLexer::script_p++;
					if ( !*hfLexer::script_p ) {
						return 0;
					}
				}
				while( *hfLexer::script_p != '\n' );
				hfLexer::line++;
				hfLexer::script_p++;
				if ( !*hfLexer::script_p ) {
					return 0;
				}
				continue;
			}
			// comments /* */
			else if (*(hfLexer::script_p+1) == '*') {
				hfLexer::script_p++;
				while( 1 ) {
					hfLexer::script_p++;
					if ( !*hfLexer::script_p ) {
						return 0;
					}
					if ( *hfLexer::script_p == '\n' ) {
						hfLexer::line++;
					}
					else if ( *hfLexer::script_p == '/' ) {
						if ( *(hfLexer::script_p-1) == '*' ) {
							break;
						}
						if ( *(hfLexer::script_p+1) == '*' ) {
							hfLexer::Warning( "nested comment" );
						}
					}
				}
				hfLexer::script_p++;
				if ( !*hfLexer::script_p ) {
					return 0;
				}
				hfLexer::script_p++;
				if ( !*hfLexer::script_p ) {
					return 0;
				}
				continue;
			}
		}
		break;
	}
	return 1;
}

/*
========================
hfLexer::SkipWhiteSpace

Reads spaces, tabs, C-like comments etc. When a newline character is found, the scripts line 
counter is increased. Returns false if there is no token left to be read.
========================
*/
bool hfLexer::SkipWhiteSpace( bool currentLine ) {
	while( 1 ) {
		assert( script_p <= end_p );
		if ( script_p == end_p ) {
			return false;
		}
		// skip white space
		while( *script_p <= ' ' ) {
			if ( script_p == end_p ) {
				return false;
			}
			if ( !*script_p ) {
				return false;
			}
			if ( *script_p == '\n' ) {
				line++;
				if ( currentLine ) {
					script_p++;
					return true;
				}
			}
			script_p++;
		}
		// skip comments
		if ( *script_p == '/' ) {
			// comments //
			if ( *(script_p+1) == '/' ) {
				script_p++;
				do {
					script_p++;
					if ( !*script_p ) {
						return false;
					}
				}
				while( *script_p != '\n' );
				line++;
				script_p++;
				if ( currentLine ) {
					return true;
				}
				if ( !*script_p ) {
					return false;
				}
				continue;
			}
			// comments /* */
			else if ( *(script_p+1) == '*' ) {
				script_p++;
				while( 1 ) {
					script_p++;
					if ( !*script_p ) {
						return false;
					}
					if ( *script_p == '\n' ) {
						line++;
					}
					else if ( *script_p == '/' ) {
						if ( *(script_p-1) == '*' ) {
							break;
						}
						if ( *(script_p+1) == '*' ) {
							Warning( "nested comment" );
						}
					}
				}
				script_p++;
				if ( !*script_p ) {
					return false;
				}
				continue;
			}
		}
		break;
	}
	return true;
}

/*
================
hfLexer::ReadEscapeCharacter
================
*/
int hfLexer::ReadEscapeCharacter( char *ch ) {
	int c, val, i;

	// step over the leading '\\'
	hfLexer::script_p++;
	// determine the escape character
	switch(*hfLexer::script_p) {
		case '\\': c = '\\'; break;
		case 'n': c = '\n'; break;
		case 'r': c = '\r'; break;
		case 't': c = '\t'; break;
		case 'v': c = '\v'; break;
		case 'b': c = '\b'; break;
		case 'f': c = '\f'; break;
		case 'a': c = '\a'; break;
		case '\'': c = '\''; break;
		case '\"': c = '\"'; break;
		case '\?': c = '\?'; break;
		case 'x':
		{
			hfLexer::script_p++;
			for (i = 0, val = 0; ; i++, hfLexer::script_p++) {
				c = *hfLexer::script_p;
				if (c >= '0' && c <= '9')
					c = c - '0';
				else if (c >= 'A' && c <= 'Z')
					c = c - 'A' + 10;
				else if (c >= 'a' && c <= 'z')
					c = c - 'a' + 10;
				else
					break;
				val = (val << 4) + c;
			}
			hfLexer::script_p--;
			if (val > 0xFF) {
				hfLexer::Warning( "too large value in escape character" );
				val = 0xFF;
			}
			c = val;
			break;
		}
		default: //NOTE: decimal ASCII code, NOT octal
		{
			if (*hfLexer::script_p < '0' || *hfLexer::script_p > '9') {
				hfLexer::Error("unknown escape char");
			}
			for (i = 0, val = 0; ; i++, hfLexer::script_p++) {
				c = *hfLexer::script_p;
				if (c >= '0' && c <= '9')
					c = c - '0';
				else
					break;
				val = val * 10 + c;
			}
			hfLexer::script_p--;
			if (val > 0xFF) {
				hfLexer::Warning( "too large value in escape character" );
				val = 0xFF;
			}
			c = val;
			break;
		}
	}
	// step over the escape character or the last digit of the number
	hfLexer::script_p++;
	// store the escape character
	*ch = c;
	// succesfully read escape character
	return 1;
}

/*
================
hfLexer::ReadString

Escape characters are interpretted.
Reads two strings with only a white space between them as one string.
================
*/
int hfLexer::ReadString( hfToken *token, int quote ) {
	int tmpline;
	const char *tmpscript_p;
	char ch;

	if ( quote == '\"' ) {
		token->type = TT_STRING;
	} else {
		token->type = TT_LITERAL;
	}

	// leading quote
	hfLexer::script_p++;

	while(1) {
		// if there is an escape character and escape characters are allowed
		if (*hfLexer::script_p == '\\' && !(hfLexer::flags & LEXFL_NOSTRINGESCAPECHARS)) {
			if ( !hfLexer::ReadEscapeCharacter( &ch ) ) {
				return 0;
			}
			token->AppendDirty( ch );
		}
		// if a trailing quote
		else if (*hfLexer::script_p == quote) {
			// step over the quote
			hfLexer::script_p++;
			// if consecutive strings should not be concatenated
			if ( (hfLexer::flags & LEXFL_NOSTRINGCONCAT) &&
					(!(hfLexer::flags & LEXFL_ALLOWBACKSLASHSTRINGCONCAT) || (quote != '\"')) ) {
				break;
			}

			tmpscript_p = hfLexer::script_p;
			tmpline = hfLexer::line;
			// read white space between possible two consecutive strings
			if ( !hfLexer::ReadWhiteSpace() ) {
				hfLexer::script_p = tmpscript_p;
				hfLexer::line = tmpline;
				break;
			}

			if ( hfLexer::flags & LEXFL_NOSTRINGCONCAT ) {
				if ( *hfLexer::script_p != '\\' ) {
					hfLexer::script_p = tmpscript_p;
					hfLexer::line = tmpline;
					break;
				}
				// step over the '\\'
				hfLexer::script_p++;
				if ( !hfLexer::ReadWhiteSpace() || ( *hfLexer::script_p != quote ) ) {
					hfLexer::Error( "expecting string after '\' terminated line" );
					return 0;
				}
			}

			// if there's no leading qoute
			if ( *hfLexer::script_p != quote ) {
				hfLexer::script_p = tmpscript_p;
				hfLexer::line = tmpline;
				break;
			}
			// step over the new leading quote
			hfLexer::script_p++;
		}
		else {
			if (*hfLexer::script_p == '\0') {
				hfLexer::Error( "missing trailing quote" );
				return 0;
			}
			if (*hfLexer::script_p == '\n') {
				hfLexer::Error( "newline inside string" );
				return 0;
			}
			token->AppendDirty( *hfLexer::script_p++ );
		}
	}
	token->data[token->len] = '\0';

	if ( token->type == TT_LITERAL ) {
		if ( !(hfLexer::flags & LEXFL_ALLOWMULTICHARLITERALS) ) {
			if ( token->Length() != 1 ) {
				hfLexer::Warning( "literal is not one character long" );
			}
		}
		token->subtype = (*token)[0];
	}
	else {
		// the sub type is the length of the string
		token->subtype = token->Length();
	}
	return 1;
}

/*
================
hfLexer::ReadName
================
*/
int hfLexer::ReadName( hfToken *token ) {
	char c;

	token->type = TT_NAME;
	do {
		token->AppendDirty( *hfLexer::script_p++ );
		c = *hfLexer::script_p;
	} while ((c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z') ||
				(c >= '0' && c <= '9') ||
				c == '_' ||
				// if treating all tokens as strings, don't parse '-' as a seperate token
				((hfLexer::flags & LEXFL_ONLYSTRINGS) && (c == '-')) ||
				// if special path name characters are allowed
				((hfLexer::flags & LEXFL_ALLOWPATHNAMES) && (c == '/' || c == '\\' || c == ':' || c == '.')) );
	token->data[token->len] = '\0';
	//the sub type is the length of the name
	token->subtype = token->Length();
	return 1;
}

/*
================
hfLexer::CheckString
================
*/
HF_INLINE int hfLexer::CheckString( const char *str ) const {
	int i;

	for ( i = 0; str[i]; i++ ) {
		if ( hfLexer::script_p[i] != str[i] ) {
			return false;
		}
	}
	return true;
}

/*
================
hfLexer::ReadNumber
================
*/
int hfLexer::ReadNumber( hfToken *token ) {
	int i;
	int dot;
	char c, c2;

	token->type = TT_NUMBER;
	token->subtype = 0;
	token->intvalue = 0;
	token->floatvalue = 0;

	c = *hfLexer::script_p;
	c2 = *(hfLexer::script_p + 1);

	if ( c == '0' && c2 != '.' ) {
		// check for a hexadecimal number
		if ( c2 == 'x' || c2 == 'X' ) {
			token->AppendDirty( *hfLexer::script_p++ );
			token->AppendDirty( *hfLexer::script_p++ );
			c = *hfLexer::script_p;
			while((c >= '0' && c <= '9') ||
						(c >= 'a' && c <= 'f') ||
						(c >= 'A' && c <= 'F')) {
				token->AppendDirty( c );
				c = *(++hfLexer::script_p);
			}
			token->subtype = TT_HEX | TT_INTEGER;
		}
		// check for a binary number
		else if ( c2 == 'b' || c2 == 'B' ) {
			token->AppendDirty( *hfLexer::script_p++ );
			token->AppendDirty( *hfLexer::script_p++ );
			c = *hfLexer::script_p;
			while( c == '0' || c == '1' ) {
				token->AppendDirty( c );
				c = *(++hfLexer::script_p);
			}
			token->subtype = TT_BINARY | TT_INTEGER;
		}
		// its an octal number
		else {
			token->AppendDirty( *hfLexer::script_p++ );
			c = *hfLexer::script_p;
			while( c >= '0' && c <= '7' ) {
				token->AppendDirty( c );
				c = *(++hfLexer::script_p);
			}
			token->subtype = TT_OCTAL | TT_INTEGER;
		}
	}
	else {
		// decimal integer or floating point number or ip address
		dot = 0;
		while( 1 ) {
			if ( c >= '0' && c <= '9' ) {
			}
			else if ( c == '.' ) {
				dot++;
			}
			else {
				break;
			}
			token->AppendDirty( c );
			c = *(++hfLexer::script_p);
		}
		if( c == 'e' && dot == 0) {
			//We have scientific notation without a decimal point
			dot++;
		}
		// if a floating point number
		if ( dot == 1 ) {
			token->subtype = TT_DECIMAL | TT_FLOAT;
			// check for floating point exponent
			if ( c == 'e' ) {
				//Append the e so that GetFloatValue code works
				token->AppendDirty( c );
				c = *(++hfLexer::script_p);
				if ( c == '-' ) {
					token->AppendDirty( c );
					c = *(++hfLexer::script_p);
				}
				else if ( c == '+' ) {
					token->AppendDirty( c );
					c = *(++hfLexer::script_p);
				}
				while( c >= '0' && c <= '9' ) {
					token->AppendDirty( c );
					c = *(++hfLexer::script_p);
				}
			}
			// check for floating point exception infinite 1.#INF or indefinite 1.#IND or NaN
			else if ( c == '#' ) {
				c2 = 4;
				if ( CheckString( "INF" ) ) {
					token->subtype |= TT_INFINITE;
				}
				else if ( CheckString( "IND" ) ) {
					token->subtype |= TT_INDEFINITE;
				}
				else if ( CheckString( "NAN" ) ) {
					token->subtype |= TT_NAN;
				}
				else if ( CheckString( "QNAN" ) ) {
					token->subtype |= TT_NAN;
					c2++;
				}
				else if ( CheckString( "SNAN" ) ) {
					token->subtype |= TT_NAN;
					c2++;
				}
				for ( i = 0; i < c2; i++ ) {
					token->AppendDirty( c );
					c = *(++hfLexer::script_p);
				}
				while( c >= '0' && c <= '9' ) {
					token->AppendDirty( c );
					c = *(++hfLexer::script_p);
				}
				if ( !(hfLexer::flags & LEXFL_ALLOWFLOATEXCEPTIONS) ) {
					token->AppendDirty( 0 );	// zero terminate for c_str
					hfLexer::Error( "parsed %s", token->c_str() );
				}
			}
		}
		else if ( dot > 1 ) {
			if ( !( hfLexer::flags & LEXFL_ALLOWIPADDRESSES ) ) {
				hfLexer::Error( "more than one dot in number" );
				return 0;
			}
			if ( dot != 3 ) {
				hfLexer::Error( "ip address should have three dots" );
				return 0;
			}
			token->subtype = TT_IPADDRESS;
		}
		else {
			token->subtype = TT_DECIMAL | TT_INTEGER;
		}
	}

	if ( token->subtype & TT_FLOAT ) {
		if ( c > ' ' ) {
			// single-precision: float
			if ( c == 'f' || c == 'F' ) {
				token->subtype |= TT_SINGLE_PRECISION;
				hfLexer::script_p++;
			}
			// extended-precision: long double
			else if ( c == 'l' || c == 'L' ) {
				token->subtype |= TT_EXTENDED_PRECISION;
				hfLexer::script_p++;
			}
			// default is double-precision: double
			else {
				token->subtype |= TT_DOUBLE_PRECISION;
			}
		}
		else {
			token->subtype |= TT_DOUBLE_PRECISION;
		}
	}
	else if ( token->subtype & TT_INTEGER ) {
		if ( c > ' ' ) {
			// default: signed long
			for ( i = 0; i < 2; i++ ) {
				// long integer
				if ( c == 'l' || c == 'L' ) {
					token->subtype |= TT_LONG;
				}
				// unsigned integer
				else if ( c == 'u' || c == 'U' ) {
					token->subtype |= TT_UNSIGNED;
				}
				else {
					break;
				}
				c = *(++hfLexer::script_p);
			}
		}
	}
	else if ( token->subtype & TT_IPADDRESS ) {
		if ( c == ':' ) {
			token->AppendDirty( c );
			c = *(++hfLexer::script_p);
			while( c >= '0' && c <= '9' ) {
				token->AppendDirty( c );
				c = *(++hfLexer::script_p);
			}
			token->subtype |= TT_IPPORT;
		}
	}
	token->data[token->len] = '\0';
	return 1;
}

/*
================
hfLexer::ReadPunctuation
================
*/
int hfLexer::ReadPunctuation( hfToken *token ) {
	int l, n, i;
	char *p;
	const punctuation_t *punc;

#ifdef PUNCTABLE
	for (n = hfLexer::punctuationtable[(unsigned int)*(hfLexer::script_p)]; n >= 0; n = hfLexer::nextpunctuation[n])
	{
		punc = &(hfLexer::punctuations[n]);
#else
	int i;

	for (i = 0; hfLexer::punctuations[i].p; i++) {
		punc = &hfLexer::punctuations[i];
#endif
		p = punc->p;
		// check for this punctuation in the script
		for ( l = 0; p[l] && hfLexer::script_p[l]; l++ ) {
			if ( hfLexer::script_p[l] != p[l] ) {
				break;
			}
		}
		if ( !p[l] ) {
			//
			token->EnsureAlloced( l+1, false );
			for ( i = 0; i <= l; i++ ) {
				token->data[i] = p[i];
			}
			token->len = l;
			//
			hfLexer::script_p += l;
			token->type = TT_PUNCTUATION;
			// sub type is the punctuation id
			token->subtype = punc->n;
			return 1;
		}
	}
	return 0;
}

/*
================
hfLexer::ReadToken
================
*/
int hfLexer::ReadToken( hfToken *token ) {
	int c;

	if ( !loaded ) {
		hfLib::common->Error( "hfLexer::ReadToken: no file loaded" );
		return 0;
	}

	if ( script_p == NULL ) {
		return 0;
	}

	// if there is a token available (from unreadToken)
	if ( tokenavailable ) {
		tokenavailable = 0;
		*token = hfLexer::token;
		return 1;
	}
	// save script pointer
	lastScript_p = script_p;
	// save line counter
	lastline = line;
	// clear the token stuff
	token->data[0] = '\0';
	token->len = 0;
	// start of the white space
	whiteSpaceStart_p = script_p;
	token->whiteSpaceStart_p = script_p;
	// read white space before token
	if ( !ReadWhiteSpace() ) {
		return 0;
	}
	// end of the white space
	hfLexer::whiteSpaceEnd_p = script_p;
	token->whiteSpaceEnd_p = script_p;
	// line the token is on
	token->line = line;
	// number of lines crossed before token
	token->linesCrossed = line - lastline;
	// clear token flags
	token->flags = 0;

	c = *hfLexer::script_p;

	// if we're keeping everything as whitespace deliminated strings
	if ( hfLexer::flags & LEXFL_ONLYSTRINGS ) {
		// if there is a leading quote
		if ( c == '\"' || c == '\'' ) {
			if (!hfLexer::ReadString( token, c )) {
				return 0;
			}
		} else if ( !hfLexer::ReadName( token ) ) {
			return 0;
		}
	}
	// if there is a number
	else if ( (c >= '0' && c <= '9') ||
			(c == '.' && (*(hfLexer::script_p + 1) >= '0' && *(hfLexer::script_p + 1) <= '9')) ) {
		if ( !hfLexer::ReadNumber( token ) ) {
			return 0;
		}
		// if names are allowed to start with a number
		if ( hfLexer::flags & LEXFL_ALLOWNUMBERNAMES ) {
			c = *hfLexer::script_p;
			if ( (c >= 'a' && c <= 'z') ||	(c >= 'A' && c <= 'Z') || c == '_' ) {
				if ( !hfLexer::ReadName( token ) ) {
					return 0;
				}
			}
		}
	}
	// if there is a leading quote
	else if ( c == '\"' || c == '\'' ) {
		if (!hfLexer::ReadString( token, c )) {
			return 0;
		}
	}
	// if there is a name
	else if ( (c >= 'a' && c <= 'z') ||	(c >= 'A' && c <= 'Z') || c == '_' ) {
		if ( !hfLexer::ReadName( token ) ) {
			return 0;
		}
	}
	// names may also start with a slash when pathnames are allowed
	else if ( ( hfLexer::flags & LEXFL_ALLOWPATHNAMES ) && ( (c == '/' || c == '\\') || c == '.' ) ) {
		if ( !hfLexer::ReadName( token ) ) {
			return 0;
		}
	}
	// check for punctuations
	else if ( !hfLexer::ReadPunctuation( token ) ) {
		hfLexer::Error( "unknown punctuation %c", c );
		return 0;
	}
	// succesfully read a token
	return 1;
}

/*
================
hfLexer::ExpectTokenString
================
*/
int hfLexer::ExpectTokenString( const char *string ) {
	hfToken token;

	if (!hfLexer::ReadToken( &token )) {
		hfLexer::Error( "couldn't find expected '%s'", string );
		return 0;
	}
	if ( token != string ) {
		hfLexer::Error( "expected '%s' but found '%s'", string, token.c_str() );
		return 0;
	}
	return 1;
}

/*
================
hfLexer::ExpectTokenType
================
*/
int hfLexer::ExpectTokenType( int type, int subtype, hfToken *token ) {
	hfStr str;

	if ( !hfLexer::ReadToken( token ) ) {
		hfLexer::Error( "couldn't read expected token" );
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
		hfLexer::Error( "expected a %s but found '%s'", str.c_str(), token->c_str() );
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
			hfLexer::Error( "expected %s but found '%s'", str.c_str(), token->c_str() );
			return 0;
		}
	}
	else if ( token->type == TT_PUNCTUATION ) {
		if ( subtype < 0 ) {
			hfLexer::Error( "BUG: wrong punctuation subtype" );
			return 0;
		}
		if ( token->subtype != subtype ) {
			hfLexer::Error( "expected '%s' but found '%s'", GetPunctuationFromId( subtype ), token->c_str() );
			return 0;
		}
	}
	return 1;
}

/*
================
hfLexer::ExpectAnyToken
================
*/
int hfLexer::ExpectAnyToken( hfToken *token ) {
	if (!hfLexer::ReadToken( token )) {
		hfLexer::Error( "couldn't read expected token" );
		return 0;
	}
	else {
		return 1;
	}
}

/*
================
hfLexer::CheckTokenString
================
*/
int hfLexer::CheckTokenString( const char *string ) {
	hfToken tok;

	if ( !ReadToken( &tok ) ) {
		return 0;
	}
	// if the given string is available
	if ( tok == string ) {
		return 1;
	}
	// unread token
	script_p = lastScript_p;
	line = lastline;
	return 0;
}

/*
================
hfLexer::CheckTokenType
================
*/
int hfLexer::CheckTokenType( int type, int subtype, hfToken *token ) {
	hfToken tok;

	if ( !ReadToken( &tok ) ) {
		return 0;
	}
	// if the type matches
	if (tok.type == type && (tok.subtype & subtype) == subtype) {
		*token = tok;
		return 1;
	}
	// unread token
	script_p = lastScript_p;
	line = lastline;
	return 0;
}

/*
================
hfLexer::PeekTokenString
================
*/
int hfLexer::PeekTokenString( const char *string ) {
	hfToken tok;

	if ( !ReadToken( &tok ) ) {
		return 0;
	}

	// unread token
	script_p = lastScript_p;
	line = lastline;

	// if the given string is available
	if ( tok == string ) {
		return 1;
	}
	return 0;
}

/*
================
hfLexer::PeekTokenType
================
*/
int hfLexer::PeekTokenType( int type, int subtype, hfToken *token ) {
	hfToken tok;

	if ( !ReadToken( &tok ) ) {
		return 0;
	}

	// unread token
	script_p = lastScript_p;
	line = lastline;

	// if the type matches
	if ( tok.type == type && ( tok.subtype & subtype ) == subtype ) {
		*token = tok;
		return 1;
	}
	return 0;
}

/*
================
hfLexer::SkipUntilString
================
*/
int hfLexer::SkipUntilString( const char *string ) {
	hfToken token;

	while(hfLexer::ReadToken( &token )) {
		if ( token == string ) {
			return 1;
		}
	}
	return 0;
}

/*
================
hfLexer::SkipRestOfLine
================
*/
int hfLexer::SkipRestOfLine() {
	hfToken token;

	while(hfLexer::ReadToken( &token )) {
		if ( token.linesCrossed ) {
			hfLexer::script_p = lastScript_p;
			hfLexer::line = lastline;
			return 1;
		}
	}
	return 0;
}

/*
=================
hfLexer::SkipBracedSection

Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
int hfLexer::SkipBracedSection( bool parseFirstBrace ) {
	hfToken token;
	int depth;

	depth = parseFirstBrace ? 0 : 1;
	do {
		if ( !ReadToken( &token ) ) {
			return false;
		}
		if ( token.type == TT_PUNCTUATION ) {
			if ( token == "{" ) {
				depth++;
			} else if ( token == "}" ) {
				depth--;
			}
		}
	} while( depth );
	return true;
}

/*
================
hfLexer::UnreadToken
================
*/
void hfLexer::UnreadToken( const hfToken *token ) {
	if ( hfLexer::tokenavailable ) {
		hfLib::common->FatalError( "hfLexer::unreadToken, unread token twice\n" );
	}
	hfLexer::token = *token;
	hfLexer::tokenavailable = 1;
}

/*
================
hfLexer::ReadTokenOnLine
================
*/
int hfLexer::ReadTokenOnLine( hfToken *token ) {
	hfToken tok;

	if (!hfLexer::ReadToken( &tok )) {
		hfLexer::script_p = lastScript_p;
		hfLexer::line = lastline;
		return false;
	}
	// if no lines were crossed before this token
	if ( !tok.linesCrossed ) {
		*token = tok;
		return true;
	}
	// restore our position
	hfLexer::script_p = lastScript_p;
	hfLexer::line = lastline;
	token->Clear();
	return false;
}

/*
================
hfLexer::ReadRestOfLine
================
*/
const char*	hfLexer::ReadRestOfLine(hfStr& out) {
	while(1) {

		if(*hfLexer::script_p == '\n') {
			hfLexer::line++;
			break;
		}

		if(!*hfLexer::script_p) {
			break;
		}

		if(*hfLexer::script_p <= ' ') {
			out += " ";
		} else {
			out += *hfLexer::script_p;
		}
		hfLexer::script_p++;

	}

	out.Strip(' ');
	return out.c_str();
}

/*
================
hfLexer::ParseInt
================
*/
int hfLexer::ParseInt() {
	hfToken token;

	if ( !hfLexer::ReadToken( &token ) ) {
		hfLexer::Error( "couldn't read expected integer" );
		return 0;
	}
	if ( token.type == TT_PUNCTUATION && token == "-" ) {
		hfLexer::ExpectTokenType( TT_NUMBER, TT_INTEGER, &token );
		return -((signed int) token.GetIntValue());
	}
	else if ( token.type != TT_NUMBER || token.subtype == TT_FLOAT ) {
		hfLexer::Error( "expected integer value, found '%s'", token.c_str() );
	}
	return token.GetIntValue();
}

/*
================
hfLexer::ParseBool
================
*/
bool hfLexer::ParseBool() {
	hfToken token;

	if ( !hfLexer::ExpectTokenType( TT_NUMBER, 0, &token ) ) {
		hfLexer::Error( "couldn't read expected boolean" );
		return false;
	}
	return ( token.GetIntValue() != 0 );
}

/*
================
hfLexer::ParseFloat
================
*/
float hfLexer::ParseFloat( bool *errorFlag ) {
	hfToken token;

	if ( errorFlag ) {
		*errorFlag = false;
	}

	if ( !hfLexer::ReadToken( &token ) ) {
		if ( errorFlag ) {
			hfLexer::Warning( "couldn't read expected floating point number" );
			*errorFlag = true;
		} else {
			hfLexer::Error( "couldn't read expected floating point number" );
		}
		return 0;
	}
	if ( token.type == TT_PUNCTUATION && token == "-" ) {
		hfLexer::ExpectTokenType( TT_NUMBER, 0, &token );
		return -token.GetFloatValue();
	}
	else if ( token.type != TT_NUMBER ) {
		if ( errorFlag ) {
			hfLexer::Warning( "expected float value, found '%s'", token.c_str() );
			*errorFlag = true;
		} else {
			hfLexer::Error( "expected float value, found '%s'", token.c_str() );
		}
	}
	return token.GetFloatValue();
}

/*
================
hfLexer::Parse1DMatrix
================
*/
int hfLexer::Parse1DMatrix( int x, float *m ) {
	int i;

	if ( !hfLexer::ExpectTokenString( "(" ) ) {
		return false;
	}

	for ( i = 0; i < x; i++ ) {
		m[i] = hfLexer::ParseFloat();
	}

	if ( !hfLexer::ExpectTokenString( ")" ) ) {
		return false;
	}
	return true;
}

/*
================
hfLexer::Parse2DMatrix
================
*/
int hfLexer::Parse2DMatrix( int y, int x, float *m ) {
	int i;

	if ( !hfLexer::ExpectTokenString( "(" ) ) {
		return false;
	}

	for ( i = 0; i < y; i++ ) {
		if ( !hfLexer::Parse1DMatrix( x, m + i * x ) ) {
			return false;
		}
	}

	if ( !hfLexer::ExpectTokenString( ")" ) ) {
		return false;
	}
	return true;
}

/*
================
hfLexer::Parse3DMatrix
================
*/
int hfLexer::Parse3DMatrix( int z, int y, int x, float *m ) {
	int i;

	if ( !hfLexer::ExpectTokenString( "(" ) ) {
		return false;
	}

	for ( i = 0 ; i < z; i++ ) {
		if ( !hfLexer::Parse2DMatrix( y, x, m + i * x*y ) ) {
			return false;
		}
	}

	if ( !hfLexer::ExpectTokenString( ")" ) ) {
		return false;
	}
	return true;
}

/*
=================
hfParser::ParseBracedSection

The next token should be an open brace.
Parses until a matching close brace is found.
Maintains exact characters between braces.

  FIXME: this should use ReadToken and replace the token white space with correct indents and newlines
=================
*/
const char *hfLexer::ParseBracedSectionExact( hfStr &out, int tabs ) {
	int		depth;
	bool	doTabs;
	bool	skipWhite;

	out.Empty();

	if ( !hfLexer::ExpectTokenString( "{" ) ) {
		return out.c_str( );
	}

	out = "{";
	depth = 1;	
	skipWhite = false;
	doTabs = tabs >= 0;

	while( depth && *hfLexer::script_p ) {
		char c = *(hfLexer::script_p++);

		switch ( c ) {
			case '\t':
			case ' ': {
				if ( skipWhite ) {
					continue;
				}
				break;
			}
			case '\n': {
				if ( doTabs ) {
					skipWhite = true;
					out += c;
					continue;
				}
				break;
			}
			case '{': {
				depth++;
				tabs++;
				break;
			}
			case '}': {
				depth--;
				tabs--;
				break;				
			}
		}

		if ( skipWhite ) {
			int i = tabs;
			if ( c == '{' ) {
				i--;
			}
			skipWhite = false;
			for ( ; i > 0; i-- ) {
				out += '\t';
			}
		}
		out += c;
	}
	return out.c_str();
}

/*
=================
hfLexer::ParseBracedSection

The next token should be an open brace.
Parses until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
const char *hfLexer::ParseBracedSection( hfStr &out ) {
	hfToken token;
	int i, depth;

	out.Empty();
	if ( !hfLexer::ExpectTokenString( "{" ) ) {
		return out.c_str();
	}
	out = "{";
	depth = 1;
	do {
		if ( !hfLexer::ReadToken( &token ) ) {
			Error( "missing closing brace" );
			return out.c_str();
		}

		// if the token is on a new line
		for ( i = 0; i < token.linesCrossed; i++ ) {
			out += "\r\n";
		}

		if ( token.type == TT_PUNCTUATION ) {
			if ( token[0] == '{' ) {
				depth++;
			}
			else if ( token[0] == '}' ) {
				depth--;
			}
		}

		if ( token.type == TT_STRING ) {
			out += "\"" + token + "\"";
		}
		else {
			out += token;
		}
		out += " ";
	} while( depth );

	return out.c_str();
}

/*
=================
hfLexer::ParseRestOfLine

  parse the rest of the line
=================
*/
const char *hfLexer::ParseRestOfLine( hfStr &out ) {
	hfToken token;

	out.Empty();
	while(hfLexer::ReadToken( &token )) {
		if ( token.linesCrossed ) {
			hfLexer::script_p = lastScript_p;
			hfLexer::line = lastline;
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
========================
hfLexer::ParseCompleteLine

Returns a string up to the \n, but doesn't eat any whitespace at the beginning of the next line.
========================
*/
const char *hfLexer::ParseCompleteLine( hfStr &out ) {
	hfToken token;
	const char	*start;

	start = script_p;

	while ( 1 ) {
		// end of buffer
		if ( *script_p == 0 ) {
			break;
		}
		if ( *script_p == '\n' ) {
			line++;
			script_p++;
			break;
		}
		script_p++;
	}

	out.Empty();
	out.Append( start, script_p - start );

	return out.c_str();
}

/*
================
hfLexer::GetLastWhiteSpace
================
*/
int hfLexer::GetLastWhiteSpace( hfStr &whiteSpace ) const {
	whiteSpace.Clear();
	for ( const char *p = whiteSpaceStart_p; p < whiteSpaceEnd_p; p++ ) {
		whiteSpace.Append( *p );
	}
	return whiteSpace.Length();
}

/*
================
hfLexer::GetLastWhiteSpaceStart
================
*/
int hfLexer::GetLastWhiteSpaceStart() const {
	return whiteSpaceStart_p - buffer;
}

/*
================
hfLexer::GetLastWhiteSpaceEnd
================
*/
int hfLexer::GetLastWhiteSpaceEnd() const {
	return whiteSpaceEnd_p - buffer;
}

/*
================
hfLexer::Reset
================
*/
void hfLexer::Reset() {
	// pointer in script buffer
	hfLexer::script_p = hfLexer::buffer;
	// pointer in script buffer before reading token
	hfLexer::lastScript_p = hfLexer::buffer;
	// begin of white space
	hfLexer::whiteSpaceStart_p = NULL;
	// end of white space
	hfLexer::whiteSpaceEnd_p = NULL;
	// set if there's a token available in hfLexer::token
	hfLexer::tokenavailable = 0;

	hfLexer::line = 1;
	hfLexer::lastline = 1;
	// clear the saved token
	hfLexer::token = "";
}

/*
================
hfLexer::EndOfFile
================
*/
bool hfLexer::EndOfFile() {
	return hfLexer::script_p >= hfLexer::end_p;
}

/*
================
hfLexer::NumLinesCrossed
================
*/
int hfLexer::NumLinesCrossed() {
	return hfLexer::line - hfLexer::lastline;
}

/*
================
hfLexer::LoadFile
================
*/
int hfLexer::LoadFile( const char *filename, bool OSPath ) {
	hfFile *fp;
	hfStr pathname;
	int length;
	char *buf;

	if ( hfLexer::loaded ) {
		hfLib::common->Error("hfLexer::LoadFile: another script already loaded");
		return false;
	}
	
	if ( !OSPath && ( baseFolder[0] != '\0' ) ) {
		pathname = va( "%s/%s", baseFolder, filename );
	} else {
		pathname = filename;
	}
	if ( OSPath ) {
		fp = hfLib::fileSystem->OpenExplicitFileRead( pathname );
	} else {
		fp = hfLib::fileSystem->OpenFileRead( pathname );
	}
	if ( !fp ) {
		return false;
	}
	length = fp->Length();
	buf = (char *) Mem_Alloc( length + 1, TAG_IDLIB_LEXER );
	buf[length] = '\0';
	fp->Read( buf, length );
	hfLexer::fileTime = fp->Timestamp();
	hfLexer::filename = fp->GetFullPath();
	hfLib::fileSystem->CloseFile( fp );

	hfLexer::buffer = buf;
	hfLexer::length = length;
	// pointer in script buffer
	hfLexer::script_p = hfLexer::buffer;
	// pointer in script buffer before reading token
	hfLexer::lastScript_p = hfLexer::buffer;
	// pointer to end of script buffer
	hfLexer::end_p = &(hfLexer::buffer[length]);

	hfLexer::tokenavailable = 0;
	hfLexer::line = 1;
	hfLexer::lastline = 1;
	hfLexer::allocated = true;
	hfLexer::loaded = true;

	return true;
}

/*
================
hfLexer::LoadMemory
================
*/
int hfLexer::LoadMemory( const char *ptr, int length, const char *name, int startLine ) {
	if ( hfLexer::loaded ) {
		hfLib::common->Error("hfLexer::LoadMemory: another script already loaded");
		return false;
	}
	hfLexer::filename = name;
	hfLexer::buffer = ptr;
	hfLexer::fileTime = 0;
	hfLexer::length = length;
	// pointer in script buffer
	hfLexer::script_p = hfLexer::buffer;
	// pointer in script buffer before reading token
	hfLexer::lastScript_p = hfLexer::buffer;
	// pointer to end of script buffer
	hfLexer::end_p = &(hfLexer::buffer[length]);

	hfLexer::tokenavailable = 0;
	hfLexer::line = startLine;
	hfLexer::lastline = startLine;
	hfLexer::allocated = false;
	hfLexer::loaded = true;

	return true;
}

/*
================
hfLexer::FreeSource
================
*/
void hfLexer::FreeSource() {
#ifdef PUNCTABLE
	if ( hfLexer::punctuationtable && hfLexer::punctuationtable != default_punctuationtable ) {
		Mem_Free( (void *) hfLexer::punctuationtable );
		hfLexer::punctuationtable = NULL;
	}
	if ( hfLexer::nextpunctuation && hfLexer::nextpunctuation != default_nextpunctuation ) {
		Mem_Free( (void *) hfLexer::nextpunctuation );
		hfLexer::nextpunctuation = NULL;
	}
#endif //PUNCTABLE
	if ( hfLexer::allocated ) {
		Mem_Free( (void *) hfLexer::buffer );
		hfLexer::buffer = NULL;
		hfLexer::allocated = false;
	}
	hfLexer::tokenavailable = 0;
	hfLexer::token = "";
	hfLexer::loaded = false;
}

/*
================
hfLexer::hfLexer
================
*/
hfLexer::hfLexer() {
	hfLexer::loaded = false;
	hfLexer::filename = "";
	hfLexer::flags = 0;
	hfLexer::SetPunctuations( NULL );
	hfLexer::allocated = false;
	hfLexer::fileTime = 0;
	hfLexer::length = 0;
	hfLexer::line = 0;
	hfLexer::lastline = 0;
	hfLexer::tokenavailable = 0;
	hfLexer::token = "";
	hfLexer::next = NULL;
	hfLexer::hadError = false;
}

/*
================
hfLexer::hfLexer
================
*/
hfLexer::hfLexer( int flags ) {
	hfLexer::loaded = false;
	hfLexer::filename = "";
	hfLexer::flags = flags;
	hfLexer::SetPunctuations( NULL );
	hfLexer::allocated = false;
	hfLexer::fileTime = 0;
	hfLexer::length = 0;
	hfLexer::line = 0;
	hfLexer::lastline = 0;
	hfLexer::tokenavailable = 0;
	hfLexer::token = "";
	hfLexer::next = NULL;
	hfLexer::hadError = false;
}

/*
================
hfLexer::hfLexer
================
*/
hfLexer::hfLexer( const char *filename, int flags, bool OSPath ) {
	hfLexer::loaded = false;
	hfLexer::flags = flags;
	hfLexer::SetPunctuations( NULL );
	hfLexer::allocated = false;
	hfLexer::token = "";
	hfLexer::next = NULL;
	hfLexer::hadError = false;
	hfLexer::LoadFile( filename, OSPath );
}

/*
================
hfLexer::hfLexer
================
*/
hfLexer::hfLexer( const char *ptr, int length, const char *name, int flags ) {
	hfLexer::loaded = false;
	hfLexer::flags = flags;
	hfLexer::SetPunctuations( NULL );
	hfLexer::allocated = false;
	hfLexer::token = "";
	hfLexer::next = NULL;
	hfLexer::hadError = false;
	hfLexer::LoadMemory( ptr, length, name );
}

/*
================
hfLexer::~hfLexer
================
*/
hfLexer::~hfLexer() {
	hfLexer::FreeSource();
}

/*
================
hfLexer::SetBaseFolder
================
*/
void hfLexer::SetBaseFolder( const char *path ) {
	hfStr::Copynz( baseFolder, path, sizeof( baseFolder ) );
}

/*
================
hfLexer::HadError
================
*/
bool hfLexer::HadError() const {
	return hadError;
}


