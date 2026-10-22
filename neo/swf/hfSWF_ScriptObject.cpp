#pragma hdrstop
#include "../hflib/hfprecompiled.h"

hfCVar swf_debugShowAddress( "swf_debugShowAddress", "0", CVAR_BOOL, "shows addresses along with object types when they are serialized" );


/*
========================
hfSWFScriptObject::swfNamedVar_t::~swfNamedVar_t
========================
*/
hfSWFScriptObject::swfNamedVar_t::~swfNamedVar_t() {
}

/*
========================
hfSWFScriptObject::swfNamedVar_t::operator=
========================
*/
hfSWFScriptObject::swfNamedVar_t & hfSWFScriptObject::swfNamedVar_t::operator=( const swfNamedVar_t & other ) {
	if ( &other != this ) {
		index = other.index;
		name = other.name;
		hashNext = other.hashNext;
		value = other.value;
		native = other.native;
		flags = other.flags;
	}
	return *this;
}

/*
========================
hfSWFScriptObject::hfSWFScriptObject
========================
*/
hfSWFScriptObject::hfSWFScriptObject() : prototype( NULL ), refCount( 1 ), noAutoDelete( false ), objectType( SWF_OBJECT_OBJECT ) {
	data.sprite = NULL;
	data.text = NULL;
	Clear();
	refCount = 1;
}

/*
========================
hfSWFScriptObject::~hfSWFScriptObject
========================
*/
hfSWFScriptObject::~hfSWFScriptObject() {
	if ( prototype != NULL ) {
		prototype->Release();
	}
}

/*
========================
hfSWFScriptObject::Alloc
========================
*/
hfSWFScriptObject *	hfSWFScriptObject::Alloc() {
	return new (TAG_SWF) hfSWFScriptObject;
}

/*
========================
hfSWFScriptObject::AddRef
========================
*/
void hfSWFScriptObject::AddRef() {
	refCount++;
}

/*
========================
hfSWFScriptObject::Release
========================
*/
void hfSWFScriptObject::Release() {
	if ( --refCount == 0 && !noAutoDelete ) {
		delete this;
	}
}

/*
========================
hfSWFScriptObject::Clear
========================
*/
void hfSWFScriptObject::Clear() {
	variables.Clear();
	for ( int i = 0; i < VARIABLE_HASH_BUCKETS; i++ ) {
		variablesHash[i] = -1;
	}
}

/*
========================
hfSWFScriptObject::HasProperty
========================
*/
bool hfSWFScriptObject::HasProperty( const char * name ) {
	return ( GetVariable( name, false ) != NULL );
}

/*
========================
hfSWFScriptObject::HasValidProperty
========================
*/
bool hfSWFScriptObject::HasValidProperty( const char * name ) {
	hfSWFScriptObject::swfNamedVar_t * const variable = GetVariable( name, false );
	if ( variable == NULL ) {
		return false;
	}
	if ( variable->native != NULL ) {
		hfSWFScriptVar nv = variable->native->Get( this );
		if ( nv.IsNULL() || nv.IsUndefined() ) {
			return false;
		}
	} else {
		if ( variable->value.IsNULL() || variable->value.IsUndefined() ) {
			return false;
		}
	}
	return true;
}

/*
========================
hfSWFScriptObject::Get
========================
*/
hfSWFScriptVar hfSWFScriptObject::Get( const char * name ) {
	swfNamedVar_t * variable = GetVariable( name, false );
	if ( variable == NULL ) {
		return hfSWFScriptVar();
	} else {
		if ( variable->native ) {
			return variable->native->Get( this );
		} else {
			return variable->value;
		}
	}
}

/*
========================
hfSWFScriptObject::Get
========================
*/
hfSWFScriptVar hfSWFScriptObject::Get( int index ) {
	swfNamedVar_t * variable = GetVariable( index, false );
	if ( variable == NULL ) {
		return hfSWFScriptVar();
	} else {
		if ( variable->native ) {
			return variable->native->Get( this );
		} else {
			return variable->value;
		}
	}
}

/*
========================
hfSWFScriptObject::GetSprite
========================
*/
hfSWFSpriteInstance * hfSWFScriptObject::GetSprite( int index ) {
	hfSWFScriptVar var = Get( index );
	return var.ToSprite();
}

/*
========================
hfSWFScriptObject::GetSprite
========================
*/
hfSWFSpriteInstance * hfSWFScriptObject::GetSprite( const char * name ) {
	hfSWFScriptVar var = Get( name );
	return var.ToSprite();
}

/*
========================
hfSWFScriptObject::GetObject
========================
*/
hfSWFScriptObject * hfSWFScriptObject::GetObject( int index ) {
	hfSWFScriptVar var = Get( index );
	if ( var.IsObject() ) {
		return var.GetObject();
	}
	return NULL;
}

/*
========================
hfSWFScriptObject::GetObject
========================
*/
hfSWFScriptObject * hfSWFScriptObject::GetObject( const char * name ) {
	hfSWFScriptVar var = Get( name );
	if ( var.IsObject() ) {
		return var.GetObject();
	}
	return NULL;
}

/*
========================
hfSWFScriptObject::GetText
========================
*/
hfSWFTextInstance * hfSWFScriptObject::GetText( int index ) {
	hfSWFScriptVar var = Get( index );
	if ( var.IsObject() ) {
		return var.GetObject()->GetText();
	}
	return NULL;
}

/*
========================
hfSWFScriptObject::GetText
========================
*/
hfSWFTextInstance * hfSWFScriptObject::GetText( const char * name ) {
	hfSWFScriptVar var = Get( name );
	if ( var.IsObject() ) {
		return var.GetObject()->GetText();
	}
	return NULL;
}

/*
========================
hfSWFScriptObject::Set
========================
*/
void hfSWFScriptObject::Set( const char * name, const hfSWFScriptVar & value ) {
	if ( objectType == SWF_OBJECT_ARRAY ) {
		if ( hfStr::Cmp( name, "length" ) == 0 ) {
			int newLength = value.ToInteger();
			for ( int i = 0; i < variables.Num(); i++ ) {
				if ( variables[i].index >= newLength ) {
					variables.RemoveIndexFast( i );
					i--;
				}
			}
			// rebuild the hash table
			for ( int i = 0; i < VARIABLE_HASH_BUCKETS; i++ ) {
				variablesHash[i] = -1;
			}
			for ( int i = 0; i < variables.Num(); i++ ) {
				int hash = hfStr::Hash( variables[i].name.c_str() ) & ( VARIABLE_HASH_BUCKETS - 1 );
				variables[i].hashNext = variablesHash[hash];
				variablesHash[hash] = i;
			}
		} else {
			int iName = atoi( name );
			if ( iName > 0 || ( iName == 0 && hfStr::Cmp( name, "0" ) == 0 ) ) {
				swfNamedVar_t * lengthVar = GetVariable( "length", true );
				if ( lengthVar->value.ToInteger() <= iName ) {
					lengthVar->value = hfSWFScriptVar( iName + 1 );
				}
			}
		}
	}

	swfNamedVar_t * variable = GetVariable( name, true );
	if ( variable->native ) {
		variable->native->Set( this, value );
	} else if ( ( variable->flags & SWF_VAR_FLAG_READONLY ) == 0 ) {
		variable->value = value;
	}
}

/*
========================
hfSWFScriptObject::Set
========================
*/
void hfSWFScriptObject::Set( int index, const hfSWFScriptVar & value ) {
	if ( index < 0 ) {
		extern hfCVar swf_debug;
		if ( swf_debug.GetBool() ) {
			hfLib::Printf( "SWF: Trying to set a negative array index.\n" );
		}
		return;
	}
	if ( objectType == SWF_OBJECT_ARRAY ) {
		swfNamedVar_t * lengthVar = GetVariable( "length", true );
		if ( lengthVar->value.ToInteger() <= index ) {
			lengthVar->value = hfSWFScriptVar( index + 1 );
		}
	}

	swfNamedVar_t * variable = GetVariable( index, true );
	if ( variable->native ) {
		variable->native->Set( this, value );
	} else if ( ( variable->flags & SWF_VAR_FLAG_READONLY ) == 0 ) {
		variable->value = value;
	}
}

/*
========================
hfSWFScriptObject::SetNative
========================
*/
void hfSWFScriptObject::SetNative( const char * name, hfSWFScriptNativeVariable * native ) {
	swfNamedVar_t * variable = GetVariable( name, true );
	variable->flags = SWF_VAR_FLAG_DONTENUM;
	variable->native = native;
	if ( native->IsReadOnly() ) {
		variable->flags |= SWF_VAR_FLAG_READONLY;
	}
}

/*
========================
hfSWFScriptObject::DefaultValue
========================
*/
hfSWFScriptVar hfSWFScriptObject::DefaultValue( bool stringHint ) {
	const char * methods[2] = { "toString", "valueOf" };
	if ( !stringHint ) {
		SwapValues( methods[0], methods[1] );
	}
	for ( int i = 0; i < 2; i++ ) {
		hfSWFScriptVar method = Get( methods[i] );
		if ( method.IsFunction() ) {
			hfSWFScriptVar value = method.GetFunction()->Call( this, hfSWFParmList() );
			if ( !value.IsObject() && !value.IsFunction() ) {
				return value;
			}
		}
	}
	switch ( objectType ) {
	case SWF_OBJECT_OBJECT:
		if ( swf_debugShowAddress.GetBool() ) {
			return hfSWFScriptVar( va( "[object:%p]", this ) );
		} else {
			return hfSWFScriptVar( "[object]" );
		}
	case SWF_OBJECT_ARRAY:
		if ( swf_debugShowAddress.GetBool() ) {
			return hfSWFScriptVar( va( "[array:%p]", this ) );
		} else {
			return hfSWFScriptVar( "[array]" );
		}
	case SWF_OBJECT_SPRITE:
		if ( data.sprite != NULL ) {
			if ( data.sprite->parent == NULL ) {
				return hfSWFScriptVar( "[_root]" );
			} else {
				return hfSWFScriptVar( va( "[%s]", data.sprite->GetName() ) );
			}
		} else {
			return hfSWFScriptVar( "[NULL]" );
		}
	case SWF_OBJECT_TEXT:
		if ( swf_debugShowAddress.GetBool() ) {
			return hfSWFScriptVar( va( "[edittext:%p]", this ) );
		} else {
			return hfSWFScriptVar( "[edittext]" );
		}
	}
	return hfSWFScriptVar( "[unknown]" );
}

/*
========================
hfSWFScriptObject::GetVariable
========================
*/
hfSWFScriptObject::swfNamedVar_t * hfSWFScriptObject::GetVariable( int index, bool create ) {
	for ( int i = 0; i < variables.Num(); i++ ) {
		if ( variables[i].index == index ) {
			return &variables[i];
		}
	}
	if ( create ) {
		swfNamedVar_t * variable = &variables.Alloc();
		variable->flags = SWF_VAR_FLAG_NONE;
		variable->index = index;
		variable->name = va( "%d", index );
		variable->native = NULL;
		int hash = hfStr::Hash( variable->name ) & ( VARIABLE_HASH_BUCKETS - 1 );
		variable->hashNext = variablesHash[hash];
		variablesHash[hash] = variables.Num() - 1;
		return variable;
	}
	return NULL;
}

/*
========================
hfSWFScriptObject::GetVariable
========================
*/
hfSWFScriptObject::swfNamedVar_t * hfSWFScriptObject::GetVariable( const char * name, bool create ) {
	int hash = hfStr::Hash( name ) & ( VARIABLE_HASH_BUCKETS - 1 );
	for ( int i = variablesHash[hash]; i >= 0; i = variables[i].hashNext ) {
		if ( variables[i].name == name ) {
			return &variables[i];
		}
	}

	if ( prototype != NULL ) {
		swfNamedVar_t * variable = prototype->GetVariable( name, false );
		if ( ( variable != NULL ) && ( variable->native || !create ) ) {
			// If the variable is native, we want to pull it from the prototype even if we're going to set it
			return variable;
		}
	}

	if ( create ) {
		swfNamedVar_t * variable = &variables.Alloc();
		variable->flags = SWF_VAR_FLAG_NONE;
		variable->index = atoi( name );
		if ( variable->index == 0 && hfStr::Cmp( name, "0" ) != 0 ) {
			variable->index = -1;
		}
		variable->name = name;
		variable->native = NULL;
		variable->hashNext = variablesHash[hash];
		variablesHash[hash] = variables.Num() - 1;
		return variable;
	}
	return NULL;
}

/*
========================
hfSWFScriptObject::MakeArray
========================
*/
void hfSWFScriptObject::MakeArray() {
	objectType = SWF_OBJECT_ARRAY;
	swfNamedVar_t * variable = GetVariable( "length", true );
	variable->value = hfSWFScriptVar( 0 );
	variable->flags = SWF_VAR_FLAG_DONTENUM;
}

/*
========================
hfSWFScriptObject::GetNestedVar
========================
*/
hfSWFScriptVar hfSWFScriptObject::GetNestedVar( const char * arg1, const char * arg2, const char * arg3, const char * arg4, const char * arg5, const char * arg6 ) {
	const char * const args[] = { arg1, arg2, arg3, arg4, arg5, arg6 };
	const int numArgs = sizeof( args ) / sizeof( const char * );

	hfStaticList< const char *, numArgs > vars;	
	for ( int i = 0; i < numArgs && args[ i ] != NULL; ++i ) {
		vars.Append( args[ i ] );
	}

	hfSWFScriptObject * baseObject = this;
	hfSWFScriptVar retVal;

	for ( int i = 0; i < vars.Num(); ++i ) {
		hfSWFScriptVar var = baseObject->Get( vars[ i ] );

		// when at the end of object path just use the latest value as result
		if ( i == vars.Num() - 1 ) {
			retVal = var;
			break;
		}

		// encountered variable in path that wasn't an object
		if ( !var.IsObject() ) {
			retVal = hfSWFScriptVar();
			break;
		}

		baseObject = var.GetObject();
	}

	return retVal;
}

/*
========================
hfSWFScriptObject::GetNestedObj
========================
*/
hfSWFScriptObject * hfSWFScriptObject::GetNestedObj( const char * arg1, const char * arg2, const char * arg3, const char * arg4, const char * arg5, const char * arg6 ) {
	hfSWFScriptVar var = GetNestedVar( arg1, arg2, arg3, arg4, arg5, arg6 );

	if ( !var.IsObject() ) {
		return NULL;
	}

	return var.GetObject();
}

/*
========================
hfSWFScriptObject::GetNestedSprite
========================
*/
hfSWFSpriteInstance * hfSWFScriptObject::GetNestedSprite( const char * arg1, const char * arg2, const char * arg3, const char * arg4, const char * arg5, const char * arg6 ) {
	hfSWFScriptVar var = GetNestedVar( arg1, arg2, arg3, arg4, arg5, arg6 );
	return var.ToSprite();

}

/*
========================
hfSWFScriptObject::GetNestedText
========================
*/
hfSWFTextInstance * hfSWFScriptObject::GetNestedText( const char * arg1, const char * arg2, const char * arg3, const char * arg4, const char * arg5, const char * arg6 ) {
	hfSWFScriptVar var = GetNestedVar( arg1, arg2, arg3, arg4, arg5, arg6 );
	return var.ToText();

}

/*
========================
hfSWFScriptObject::PrintToConsole
========================
*/
void hfSWFScriptObject::PrintToConsole() const {
	if ( variables.Num() > 0 ) {
		hfLib::Printf( "%d subelements:\n", variables.Num() );
		int maxVarLength = 0;

		for ( int i = 0; i < variables.Num(); ++i ) {
			const hfSWFScriptObject::swfNamedVar_t & nv = variables[ i ];
			const int nameLength = hfStr::Length( nv.name );
			if ( maxVarLength < nameLength ) {
				maxVarLength = nameLength;
			}
		}

		maxVarLength += 2;	// a little extra padding

		const char * const fmt = va( "%%-%ds %%-10s %%-s\n", maxVarLength );
		hfLib::Printf( fmt, "Name", "Type", "Value" );
		hfLib::Printf( "------------------------------------------------------------\n" );
		for ( int i = 0; i < variables.Num(); ++i ) {
			const hfSWFScriptObject::swfNamedVar_t & nv = variables[ i ];
			hfLib::Printf( fmt, nv.name.c_str(), nv.value.TypeOf(),
				nv.value.ToString().c_str() );
		}
	} else {
		hfLib::Printf( "No subelements\n" );
	}
}
