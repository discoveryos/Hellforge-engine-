#ifndef __SWF_SCRIPTFUNCTION_H__
#define __SWF_SCRIPTFUNCTION_H__

/*
========================
Interface for calling functions from script
========================
*/
class hfSWFScriptFunction {
public:
	virtual ~hfSWFScriptFunction() {};

	virtual hfSWFScriptVar	Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ){ return hfSWFScriptVar(); }; // this should never be hit
	virtual void			AddRef(){};
	virtual void			Release(){};
	virtual hfSWFScriptObject *GetPrototype() { return NULL; }
	virtual void			SetPrototype( hfSWFScriptObject * _object ) { }
};

/*
========================
Interface for calling functions from script, implemented statically
========================
*/
class hfSWFScriptFunction_Static : public hfSWFScriptFunction {
public:
							hfSWFScriptFunction_Static() { }
	virtual void			AddRef() { }
	virtual void			Release() { }
};

/*
========================
Interface for calling functions from script, implemented natively on a nested class object
========================
*/
template< typename T >
class hfSWFScriptFunction_Nested : public hfSWFScriptFunction {
protected:
	T * pThis;
public:
							hfSWFScriptFunction_Nested() : pThis( NULL ) { }

	hfSWFScriptFunction *	Bind( T * _pThis ) { pThis = _pThis; return this; }
	virtual void			AddRef() { }
	virtual void			Release() { }
};

/*
========================
Interface for calling functions from script, with reference counting
NOTE: The ref count starts at 0 here because it assumes you do an AddRef on the allocated
object.  The proper way is to start it at 1 and force the caller to Release, but that's
really kind of a pain in the ass.  It was made to be used like this:
object->Set( "myFunction", new hfSWFScriptFunction_MyFunction() );
========================
*/
class hfSWFScriptFunction_RefCounted : public hfSWFScriptFunction {
public:
	hfSWFScriptFunction_RefCounted() : refCount( 0 ) { }
	void AddRef() { refCount++; }
	void Release() { if ( --refCount <= 0 ) { delete this; } }
private:
	int refCount;
};

/*
========================
Action Scripts can define a pool of constants then push values from that pool
The documentation isn't very clear on the scope of these things, but from what
I've gathered by testing, pool is per-function and copied into the function
whenever that function is declared.
========================
*/
class hfSWFConstantPool {
public:
						hfSWFConstantPool();
						~hfSWFConstantPool() { Clear(); }

	void				Clear();
	void				Copy( const hfSWFConstantPool & other );
	hfSWFScriptString * Get( int n ) { return pool[n]; }
	void				Append( hfSWFScriptString * s ) { pool.Append( s ); }

private:
	hfList< hfSWFScriptString *, TAG_SWF > pool;
};

/*
========================
The hfSWFStack class is just a helper routine for treating an hfList like a stack
========================
*/
class hfSWFStack : public hfList< hfSWFScriptVar > {
public:
	hfSWFScriptVar & A() { return operator[]( Num() - 1 ); }
	hfSWFScriptVar & B() { return operator[]( Num() - 2 ); }
	hfSWFScriptVar & C() { return operator[]( Num() - 3 ); }
	hfSWFScriptVar & D() { return operator[]( Num() - 4 ); }
	void Pop( int n )	{ SetNum( Num() - n ); }
};

/*
========================
hfSWFScriptFunction_Script is a script function that's implemented in action script
========================
*/
class hfSWFScriptFunction_Script : public hfSWFScriptFunction {
public:
				hfSWFScriptFunction_Script() : refCount( 1 ), flags( 0 ), prototype( NULL ), data( NULL ), length( 0 ), defaultSprite( NULL ) { registers.SetNum( 4 ); }
	virtual		~hfSWFScriptFunction_Script();

	static hfSWFScriptFunction_Script *	Alloc() { return new (TAG_SWF) hfSWFScriptFunction_Script; }
	void	AddRef() { refCount++; }
	void	Release() { if ( --refCount == 0 ) { delete this; } }

	// This could all be passed to Alloc (and was at one time) but in some places it's far more convenient to specify each separately
	void	SetFlags( uint16 _flags )								{ flags = _flags; }
	void	SetData( const byte * _data, uint32 _length )			{ data = _data; length = _length; }
	void	SetScope( hfList<hfSWFScriptObject *> & scope );
	void	SetConstants( const hfSWFConstantPool & _constants )	{ constants.Copy( _constants ); }
	void	SetDefaultSprite( hfSWFSpriteInstance * _sprite )		{ defaultSprite = _sprite; }
	void	AllocRegisters( int numRegs	)							{ registers.SetNum( numRegs ); }
	void	AllocParameters( int numParms )							{ parameters.SetNum( numParms ); }
	void	SetParameter( uint8 n, uint8 r, const char * name )		{ parameters[n].reg = r; parameters[n].name = name; }

	hfSWFScriptObject * GetPrototype()	    						{ return prototype; }
	void	SetPrototype( hfSWFScriptObject * _prototype )			{ _prototype->AddRef(); assert( prototype == NULL ); prototype = _prototype; }

	virtual hfSWFScriptVar	Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms );

private:
	hfSWFScriptVar Run( hfSWFScriptObject * thisObject, hfSWFStack & stack, hfSWFBitStream & bitstream );

private:
	int					refCount;

	uint16				flags;
	const  byte *		data;
	uint32				length;
	hfSWFScriptObject * prototype;

	hfSWFSpriteInstance * defaultSprite;		// some actions have an implicit sprite they work off of (e.g. Action_GotoFrame outside of object scope)

	hfList< hfSWFScriptObject *, TAG_SWF > scope;

	hfSWFConstantPool	constants;
	hfList< hfSWFScriptVar, TAG_SWF > registers;

	struct parmInfo_t {
		const char * name;
		uint8 reg;
	};
	hfList< parmInfo_t, TAG_SWF > parameters;
};

#endif // !__SWF_SCRIPTFUNCTION_H__

