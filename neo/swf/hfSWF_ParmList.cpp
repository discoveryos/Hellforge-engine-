#pragma hdrstop
#include "../hflib/hfprecompiled.h"

void hfSWFParmList::Append( const hfSWFScriptVar & other ) {
	hfSWFScriptVar * var = Alloc();
	if ( var != NULL ) {
		*var = other;
	}
}
void hfSWFParmList::Append( hfSWFScriptObject * o ) {
	hfSWFScriptVar * var = Alloc();
	if ( var != NULL ) {
		var->SetObject( o );
	}
}
void hfSWFParmList::Append( hfSWFScriptFunction * f ) {
	hfSWFScriptVar * var = Alloc();
	if ( var != NULL ) {
		var->SetFunction( f );
	}
}
void hfSWFParmList::Append( const char * s ) {
	hfSWFScriptVar * var = Alloc();
	if ( var != NULL ) {
		var->SetString( s );
	}
}
void hfSWFParmList::Append( const hfStr & s ) {
	hfSWFScriptVar * var = Alloc();
	if ( var != NULL ) {
		var->SetString( s );
	}
}
void hfSWFParmList::Append( hfSWFScriptString * s ) {
	hfSWFScriptVar * var = Alloc();
	if ( var != NULL ) {
		var->SetString( s );
	}
}
void hfSWFParmList::Append( const float f ) {
	hfSWFScriptVar * var = Alloc();
	if ( var != NULL ) {
		var->SetFloat( f );
	}
}
void hfSWFParmList::Append( const int32 i ) {
	hfSWFScriptVar * var = Alloc();
	if ( var != NULL ) {
		var->SetInteger( i );
	}
}
void hfSWFParmList::Append( const bool b ) {
	hfSWFScriptVar * var = Alloc();
	if ( var != NULL ) {
		var->SetBool( b );
	}
}

