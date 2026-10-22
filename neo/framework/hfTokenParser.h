#ifndef __TOKENPARSER_H__
#define __TOKENPARSER_H__
class hfBinaryToken {
public:
	hfBinaryToken() {
		tokenType = 0;
		tokenSubType = 0;
	}
	hfBinaryToken( const hfToken &tok ) {
		token = tok.c_str();
		tokenType = tok.type;
		tokenSubType = tok.subtype;
	}
	bool operator==( const hfBinaryToken &b ) const { 
		return ( tokenType == b.tokenType && tokenSubType == b.tokenSubType && token.Cmp( b.token ) == 0 );
	}
	void Read( hfFile *inFile ) {
		inFile->ReadString( token );
		inFile->ReadBig( tokenType );
		inFile->ReadBig( tokenSubType );
	}
	void Write( hfFile *inFile ) {
		inFile->WriteString( token );
		inFile->WriteBig( tokenType );
		inFile->WriteBig( tokenSubType );
	}
	hfStr token;
	int8  tokenType;
	short tokenSubType;
};

class hfTokenIndexes {
public:
	hfTokenIndexes() {}
	void Clear() {
		tokenIndexes.Clear();
	}
	int Append( short sdx ) {
		return tokenIndexes.Append( sdx );
	}
	int Num() {
		return tokenIndexes.Num();
	}
	void SetNum( int num ) {
		tokenIndexes.SetNum( num );
	}
	short &	operator[]( const int index ) {
		return tokenIndexes[ index ];
	}
	void SetName( const char *name ) {
		fileName = name;
	}
	const char *GetName() {
		return fileName.c_str();
	}
	void Write( hfFile *outFile ) {
		outFile->WriteString( fileName );
		outFile->WriteBig( ( int )tokenIndexes.Num() );
		outFile->WriteBigArray( tokenIndexes.Ptr(), tokenIndexes.Num() );
	}
	void Read( hfFile *inFile ) {
		inFile->ReadString( fileName );
		int num;
		inFile->ReadBig( num );
		tokenIndexes.SetNum( num );
		inFile->ReadBigArray( tokenIndexes.Ptr(), num );
	}
private:
	hfList< short > tokenIndexes;
	hfStr fileName;
};

class hfTokenParser {
public:
	hfTokenParser() {
		timeStamp = FILE_NOT_FOUND_TIMESTAMP;
		preloaded = false;
		currentToken = 0;
		currentTokenList = 0;
	}
	~hfTokenParser() {
		Clear();
	}
	void Clear() {
		tokens.Clear();
		guiTokenIndexes.Clear();
		currentToken = 0;
		currentTokenList = -1;
		preloaded = false;
	}
	void LoadFromFile( const char *filename );
	void WriteToFile (const char *filename );
	void LoadFromParser( hfParser &parser, const char *guiName );

	bool StartParsing( const char *fileName );
	void DoneParsing() { currentTokenList = -1; }

	bool IsLoaded() { return tokens.Num() > 0; }
	bool ReadToken( hfToken * tok );
	int	ExpectTokenString( const char *string );
	int	ExpectTokenType( int type, int subtype, hfToken *token );
	int ExpectAnyToken( hfToken *token );
	void SetMarker() {}
	void UnreadToken( const hfToken *token );
	void Error( VERIFY_FORMAT_STRING const char *str, ... );
	void Warning( VERIFY_FORMAT_STRING const char *str, ... );
	int ParseInt();
	bool ParseBool();
	float ParseFloat( bool *errorFlag = NULL );
	void UpdateTimeStamp( HF_TIME_T &t ) {
		if ( t > timeStamp ) {
			timeStamp = t;
		}
	}
private:
	hfList< hfBinaryToken > tokens;
	hfList< hfTokenIndexes > guiTokenIndexes;
	int currentToken;
	int currentTokenList;
	HF_TIME_T timeStamp;
	bool preloaded;
};

#endif /* !__TOKENPARSER_H__ */

