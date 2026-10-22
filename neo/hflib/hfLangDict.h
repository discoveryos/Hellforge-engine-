#ifndef __LANGDICT_H__
#define __LANGDICT_H__

class hfLangKeyValue {
public:
	hfLangKeyValue() : key( NULL ), value( NULL ) { }
	hfLangKeyValue( char * k, char * v ) : key ( k ), value( v ) { }
	char * key;
	char * value;
};

class hfStrId;

/*
================================================
hfLangDict is a simple Dictionary used specifically for the 
LocalizedStringTables.
================================================
*/
class hfLangDict {
public:
	static const char *		KEY_PREFIX;
	static const int		KEY_PREFIX_LEN;
	static const int		MAX_REDIRECTION_DEPTH = 2;

	hfLangDict();
	~hfLangDict();

	void					Clear();
	bool					Load( const byte * buffer, const int bufferLen, const char * name );
	bool					Save( const char * fileName );

	const char *			GetString( const char * str ) const;		// returns str if string not found
	const char *			FindString( const char * str ) const;		// returns NULL if string not found

	const char *			AddString( const char * str );				// returns a randomly generated key
	bool					DeleteString( const char * key );			// returns false if the key doesn't exist
	bool					RenameStringKey( const char * oldKey, const char * newKey );

	bool					SetString( const char * key, const char * val );	// Returns false if the key doesn't exist
	void					AddKeyVal( const char * key, const char * val );	// Like SetString, but adds it if it doesn't already exist

	int						GetNumKeyVals() const;
	const hfLangKeyValue *	GetKeyVal( int i ) const;
	bool					DeleteString( const int idx );

	const char *			GetLocalizedString( const hfStrId & strId ) const;

	// returns true if the string starts with the KEY_PREFIX string
	static bool				IsStringId( const char * str );

private:
	hfDynamicBlockAlloc< char, 100 * 1024, 16 > blockAlloc;
	hfList< hfLangKeyValue >	keyVals;
	hfHashIndex keyIndex;

private:
	int						FindStringIndex( const char * str ) const;
	const char *			FindString_r( const char * str, int & depth ) const;

	friend class hfStrId;
};

/*
================================================
hfLocalization 
================================================
*/
class hfLocalization {
public:
	static const char *		GetString( const char * inString );		// returns inString if string not found
	static const char *		FindString( const char * inString );	// Returns NULL if string not found

	static void				ClearDictionary();
	static bool				LoadDictionary( const byte * buffer, const int bufferLen, const char * name );

	// This is only here for tools, normal code should only ever call GetString
	static hfLangDict &		GetDictionary() { return languageDict; }

	static utf8Encoding_t	VerifyUTF8( const uint8 * buffer, const int bufferLen, const char * name );

private:
	static hfLangDict					languageDict;
	friend class hfStrId;
};

/*
================================================
hfStrId represents a localized String as a String ID.
================================================
*/
class hfStrId {
public:
	hfStrId() : index( -1 ) { }
	hfStrId( const hfStrId & other ) : index( other.index ) { }

	explicit hfStrId( int i ) : index( i ) { }
	explicit hfStrId( const char * key ) { Set( key ); }
	explicit hfStrId( const hfStr & key ) { Set( key ); }

	void operator=( const char * key ) { Set( key ); }
	void operator=( const hfStr & key ) { Set( key ); }
	void operator=( const hfStrId & other ) { index = other.index; }

	bool operator==( const hfStrId & other ) const { return index == other.index; }
	bool operator!=( const hfStrId & other ) const { return index != other.index; }

	void			Set( const char * key );

	void			Empty() { index = -1; }
	bool			IsEmpty() const { return index < 0; }

	const char *	GetKey() const;
	const char *	GetLocalizedString() const;

	int				GetIndex() const { return index; }
	void			SetIndex( int i ) { index = i; }

private:
	int index;	// Index into the language dictionary
};

#endif // !__LANGDICT_H__

