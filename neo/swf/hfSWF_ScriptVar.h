#ifndef __SWF_SCRIPTVAR_H__
#define __SWF_SCRIPTVAR_H__

class hfSWFScriptObject;
class hfSWFScriptFunction;

/*
========================
A reference counted string
========================
*/
class hfSWFScriptString : public hfStr {
public:
	hfSWFScriptString( const hfStr & s ) : hfStr( s ), refCount( 1 ) { }

	static hfSWFScriptString * Alloc( const hfStr & s ) { return new (TAG_SWF) hfSWFScriptString( s ); }
	HF_INLINE void	AddRef() { refCount++; }
	HF_INLINE void	Release() { if ( --refCount == 0 ) { delete this; } }

private:
	int refCount;
};

/*
========================
A variable in an action script
these can be on the stack, in a script object, passed around as parameters, etc
they can contain raw data (int, float), strings, functions, or objects
========================
*/
class hfSWFScriptVar {
public:
	hfSWFScriptVar() : type( SWF_VAR_UNDEF ) { }
	hfSWFScriptVar( const hfSWFScriptVar & other );
	hfSWFScriptVar( hfSWFScriptObject * o ) : type( SWF_VAR_UNDEF ) { SetObject( o ); }
	hfSWFScriptVar( hfStrId s ) : type( SWF_VAR_UNDEF ) { SetString( s ); }
	hfSWFScriptVar( const hfStr & s ) : type( SWF_VAR_UNDEF ) { SetString( s ); }
	hfSWFScriptVar( const char * s ) : type( SWF_VAR_UNDEF ) { SetString( hfStr( s ) ); }
	hfSWFScriptVar( float f ) : type( SWF_VAR_UNDEF ) { SetFloat( f ); }
	hfSWFScriptVar( bool b ) : type( SWF_VAR_UNDEF ) { SetBool( b ); }
	hfSWFScriptVar( int32 i ) : type( SWF_VAR_UNDEF ) { SetInteger( i ); }
	hfSWFScriptVar( hfSWFScriptFunction * nf ) : type( SWF_VAR_UNDEF ) { SetFunction( nf ); }
	~hfSWFScriptVar();

	hfSWFScriptVar & operator=( const hfSWFScriptVar & other );

	// implements ECMA 262 11.9.3
	bool AbstractEquals( const hfSWFScriptVar & other );
	bool StrictEquals( const hfSWFScriptVar & other );

	void SetString( hfStrId s )	{ Free(); type = SWF_VAR_STRINGID; value.i = s.GetIndex(); }
	void SetString( const hfStr & s )	{ Free(); type = SWF_VAR_STRING; value.string = hfSWFScriptString::Alloc( s ); }
	void SetString( const char * s )	{ Free(); type = SWF_VAR_STRING; value.string = hfSWFScriptString::Alloc( s ); }
	void SetString( hfSWFScriptString * s )	{ Free(); type = SWF_VAR_STRING; value.string = s; s->AddRef(); }
	void SetFloat( float f )			{ Free(); type = SWF_VAR_FLOAT; value.f = f; }
	void SetNULL()						{ Free(); type = SWF_VAR_NULL; }
	void SetUndefined()					{ Free(); type = SWF_VAR_UNDEF; }
	void SetBool( bool b )				{ Free(); type = SWF_VAR_BOOL; value.b = b; }
	void SetInteger( int32 i )			{ Free(); type = SWF_VAR_INTEGER; value.i = i; }

	void SetObject( hfSWFScriptObject * o );
	void SetFunction( hfSWFScriptFunction * f );

	hfStr	ToString() const;
	float	ToFloat() const;
	bool	ToBool() const;
	int32	ToInteger() const;

	hfSWFScriptObject *		GetObject() { assert( type == SWF_VAR_OBJECT ); return value.object; }
	hfSWFScriptObject *		GetObject() const { assert( type == SWF_VAR_OBJECT ); return value.object; }
	hfSWFScriptFunction *	GetFunction() { assert( type == SWF_VAR_FUNCTION ); return value.function; }
	hfSWFSpriteInstance *	ToSprite();
	hfSWFTextInstance *		ToText();

	hfSWFScriptVar			GetNestedVar( const char * arg1, const char * arg2 = NULL, const char * arg3 = NULL, const char * arg4 = NULL, const char * arg5 = NULL, const char * arg6 = NULL );
	hfSWFScriptObject *		GetNestedObj( const char * arg1, const char * arg2 = NULL, const char * arg3 = NULL, const char * arg4 = NULL, const char * arg5 = NULL, const char * arg6 = NULL );
	hfSWFSpriteInstance *	GetNestedSprite( const char * arg1, const char * arg2 = NULL, const char * arg3 = NULL, const char * arg4 = NULL, const char * arg5 = NULL, const char * arg6 = NULL );
	hfSWFTextInstance *		GetNestedText( const char * arg1, const char * arg2 = NULL, const char * arg3 = NULL, const char * arg4 = NULL, const char * arg5 = NULL, const char * arg6 = NULL );

	const char *			TypeOf() const;

	// debug print of this variable to the console
	void					PrintToConsole() const;

	bool IsString()		const { return ( type == SWF_VAR_STRING ) || ( type == SWF_VAR_STRINGID ); }
	bool IsNULL()		const { return ( type == SWF_VAR_NULL ); }
	bool IsUndefined()	const { return ( type == SWF_VAR_UNDEF ); }
	bool IsValid()		const { return ( type != SWF_VAR_UNDEF ) && ( type != SWF_VAR_NULL ); }
	bool IsFunction()	const { return ( type == SWF_VAR_FUNCTION ); }
	bool IsObject()		const { return ( type == SWF_VAR_OBJECT ); }
	bool IsNumeric()	const { return ( type == SWF_VAR_FLOAT ) || ( type == SWF_VAR_INTEGER ) || ( type == SWF_VAR_BOOL ); }

	enum swfScriptVarType {
		SWF_VAR_STRINGID,
		SWF_VAR_STRING,
		SWF_VAR_FLOAT,
		SWF_VAR_NULL,
		SWF_VAR_UNDEF,
		SWF_VAR_BOOL,
		SWF_VAR_INTEGER,
		SWF_VAR_FUNCTION,
		SWF_VAR_OBJECT
	};

	swfScriptVarType	GetType() const { return type; }

private:
	void Free();
	swfScriptVarType type;

	union swfScriptVarValue_t {
		float	f;
		int32	i;
		bool	b;
		hfSWFScriptObject * object;
		hfSWFScriptString * string;
		hfSWFScriptFunction * function;
	} value;
};

#endif // !__SWF_SCRIPTVAR_H__

