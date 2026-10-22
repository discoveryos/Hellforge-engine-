#ifndef __SWF_SCRIPTOBJECT_H__
#define __SWF_SCRIPTOBJECT_H__

class hfSWFSpriteInstance;

/*
========================
This is the base class for script variables which are implemented in code
========================
*/
class hfSWFScriptNativeVariable {
public:
	virtual bool IsReadOnly() { return false; }
	virtual void Set( class hfSWFScriptObject * object, const hfSWFScriptVar & value ) = 0;
	virtual hfSWFScriptVar Get( class hfSWFScriptObject * object ) = 0;
};

#define SWF_NATIVE_VAR_DECLARE( x ) \
	class hfSWFScriptNativeVar_##x : public hfSWFScriptNativeVariable {			\
	public:																		\
		void Set( class hfSWFScriptObject * object, const hfSWFScriptVar & value );	\
		hfSWFScriptVar Get( class hfSWFScriptObject * object );					\
	} swfScriptVar_##x;

#define SWF_NATIVE_VAR_DECLARE_READONLY( x )									\
	class hfSWFScriptNativeVar_##x : public hfSWFScriptNativeVariable {			\
	public:																		\
		bool IsReadOnly() { return true; }										\
		void Set( class hfSWFScriptObject * object, const hfSWFScriptVar & value ) { assert( false ); } \
		hfSWFScriptVar Get( class hfSWFScriptObject * object );					\
	} swfScriptVar_##x;

/*
========================
This is a helper class for quickly setting up native variables which need access to a parent class
========================
*/
template< typename T >
class hfSWFScriptNativeVariable_Nested : public hfSWFScriptNativeVariable {
public:
	hfSWFScriptNativeVariable_Nested() : pThis( NULL ) { }
	hfSWFScriptNativeVariable_Nested * Bind( T * p ) { pThis = p; return this; }
	virtual void Set( class hfSWFScriptObject * object, const hfSWFScriptVar & value ) = 0;
	virtual hfSWFScriptVar Get( class hfSWFScriptObject * object ) = 0;
protected:
	T * pThis;
};

#define SWF_NATIVE_VAR_DECLARE_NESTED( x, y ) \
	class hfSWFScriptNativeVar_##x : public hfSWFScriptNativeVariable_Nested<y> {	\
	public:																			\
		void Set( class hfSWFScriptObject * object, const hfSWFScriptVar & value );	\
		hfSWFScriptVar Get( class hfSWFScriptObject * object );						\
	} swfScriptVar_##x;

#define SWF_NATIVE_VAR_DECLARE_NESTED_READONLY( x, y, z )							\
	class hfSWFScriptNativeVar_##x : public hfSWFScriptNativeVariable_Nested<y> {	\
	public:																			\
		bool IsReadOnly() { return true; }											\
		void Set( class hfSWFScriptObject * object, const hfSWFScriptVar & value ) { assert( false ); } \
		hfSWFScriptVar Get( class hfSWFScriptObject * object ) { return pThis->z; }	\
	} swfScriptVar_##x;

/*
========================
An object in an action script is a collection of variables. functions are also variables.
========================
*/
class hfSWFScriptObject {
public:
							hfSWFScriptObject();
	virtual					~hfSWFScriptObject();

	static hfSWFScriptObject *	Alloc();
	void					AddRef();
	void					Release();
	void					SetNoAutoDelete( bool b ) { noAutoDelete = b; }

	void					Clear();

	void					MakeArray();

	void					SetSprite( hfSWFSpriteInstance * s ) { objectType = SWF_OBJECT_SPRITE; data.sprite = s; }
	hfSWFSpriteInstance *	GetSprite() { return ( objectType == SWF_OBJECT_SPRITE ) ? data.sprite : NULL; }

	void					SetText( hfSWFTextInstance * t ) { objectType = SWF_OBJECT_TEXT; data.text = t; }
	hfSWFTextInstance *		GetText() { return ( objectType == SWF_OBJECT_TEXT ) ? data.text : NULL; }

	// Also accessible via __proto__ property
	hfSWFScriptObject *		GetPrototype() { return prototype; }
	void					SetPrototype( hfSWFScriptObject *_prototype ) { assert( prototype == NULL ); prototype = _prototype; prototype->AddRef(); }
	hfSWFScriptVar			Get( int index );
	hfSWFScriptVar			Get( const char * name );
	hfSWFSpriteInstance *	GetSprite( int index );
	hfSWFSpriteInstance *	GetSprite( const char * name );
	hfSWFScriptObject *		GetObject( int index );
	hfSWFScriptObject *		GetObject( const char * name );
	hfSWFTextInstance *		GetText( int index );
	hfSWFTextInstance *		GetText( const char * name );
	void					Set( int index, const hfSWFScriptVar & value );
	void					Set( const char * name, const hfSWFScriptVar & value );
	void					SetNative( const char * name, hfSWFScriptNativeVariable * native );
	bool					HasProperty( const char * name );
	bool					HasValidProperty( const char * name );
	hfSWFScriptVar			DefaultValue( bool stringHint );

	// This is to implement for-in (fixme: respect DONTENUM flag)
	int						NumVariables() { return variables.Num(); }
	const char *			EnumVariable( int i ) { return variables[i].name; }
	
	hfSWFScriptVar			GetNestedVar( const char * arg1, const char * arg2 = NULL, const char * arg3 = NULL, const char * arg4 = NULL, const char * arg5 = NULL, const char * arg6 = NULL );
	hfSWFScriptObject *		GetNestedObj( const char * arg1, const char * arg2 = NULL, const char * arg3 = NULL, const char * arg4 = NULL, const char * arg5 = NULL, const char * arg6 = NULL );
	hfSWFSpriteInstance *	GetNestedSprite( const char * arg1, const char * arg2 = NULL, const char * arg3 = NULL, const char * arg4 = NULL, const char * arg5 = NULL, const char * arg6 = NULL );
	hfSWFTextInstance *		GetNestedText( const char * arg1, const char * arg2 = NULL, const char * arg3 = NULL, const char * arg4 = NULL, const char * arg5 = NULL, const char * arg6 = NULL );

	void					PrintToConsole() const;

private:
	int refCount;
	bool noAutoDelete;

	enum swfNamedVarFlags_t {
		SWF_VAR_FLAG_NONE = 0,
		SWF_VAR_FLAG_READONLY = BIT(1),
		SWF_VAR_FLAG_DONTENUM = BIT(2)
	};
	struct swfNamedVar_t {
									swfNamedVar_t() : native( NULL ) { }
									~swfNamedVar_t();
									swfNamedVar_t & operator=( const swfNamedVar_t & other );

		int							index;
		int							hashNext;
		hfStr						name;
		hfSWFScriptVar				value;
		hfSWFScriptNativeVariable *	native;
		int							flags;
	};
	hfList< swfNamedVar_t, TAG_SWF >	variables;

	static const int VARIABLE_HASH_BUCKETS = 16;
	int	variablesHash[VARIABLE_HASH_BUCKETS];

	hfSWFScriptObject *		prototype;

	enum swfObjectType_t {
		SWF_OBJECT_OBJECT,
		SWF_OBJECT_ARRAY,
		SWF_OBJECT_SPRITE,
		SWF_OBJECT_TEXT
	} objectType;

	union swfObjectData_t {
		hfSWFSpriteInstance *	sprite;			// only valid if objectType == SWF_OBJECT_SPRITE
		hfSWFTextInstance *		text;			// only valid if objectType == SWF_OBJECT_TEXT
	} data;

	swfNamedVar_t *	GetVariable( int index, bool create );
	swfNamedVar_t *	GetVariable( const char * name, bool create );
};

#endif // !__SWF_SCRIPTOBJECT_H__

