#ifndef __CALLBACK_H__
#define __CALLBACK_H__

/*
================================================================================================
This file defines a set of template functors for generating callbacks, specifically
the OnChange handlers in the CVar system.
================================================================================================
*/

/*
================================================
hfCallback
================================================
*/
class hfCallback {
public:
	virtual ~hfCallback() {}
	virtual void Call() = 0;
	virtual hfCallback * Clone() const = 0;
};

/*
================================================
hfCallbackStatic

Callback class that forwards the call to a c-style function
================================================
*/
class hfCallbackStatic : public hfCallback {
public:
	hfCallbackStatic( void (*f)() ) {
		this->f = f;
	}
	void Call() {
		f();
	}
	hfCallback * Clone() const {
		//hfScopedGlobalHeap	everythingHereGoesInTheGlobalHeap;
		return new (TAG_FUNC_CALLBACK) hfCallbackStatic( f );
	}
private:
	void (*f)();
};

/*
================================================
hfCallbackBindMem

Callback class that forwards the call to a member function
================================================
*/
template< class T >
class hfCallbackBindMem : public hfCallback {
public:
	hfCallbackBindMem( T * t, void (T::*f)() ) {
		this->t = t;
		this->f = f;
	}
	void Call() {
		(t->*f)();
	}
	hfCallback * Clone() const {
		return new (TAG_FUNC_CALLBACK) hfCallbackBindMem( t, f );
	}
private:
	T * t;
	void (T::*f)();
};

/*
================================================
hfCallbackBindMemArg1

Callback class that forwards the call to a member function with an additional constant parameter
================================================
*/
template< class T, typename A1 >
class hfCallbackBindMemArg1 : public hfCallback {
public:
	hfCallbackBindMemArg1( T * t_, void (T::*f_)( A1 ), A1 a1_ ) :
		t( t_ ),
		f( f_ ),
		a1( a1_ ) {
	}
	void Call() {
		(t->*f)( a1 );
	}
	hfCallback * Clone() const {
		return new (TAG_FUNC_CALLBACK) hfCallbackBindMemArg1( t, f, a1 );
	}
private:
	T * t;
	void (T::*f)( A1 );
	A1 a1;

	// hack to get to compile on the 360 with reference arguments
	// with this on the PC, the MakeCallback function fails compilation because it's returning a copy
	// therefore, the Arg1 callbacks can't have arguments that are references
	//DISALLOW_COPY_AND_ASSIGN( hfCallbackBindMemArg1 );
};

/*
================================================================================================

	These are needed because we can't derive the type of an object from the type passed to the 
	constructor. If it weren't for these, we'd have to manually specify the type:

		hfCallbackBindMem<hfFoo>( this, &hfFoo::MyFunction );
	becomes: 
		MakeCallback( this, &hfFoo::MyFunction );

================================================================================================
*/

/*
========================
MakeCallback
========================
*/
HF_INLINE_EXTERN hfCallbackStatic MakeCallback( void (*f)(void) ) { 
	return hfCallbackStatic( f ); 
}

/*
========================
MakeCallback
========================
*/
template < class T >
HF_INLINE_EXTERN hfCallbackBindMem<T> MakeCallback( T * t, void (T::*f)(void) ) { 
	return hfCallbackBindMem<T>( t, f ); 
}

/*
========================
MakeCallback
========================
*/
template < class T, typename A1 >
HF_INLINE_EXTERN hfCallbackBindMemArg1<T, A1> MakeCallback( T * t, void (T::*f)( A1 ), A1 a1 ) { 
	return hfCallbackBindMemArg1<T, A1>( t, f, a1 );
}

#endif // __CALLBACK_H__


