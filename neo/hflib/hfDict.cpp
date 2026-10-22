#include "hfprecompiled.h"
#pragma hdrstop

hfStrPool		hfDict::globalKeys;
hfStrPool		hfDict::globalValues;

/*
================
hfDict::operator=

  clear existing key/value pairs and copy all key/value pairs from other
================
*/
hfDict &hfDict::operator=( const hfDict &other ) {
	int i;

	// check for assignment to self
	if ( this == &other ) {
		return *this;
	}

	Clear();

	args = other.args;
	argHash = other.argHash;

	for ( i = 0; i < args.Num(); i++ ) {
		args[i].key = globalKeys.CopyString( args[i].key );
		args[i].value = globalValues.CopyString( args[i].value );
	}

	return *this;
}

/*
================
hfDict::Copy

  copy all key value pairs without removing existing key/value pairs not present in the other dict
================
*/
void hfDict::Copy( const hfDict &other ) {
	int i, n, *found;
	hfKeyValue kv;

	// check for assignment to self
	if ( this == &other ) {
		return;
	}

	n = other.args.Num();

	if ( args.Num() ) {
		found = (int *) _alloca16( other.args.Num() * sizeof( int ) );
        for ( i = 0; i < n; i++ ) {
			found[i] = FindKeyIndex( other.args[i].GetKey() );
		}
	} else {
		found = NULL;
	}

	for ( i = 0; i < n; i++ ) {
		if ( found && found[i] != -1 ) {
			// first set the new value and then free the old value to allow proper self copying
			const hfPoolStr *oldValue = args[found[i]].value;
			args[found[i]].value = globalValues.CopyString( other.args[i].value );
			globalValues.FreeString( oldValue );
		} else {
			kv.key = globalKeys.CopyString( other.args[i].key );
			kv.value = globalValues.CopyString( other.args[i].value );
			argHash.Add( argHash.GenerateKey( kv.GetKey(), false ), args.Append( kv ) );
		}
	}
}

/*
================
hfDict::TransferKeyValues

  clear existing key/value pairs and transfer key/value pairs from other
================
*/
void hfDict::TransferKeyValues( hfDict &other ) {
	int i, n;

	if ( this == &other ) {
		return;
	}

	if ( other.args.Num() && other.args[0].key->GetPool() != &globalKeys ) {
		common->FatalError( "hfDict::TransferKeyValues: can't transfer values across a DLL boundary" );
		return;
	}

	Clear();

	n = other.args.Num();
	args.SetNum( n );
	for ( i = 0; i < n; i++ ) {
		args[i].key = other.args[i].key;
		args[i].value = other.args[i].value;
	}
	argHash = other.argHash;

	other.args.Clear();
	other.argHash.Free();
}

/*
================
hfDict::Parse
================
*/
bool hfDict::Parse( hfParser &parser ) {
	hfToken	token;
	hfToken	token2;
	bool	errors;

	errors = false;

	parser.ExpectTokenString( "{" );
	parser.ReadToken( &token );
	while( ( token.type != TT_PUNCTUATION ) || ( token != "}" ) ) {
		if ( token.type != TT_STRING ) {
			parser.Error( "Expected quoted string, but found '%s'", token.c_str() );
		}

		if ( !parser.ReadToken( &token2 ) ) {
			parser.Error( "Unexpected end of file" );
		}

		if ( FindKey( token ) ) {
			parser.Warning( "'%s' already defined", token.c_str() );
			errors = true;
		}
		Set( token, token2 );

		if ( !parser.ReadToken( &token ) ) {
			parser.Error( "Unexpected end of file" );
		}
	}

	return !errors;
}

/*
================
hfDict::SetDefaults
================
*/
void hfDict::SetDefaults( const hfDict *dict ) {
	int i, n;
	const hfKeyValue *kv, *def;
	hfKeyValue newkv;

	n = dict->args.Num();
	for( i = 0; i < n; i++ ) {
		def = &dict->args[i];
		kv = FindKey( def->GetKey() );
		if ( !kv ) {
			newkv.key = globalKeys.CopyString( def->key );
			newkv.value = globalValues.CopyString( def->value );
			argHash.Add( argHash.GenerateKey( newkv.GetKey(), false ), args.Append( newkv ) );
		}
	}
}

/*
================
hfDict::Clear
================
*/
void hfDict::Clear() {
	int i;

	for( i = 0; i < args.Num(); i++ ) {
		globalKeys.FreeString( args[i].key );
		globalValues.FreeString( args[i].value );
	}

	args.Clear();
	argHash.Free();
}

/*
================
hfDict::Print
================
*/
void hfDict::Print() const {
	int i;
	int n;

	n = args.Num();
	for( i = 0; i < n; i++ ) {
		hfLib::common->Printf( "%s = %s\n", args[i].GetKey().c_str(), args[i].GetValue().c_str() );
	}
}

int KeyCompare( const hfKeyValue *a, const hfKeyValue *b ) {
	return hfStr::Cmp( a->GetKey(), b->GetKey() );
}

/*
================
hfDict::Checksum
================
*/
int	hfDict::Checksum() const {
	unsigned long ret;
	int i, n;

	hfList<hfKeyValue> sorted = args;
	sorted.SortWithTemplate( hfSort_KeyValue() );
	n = sorted.Num();
	CRC32_InitChecksum( ret );
	for( i = 0; i < n; i++ ) {
		CRC32_UpdateChecksum( ret, sorted[i].GetKey().c_str(), sorted[i].GetKey().Length() );
		CRC32_UpdateChecksum( ret, sorted[i].GetValue().c_str(), sorted[i].GetValue().Length() );
	}
	CRC32_FinishChecksum( ret );
	return ret;
}

/*
================
hfDict::Allocated
================
*/
size_t hfDict::Allocated() const {
	int		i;
	size_t	size;

	size = args.Allocated() + argHash.Allocated();
	for( i = 0; i < args.Num(); i++ ) {
		size += args[i].Size();
	}

	return size;
}

/*
================
hfDict::Set
================
*/
void hfDict::Set( const char *key, const char *value ) {
	int i;
	hfKeyValue kv;

	if ( key == NULL || key[0] == '\0' ) {
		return;
	}

	i = FindKeyIndex( key );
	if ( i != -1 ) {
		// first set the new value and then free the old value to allow proper self copying
		const hfPoolStr *oldValue = args[i].value;
		args[i].value = globalValues.AllocString( value );
		globalValues.FreeString( oldValue );
	} else {
		kv.key = globalKeys.AllocString( key );
		kv.value = globalValues.AllocString( value );
		argHash.Add( argHash.GenerateKey( kv.GetKey(), false ), args.Append( kv ) );
	}
}

/*
================
hfDict::GetFloat
================
*/
bool hfDict::GetFloat( const char *key, const char *defaultString, float &out ) const {
	const char	*s;
	bool		found;

	found = GetString( key, defaultString, &s );
	out = atof( s );
	return found;
}

/*
================
hfDict::GetInt
================
*/
bool hfDict::GetInt( const char *key, const char *defaultString, int &out ) const {
	const char	*s;
	bool		found;

	found = GetString( key, defaultString, &s );
	out = atoi( s );
	return found;
}

/*
================
hfDict::GetBool
================
*/
bool hfDict::GetBool( const char *key, const char *defaultString, bool &out ) const {
	const char	*s;
	bool		found;

	found = GetString( key, defaultString, &s );
	out = ( atoi( s ) != 0 );
	return found;
}

/*
================
hfDict::GetFloat
================
*/
bool hfDict::GetFloat( const char *key, const float defaultFloat, float &out ) const {
	const hfKeyValue *kv = FindKey( key );
	if ( kv ) {
		out = atof( kv->GetValue() );
		return true;
	} else {
		out = defaultFloat;
		return false;
	}
}

/*
================
hfDict::GetInt
================
*/
bool hfDict::GetInt( const char *key, const int defaultInt, int &out ) const {
	const hfKeyValue *kv = FindKey( key );
	if ( kv ) {
		out = atoi( kv->GetValue() );
		return true;
	} else {
		out = defaultInt;
		return false;
	}
}

/*
================
hfDict::GetBool
================
*/
bool hfDict::GetBool( const char *key, const bool defaultBool, bool &out ) const {
	const hfKeyValue *kv = FindKey( key );
	if ( kv ) {
		out = ( atoi( kv->GetValue() ) != 0 );
		return true;
	} else {
		out = defaultBool;
		return false;
	}
}

/*
================
hfDict::GetAngles
================
*/
bool hfDict::GetAngles( const char *key, const char *defaultString, hfAngles &out ) const {
	bool		found;
	const char	*s;
	
	if ( !defaultString ) {
		defaultString = "0 0 0";
	}

	found = GetString( key, defaultString, &s );
	out.Zero();	
	sscanf( s, "%f %f %f", &out.pitch, &out.yaw, &out.roll );
	return found;
}

/*
================
hfDict::GetVector
================
*/
bool hfDict::GetVector( const char *key, const char *defaultString, hfVec3 &out ) const {
	bool		found;
	const char	*s;
	
	if ( !defaultString ) {
		defaultString = "0 0 0";
	}

	found = GetString( key, defaultString, &s );
	out.Zero();
	sscanf( s, "%f %f %f", &out.x, &out.y, &out.z );
	return found;
}

/*
================
hfDict::GetVec2
================
*/
bool hfDict::GetVec2( const char *key, const char *defaultString, hfVec2 &out ) const {
	bool		found;
	const char	*s;
	
	if ( !defaultString ) {
		defaultString = "0 0";
	}

	found = GetString( key, defaultString, &s );
	out.Zero();
	sscanf( s, "%f %f", &out.x, &out.y );
	return found;
}

/*
================
hfDict::GetVec4
================
*/
bool hfDict::GetVec4( const char *key, const char *defaultString, hfVec4 &out ) const {
	bool		found;
	const char	*s;
	
	if ( !defaultString ) {
		defaultString = "0 0 0 0";
	}

	found = GetString( key, defaultString, &s );
	out.Zero();
	sscanf( s, "%f %f %f %f", &out.x, &out.y, &out.z, &out.w );
	return found;
}

/*
================
hfDict::GetMatrix
================
*/
bool hfDict::GetMatrix( const char *key, const char *defaultString, hfMat3 &out ) const {
	const char	*s;
	bool		found;
		
	if ( !defaultString ) {
		defaultString = "1 0 0 0 1 0 0 0 1";
	}

	found = GetString( key, defaultString, &s );
	out.Identity();		// sccanf has a bug in it on Mac OS 9.  Sigh.
	sscanf( s, "%f %f %f %f %f %f %f %f %f", &out[0].x, &out[0].y, &out[0].z, &out[1].x, &out[1].y, &out[1].z, &out[2].x, &out[2].y, &out[2].z );
	return found;
}

/*
================
WriteString
================
*/
static void WriteString( const char *s, hfFile *f ) {
	int	len = strlen( s );
	if ( len >= MAX_STRING_CHARS-1 ) {
		hfLib::common->Error( "hfDict::WriteToFileHandle: bad string" );
	}
	f->Write( s, strlen(s) + 1 );
}

/*
================
hfDict::FindKey
================
*/
const hfKeyValue *hfDict::FindKey( const char *key ) const {
	int i, hash;

	if ( key == NULL || key[0] == '\0' ) {
		hfLib::common->DWarning( "hfDict::FindKey: empty key" );
		return NULL;
	}

	hash = argHash.GenerateKey( key, false );
	for ( i = argHash.First( hash ); i != -1; i = argHash.Next( i ) ) {
		if ( args[i].GetKey().Icmp( key ) == 0 ) {
			return &args[i];
		}
	}

	return NULL;
}

/*
================
hfDict::FindKeyIndex
================
*/
int hfDict::FindKeyIndex( const char *key ) const {

	if ( key == NULL || key[0] == '\0' ) {
		hfLib::common->DWarning( "hfDict::FindKeyIndex: empty key" );
		return 0;
	}

	int hash = argHash.GenerateKey( key, false );
	for ( int i = argHash.First( hash ); i != -1; i = argHash.Next( i ) ) {
		if ( args[i].GetKey().Icmp( key ) == 0 ) {
			return i;
		}
	}

	return -1;
}

/*
================
hfDict::Delete
================
*/
void hfDict::Delete( const char *key ) {
	int hash, i;

	hash = argHash.GenerateKey( key, false );
	for ( i = argHash.First( hash ); i != -1; i = argHash.Next( i ) ) {
		if ( args[i].GetKey().Icmp( key ) == 0 ) {
			globalKeys.FreeString( args[i].key );
			globalValues.FreeString( args[i].value );
			args.RemoveIndex( i );
			argHash.RemoveIndex( hash, i );
			break;
		}
	}

#if 0
	// make sure all keys can still be found in the hash index
	for ( i = 0; i < args.Num(); i++ ) {
		assert( FindKey( args[i].GetKey() ) != NULL );
	}
#endif
}

/*
================
hfDict::MatchPrefix
================
*/
const hfKeyValue *hfDict::MatchPrefix( const char *prefix, const hfKeyValue *lastMatch ) const {
	int	i;
	int len;
	int start;

	assert( prefix );
	len = strlen( prefix );

	start = -1;
	if ( lastMatch ) {
		start = args.FindIndex( *lastMatch );
		assert( start >= 0 );
		if ( start < 1 ) {
			start = 0;
		}
	}

	for( i = start + 1; i < args.Num(); i++ ) {
		if ( !args[i].GetKey().Icmpn( prefix, len ) ) {
			return &args[i];
		}
	}
	return NULL;
}

/*
================
hfDict::RandomPrefix
================
*/
const char *hfDict::RandomPrefix( const char *prefix, hfRandom &random ) const {
	int count;
	const int MAX_RANDOM_KEYS = 2048;
	const char *list[MAX_RANDOM_KEYS];
	const hfKeyValue *kv;

	list[0] = "";
	for ( count = 0, kv = MatchPrefix( prefix ); kv != NULL && count < MAX_RANDOM_KEYS; kv = MatchPrefix( prefix, kv ) ) {
		list[count++] = kv->GetValue().c_str();
	}
	return list[random.RandomInt( count )];
}

/*
================
hfDict::WriteToFileHandle
================
*/
void hfDict::WriteToFileHandle( hfFile *f ) const {
	int c = LittleLong( args.Num() );
	f->Write( &c, sizeof( c ) );
	for ( int i = 0; i < args.Num(); i++ ) {	// don't loop on the swapped count use the original
		WriteString( args[i].GetKey().c_str(), f );
		WriteString( args[i].GetValue().c_str(), f );
	}
}

/*
================
ReadString
================
*/
static hfStr ReadString( hfFile *f ) {
	char	str[MAX_STRING_CHARS];
	int		len;

	for ( len = 0; len < MAX_STRING_CHARS; len++ ) {
		f->Read( (void *)&str[len], 1 );
		if ( str[len] == 0 ) {
			break;
		}
	}
	if ( len == MAX_STRING_CHARS ) {
		hfLib::common->Error( "hfDict::ReadFromFileHandle: bad string" );
	}

	return hfStr( str );
}

/*
================
hfDict::ReadFromFileHandle
================
*/
void hfDict::ReadFromFileHandle( hfFile *f ) {
	int c;
	hfStr key, val;

	Clear();

	f->Read( &c, sizeof( c ) );
	c = LittleLong( c );
	for ( int i = 0; i < c; i++ ) {
		key = ReadString( f );
		val = ReadString( f );
		Set( key, val );
	}
}

/*
========================
hfDict::Serialize
========================
*/
void hfDict::Serialize( hfSerializer & ser ) {
	if ( ser.IsReading() ) {
		Clear();
	}

	int num = args.Num();
	ser.SerializePacked( num );
	for ( int i = 0; i < num; i++ ) {
		hfStr key;
		hfStr val; 

		if ( ser.IsWriting() ) {
			key = args[i].GetKey();
			val = args[i].GetValue();
		}

		ser.SerializeString( key );
		ser.SerializeString( val );

		if ( ser.IsReading() ) {
			Set( key.c_str(), val.c_str() );
		}
	}
}

/*
================
hfDict::WriteToIniFile
================
*/
void hfDict::WriteToIniFile( hfFile * f ) const {
	// make a copy so we don't affect the checksum of the original dict
	hfList< hfKeyValue > sortedArgs( args );
	sortedArgs.SortWithTemplate( hfSort_KeyValue() );

	hfList< hfStr > prefixList;
	hfTempArray< int > prefixIndex( sortedArgs.Num() );	// for each keyValue in the args, this is an index into which prefix it uses.
														// 0 means no prefix, otherwise, it's an index + (-1) into prefixList
														// we do this so we can print all the non-prefix based pairs first
	hfStr prevPrefix = "";
	hfStr skipFirstLine = "";

	// Scan for all the prefixes
	for ( int i = 0; i < sortedArgs.Num(); i++ ) {
		const hfKeyValue * kv = &sortedArgs[i];
		int slashPosition = kv->GetKey().Last( '/' );
		if ( slashPosition != hfStr::INVALID_POSITION ) {
			hfStr prefix = kv->GetKey().Mid( 0, slashPosition );
			if ( prefix != prevPrefix ) {
				prevPrefix = prefix;
				prefixList.Append( prefix );
			}
			prefixIndex[i] = prefixList.Num();
		} else {
			prefixIndex[i] = 0;

			// output all the prefix-less first
			hfStr str = va( "%s=%s\n", kv->GetKey().c_str(), hfStr::CStyleQuote( kv->GetValue() ) );
			f->Write( (void *)str.c_str(), str.Length() );

			skipFirstLine = "\n";
		}
	}

	int prevPrefixIndex = 0;
	int prefixLength = 0;

	// output all the rest without their prefix
	for ( int i = 0; i < sortedArgs.Num(); i++ ) {
		if ( prefixIndex[i] == 0 ) {
			continue;
		}

		if ( prefixIndex[i] != prevPrefixIndex ) {
			prevPrefixIndex = prefixIndex[i];
			prefixLength = prefixList[prevPrefixIndex - 1].Length() + 1; // to skip past the '/' too

			// output prefix
			hfStr str = va( "%s[%s]\n", skipFirstLine.c_str(), prefixList[prevPrefixIndex - 1].c_str() );
			f->Write( (void *)str.c_str(), str.Length() );
		}

		const hfKeyValue * kv = &sortedArgs[i];
		hfStr str = va( "%s=%s\n", kv->GetKey().c_str() + prefixLength, hfStr::CStyleQuote( kv->GetValue() ) );
		f->Write( (void *)str.c_str(), str.Length() );
	}
}

/*
================
hfDict::ReadFromIniFile
================
*/
bool hfDict::ReadFromIniFile( hfFile * f ) {
	int length = f->Length();
	hfTempArray< char > buffer( length );
	if ( (int)f->Read( buffer.Ptr(), length ) != length ) {
		return false;
	}
	buffer[length-1] = NULL;	// Since the .ini files are not null terminated, make sure we mark where the end of the .ini file is in our read buffer

	hfLexer parser( LEXFL_NOFATALERRORS | LEXFL_ALLOWPATHNAMES /*| LEXFL_ONLYSTRINGS */);
	hfStr name = f->GetName();
	name.Append( " dictionary INI reader" );
	if ( !parser.LoadMemory( ( const char* )buffer.Ptr(), length, name.c_str() ) ) {
		return false;
	}

	hfToken	token;
	hfToken	token2;
	hfStr prefix = "";
	hfStr valueStr;
	bool success = true;

	Clear();

	const punctuation_t ini_punctuations[] = {
		{ "[", P_SQBRACKETOPEN },
		{ "]", P_SQBRACKETCLOSE },
		{ "=", P_ASSIGN },
		{ NULL, 0 }
	};
	parser.SetPunctuations( ini_punctuations );

	while ( success && !parser.EndOfFile() ) {
		if ( parser.PeekTokenType( TT_PUNCTUATION, P_SQBRACKETOPEN, &token ) ) {
			success = success && parser.ExpectTokenType( TT_PUNCTUATION, P_SQBRACKETOPEN, &token );
			success = success && parser.ReadToken( &token );
			prefix = token.c_str();
			prefix.Append( '/' );
			success = success && parser.ExpectTokenType( TT_PUNCTUATION, P_SQBRACKETCLOSE, &token );
		}
		
		if ( !parser.PeekTokenType( TT_NAME, 0, &token ) ) {
			// end of file most likely
			break;
		}

		success = success && parser.ExpectTokenType( TT_NAME, 0, &token );
		success = success && parser.ExpectTokenType( TT_PUNCTUATION, P_ASSIGN, &token2 );
		success = success && ( parser.ParseRestOfLine( valueStr ) != NULL );

		valueStr = hfStr::CStyleUnQuote( valueStr );

		hfStr key = va( "%s%s", prefix.c_str(), token.c_str() );
		if ( FindKey( key.c_str() ) ) {
			parser.Warning( "'%s' already defined", key.c_str() );
		}

		Set( key.c_str(), valueStr.c_str() );
	}

	return success;
}

CONSOLE_COMMAND( TestDictIniFile, "Tests the writing/reading of various items in a dict to/from an ini file", 0 ) {
	// Write to the file
	hfFile * file = fileSystem->OpenFileWrite( "hfDict_ini_test.ini" );
	if ( file == NULL ) {
		hfLib::Printf( "[^1FAILED^0] Couldn't open file for writing.\n" );
		return;
	}

	hfDict vars;
	vars.SetInt( "section1/section3/a", -1 );
	vars.SetInt( "section1/section3/b", 0 );
	vars.SetInt( "section1/section3/c", 3 );
	vars.SetFloat( "section2/d", 4.0f );
	vars.SetFloat( "section2/e", -5.0f );
	vars.SetBool( "section2/f", true );
	vars.SetBool( "section1/g", false );
	vars.Set( "section1/h", "test1" );
	vars.Set( "i", "1234" );
	vars.SetInt( "j", 9 );
	vars.WriteToIniFile( file );
	delete file;

	// Read from the file
	file = fileSystem->OpenFileRead( "hfDict_ini_test.ini" );
	if ( file == NULL ) {
		hfLib::Printf( "[^1FAILED^0] Couldn't open file for reading.\n" );
	}

	hfDict readVars;
	readVars.ReadFromIniFile( file );
	delete file;

	if ( vars.Checksum() != readVars.Checksum() ) {
		hfLib::Printf( "[^1FAILED^0] Dictionaries do not match.\n" );
	} else {
		hfLib::Printf( "[^2PASSED^0] Dictionaries match.\n" );
	}

	// Output results
	for ( int i = 0; i < readVars.GetNumKeyVals(); i++ ) {
		const hfKeyValue * kv = readVars.GetKeyVal( i );
		hfLib::Printf( "%s=%s\n", kv->GetKey().c_str(), kv->GetValue().c_str() );
	}
}

/*
================
hfDict::Init
================
*/
void hfDict::Init() {
	globalKeys.SetCaseSensitive( false );
	globalValues.SetCaseSensitive( true );
}

/*
================
hfDict::Shutdown
================
*/
void hfDict::Shutdown() {
	globalKeys.Clear();
	globalValues.Clear();
}

/*
================
hfDict::ShowMemoryUsage_f
================
*/
void hfDict::ShowMemoryUsage_f( const hfCmdArgs &args ) {
	hfLib::common->Printf( "%5d KB in %d keys\n", globalKeys.Size() >> 10, globalKeys.Num() );
	hfLib::common->Printf( "%5d KB in %d values\n", globalValues.Size() >> 10, globalValues.Num() );
}

/*
================
hfDict::ListKeys_f
================
*/
void hfDict::ListKeys_f( const hfCmdArgs &args ) {
	hfLib::Printf( "Not implemented due to sort impl issues.\n" );
	//int i;
	//hfList<const hfPoolStr *> keyStrings;

	//for ( i = 0; i < globalKeys.Num(); i++ ) {
	//	keyStrings.Append( globalKeys[i] );
	//}
	//keyStrings.SortWithTemplate( hfSort_PoolStrPtr() );
	//for ( i = 0; i < keyStrings.Num(); i++ ) {
	//	hfLib::common->Printf( "%s\n", keyStrings[i]->c_str() );
	//}
	//hfLib::common->Printf( "%5d keys\n", keyStrings.Num() );
}

/*
================
hfDict::ListValues_f
================
*/
void hfDict::ListValues_f( const hfCmdArgs &args ) {
	hfLib::Printf( "Not implemented due to sort impl issues.\n" );
	//int i;
	//hfList<const hfPoolStr *> valueStrings;

	//for ( i = 0; i < globalValues.Num(); i++ ) {
	//	valueStrings.Append( globalValues[i] );
	//}
	//valueStrings.SortWithTemplate( hfSort_PoolStrPtr() );
	//for ( i = 0; i < valueStrings.Num(); i++ ) {
	//	hfLib::common->Printf( "%s\n", valueStrings[i]->c_str() );
	//}
	//hfLib::common->Printf( "%5d values\n", valueStrings.Num() );
}

