#ifndef __DICT_H__
#define __DICT_H__

class hfSerializer;

/*
===============================================================================

Key/value dictionary

This is a dictionary class that tracks an arbitrary number of key / value
pair combinations. It is used for map entity spawning, GUI state management,
and other things.

Keys are compared case-insensitive.

Does not allocate memory until the first key/value pair is added.

===============================================================================
*/

class hfKeyValue {
	friend class hfDict;

public:
	const hfStr &		GetKey() const { return *key; }
	const hfStr &		GetValue() const { return *value; }

	size_t				Allocated() const { return key->Allocated() + value->Allocated(); }
	size_t				Size() const { return sizeof( *this ) + key->Size() + value->Size(); }

	bool				operator==( const hfKeyValue &kv ) const { return ( key == kv.key && value == kv.value ); }

private:
	const hfPoolStr *	key;
	const hfPoolStr *	value;
};

/*
================================================
hfSort_KeyValue 
================================================
*/
class hfSort_KeyValue : public hfSort_Quick< hfKeyValue, hfSort_KeyValue > {
public:
	int Compare( const hfKeyValue & a, const hfKeyValue & b ) const { return a.GetKey().Icmp( b.GetKey() ); }
};

class hfDict {
public:
						hfDict();
						hfDict( const hfDict &other );	// allow declaration with assignment
						~hfDict();

						// set the granularity for the index
	void				SetGranularity( int granularity );
						// set hash size
	void				SetHashSize( int hashSize );
						// clear existing key/value pairs and copy all key/value pairs from other
	hfDict &			operator=( const hfDict &other );
						// copy from other while leaving existing key/value pairs in place
	void				Copy( const hfDict &other );
						// clear existing key/value pairs and transfer key/value pairs from other
	void				TransferKeyValues( hfDict &other );
						// parse dict from parser
	bool				Parse( hfParser &parser );
						// copy key/value pairs from other dict not present in this dict
	void				SetDefaults( const hfDict *dict );
						// clear dict freeing up memory
	void				Clear();
						// print the dict
	void				Print() const;

	size_t				Allocated() const;
	size_t				Size() const { return sizeof( *this ) + Allocated(); }

	void				Set( const char *key, const char *value );
	void				SetFloat( const char *key, float val );
	void				SetInt( const char *key, int val );
	void				SetBool( const char *key, bool val );
	void				SetVector( const char *key, const hfVec3 &val );
	void				SetVec2( const char *key, const hfVec2 &val );
	void				SetVec4( const char *key, const hfVec4 &val );
	void				SetAngles( const char *key, const hfAngles &val );
	void				SetMatrix( const char *key, const hfMat3 &val );

						// these return default values of 0.0, 0 and false
	const char *		GetString( const char *key, const char *defaultString = "" ) const;
	float				GetFloat( const char *key, const char *defaultString ) const;
	int					GetInt( const char *key, const char *defaultString ) const;
	bool				GetBool( const char *key, const char *defaultString ) const;
	float				GetFloat( const char *key, const float defaultFloat = 0.0f ) const;
	int					GetInt( const char *key, const int defaultInt = 0 ) const;
	bool				GetBool( const char *key, const bool defaultBool = false ) const;
	hfVec3				GetVector( const char *key, const char *defaultString = NULL ) const;
	hfVec2				GetVec2( const char *key, const char *defaultString = NULL ) const;
	hfVec4				GetVec4( const char *key, const char *defaultString = NULL ) const;
	hfAngles			GetAngles( const char *key, const char *defaultString = NULL ) const;
	hfMat3				GetMatrix( const char *key, const char *defaultString = NULL ) const;

	bool				GetString( const char *key, const char *defaultString, const char **out ) const;
	bool				GetString( const char *key, const char *defaultString, hfStr &out ) const;
	bool				GetFloat( const char *key, const char *defaultString, float &out ) const;
	bool				GetInt( const char *key, const char *defaultString, int &out ) const;
	bool				GetBool( const char *key, const char *defaultString, bool &out ) const;
	bool				GetFloat( const char *key, const float defaultFloat, float &out ) const;
	bool				GetInt( const char *key, const int defaultInt, int &out ) const;
	bool				GetBool( const char *key, const bool defaultBool, bool &out ) const;
	bool				GetVector( const char *key, const char *defaultString, hfVec3 &out ) const;
	bool				GetVec2( const char *key, const char *defaultString, hfVec2 &out ) const;
	bool				GetVec4( const char *key, const char *defaultString, hfVec4 &out ) const;
	bool				GetAngles( const char *key, const char *defaultString, hfAngles &out ) const;
	bool				GetMatrix( const char *key, const char *defaultString, hfMat3 &out ) const;

	int					GetNumKeyVals() const;
	const hfKeyValue *	GetKeyVal( int index ) const;
						// returns the key/value pair with the given key
						// returns NULL if the key/value pair does not exist
	const hfKeyValue *	FindKey( const char *key ) const;
						// returns the index to the key/value pair with the given key
						// returns -1 if the key/value pair does not exist
	int					FindKeyIndex( const char *key ) const;
						// delete the key/value pair with the given key
	void				Delete( const char *key );
						// finds the next key/value pair with the given key prefix.
						// lastMatch can be used to do additional searches past the first match.
	const hfKeyValue *	MatchPrefix( const char *prefix, const hfKeyValue *lastMatch = NULL ) const;
						// randomly chooses one of the key/value pairs with the given key prefix and returns it's value
	const char *		RandomPrefix( const char *prefix, hfRandom &random ) const;

	void				WriteToFileHandle( hfFile *f ) const;
	void				ReadFromFileHandle( hfFile *f );

	void				WriteToIniFile( hfFile * f ) const;
	bool				ReadFromIniFile( hfFile * f );

	void				Serialize( hfSerializer & ser );

						// returns a unique checksum for this dictionary's content
	int					Checksum() const;

	static void			Init();
	static void			Shutdown();

	static void			ShowMemoryUsage_f( const hfCmdArgs &args );
	static void			ListKeys_f( const hfCmdArgs &args );
	static void			ListValues_f( const hfCmdArgs &args );

private:
	hfList<hfKeyValue>	args;
	hfHashIndex			argHash;

	static hfStrPool	globalKeys;
	static hfStrPool	globalValues;
};


HF_INLINE hfDict::hfDict() {
	args.SetGranularity( 16 );
	argHash.SetGranularity( 16 );
	argHash.Clear( 128, 16 );
}

HF_INLINE hfDict::hfDict( const hfDict &other ) {
	*this = other;
}

HF_INLINE hfDict::~hfDict() {
	Clear();
}

HF_INLINE void hfDict::SetGranularity( int granularity ) {
	args.SetGranularity( granularity );
	argHash.SetGranularity( granularity );
}

HF_INLINE void hfDict::SetHashSize( int hashSize ) {
	if ( args.Num() == 0 ) {
		argHash.Clear( hashSize, 16 );
	}
}

HF_INLINE void hfDict::SetFloat( const char *key, float val ) {
	Set( key, va( "%f", val ) );
}

HF_INLINE void hfDict::SetInt( const char *key, int val ) {
	Set( key, va( "%i", val ) );
}

HF_INLINE void hfDict::SetBool( const char *key, bool val ) {
	Set( key, va( "%i", val ) );
}

HF_INLINE void hfDict::SetVector( const char *key, const hfVec3 &val ) {
	Set( key, val.ToString() );
}

HF_INLINE void hfDict::SetVec4( const char *key, const hfVec4 &val ) {
	Set( key, val.ToString() );
}

HF_INLINE void hfDict::SetVec2( const char *key, const hfVec2 &val ) {
	Set( key, val.ToString() );
}

HF_INLINE void hfDict::SetAngles( const char *key, const hfAngles &val ) {
	Set( key, val.ToString() );
}

HF_INLINE void hfDict::SetMatrix( const char *key, const hfMat3 &val ) {
	Set( key, val.ToString() );
}

HF_INLINE bool hfDict::GetString( const char *key, const char *defaultString, const char **out ) const {
	const hfKeyValue *kv = FindKey( key );
	if ( kv ) {
		*out = kv->GetValue();
		return true;
	}
	*out = defaultString;
	return false;
}

HF_INLINE bool hfDict::GetString( const char *key, const char *defaultString, hfStr &out ) const {
	const hfKeyValue *kv = FindKey( key );
	if ( kv ) {
		out = kv->GetValue();
		return true;
	}
	out = defaultString;
	return false;
}

HF_INLINE const char *hfDict::GetString( const char *key, const char *defaultString ) const {
	const hfKeyValue *kv = FindKey( key );
	if ( kv ) {
		return kv->GetValue();
	}
	return defaultString;
}

HF_INLINE float hfDict::GetFloat( const char *key, const char *defaultString ) const {
	return atof( GetString( key, defaultString ) );
}

HF_INLINE int hfDict::GetInt( const char *key, const char *defaultString ) const {
	return atoi( GetString( key, defaultString ) );
}

HF_INLINE bool hfDict::GetBool( const char *key, const char *defaultString ) const {
	return ( atoi( GetString( key, defaultString ) ) != 0 );
}

HF_INLINE float hfDict::GetFloat( const char *key, const float defaultFloat ) const {
	const hfKeyValue *kv = FindKey( key );
	if ( kv ) {
		return atof( kv->GetValue() );
	}
	return defaultFloat;
}

HF_INLINE int hfDict::GetInt( const char *key, int defaultInt ) const {
	const hfKeyValue *kv = FindKey( key );
	if ( kv ) {
		return atoi( kv->GetValue() );
	}
	return defaultInt;
}

HF_INLINE bool hfDict::GetBool( const char *key, const bool defaultBool ) const {
	const hfKeyValue *kv = FindKey( key );
	if ( kv ) {
		return atoi( kv->GetValue() ) != 0;
	}
	return defaultBool;
}

HF_INLINE hfVec3 hfDict::GetVector( const char *key, const char *defaultString ) const {
	hfVec3 out;
	GetVector( key, defaultString, out );
	return out;
}

HF_INLINE hfVec2 hfDict::GetVec2( const char *key, const char *defaultString ) const {
	hfVec2 out;
	GetVec2( key, defaultString, out );
	return out;
}

HF_INLINE hfVec4 hfDict::GetVec4( const char *key, const char *defaultString ) const {
	hfVec4 out;
	GetVec4( key, defaultString, out );
	return out;
}

HF_INLINE hfAngles hfDict::GetAngles( const char *key, const char *defaultString ) const {
	hfAngles out;
	GetAngles( key, defaultString, out );
	return out;
}

HF_INLINE hfMat3 hfDict::GetMatrix( const char *key, const char *defaultString ) const {
	hfMat3 out;
	GetMatrix( key, defaultString, out );
	return out;
}

HF_INLINE int hfDict::GetNumKeyVals() const {
	return args.Num();
}

HF_INLINE const hfKeyValue *hfDict::GetKeyVal( int index ) const {
	if ( index >= 0 && index < args.Num() ) {
		return &args[ index ];
	}
	return NULL;
}

#endif /* !__DICT_H__ */

