#pragma hdrstop
#include "../hflib/hfprecompiled.h"

extern hfCVar swf_debugShowAddress;

/*
========================
hfSWFScriptVar::hfSWFScriptVar
========================
*/
hfSWFScriptVar::hfSWFScriptVar( const hfSWFScriptVar & other ) {
	type = other.type;
	value = other.value;
	if ( other.type == SWF_VAR_STRING ) {
		other.value.string->AddRef();
	} else if ( other.type == SWF_VAR_OBJECT ) {
		other.value.object->AddRef();
	} else if ( other.type == SWF_VAR_FUNCTION ) {
		other.value.function->AddRef();
	}
}

/*
========================
hfSWFScriptVar::operator=
========================
*/
hfSWFScriptVar & hfSWFScriptVar::operator=( const hfSWFScriptVar & other ) {
	if ( this != &other ) {
		Free();
		type = other.type;
		value = other.value;
		if ( other.type == SWF_VAR_STRING ) {
			other.value.string->AddRef();
		} else if ( other.type == SWF_VAR_OBJECT ) {
			other.value.object->AddRef();
		} else if ( other.type == SWF_VAR_FUNCTION ) {
			other.value.function->AddRef();
		}
	}
	return *this;
}

/*
========================
hfSWFScriptVar::~hfSWFScriptVar
========================
*/
hfSWFScriptVar::~hfSWFScriptVar() {
	Free();
}

/*
========================
hfSWFScriptVar::Free
========================
*/
void hfSWFScriptVar::Free() {
	if ( type == SWF_VAR_STRING ) {
		value.string->Release();
	} else if ( type == SWF_VAR_OBJECT ) {
		value.object->Release();
	} else if ( type == SWF_VAR_FUNCTION ) {
		value.function->Release();
	}
	value.string = NULL;
	value.function = NULL;
	value.object = NULL;
	type = SWF_VAR_UNDEF;
}

/*
========================
hfSWFScriptVar::SetObject
========================
*/
void hfSWFScriptVar::SetObject( hfSWFScriptObject * o ) {
	Free();
	if ( o == NULL ) {
		type = SWF_VAR_NULL;
	} else {
		type = SWF_VAR_OBJECT;
		value.object = o;
		o->AddRef();
	}
}

/*
========================
hfSWFScriptVar::SetFunction
========================
*/
void hfSWFScriptVar::SetFunction( hfSWFScriptFunction * f ) {
	Free();
	if ( f == NULL ) {
		type = SWF_VAR_NULL;
	} else {
		type = SWF_VAR_FUNCTION;
		value.function = f;
		f->AddRef();
	}
}

/*
========================
hfSWFScriptVar::StrictEquals
========================
*/
bool hfSWFScriptVar::StrictEquals( const hfSWFScriptVar & other ) {
	if ( type != other.type ) {
		return false;
	}
	switch ( type ) {
	case SWF_VAR_STRINGID:	return ( value.i == other.value.i );
	case SWF_VAR_STRING:	return ( *value.string == *other.value.string );
	case SWF_VAR_FLOAT:		return ( value.f == other.value.f );
	case SWF_VAR_BOOL:		return ( value.b == other.value.b );
	case SWF_VAR_INTEGER:	return ( value.i == other.value.i );
	case SWF_VAR_NULL:		return true;
	case SWF_VAR_UNDEF:		return true;
	case SWF_VAR_OBJECT:	return ( value.object == other.value.object );
	case SWF_VAR_FUNCTION:	return ( value.function == other.value.function );
	default:				assert( false ); return false;
	}
}

/*
========================
hfSWFScriptVar::AbstractEquals
========================
*/
bool hfSWFScriptVar::AbstractEquals( const hfSWFScriptVar & other ) {
	if ( type == other.type ) {
		switch ( type ) {
		case SWF_VAR_STRINGID:	return ( value.i == other.value.i );
		case SWF_VAR_STRING:	return ( *value.string == *other.value.string );
		case SWF_VAR_FLOAT:		return ( value.f == other.value.f );
		case SWF_VAR_BOOL:		return ( value.b == other.value.b );
		case SWF_VAR_INTEGER:	return ( value.i == other.value.i );
		case SWF_VAR_NULL:		return true;
		case SWF_VAR_UNDEF:		return true;
		case SWF_VAR_OBJECT:	return ( value.object == other.value.object );
		case SWF_VAR_FUNCTION:	return ( value.function == other.value.function );
		default:				assert( false ); return false;
		}
	}
	switch ( type ) {
		case SWF_VAR_STRINGID: 		return ToString() == other.ToString();
		case SWF_VAR_STRING:
			switch ( other.type ) {
			case SWF_VAR_STRINGID:	return *value.string == other.ToString();
			case SWF_VAR_FLOAT:		return ToFloat() == other.value.f;
			case SWF_VAR_BOOL:		return ToBool() == other.value.b;
			case SWF_VAR_INTEGER:	return ToInteger() == other.value.i;
			case SWF_VAR_OBJECT:	return *value.string == other.ToString();
			default:				return false;
			}
		case SWF_VAR_FLOAT:		return ( other.ToFloat() == value.f );
		case SWF_VAR_BOOL:		return ( other.ToBool() == value.b );
		case SWF_VAR_INTEGER:	return ( other.ToInteger() == value.i );
		case SWF_VAR_NULL:		return ( other.type == SWF_VAR_UNDEF );
		case SWF_VAR_UNDEF:		return ( other.type == SWF_VAR_NULL );
		case SWF_VAR_OBJECT:
			switch ( other.type ) {
			case SWF_VAR_STRING:	return ToString() == *other.value.string;
			case SWF_VAR_FLOAT:		return ToFloat() == other.value.f;
			case SWF_VAR_BOOL:		return ToBool() == other.value.b;
			case SWF_VAR_INTEGER:	return ToInteger() == other.value.i;
			default:				return false;
			}
		case SWF_VAR_FUNCTION:		return false;
		default:				assert( false ); return false;
	}
}

/*
========================
hfSWFScriptVar::ToString
========================
*/
hfStr hfSWFScriptVar::ToString() const {
	switch ( type ) {
		case SWF_VAR_STRINGID:	return hfStrId( value.i ).GetLocalizedString();
		case SWF_VAR_STRING:	return *value.string;

		case SWF_VAR_FLOAT:		return va( "%g", value.f );
		case SWF_VAR_BOOL:		return value.b ? "true" : "false";
		case SWF_VAR_INTEGER:	return va( "%i", value.i );

		case SWF_VAR_NULL:		return "[null]";
		case SWF_VAR_UNDEF:		return "[undefined]";
		case SWF_VAR_OBJECT:	return value.object->DefaultValue( true ).ToString();
		case SWF_VAR_FUNCTION:	
			if ( swf_debugShowAddress.GetBool() ) {
				return va( "[function:%p]", value.function );
			} else {
				return "[function]";
			}
		default:				assert( false ); return "";
	}
}

/*
========================
hfSWFScriptVar::ToFloat
========================
*/
float hfSWFScriptVar::ToFloat() const {
	switch ( type ) {
		case SWF_VAR_STRING:	return atof( *value.string );

		case SWF_VAR_FLOAT:		return value.f;
		case SWF_VAR_BOOL:		return (float)value.b;
		case SWF_VAR_INTEGER:	return (float)value.i;

		case SWF_VAR_OBJECT:	return value.object->DefaultValue( false ).ToFloat();

		case SWF_VAR_FUNCTION:
		case SWF_VAR_NULL:
		case SWF_VAR_UNDEF:		return 0.0f;
		default:				assert( false ); return 0.0f;
	}
}

/*
========================
hfSWFScriptVar::ToBool
========================
*/
bool hfSWFScriptVar::ToBool() const {
	switch ( type ) {
		case SWF_VAR_STRING:	return ( value.string->Icmp( "true" ) == 0 || value.string->Icmp( "1" ) == 0 );

		case SWF_VAR_FLOAT:		return ( value.f != 0.0f );
		case SWF_VAR_BOOL:		return value.b;
		case SWF_VAR_INTEGER:	return value.i != 0;

		case SWF_VAR_OBJECT:	return value.object->DefaultValue( false ).ToBool();

		case SWF_VAR_FUNCTION:
		case SWF_VAR_NULL:
		case SWF_VAR_UNDEF:		return false;
		default:				assert( false ); return false;
	}
}

/*
========================
hfSWFScriptVar::ToInteger
========================
*/
int32 hfSWFScriptVar::ToInteger() const {
	switch ( type ) {
		case SWF_VAR_STRING:	return atoi( *value.string );

		case SWF_VAR_FLOAT:		return hfMath::Ftoi( value.f );

		case SWF_VAR_BOOL:		return value.b ? 1 : 0;
		case SWF_VAR_INTEGER:	return value.i;

		case SWF_VAR_OBJECT:	return value.object->DefaultValue( false ).ToInteger();

		case SWF_VAR_FUNCTION:
		case SWF_VAR_NULL:
		case SWF_VAR_UNDEF:		return 0;
		default:				assert( false ); return 0;
	}
}

/*
========================
hfSWFScriptVar::ToSprite
========================
*/
hfSWFSpriteInstance * hfSWFScriptVar::ToSprite() {
	if ( IsObject() && value.object != NULL ) {
		return value.object->GetSprite();
	}

	return NULL;
}

/*
========================
hfSWFScriptVar::ToText
========================
*/
hfSWFTextInstance * hfSWFScriptVar::ToText() {
	if ( IsObject() && value.object != NULL ) {
		return value.object->GetText();
	}

	return NULL;
}

/*
========================
hfSWFScriptVar::GetNestedVar
========================
*/
hfSWFScriptVar hfSWFScriptVar::GetNestedVar( const char * arg1, const char * arg2, const char * arg3, const char * arg4, const char * arg5, const char * arg6 ) {
	if ( !IsObject() ) {
		return hfSWFScriptVar();
	}

	return GetObject()->GetNestedVar( arg1, arg2, arg3, arg4, arg5, arg6 );
}

/*
========================
hfSWFScriptVar::GetNestedObj
========================
*/
hfSWFScriptObject * hfSWFScriptVar::GetNestedObj( const char * arg1, const char * arg2, const char * arg3, const char * arg4, const char * arg5, const char * arg6 ) {
	if ( !IsObject() ) {
		return NULL;
	}

	return GetObject()->GetNestedObj( arg1, arg2, arg3, arg4, arg5, arg6 );
}

/*
========================
hfSWFScriptVar::GetNestedSprite
========================
*/
hfSWFSpriteInstance * hfSWFScriptVar::GetNestedSprite( const char * arg1, const char * arg2, const char * arg3, const char * arg4, const char * arg5, const char * arg6 ) {
	if ( !IsObject() ) {
		return NULL;
	}

	return GetObject()->GetNestedSprite( arg1, arg2, arg3, arg4, arg5, arg6 );
}

/*
========================
hfSWFScriptVar::GetNestedSprite
========================
*/
hfSWFTextInstance * hfSWFScriptVar::GetNestedText( const char * arg1, const char * arg2, const char * arg3, const char * arg4, const char * arg5, const char * arg6 ) {
	if ( !IsObject() ) {
		return NULL;
	}

	return GetObject()->GetNestedText( arg1, arg2, arg3, arg4, arg5, arg6 );
}

/*
========================
hfSWFScriptVar::TypeOf
========================
*/
const char * hfSWFScriptVar::TypeOf() const {
	switch ( type ) {
	case SWF_VAR_STRINGID:	return "stringid";
	case SWF_VAR_STRING:	return "string";

	case SWF_VAR_FLOAT:		return "number";
	case SWF_VAR_BOOL:		return "boolean";
	case SWF_VAR_INTEGER:	return "number";

	case SWF_VAR_OBJECT:	
		if ( value.object->GetSprite() != NULL ) {
			return "movieclip";
		} else if ( value.object->GetText() != NULL ) {
			return "text";
		} else {
			return "object";
		}

	case SWF_VAR_FUNCTION:	return "function";
	case SWF_VAR_NULL:		return "null";
	case SWF_VAR_UNDEF:		return "undefined";
	default:				assert( false ); return "";
	}
}

/*
========================
hfSWFScriptVar::PrintToConsole
========================
*/
void hfSWFScriptVar::PrintToConsole() const {
	hfLib::Printf( "Object type: %s\n", TypeOf() );

	if ( IsObject() ) {
		GetObject()->PrintToConsole();
	} else if ( IsNumeric() ) {
		hfLib::Printf( "%d\n", ToInteger() );
	} else if ( IsString() ) {
		hfLib::Printf( "%s\n", ToString().c_str() );
	} else {
		hfLib::Printf( "unknown\n" );
	}
}

