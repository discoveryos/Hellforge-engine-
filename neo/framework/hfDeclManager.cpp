#include "../hflib/hfprecompiled.h"
#pragma hdrstop

/*

GUIs and script remain separately parsed

Following a parse, all referenced media (and other decls) will have been touched.

sinTable and cosTable are required for the rotate material keyword to function

A new FindType on a purged decl will cause it to be reloaded, but a stale pointer to a purged
decl will look like a defaulted decl.

Moving a decl from one file to another will not be handled correctly by a reload, the material
will be defaulted.

NULL or empty decl names will always return NULL
	Should probably make a default decl for this

Decls are initially created without a textSource
A parse without textSource set should always just call MakeDefault()
A parse that has an error should internally call MakeDefault()
A purge does nothing to a defaulted decl

Should we have a "purged" media state separate from the "defaulted" media state?

reloading over a decl name that was defaulted

reloading over a decl name that was valid

missing reload over a previously explicit definition

*/

#define USE_COMPRESSED_DECLS
//#define GET_HUFFMAN_FREQUENCIES

class hfDeclType {
public:
	hfStr						typeName;
	declType_t					type;
	hfDecl *					(*allocator)();
};

class hfDeclFolder {
public:
	hfStr						folder;
	hfStr						extension;
	declType_t					defaultType;
};

class hfDeclFile;

class hfDeclLocal : public hfDeclBase {
	friend class hfDeclFile;
	friend class hfDeclManagerLocal;

public:
								hfDeclLocal();
	virtual 					~hfDeclLocal() {};
	virtual const char *		GetName() const;
	virtual declType_t			GetType() const;
	virtual declState_t			GetState() const;
	virtual bool				IsImplicit() const;
	virtual bool				IsValid() const;
	virtual void				Invalidate();
	virtual void				Reload();
	virtual void				EnsureNotPurged();
	virtual int					Index() const;
	virtual int					GetLineNum() const;
	virtual const char *		GetFileName() const;
	virtual size_t				Size() const;
	virtual void				GetText( char *text ) const;
	virtual int					GetTextLength() const;
	virtual void				SetText( const char *text );
	virtual bool				ReplaceSourceFileText();
	virtual bool				SourceFileChanged() const;
	virtual void				MakeDefault();
	virtual bool				EverReferenced() const;

protected:
	virtual bool				SetDefaultText();
	virtual const char *		DefaultDefinition() const;
	virtual bool				Parse( const char *text, const int textLength, bool allowBinaryVersion );
	virtual void				FreeData();
	virtual void				List() const;
	virtual void				Print() const;

protected:
	void						AllocateSelf();

								// Parses the decl definition.
								// After calling parse, a decl will be guaranteed usable.
	void						ParseLocal();

								// Does a MakeDefualt, but flags the decl so that it
								// will Parse() the next time the decl is found.
	void						Purge();

								// Set textSource possible with compression.
	void						SetTextLocal( const char *text, const int length );

private:
	hfDecl *					self;

	hfStr						name;					// name of the decl
	char *						textSource;				// decl text definition
	int							textLength;				// length of textSource
	int							compressedLength;		// compressed length
	hfDeclFile *				sourceFile;				// source file in which the decl was defined
	int							sourceTextOffset;		// offset in source file to decl text
	int							sourceTextLength;		// length of decl text in source file
	int							sourceLine;				// this is where the actual declaration token starts
	int							checksum;				// checksum of the decl text
	declType_t					type;					// decl type
	declState_t					declState;				// decl state
	int							index;					// index in the per-type list

	bool						parsedOutsideLevelLoad;	// these decls will never be purged
	bool						everReferenced;			// set to true if the decl was ever used
	bool						referencedThisLevel;	// set to true when the decl is used for the current level
	bool						redefinedInReload;		// used during file reloading to make sure a decl that has
														// its source removed will be defaulted
	hfDeclLocal *				nextInFile;				// next decl in the decl file
};

class hfDeclFile {
public:
								hfDeclFile();
								hfDeclFile( const char *fileName, declType_t defaultType );

	void						Reload( bool force );
	int							LoadAndParse();

public:
	hfStr						fileName;
	declType_t					defaultType;

	HF_TIME_T						timestamp;
	int							checksum;
	int							fileSize;
	int							numLines;

	hfDeclLocal *				decls;
};

class hfDeclManagerLocal : public hfDeclManager {
	friend class hfDeclLocal;

public:
	virtual void				Init();
	virtual void				Init2();
	virtual void				Shutdown();
	virtual void				Reload( bool force );
	virtual void				BeginLevelLoad();
	virtual void				EndLevelLoad();
	virtual void				RegisterDeclType( const char *typeName, declType_t type, hfDecl *(*allocator)() );
	virtual void				RegisterDeclFolder( const char *folder, const char *extension, declType_t defaultType );
	virtual int					GetChecksum() const;
	virtual int					GetNumDeclTypes() const;
	virtual int					GetNumDecls( declType_t type );
	virtual const char *		GetDeclNameFromType( declType_t type ) const;
	virtual declType_t			GetDeclTypeFromName( const char *typeName ) const;
	virtual const hfDecl *		FindType( declType_t type, const char *name, bool makeDefault = true );
	virtual const hfDecl *		DeclByIndex( declType_t type, int index, bool forceParse = true );

	virtual const hfDecl*		FindDeclWithoutParsing( declType_t type, const char *name, bool makeDefault = true );
	virtual void				ReloadFile( const char* filename, bool force );

	virtual void				ListType( const hfCmdArgs &args, declType_t type );
	virtual void				PrintType( const hfCmdArgs &args, declType_t type );

	virtual hfDecl *			CreateNewDecl( declType_t type, const char *name, const char *fileName );

	//BSM Added for the material editors rename capabilities
	virtual bool				RenameDecl( declType_t type, const char* oldName, const char* newName );

	virtual void				MediaPrint( VERIFY_FORMAT_STRING const char *fmt, ... );
	virtual void				WritePrecacheCommands( hfFile *f );

	virtual const hfMaterial *		FindMaterial( const char *name, bool makeDefault = true );
	virtual const hfDeclSkin *		FindSkin( const char *name, bool makeDefault = true );
	virtual const hfSoundShader *	FindSound( const char *name, bool makeDefault = true );

	virtual const hfMaterial *		MaterialByIndex( int index, bool forceParse = true );
	virtual const hfDeclSkin *		SkinByIndex( int index, bool forceParse = true );
	virtual const hfSoundShader *	SoundByIndex( int index, bool forceParse = true );

	virtual void					Touch( const hfDecl * decl );

public:
	static void					MakeNameCanonical( const char *name, char *result, int maxLength );
	hfDeclLocal *				FindTypeWithoutParsing( declType_t type, const char *name, bool makeDefault = true );

	hfDeclType *				GetDeclType( int type ) const { return declTypes[type]; }
	const hfDeclFile *			GetImplicitDeclFile() const { return &implicitDecls; }

	void						ConvertPDAsToStrings( const hfCmdArgs &args );

private:
	hfSysMutex					mutex;

	hfList<hfDeclType *, TAG_IDLIB_LIST_DECL>		declTypes;
	hfList<hfDeclFolder *, TAG_IDLIB_LIST_DECL>		declFolders;

	hfList<hfDeclFile *, TAG_IDLIB_LIST_DECL>		loadedFiles;
	hfHashIndex					hashTables[DECL_MAX_TYPES];
	hfList<hfDeclLocal *, TAG_IDLIB_LIST_DECL>		linearLists[DECL_MAX_TYPES];
	hfDeclFile					implicitDecls;	// this holds all the decls that were created because explicit
												// text definitions were not found. Decls that became default
												// because of a parse error are not in this list.
	int							checksum;		// checksum of all loaded decl text
	int							indent;			// for MediaPrint
	bool						insideLevelLoad;

	static hfCVar				decl_show;

private:
	static void					ListDecls_f( const hfCmdArgs &args );
	static void					ReloadDecls_f( const hfCmdArgs &args );
	static void					TouchDecl_f( const hfCmdArgs &args );
};

hfCVar hfDeclManagerLocal::decl_show( "decl_show", "0", CVAR_SYSTEM, "set to 1 to print parses, 2 to also print references", 0, 2, hfCmdSystem::ArgCompletion_Integer<0,2> );

hfDeclManagerLocal	declManagerLocal;
hfDeclManager *		declManager = &declManagerLocal;

/*
====================================================================================

 decl text huffman compression

====================================================================================
*/

const int MAX_HUFFMAN_SYMBOLS	= 256;

typedef struct huffmanNode_s {
	int						symbol;
	int						frequency;
	struct huffmanNode_s *	next;
	struct huffmanNode_s *	children[2];
} huffmanNode_t;

typedef struct huffmanCode_s {
	unsigned long			bits[8];
	int						numBits;
} huffmanCode_t;

// compression ratio = 64%
static int huffmanFrequencies[] = {
    0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
    0x00000001, 0x00078fb6, 0x000352a7, 0x00000002, 0x00000001, 0x0002795e, 0x00000001, 0x00000001,
    0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
    0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
    0x00049600, 0x000000dd, 0x00018732, 0x0000005a, 0x00000007, 0x00000092, 0x0000000a, 0x00000919,
    0x00002dcf, 0x00002dda, 0x00004dfc, 0x0000039a, 0x000058be, 0x00002d13, 0x00014d8c, 0x00023c60,
    0x0002ddb0, 0x0000d1fc, 0x000078c4, 0x00003ec7, 0x00003113, 0x00006b59, 0x00002499, 0x0000184a,
    0x0000250b, 0x00004e38, 0x000001ca, 0x00000011, 0x00000020, 0x000023da, 0x00000012, 0x00000091,
    0x0000000b, 0x00000b14, 0x0000035d, 0x0000137e, 0x000020c9, 0x00000e11, 0x000004b4, 0x00000737,
    0x000006b8, 0x00001110, 0x000006b3, 0x000000fe, 0x00000f02, 0x00000d73, 0x000005f6, 0x00000be4,
    0x00000d86, 0x0000014d, 0x00000d89, 0x0000129b, 0x00000db3, 0x0000015a, 0x00000167, 0x00000375,
    0x00000028, 0x00000112, 0x00000018, 0x00000678, 0x0000081a, 0x00000677, 0x00000003, 0x00018112,
    0x00000001, 0x000441ee, 0x000124b0, 0x0001fa3f, 0x00026125, 0x0005a411, 0x0000e50f, 0x00011820,
    0x00010f13, 0x0002e723, 0x00003518, 0x00005738, 0x0002cc26, 0x0002a9b7, 0x0002db81, 0x0003b5fa,
    0x000185d2, 0x00001299, 0x00030773, 0x0003920d, 0x000411cd, 0x00018751, 0x00005fbd, 0x000099b0,
    0x00009242, 0x00007cf2, 0x00002809, 0x00005a1d, 0x00000001, 0x00005a1d, 0x00000001, 0x00000001,

    0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
    0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
    0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
    0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
    0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
    0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
    0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
    0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
    0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
    0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
    0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
    0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
    0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
    0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
    0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
    0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
};

static huffmanCode_t huffmanCodes[MAX_HUFFMAN_SYMBOLS];
static huffmanNode_t *huffmanTree = NULL;
static int totalUncompressedLength = 0;
static int totalCompressedLength = 0;
static int maxHuffmanBits = 0;


/*
================
ClearHuffmanFrequencies
================
*/
void ClearHuffmanFrequencies() {
	int i;

	for( i = 0; i < MAX_HUFFMAN_SYMBOLS; i++ ) {
		huffmanFrequencies[i] = 1;
	}
}

/*
================
InsertHuffmanNode
================
*/
huffmanNode_t *InsertHuffmanNode( huffmanNode_t *firstNode, huffmanNode_t *node ) {
	huffmanNode_t *n, *lastNode;

	lastNode = NULL;
	for ( n = firstNode; n; n = n->next ) {
		if ( node->frequency <= n->frequency ) {
			break;
		}
		lastNode = n;
	}
	if ( lastNode ) {
		node->next = lastNode->next;
		lastNode->next = node;
	} else {
		node->next = firstNode;
		firstNode = node;
	}
	return firstNode;
}

/*
================
BuildHuffmanCode_r
================
*/
void BuildHuffmanCode_r( huffmanNode_t *node, huffmanCode_t code, huffmanCode_t codes[MAX_HUFFMAN_SYMBOLS] ) {
	if ( node->symbol == -1 ) {
		huffmanCode_t newCode = code;
		assert( code.numBits < sizeof( codes[0].bits ) * 8 );
		newCode.numBits++;
		if ( code.numBits > maxHuffmanBits ) {
			maxHuffmanBits = newCode.numBits;
		}
		BuildHuffmanCode_r( node->children[0], newCode, codes );
		newCode.bits[code.numBits >> 5] |= 1 << ( code.numBits & 31 );
		BuildHuffmanCode_r( node->children[1], newCode, codes );
	} else {
		assert( code.numBits <= sizeof( codes[0].bits ) * 8 );
		codes[node->symbol] = code;
	}
}

/*
================
FreeHuffmanTree_r
================
*/
void FreeHuffmanTree_r( huffmanNode_t *node ) {
	if ( node->symbol == -1 ) {
		FreeHuffmanTree_r( node->children[0] );
		FreeHuffmanTree_r( node->children[1] );
	}
	delete node;
}

/*
================
HuffmanHeight_r
================
*/
int HuffmanHeight_r( huffmanNode_t *node ) {
	if ( node == NULL ) {
		return -1;
	}
	int left = HuffmanHeight_r( node->children[0] );
	int right = HuffmanHeight_r( node->children[1] );
	if ( left > right ) {
		return left + 1;
	}
	return right + 1;
}

/*
================
SetupHuffman
================
*/
void SetupHuffman() {
	int i, height;
	huffmanNode_t *firstNode, *node;
	huffmanCode_t code;

	firstNode = NULL;
	for( i = 0; i < MAX_HUFFMAN_SYMBOLS; i++ ) {
		node = new (TAG_DECL) huffmanNode_t;
		node->symbol = i;
		node->frequency = huffmanFrequencies[i];
		node->next = NULL;
		node->children[0] = NULL;
		node->children[1] = NULL;
		firstNode = InsertHuffmanNode( firstNode, node );
	}

	for( i = 1; i < MAX_HUFFMAN_SYMBOLS; i++ ) {
		node = new (TAG_DECL) huffmanNode_t;
		node->symbol = -1;
		node->frequency = firstNode->frequency + firstNode->next->frequency;
		node->next = NULL;
		node->children[0] = firstNode;
		node->children[1] = firstNode->next;
		firstNode = InsertHuffmanNode( firstNode->next->next, node );
	}

	maxHuffmanBits = 0;
	memset( &code, 0, sizeof( code ) );
	BuildHuffmanCode_r( firstNode, code, huffmanCodes );

	huffmanTree = firstNode;

	height = HuffmanHeight_r( firstNode );
	assert( maxHuffmanBits == height );
}

/*
================
ShutdownHuffman
================
*/
void ShutdownHuffman() {
	if ( huffmanTree ) {
		FreeHuffmanTree_r( huffmanTree );
	}
}

/*
================
HuffmanCompressText
================
*/
int HuffmanCompressText( const char *text, int textLength, byte *compressed, int maxCompressedSize ) {
	int i, j;
	hfBitMsg msg;

	totalUncompressedLength += textLength;

	msg.InitWrite( compressed, maxCompressedSize );
	msg.BeginWriting();
	for ( i = 0; i < textLength; i++ ) {
		const huffmanCode_t &code = huffmanCodes[(unsigned char)text[i]];
		for ( j = 0; j < ( code.numBits >> 5 ); j++ ) {
			msg.WriteBits( code.bits[j], 32 );
		}
		if ( code.numBits & 31 ) {
			msg.WriteBits( code.bits[j], code.numBits & 31 );
		}
	}

	totalCompressedLength += msg.GetSize();

	return msg.GetSize();
}

/*
================
HuffmanDecompressText
================
*/
int HuffmanDecompressText( char *text, int textLength, const byte *compressed, int compressedSize ) {
	int i, bit;
	hfBitMsg msg;
	huffmanNode_t *node;

	msg.InitRead( compressed, compressedSize );
	msg.SetSize( compressedSize );
	msg.BeginReading();
	for ( i = 0; i < textLength; i++ ) {
		node = huffmanTree;
		do {
			bit = msg.ReadBits( 1 );
			node = node->children[bit];
		} while( node->symbol == -1 );
		text[i] = node->symbol;
	}
	text[i] = '\0';
	return msg.GetReadCount();
}

/*
================
ListHuffmanFrequencies_f
================
*/
void ListHuffmanFrequencies_f( const hfCmdArgs &args ) {
	int		i;
	float compression;
	compression = !totalUncompressedLength ? 100 : 100 * totalCompressedLength / totalUncompressedLength;
	common->Printf( "// compression ratio = %d%%\n", (int)compression );
	common->Printf( "static int huffmanFrequencies[] = {\n" );
	for( i = 0; i < MAX_HUFFMAN_SYMBOLS; i += 8 ) {
		common->Printf( "\t0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x,\n",
							huffmanFrequencies[i+0], huffmanFrequencies[i+1],
							huffmanFrequencies[i+2], huffmanFrequencies[i+3],
							huffmanFrequencies[i+4], huffmanFrequencies[i+5],
							huffmanFrequencies[i+6], huffmanFrequencies[i+7]);
	}
	common->Printf( "}\n" );
}

void ConvertPDAsToStrings_f( const hfCmdArgs &args ) {
	declManagerLocal.ConvertPDAsToStrings( args );
}

/*
====================================================================================

 hfDeclFile

====================================================================================
*/

/*
================
hfDeclFile::hfDeclFile
================
*/
hfDeclFile::hfDeclFile( const char *fileName, declType_t defaultType ) {
	this->fileName = fileName;
	this->defaultType = defaultType;
	this->timestamp = 0;
	this->checksum = 0;
	this->fileSize = 0;
	this->numLines = 0;
	this->decls = NULL;
}

/*
================
hfDeclFile::hfDeclFile
================
*/
hfDeclFile::hfDeclFile() {
	this->fileName = "<implicit file>";
	this->defaultType = DECL_MAX_TYPES;
	this->timestamp = 0;
	this->checksum = 0;
	this->fileSize = 0;
	this->numLines = 0;
	this->decls = NULL;
}

/*
================
hfDeclFile::Reload

ForceReload will cause it to reload even if the timestamp hasn't changed
================
*/
void hfDeclFile::Reload( bool force ) {
	// check for an unchanged timestamp
	if ( !force && timestamp != 0 ) {
		HF_TIME_T	testTimeStamp;
		fileSystem->ReadFile( fileName, NULL, &testTimeStamp );

		if ( testTimeStamp == timestamp ) {
			return;
		}
	}

	// parse the text
	LoadAndParse();
}

/*
================
hfDeclFile::LoadAndParse

This is used during both the initial load, and any reloads
================
*/
int c_savedMemory = 0;

int hfDeclFile::LoadAndParse() {
	int			i, numTypes;
	hfLexer		src;
	hfToken		token;
	int			startMarker;
	char *		buffer;
	int			length, size;
	int			sourceLine;
	hfStr		name;
	hfDeclLocal *newDecl;
	bool		reparse;

	// load the text
	common->DPrintf( "...loading '%s'\n", fileName.c_str() );
	length = fileSystem->ReadFile( fileName, (void **)&buffer, &timestamp );
	if ( length == -1 ) {
		common->FatalError( "couldn't load %s", fileName.c_str() );
		return 0;
	}

	if ( !src.LoadMemory( buffer, length, fileName ) ) {
		common->Error( "Couldn't parse %s", fileName.c_str() );
		Mem_Free( buffer );
		return 0;
	}

	// mark all the defs that were from the last reload of this file
	for ( hfDeclLocal *decl = decls; decl; decl = decl->nextInFile ) {
		decl->redefinedInReload = false;
	}

	src.SetFlags( DECL_LEXER_FLAGS );

	checksum = MD5_BlockChecksum( buffer, length );

	fileSize = length;

	// scan through, identifying each individual declaration
	while( 1 ) {

		startMarker = src.GetFileOffset();
		sourceLine = src.GetLineNum();

		// parse the decl type name
		if ( !src.ReadToken( &token ) ) {
			break;
		}

		declType_t identifiedType = DECL_MAX_TYPES;

		// get the decl type from the type name
		numTypes = declManagerLocal.GetNumDeclTypes();
		for ( i = 0; i < numTypes; i++ ) {
			hfDeclType *typeInfo = declManagerLocal.GetDeclType( i );
			if ( typeInfo != NULL && typeInfo->typeName.Icmp( token ) == 0 ) {
				identifiedType = (declType_t) typeInfo->type;
				break;
			}
		}

		if ( i >= numTypes ) {

			if ( token.Icmp( "{" ) == 0 ) {

				// if we ever see an open brace, we somehow missed the [type] <name> prefix
				src.Warning( "Missing decl name" );
				src.SkipBracedSection( false );
				continue;

			} else {

				if ( defaultType == DECL_MAX_TYPES ) {
					src.Warning( "No type" );
					continue;
				}
				src.UnreadToken( &token );
				// use the default type
				identifiedType = defaultType;
			}
		}

		// now parse the name
		if ( !src.ReadToken( &token ) ) {
			src.Warning( "Type without definition at end of file" );
			break;
		}

		if ( !token.Icmp( "{" ) ) {
			// if we ever see an open brace, we somehow missed the [type] <name> prefix
			src.Warning( "Missing decl name" );
			src.SkipBracedSection( false );
			continue;
		}

		// FIXME: export decls are only used by the model exporter, they are skipped here for now
		if ( identifiedType == DECL_MODELEXPORT ) {
			src.SkipBracedSection();
			continue;
		}

		name = token;

		// make sure there's a '{'
		if ( !src.ReadToken( &token ) ) {
			src.Warning( "Type without definition at end of file" );
			break;
		}
		if ( token != "{" ) {
			src.Warning( "Expecting '{' but found '%s'", token.c_str() );
			continue;
		}
		src.UnreadToken( &token );

		// now take everything until a matched closing brace
		src.SkipBracedSection();
		size = src.GetFileOffset() - startMarker;

		// look it up, possibly getting a newly created default decl
		reparse = false;
		newDecl = declManagerLocal.FindTypeWithoutParsing( identifiedType, name, false );
		if ( newDecl ) {
			// update the existing copy
			if ( newDecl->sourceFile != this || newDecl->redefinedInReload ) {
				src.Warning( "%s '%s' previously defined at %s:%i", declManagerLocal.GetDeclNameFromType( identifiedType ),
								name.c_str(), newDecl->sourceFile->fileName.c_str(), newDecl->sourceLine );
				continue;
			}
			if ( newDecl->declState != DS_UNPARSED ) {
				reparse = true;
			}
		} else {
			// allow it to be created as a default, then add it to the per-file list
			newDecl = declManagerLocal.FindTypeWithoutParsing( identifiedType, name, true );
			newDecl->nextInFile = this->decls;
			this->decls = newDecl;
		}

		newDecl->redefinedInReload = true;

		if ( newDecl->textSource ) {
			Mem_Free( newDecl->textSource );
			newDecl->textSource = NULL;
		}

		newDecl->SetTextLocal( buffer + startMarker, size );
		newDecl->sourceFile = this;
		newDecl->sourceTextOffset = startMarker;
		newDecl->sourceTextLength = size;
		newDecl->sourceLine = sourceLine;
		newDecl->declState = DS_UNPARSED;

		// if it is currently in use, reparse it immedaitely
		if ( reparse ) {
			newDecl->ParseLocal();
		}
	}

	numLines = src.GetLineNum();

	Mem_Free( buffer );

	// any defs that weren't redefinedInReload should now be defaulted
	for ( hfDeclLocal *decl = decls ; decl ; decl = decl->nextInFile ) {
		if ( decl->redefinedInReload == false ) {
			decl->MakeDefault();
			decl->sourceTextOffset = decl->sourceFile->fileSize;
			decl->sourceTextLength = 0;
			decl->sourceLine = decl->sourceFile->numLines;
		}
	}

	return checksum;
}

/*
====================================================================================

 hfDeclManagerLocal

====================================================================================
*/

const char *listDeclStrings[] = { "current", "all", "ever", NULL };

/*
===================
hfDeclManagerLocal::Init
===================
*/
void hfDeclManagerLocal::Init() {

	common->Printf( "----- Initializing Decls -----\n" );

	checksum = 0;

#ifdef USE_COMPRESSED_DECLS
	SetupHuffman();
#endif

#ifdef GET_HUFFMAN_FREQUENCIES
	ClearHuffmanFrequencies();
#endif

	// decls used throughout the engine
	RegisterDeclType( "table",				DECL_TABLE,			hfDeclAllocator<hfDeclTable> );
	RegisterDeclType( "material",			DECL_MATERIAL,		hfDeclAllocator<hfMaterial> );
	RegisterDeclType( "skin",				DECL_SKIN,			hfDeclAllocator<hfDeclSkin> );
	RegisterDeclType( "sound",				DECL_SOUND,			hfDeclAllocator<hfSoundShader> );

	RegisterDeclType( "entityDef",			DECL_ENTITYDEF,		hfDeclAllocator<hfDeclEntityDef> );
	RegisterDeclType( "mapDef",				DECL_MAPDEF,		hfDeclAllocator<hfDeclEntityDef> );
	RegisterDeclType( "fx",					DECL_FX,			hfDeclAllocator<hfDeclFX> );
	RegisterDeclType( "particle",			DECL_PARTICLE,		hfDeclAllocator<hfDeclParticle> );
	RegisterDeclType( "articulatedFigure",	DECL_AF,			hfDeclAllocator<hfDeclAF> );
	RegisterDeclType( "pda",				DECL_PDA,			hfDeclAllocator<hfDeclPDA> );
	RegisterDeclType( "email",				DECL_EMAIL,			hfDeclAllocator<hfDeclEmail> );
	RegisterDeclType( "video",				DECL_VIDEO,			hfDeclAllocator<hfDeclVideo> );
	RegisterDeclType( "audio",				DECL_AUDIO,			hfDeclAllocator<hfDeclAudio> );

	RegisterDeclFolder( "materials",		".mtr",				DECL_MATERIAL );

	// add console commands
	cmdSystem->AddCommand( "listDecls", ListDecls_f, CMD_FL_SYSTEM, "lists all decls" );

	cmdSystem->AddCommand( "reloadDecls", ReloadDecls_f, CMD_FL_SYSTEM, "reloads decls" );
	cmdSystem->AddCommand( "touch", TouchDecl_f, CMD_FL_SYSTEM, "touches a decl" );

	cmdSystem->AddCommand( "listTables", hfListDecls_f<DECL_TABLE>, CMD_FL_SYSTEM, "lists tables", hfCmdSystem::ArgCompletion_String<listDeclStrings> );
	cmdSystem->AddCommand( "listMaterials", hfListDecls_f<DECL_MATERIAL>, CMD_FL_SYSTEM, "lists materials", hfCmdSystem::ArgCompletion_String<listDeclStrings> );
	cmdSystem->AddCommand( "listSkins", hfListDecls_f<DECL_SKIN>, CMD_FL_SYSTEM, "lists skins", hfCmdSystem::ArgCompletion_String<listDeclStrings> );
	cmdSystem->AddCommand( "listSoundShaders", hfListDecls_f<DECL_SOUND>, CMD_FL_SYSTEM, "lists sound shaders", hfCmdSystem::ArgCompletion_String<listDeclStrings> );

	cmdSystem->AddCommand( "listEntityDefs", hfListDecls_f<DECL_ENTITYDEF>, CMD_FL_SYSTEM, "lists entity defs", hfCmdSystem::ArgCompletion_String<listDeclStrings> );
	cmdSystem->AddCommand( "listFX", hfListDecls_f<DECL_FX>, CMD_FL_SYSTEM, "lists FX systems", hfCmdSystem::ArgCompletion_String<listDeclStrings> );
	cmdSystem->AddCommand( "listParticles", hfListDecls_f<DECL_PARTICLE>, CMD_FL_SYSTEM, "lists particle systems", hfCmdSystem::ArgCompletion_String<listDeclStrings> );
	cmdSystem->AddCommand( "listAF", hfListDecls_f<DECL_AF>, CMD_FL_SYSTEM, "lists articulated figures", hfCmdSystem::ArgCompletion_String<listDeclStrings>);

	cmdSystem->AddCommand( "listPDAs", hfListDecls_f<DECL_PDA>, CMD_FL_SYSTEM, "lists PDAs", hfCmdSystem::ArgCompletion_String<listDeclStrings> );
	cmdSystem->AddCommand( "listEmails", hfListDecls_f<DECL_EMAIL>, CMD_FL_SYSTEM, "lists Emails", hfCmdSystem::ArgCompletion_String<listDeclStrings> );
	cmdSystem->AddCommand( "listVideos", hfListDecls_f<DECL_VIDEO>, CMD_FL_SYSTEM, "lists Videos", hfCmdSystem::ArgCompletion_String<listDeclStrings> );
	cmdSystem->AddCommand( "listAudios", hfListDecls_f<DECL_AUDIO>, CMD_FL_SYSTEM, "lists Audios", hfCmdSystem::ArgCompletion_String<listDeclStrings> );

	cmdSystem->AddCommand( "printTable", hfPrintDecls_f<DECL_TABLE>, CMD_FL_SYSTEM, "prints a table", hfCmdSystem::ArgCompletion_Decl<DECL_TABLE> );
	cmdSystem->AddCommand( "printMaterial", hfPrintDecls_f<DECL_MATERIAL>, CMD_FL_SYSTEM, "prints a material", hfCmdSystem::ArgCompletion_Decl<DECL_MATERIAL> );
	cmdSystem->AddCommand( "printSkin", hfPrintDecls_f<DECL_SKIN>, CMD_FL_SYSTEM, "prints a skin", hfCmdSystem::ArgCompletion_Decl<DECL_SKIN> );
	cmdSystem->AddCommand( "printSoundShader", hfPrintDecls_f<DECL_SOUND>, CMD_FL_SYSTEM, "prints a sound shader", hfCmdSystem::ArgCompletion_Decl<DECL_SOUND> );

	cmdSystem->AddCommand( "printEntityDef", hfPrintDecls_f<DECL_ENTITYDEF>, CMD_FL_SYSTEM, "prints an entity def", hfCmdSystem::ArgCompletion_Decl<DECL_ENTITYDEF> );
	cmdSystem->AddCommand( "printFX", hfPrintDecls_f<DECL_FX>, CMD_FL_SYSTEM, "prints an FX system", hfCmdSystem::ArgCompletion_Decl<DECL_FX> );
	cmdSystem->AddCommand( "printParticle", hfPrintDecls_f<DECL_PARTICLE>, CMD_FL_SYSTEM, "prints a particle system", hfCmdSystem::ArgCompletion_Decl<DECL_PARTICLE> );
	cmdSystem->AddCommand( "printAF", hfPrintDecls_f<DECL_AF>, CMD_FL_SYSTEM, "prints an articulated figure", hfCmdSystem::ArgCompletion_Decl<DECL_AF> );

	cmdSystem->AddCommand( "printPDA", hfPrintDecls_f<DECL_PDA>, CMD_FL_SYSTEM, "prints an PDA", hfCmdSystem::ArgCompletion_Decl<DECL_PDA> );
	cmdSystem->AddCommand( "printEmail", hfPrintDecls_f<DECL_EMAIL>, CMD_FL_SYSTEM, "prints an Email", hfCmdSystem::ArgCompletion_Decl<DECL_EMAIL> );
	cmdSystem->AddCommand( "printVideo", hfPrintDecls_f<DECL_VIDEO>, CMD_FL_SYSTEM, "prints an Audio", hfCmdSystem::ArgCompletion_Decl<DECL_VIDEO> );
	cmdSystem->AddCommand( "printAudio", hfPrintDecls_f<DECL_AUDIO>, CMD_FL_SYSTEM, "prints a Video", hfCmdSystem::ArgCompletion_Decl<DECL_AUDIO> );

	cmdSystem->AddCommand( "listHuffmanFrequencies", ListHuffmanFrequencies_f, CMD_FL_SYSTEM, "lists decl text character frequencies" );

	cmdSystem->AddCommand( "convertPDAsToStrings", ConvertPDAsToStrings_f, CMD_FL_SYSTEM, "Converts *.pda files to text which can be plugged into *.lang files." );

	common->Printf( "------------------------------\n" );
}

void hfDeclManagerLocal::Init2() {
	RegisterDeclFolder( "skins",			".skin",			DECL_SKIN );
	RegisterDeclFolder( "sound",			".sndshd",			DECL_SOUND );
}

/*
===================
hfDeclManagerLocal::Shutdown
===================
*/
void hfDeclManagerLocal::Shutdown() {
	int			i, j;
	hfDeclLocal *decl;

	// free decls
	for ( i = 0; i < DECL_MAX_TYPES; i++ ) {
		for ( j = 0; j < linearLists[i].Num(); j++ ) {
			decl = linearLists[i][j];
			if ( decl->self != NULL ) {
				decl->self->FreeData();
				delete decl->self;
			}
			if ( decl->textSource ) {
				Mem_Free( decl->textSource );
				decl->textSource = NULL;
			}
			delete decl;
		}
		linearLists[i].Clear();
		hashTables[i].Free();
	}

	// free decl files
	loadedFiles.DeleteContents( true );

	// free the decl types and folders
	declTypes.DeleteContents( true );
	declFolders.DeleteContents( true );

#ifdef USE_COMPRESSED_DECLS
	ShutdownHuffman();
#endif
}

/*
===================
hfDeclManagerLocal::Reload
===================
*/
void hfDeclManagerLocal::Reload( bool force ) {
	for ( int i = 0; i < loadedFiles.Num(); i++ ) {
		loadedFiles[i]->Reload( force );
	}
}

/*
===================
hfDeclManagerLocal::BeginLevelLoad
===================
*/
void hfDeclManagerLocal::BeginLevelLoad() {
	insideLevelLoad = true;

	// clear all the referencedThisLevel flags and purge all the data
	// so the next reference will cause a reparse
	for ( int i = 0; i < DECL_MAX_TYPES; i++ ) {
		int	num = linearLists[i].Num();
		for ( int j = 0 ; j < num ; j++ ) {
			hfDeclLocal *decl = linearLists[i][j];
			decl->Purge();
		}
	}
}

/*
===================
hfDeclManagerLocal::EndLevelLoad
===================
*/
void hfDeclManagerLocal::EndLevelLoad() {
	insideLevelLoad = false;

	// we don't need to do anything here, but the image manager, model manager,
	// and sound sample manager will need to free media that was not referenced
}

/*
===================
hfDeclManagerLocal::RegisterDeclType
===================
*/
void hfDeclManagerLocal::RegisterDeclType( const char *typeName, declType_t type, hfDecl *(*allocator)() ) {
	hfDeclType *declType;

	if ( type < declTypes.Num() && declTypes[(int)type] ) {
		common->Warning( "hfDeclManager::RegisterDeclType: type '%s' already exists", typeName );
		return;
	}

	declType = new (TAG_DECL) hfDeclType;
	declType->typeName = typeName;
	declType->type = type;
	declType->allocator = allocator;

	if ( (int)type + 1 > declTypes.Num() ) {
		declTypes.AssureSize( (int)type + 1, NULL );
	}
	declTypes[type] = declType;
}

/*
===================
hfDeclManagerLocal::RegisterDeclFolder
===================
*/
void hfDeclManagerLocal::RegisterDeclFolder( const char *folder, const char *extension, declType_t defaultType ) {
	int i, j;
	hfStr fileName;
	hfDeclFolder *declFolder;
	hfFileList *fileList;
	hfDeclFile *df;

	// check whether this folder / extension combination already exists
	for ( i = 0; i < declFolders.Num(); i++ ) {
		if ( declFolders[i]->folder.Icmp( folder ) == 0 && declFolders[i]->extension.Icmp( extension ) == 0 ) {
			break;
		}
	}
	if ( i < declFolders.Num() ) {
		declFolder = declFolders[i];
	} else {
		declFolder = new (TAG_DECL) hfDeclFolder;
		declFolder->folder = folder;
		declFolder->extension = extension;
		declFolder->defaultType = defaultType;
		declFolders.Append( declFolder );
	}

	// scan for decl files
	fileList = fileSystem->ListFiles( declFolder->folder, declFolder->extension, true );

	// load and parse decl files
	for ( i = 0; i < fileList->GetNumFiles(); i++ ) {
		fileName = declFolder->folder + "/" + fileList->GetFile( i );

		// check whether this file has already been loaded
		for ( j = 0; j < loadedFiles.Num(); j++ ) {
			if ( fileName.Icmp( loadedFiles[j]->fileName ) == 0 ) {
				break;
			}
		}
		if ( j < loadedFiles.Num() ) {
			df = loadedFiles[j];
		} else {
			df = new (TAG_DECL) hfDeclFile( fileName, defaultType );
			loadedFiles.Append( df );
		}
		df->LoadAndParse();
	}

	fileSystem->FreeFileList( fileList );
}

/*
===================
hfDeclManagerLocal::GetChecksum
===================
*/
int hfDeclManagerLocal::GetChecksum() const {
	int i, j, total, num;
	int *checksumData;

	// get the total number of decls
	total = 0;
	for ( i = 0; i < DECL_MAX_TYPES; i++ ) {
		total += linearLists[i].Num();
	}

	checksumData = (int *) _alloca16( total * 2 * sizeof( int ) );

	total = 0;
	for ( i = 0; i < DECL_MAX_TYPES; i++ ) {
		declType_t type = (declType_t) i;

		// FIXME: not particularly pretty but PDAs and associated decls are localized and should not be checksummed
		if ( type == DECL_PDA || type == DECL_VIDEO || type == DECL_AUDIO || type == DECL_EMAIL ) {
			continue;
		}
 
		num = linearLists[i].Num();
		for ( j = 0; j < num; j++ ) {
			hfDeclLocal *decl = linearLists[i][j];

			if ( decl->sourceFile == &implicitDecls ) {
				continue;
			}

			checksumData[total*2+0] = total;
			checksumData[total*2+1] = decl->checksum;
			total++;
		}
	}

	LittleRevBytes( checksumData, sizeof(int), total * 2 );
	return MD5_BlockChecksum( checksumData, total * 2 * sizeof( int ) );
}

/*
===================
hfDeclManagerLocal::GetNumDeclTypes
===================
*/
int hfDeclManagerLocal::GetNumDeclTypes() const {
	return declTypes.Num();
}

/*
===================
hfDeclManagerLocal::GetDeclNameFromType
===================
*/
const char * hfDeclManagerLocal::GetDeclNameFromType( declType_t type ) const {
	int typeIndex = (int)type;

	if ( typeIndex < 0 || typeIndex >= declTypes.Num() || declTypes[typeIndex] == NULL ) {
		common->FatalError( "hfDeclManager::GetDeclNameFromType: bad type: %i", typeIndex );
	}
	return declTypes[typeIndex]->typeName;
}

/*
===================
hfDeclManagerLocal::GetDeclTypeFromName
===================
*/
declType_t hfDeclManagerLocal::GetDeclTypeFromName( const char *typeName ) const {
	int i;

	for ( i = 0; i < declTypes.Num(); i++ ) {
		if ( declTypes[i] && declTypes[i]->typeName.Icmp( typeName ) == 0 ) {
			return (declType_t)declTypes[i]->type;
		}
	}
	return DECL_MAX_TYPES;
}

/*
=================
hfDeclManagerLocal::FindType

External users will always cause the decl to be parsed before returning
=================
*/
const hfDecl *hfDeclManagerLocal::FindType( declType_t type, const char *name, bool makeDefault ) {
	hfDeclLocal *decl;

	hfScopedCriticalSection cs( mutex );

	if ( !name || !name[0] ) {
		name = "_emptyName";
		//common->Warning( "hfDeclManager::FindType: empty %s name", GetDeclType( (int)type )->typeName.c_str() );
	}

	decl = FindTypeWithoutParsing( type, name, makeDefault );
	if ( !decl ) {
		return NULL;
	}	

	decl->AllocateSelf();

	// if it hasn't been parsed yet, parse it now
	if ( decl->declState == DS_UNPARSED ) {
		if ( !hfLib::IsMainThread() ) {
			// we can't load images from a background thread on OpenGL,
			// the renderer on the main thread should parse it if needed
			hfLib::Error( "Attempted to load %s decl '%s' from game thread!", GetDeclNameFromType( type ), name );
		}
		decl->ParseLocal();
	}

	// mark it as referenced
	decl->referencedThisLevel = true;
	decl->everReferenced = true;
	if ( insideLevelLoad ) {
		decl->parsedOutsideLevelLoad = false;
	}

	return decl->self;
}

/*
===============
hfDeclManagerLocal::FindDeclWithoutParsing
===============
*/
const hfDecl* hfDeclManagerLocal::FindDeclWithoutParsing( declType_t type, const char *name, bool makeDefault) {
	hfDeclLocal* decl;
	decl = FindTypeWithoutParsing(type, name, makeDefault);
	if(decl) {
		return decl->self;
	}
	return NULL;
}

/*
===============
hfDeclManagerLocal::ReloadFile
===============
*/
void hfDeclManagerLocal::ReloadFile( const char* filename, bool force ) {
	for ( int i = 0; i < loadedFiles.Num(); i++ ) {
		if(!loadedFiles[i]->fileName.Icmp(filename)) {
			checksum ^= loadedFiles[i]->checksum;
			loadedFiles[i]->Reload( force );
			checksum ^= loadedFiles[i]->checksum;
		}
	}
}

/*
===================
hfDeclManagerLocal::GetNumDecls
===================
*/
int hfDeclManagerLocal::GetNumDecls( declType_t type ) {
	int typeIndex = (int)type;

	if ( typeIndex < 0 || typeIndex >= declTypes.Num() || declTypes[typeIndex] == NULL ) {
		common->FatalError( "hfDeclManager::GetNumDecls: bad type: %i", typeIndex );
		return 0;
	}
	return linearLists[ typeIndex ].Num();
}

/*
===================
hfDeclManagerLocal::DeclByIndex
===================
*/
const hfDecl *hfDeclManagerLocal::DeclByIndex( declType_t type, int index, bool forceParse ) {
	int typeIndex = (int)type;

	if ( typeIndex < 0 || typeIndex >= declTypes.Num() || declTypes[typeIndex] == NULL ) {
		common->FatalError( "hfDeclManager::DeclByIndex: bad type: %i", typeIndex );
		return NULL;
	}
	if ( index < 0 || index >= linearLists[ typeIndex ].Num() ) {
		common->Error( "hfDeclManager::DeclByIndex: out of range" );
	}
	hfDeclLocal *decl = linearLists[ typeIndex ][ index ];

	decl->AllocateSelf();

	if ( forceParse && decl->declState == DS_UNPARSED ) {
		decl->ParseLocal();
	}

	return decl->self;
}

/*
===================
hfDeclManagerLocal::ListType

list*
Lists decls currently referenced

list* ever
Lists decls that have been referenced at least once since app launched

list* all
Lists every decl declared, even if it hasn't been referenced or parsed

FIXME: alphabetized, wildcards?
===================
*/
void hfDeclManagerLocal::ListType( const hfCmdArgs &args, declType_t type ) {
	bool all, ever;

	if ( !hfStr::Icmp( args.Argv( 1 ), "all" ) ) {
		all = true;
	} else {
		all = false;
	}
	if ( !hfStr::Icmp( args.Argv( 1 ), "ever" ) ) {
		ever = true;
	} else {
		ever = false;
	}

	common->Printf( "--------------------\n" );
	int printed = 0;
	int	count = linearLists[ (int)type ].Num();
	for ( int i = 0 ; i < count ; i++ ) {
		hfDeclLocal *decl = linearLists[ (int)type ][ i ];

		if ( !all && decl->declState == DS_UNPARSED ) {
			continue;
		}

		if ( !all && !ever && !decl->referencedThisLevel ) {
			continue;
		}

		if ( decl->referencedThisLevel ) {
			common->Printf( "*" );
		} else if ( decl->everReferenced ) {
			common->Printf( "." );
		} else {
			common->Printf( " " );
		}
		if ( decl->declState == DS_DEFAULTED ) {
			common->Printf( "D" );
		} else {
			common->Printf( " " );
		}
		common->Printf( "%4i: ", decl->index );
		printed++;
		if ( decl->declState == DS_UNPARSED ) {
			// doesn't have any type specific data yet
			common->Printf( "%s\n", decl->GetName() );
		} else {
			decl->self->List();
		}
	}

	common->Printf( "--------------------\n" );
	common->Printf( "%i of %i %s\n", printed, count, declTypes[type]->typeName.c_str() );
}

/*
===================
hfDeclManagerLocal::PrintType
===================
*/
void hfDeclManagerLocal::PrintType( const hfCmdArgs &args, declType_t type ) {
	// individual decl types may use additional command parameters
	if ( args.Argc() < 2 ) {
		common->Printf( "USAGE: Print<decl type> <decl name> [type specific parms]\n" );
		return;
	}

	// look it up, skipping the public path so it won't parse or reference
	hfDeclLocal *decl = FindTypeWithoutParsing( type, args.Argv( 1 ), false );
	if ( !decl ) {
		common->Printf( "%s '%s' not found.\n", declTypes[ type ]->typeName.c_str(), args.Argv( 1 ) );
		return;
	}

	// print information common to all decls
	common->Printf( "%s %s:\n", declTypes[ type ]->typeName.c_str(), decl->name.c_str() );
	common->Printf( "source: %s:%i\n", decl->sourceFile->fileName.c_str(), decl->sourceLine );
	common->Printf( "----------\n" );
	if ( decl->textSource != NULL ) {
		char *declText = (char *)_alloca( decl->textLength + 1 );
		decl->GetText( declText );
		common->Printf( "%s\n", declText );
	} else {
		common->Printf( "NO SOURCE\n" );
	}
	common->Printf( "----------\n" );
	switch( decl->declState ) {
		case DS_UNPARSED:
			common->Printf( "Unparsed.\n" );
			break;
		case DS_DEFAULTED:
			common->Printf( "<DEFAULTED>\n" );
			break;
		case DS_PARSED:
			common->Printf( "Parsed.\n" );
			break;
	}

	if ( decl->referencedThisLevel ) {
		common->Printf( "Currently referenced this level.\n" );
	} else if ( decl->everReferenced ) {
		common->Printf( "Referenced in a previous level.\n" );
	} else {
		common->Printf( "Never referenced.\n" );
	}

	// allow type-specific data to be printed
	if ( decl->self != NULL ) {
		decl->self->Print();
	}
}

/*
===================
hfDeclManagerLocal::CreateNewDecl
===================
*/
hfDecl *hfDeclManagerLocal::CreateNewDecl( declType_t type, const char *name, const char *_fileName ) {
	int typeIndex = (int)type;
	int i, hash;

	if ( typeIndex < 0 || typeIndex >= declTypes.Num() || declTypes[typeIndex] == NULL || typeIndex >= DECL_MAX_TYPES ) {
		common->FatalError( "hfDeclManager::CreateNewDecl: bad type: %i", typeIndex );
		return NULL;
	}

	char  canonicalName[MAX_STRING_CHARS];

	MakeNameCanonical( name, canonicalName, sizeof( canonicalName ) );

	hfStr fileName = _fileName;
	fileName.BackSlashesToSlashes();

	// see if it already exists
	hash = hashTables[typeIndex].GenerateKey( canonicalName, false );
	for ( i = hashTables[typeIndex].First( hash ); i >= 0; i = hashTables[typeIndex].Next( i ) ) {
		if ( linearLists[typeIndex][i]->name.Icmp( canonicalName ) == 0 ) {
			linearLists[typeIndex][i]->AllocateSelf();
			return linearLists[typeIndex][i]->self;
		}
	}

	hfDeclFile *sourceFile;

	// find existing source file or create a new one
	for ( i = 0; i < loadedFiles.Num(); i++ ) {
		if ( loadedFiles[i]->fileName.Icmp( fileName ) == 0 ) {
			break;
		}
	}
	if ( i < loadedFiles.Num() ) {
		sourceFile = loadedFiles[i];
	} else {
		sourceFile = new (TAG_DECL) hfDeclFile( fileName, type );
		loadedFiles.Append( sourceFile );
	}

	hfDeclLocal *decl = new (TAG_DECL) hfDeclLocal;
	decl->name = canonicalName;
	decl->type = type;
	decl->declState = DS_UNPARSED;
	decl->AllocateSelf();
	hfStr header = declTypes[typeIndex]->typeName;
	hfStr defaultText = decl->self->DefaultDefinition();


	int size = header.Length() + 1 + hfStr::Length( canonicalName ) + 1 + defaultText.Length();
	char *declText = ( char * ) _alloca( size + 1 );

	memcpy( declText, header, header.Length() );
	declText[header.Length()] = ' ';
	memcpy( declText + header.Length() + 1, canonicalName, hfStr::Length( canonicalName ) );
	declText[header.Length() + 1 + hfStr::Length( canonicalName )] = ' ';
	memcpy( declText + header.Length() + 1 + hfStr::Length( canonicalName ) + 1, defaultText, defaultText.Length() + 1 );

	decl->SetTextLocal( declText, size );
	decl->sourceFile = sourceFile;
	decl->sourceTextOffset = sourceFile->fileSize;
	decl->sourceTextLength = 0;
	decl->sourceLine = sourceFile->numLines;

	decl->ParseLocal();

	// add this decl to the source file list
	decl->nextInFile = sourceFile->decls;
	sourceFile->decls = decl;

	// add it to the hash table and linear list
	decl->index = linearLists[typeIndex].Num();
	hashTables[typeIndex].Add( hash, linearLists[typeIndex].Append( decl ) );

	return decl->self;
}

/*
===============
hfDeclManagerLocal::RenameDecl
===============
*/
bool hfDeclManagerLocal::RenameDecl( declType_t type, const char* oldName, const char* newName ) {

	char canonicalOldName[MAX_STRING_CHARS];
	MakeNameCanonical( oldName, canonicalOldName, sizeof( canonicalOldName ));

	char canonicalNewName[MAX_STRING_CHARS];
	MakeNameCanonical( newName, canonicalNewName, sizeof( canonicalNewName ) );

	hfDeclLocal	*decl = NULL;

	// make sure it already exists
	int typeIndex = (int)type;
	int i, hash;
	hash = hashTables[typeIndex].GenerateKey( canonicalOldName, false );
	for ( i = hashTables[typeIndex].First( hash ); i >= 0; i = hashTables[typeIndex].Next( i ) ) {
		if ( linearLists[typeIndex][i]->name.Icmp( canonicalOldName ) == 0 ) {
			decl = linearLists[typeIndex][i];
			break;
		}
	}
	if(!decl)
		return false;

	//if ( !hashTables[(int)type].Get( canonicalOldName, &declPtr ) )
	//	return false;

	//decl = *declPtr;

	//Change the name
	decl->name = canonicalNewName;


	// add it to the hash table
	//hashTables[(int)decl->type].Set( decl->name, decl );
	int newhash = hashTables[typeIndex].GenerateKey( canonicalNewName, false );
	hashTables[typeIndex].Add( newhash, decl->index );

	//Remove the old hash item
	hashTables[typeIndex].Remove(hash, decl->index);

	return true;
}

/*
===================
hfDeclManagerLocal::MediaPrint

This is just used to nicely indent media caching prints
===================
*/
void hfDeclManagerLocal::MediaPrint( const char *fmt, ... ) {
	if ( !decl_show.GetInteger() ) {
		return;
	}
	for ( int i = 0 ; i < indent ; i++ ) {
		common->Printf( "    " );
	}
	va_list		argptr;
	char		buffer[1024];
	va_start (argptr,fmt);
	hfStr::vsnPrintf( buffer, sizeof(buffer), fmt, argptr );
	va_end (argptr);
	buffer[sizeof(buffer)-1] = '\0';

	common->Printf( "%s", buffer );
}

/*
===================
hfDeclManagerLocal::WritePrecacheCommands
===================
*/
void hfDeclManagerLocal::WritePrecacheCommands( hfFile *f ) {
	for ( int i = 0; i < declTypes.Num(); i++ ) {
		int num;

		if ( declTypes[i] == NULL ) {
			continue;
		}

		num = linearLists[i].Num();

		for ( int j = 0 ; j < num ; j++ ) {
			hfDeclLocal *decl = linearLists[i][j];

			if ( !decl->referencedThisLevel ) {
				continue;
			}

			char	str[1024];
			sprintf( str, "touch %s %s\n", declTypes[i]->typeName.c_str(), decl->GetName() );
			common->Printf( "%s", str );
			f->Printf( "%s", str );
		}
	}
}

/********************************************************************/

const hfMaterial *hfDeclManagerLocal::FindMaterial( const char *name, bool makeDefault ) {
	return static_cast<const hfMaterial *>( FindType( DECL_MATERIAL, name, makeDefault ) );
}

const hfMaterial *hfDeclManagerLocal::MaterialByIndex( int index, bool forceParse ) {
	return static_cast<const hfMaterial *>( DeclByIndex( DECL_MATERIAL, index, forceParse ) );
}

/********************************************************************/

const hfDeclSkin *hfDeclManagerLocal::FindSkin( const char *name, bool makeDefault ) {
	return static_cast<const hfDeclSkin *>( FindType( DECL_SKIN, name, makeDefault ) );
}

const hfDeclSkin *hfDeclManagerLocal::SkinByIndex( int index, bool forceParse ) {
	return static_cast<const hfDeclSkin *>( DeclByIndex( DECL_SKIN, index, forceParse ) );
}

/********************************************************************/

const hfSoundShader *hfDeclManagerLocal::FindSound( const char *name, bool makeDefault ) {
	return static_cast<const hfSoundShader *>( FindType( DECL_SOUND, name, makeDefault ) );
}

const hfSoundShader *hfDeclManagerLocal::SoundByIndex( int index, bool forceParse ) {
	return static_cast<const hfSoundShader *>( DeclByIndex( DECL_SOUND, index, forceParse ) );
}

/*
===================
hfDeclManagerLocal::Touch
===================
*/
void hfDeclManagerLocal::Touch( const hfDecl * decl ) {

	if( decl->base->GetState() ==  DS_UNPARSED ) {
		// This should parse the decl as well.
		FindType( decl->GetType(), decl->GetName() );
	}
}

/*
===================
hfDeclManagerLocal::MakeNameCanonical
===================
*/
void hfDeclManagerLocal::MakeNameCanonical( const char *name, char *result, int maxLength ) {
	int i, lastDot;

	lastDot = -1;
	for ( i = 0; i < maxLength && name[i] != '\0'; i++ ) {
		int c = name[i];
		if ( c == '\\' ) {
			result[i] = '/';
		} else if ( c == '.' ) {
			lastDot = i;
			result[i] = c;
		} else {
			result[i] = hfStr::ToLower( c );
		}
	}
	if ( lastDot != -1 ) {
		result[lastDot] = '\0';
	} else {
		result[i] = '\0';
	}
}

/*
================
hfDeclManagerLocal::ListDecls_f
================
*/
void hfDeclManagerLocal::ListDecls_f( const hfCmdArgs &args ) {
	int		i, j;
	int		totalDecls = 0;
	int		totalText = 0;
	int		totalStructs = 0;

	for ( i = 0; i < declManagerLocal.declTypes.Num(); i++ ) {
		int size, num;

		if ( declManagerLocal.declTypes[i] == NULL ) {
			continue;
		}

		num = declManagerLocal.linearLists[i].Num();
		totalDecls += num;

		size = 0;
		for ( j = 0; j < num; j++ ) {
			size += declManagerLocal.linearLists[i][j]->Size();
			if ( declManagerLocal.linearLists[i][j]->self != NULL ) {
				size += declManagerLocal.linearLists[i][j]->self->Size();
			}
		}
		totalStructs += size;

		common->Printf( "%4ik %4i %s\n", size >> 10, num, declManagerLocal.declTypes[i]->typeName.c_str() );
	}

	for ( i = 0 ; i < declManagerLocal.loadedFiles.Num() ; i++ ) {
		hfDeclFile	*df = declManagerLocal.loadedFiles[i];
		totalText += df->fileSize;
	}

	common->Printf( "%i total decls is %i decl files\n", totalDecls, declManagerLocal.loadedFiles.Num() );
	common->Printf( "%iKB in text, %iKB in structures\n", totalText >> 10, totalStructs >> 10 );
}

/*
===================
hfDeclManagerLocal::ReloadDecls_f

Reload will not find any new files created in the directories, it
will only reload existing files.

A reload will never cause anything to be purged.
===================
*/
void hfDeclManagerLocal::ReloadDecls_f( const hfCmdArgs &args ) {
	bool	force;

	if ( !hfStr::Icmp( args.Argv( 1 ), "all" ) ) {
		force = true;
		common->Printf( "reloading all decl files:\n" );
	} else {
		force = false;
		common->Printf( "reloading changed decl files:\n" );
	}

	declManagerLocal.Reload( force );
}

/*
===================
hfDeclManagerLocal::TouchDecl_f
===================
*/
void hfDeclManagerLocal::TouchDecl_f( const hfCmdArgs &args ) {
	int	i;

	if ( args.Argc() != 3 ) {
		common->Printf( "usage: touch <type> <name>\n" );
		common->Printf( "valid types: " );
		for ( int i = 0 ; i < declManagerLocal.declTypes.Num() ; i++ ) {
			if ( declManagerLocal.declTypes[i] ) {
				common->Printf( "%s ", declManagerLocal.declTypes[i]->typeName.c_str() );
			}
		}
		common->Printf( "\n" );
		return;
	}

	for ( i = 0; i < declManagerLocal.declTypes.Num(); i++ ) {
		if ( declManagerLocal.declTypes[i] && declManagerLocal.declTypes[i]->typeName.Icmp( args.Argv( 1 ) ) == 0 ) {
			break;
		}
	}
	if ( i >= declManagerLocal.declTypes.Num() ) {
		common->Printf( "unknown decl type '%s'\n", args.Argv( 1 ) );
		return;
	}

	const hfDecl *decl = declManagerLocal.FindType( (declType_t)i, args.Argv( 2 ), false );
	if ( !decl ) {
		common->Printf( "%s '%s' not found\n", declManagerLocal.declTypes[i]->typeName.c_str(), args.Argv( 2 ) );
	}
}

/*
===================
hfDeclManagerLocal::FindTypeWithoutParsing

This finds or creats the decl, but does not cause a parse.  This is only used internally.
===================
*/
hfDeclLocal *hfDeclManagerLocal::FindTypeWithoutParsing( declType_t type, const char *name, bool makeDefault ) {
	int typeIndex = (int)type;
	int i, hash;

	if ( typeIndex < 0 || typeIndex >= declTypes.Num() || declTypes[typeIndex] == NULL || typeIndex >= DECL_MAX_TYPES ) {
		common->FatalError( "hfDeclManager::FindTypeWithoutParsing: bad type: %i", typeIndex );
		return NULL;
	}

	char canonicalName[MAX_STRING_CHARS];

	MakeNameCanonical( name, canonicalName, sizeof( canonicalName ) );

	// see if it already exists
	hash = hashTables[typeIndex].GenerateKey( canonicalName, false );
	for ( i = hashTables[typeIndex].First( hash ); i >= 0; i = hashTables[typeIndex].Next( i ) ) {
		if ( linearLists[typeIndex][i]->name.Icmp( canonicalName ) == 0 ) {
			// only print these when decl_show is set to 2, because it can be a lot of clutter
			if ( decl_show.GetInteger() > 1 ) {
				MediaPrint( "referencing %s %s\n", declTypes[ type ]->typeName.c_str(), name );
			}
			return linearLists[typeIndex][i];
		}
	}

	if ( !makeDefault ) {
		return NULL;
	}

	hfDeclLocal *decl = new (TAG_DECL) hfDeclLocal;
	decl->self = NULL;
	decl->name = canonicalName;
	decl->type = type;
	decl->declState = DS_UNPARSED;
	decl->textSource = NULL;
	decl->textLength = 0;
	decl->sourceFile = &implicitDecls;
	decl->referencedThisLevel = false;
	decl->everReferenced = false;
	decl->parsedOutsideLevelLoad = !insideLevelLoad;

	// add it to the linear list and hash table
	decl->index = linearLists[typeIndex].Num();
	hashTables[typeIndex].Add( hash, linearLists[typeIndex].Append( decl ) );

	return decl;
}

/*
=================
hfDeclManagerLocal::ConvertPDAsToStrings
=================
*/
void hfDeclManagerLocal::ConvertPDAsToStrings( const hfCmdArgs &args ) {

	hfStr pdaStringsFileName = "temppdas/pdas.lang";
	hfFileLocal file( fileSystem->OpenFileWrite( pdaStringsFileName ) );

	if ( file == NULL ) {
		hfLib::Printf( "Failed to Convert PDA data to Strings.\n" );
	}

	int totalEmailCount = 0;
	int totalAudioCount = 0;
	int totalVideoCount = 0;
	hfStr headEnd = "\t\"#str_%s_";
	hfStr tailEnd = "\"\t\"%s\"\n";
	hfStr temp;

	int count = linearLists[ DECL_PDA ].Num();
	for ( int i = 0; i < count; i++ ) {
		const hfDeclPDA *decl = static_cast< const hfDeclPDA * >( FindType( DECL_PDA, linearLists[ DECL_PDA ][ i ]->GetName(), false ) );

		hfStr pdaBaseStrId = va( headEnd.c_str(), decl->GetName() );

		temp = va( "\n\n//////// %s PDA ////////////\n", decl->GetName() );
		file->Write( temp, temp.Length() );

		hfStr pdaBase = pdaBaseStrId + "pda_%s" + tailEnd;
		// Pda Name
		temp = va( pdaBase.c_str(), "name", decl->GetPdaName() );
		file->Write( temp, temp.Length() );
		// Full Name
		temp = va( pdaBase.c_str(), "fullname", decl->GetFullName() );
		file->Write( temp, temp.Length() );
		// ID
		temp = va( pdaBase.c_str(), "id", decl->GetID() );
		file->Write( temp, temp.Length() );
		// Post
		temp = va( pdaBase.c_str(), "post", decl->GetPost() );
		file->Write( temp, temp.Length() );
		// Title
		temp = va( pdaBase.c_str(), "title", decl->GetTitle() );
		file->Write( temp, temp.Length() );
		// Security
		temp = va( pdaBase.c_str(), "security", decl->GetSecurity() );
		file->Write( temp, temp.Length() );

		int emailCount = decl->GetNumEmails();
		for ( int emailIter = 0; emailIter < emailCount; emailIter++ ) {
			const hfDeclEmail * email = decl->GetEmailByIndex( emailIter );

			hfStr emailBaseStrId = va( headEnd.c_str(), email->GetName() );
			hfStr emailBase = emailBaseStrId + "email_%s" + tailEnd;

			file->Write( "\t//Email\n", 9 );
			// Date
			temp = va( emailBase, "date", email->GetDate() );
			file->Write( temp, temp.Length() );
			// To
			temp = va( emailBase, "to", email->GetTo() );
			file->Write( temp, temp.Length() );
			// From
			temp = va( emailBase, "from", email->GetFrom() );
			file->Write( temp, temp.Length() );
			// Subject
			temp = va( emailBase, "subject", email->GetSubject() );
			file->Write( temp, temp.Length() );
			// Body
			hfStr body = email->GetBody();
			body.Replace( "\n", "\\n" );
			temp = va( emailBase, "text", body.c_str() );
			file->Write( temp, temp.Length() );

			totalEmailCount++;
		}

		int audioCount = decl->GetNumAudios();
		for ( int audioIter = 0; audioIter < audioCount; audioIter++ ) {
			const hfDeclAudio * audio = decl->GetAudioByIndex( audioIter );

			hfStr audioBaseStrId = va( headEnd.c_str(), audio->GetName() );
			hfStr audioBase = audioBaseStrId + "audio_%s" + tailEnd;

			file->Write( "\t//Audio\n", 9 );
			// Name
			temp = va( audioBase, "name", audio->GetAudioName() );
			file->Write( temp, temp.Length() );
			// Info
			hfStr info = audio->GetInfo();
			info.Replace( "\n", "\\n" );
			temp = va( audioBase, "info", info.c_str() );
			file->Write( temp, temp.Length() );

			totalAudioCount++;
		}
	}

	int infoEmailCount = linearLists[ DECL_EMAIL ].Num();
	if ( infoEmailCount > 0 ) {
		temp = "\n\n//////// PDA Info Emails ////////////\n";
		file->Write( temp, temp.Length() );
	}
	for ( int i = 0; i < infoEmailCount; i++ ) {
		const hfDeclEmail * email = static_cast< const hfDeclEmail * >( FindType( DECL_EMAIL, linearLists[ DECL_EMAIL ][ i ]->GetName(), false ) );

		hfStr filename = email->base->GetFileName();
		if ( filename.Icmp( "newpdas/info_emails.pda" ) != 0 ) {
			continue;
		}

		hfStr emailBaseStrId = va( "\t\"#str_%s_", email->GetName() );
		hfStr emailBase = emailBaseStrId + "email_%s" + tailEnd;

		file->Write( "\t//Email\n", 9 );

		// Date
		temp = va( emailBase, "date", email->GetDate() );
		file->Write( temp, temp.Length() );
		// To
		temp = va( emailBase, "to", email->GetTo() );
		file->Write( temp, temp.Length() );
		// From
		temp = va( emailBase, "from", email->GetFrom() );
		file->Write( temp, temp.Length() );
		// Subject
		temp = va( emailBase, "subject", email->GetSubject() );
		file->Write( temp, temp.Length() );
		// Body
		hfStr body = email->GetBody();
		body.Replace( "\n", "\\n" );
		temp = va( emailBase, "text", body.c_str() );
		file->Write( temp, temp.Length() );

		totalEmailCount++;
	}

	int videoCount = linearLists[ DECL_VIDEO ].Num();
	if ( videoCount > 0 ) {
		temp = "\n\n//////// PDA Videos ////////////\n";
		file->Write( temp, temp.Length() );
	}
	for ( int i = 0; i < videoCount; i++ ) {
		const hfDeclVideo * video = static_cast< const hfDeclVideo * >( FindType( DECL_VIDEO, linearLists[ DECL_VIDEO ][ i ]->GetName(), false ) );

		hfStr videoBaseStrId = va( "\t\"#str_%s_", video->GetName() );
		hfStr videoBase = videoBaseStrId + "video_%s" + tailEnd;

		file->Write( "\t//Video\n", 9 );

		// Name
		temp = va( videoBase, "name", video->GetVideoName() );
		file->Write( temp, temp.Length() );
		// Info
		hfStr info = video->GetInfo();
		info.Replace( "\n", "\\n" );
		temp = va( videoBase, "info", info.c_str() );
		file->Write( temp, temp.Length() );

		totalVideoCount++;
	}

	file->Flush();

	hfLib::Printf( "\nData written to %s\n", pdaStringsFileName.c_str() );
	hfLib::Printf( "----------------------------\n" );
	hfLib::Printf( "Wrote %d PDAs.\n", count );
	hfLib::Printf( "Wrote %d Emails.\n", totalEmailCount );
	hfLib::Printf( "Wrote %d Audio Records.\n", totalAudioCount );
	hfLib::Printf( "Wrote %d Video Records.\n", totalVideoCount );
	hfLib::Printf( "Please copy the results into the appropriate .lang file.\n" );
}

/*
====================================================================================

	hfDeclLocal

====================================================================================
*/

/*
=================
hfDeclLocal::hfDeclLocal
=================
*/
hfDeclLocal::hfDeclLocal() {
	name = "unnamed";
	textSource = NULL;
	textLength = 0;
	compressedLength = 0;
	sourceFile = NULL;
	sourceTextOffset = 0;
	sourceTextLength = 0;
	sourceLine = 0;
	checksum = 0;
	type = DECL_ENTITYDEF;
	index = 0;
	declState = DS_UNPARSED;
	parsedOutsideLevelLoad = false;
	referencedThisLevel = false;
	everReferenced = false;
	redefinedInReload = false;
	nextInFile = NULL;
}

/*
=================
hfDeclLocal::GetName
=================
*/
const char *hfDeclLocal::GetName() const {
	return name.c_str();
}

/*
=================
hfDeclLocal::GetType
=================
*/
declType_t hfDeclLocal::GetType() const {
	return type;
}

/*
=================
hfDeclLocal::GetState
=================
*/
declState_t hfDeclLocal::GetState() const {
	return declState;
}

/*
=================
hfDeclLocal::IsImplicit
=================
*/
bool hfDeclLocal::IsImplicit() const {
	return ( sourceFile == declManagerLocal.GetImplicitDeclFile() );
}

/*
=================
hfDeclLocal::IsValid
=================
*/
bool hfDeclLocal::IsValid() const {
	return ( declState != DS_UNPARSED );
}

/*
=================
hfDeclLocal::Invalidate
=================
*/
void hfDeclLocal::Invalidate() {
	declState = DS_UNPARSED;
}

/*
=================
hfDeclLocal::EnsureNotPurged
=================
*/
void hfDeclLocal::EnsureNotPurged() {
	if ( declState == DS_UNPARSED ) {
		ParseLocal();
	}
}

/*
=================
hfDeclLocal::Index
=================
*/
int hfDeclLocal::Index() const {
	return index;
}

/*
=================
hfDeclLocal::GetLineNum
=================
*/
int hfDeclLocal::GetLineNum() const {
	return sourceLine;
}

/*
=================
hfDeclLocal::GetFileName
=================
*/
const char *hfDeclLocal::GetFileName() const {
	return ( sourceFile ) ? sourceFile->fileName.c_str() : "*invalid*";
}

/*
=================
hfDeclLocal::Size
=================
*/
size_t hfDeclLocal::Size() const {
	return sizeof( hfDecl ) + name.Allocated();
}

/*
=================
hfDeclLocal::GetText
=================
*/
void hfDeclLocal::GetText( char *text ) const {
#ifdef USE_COMPRESSED_DECLS
	HuffmanDecompressText( text, textLength, (byte *)textSource, compressedLength );
#else
	memcpy( text, textSource, textLength+1 );
#endif
}

/*
=================
hfDeclLocal::GetTextLength
=================
*/
int hfDeclLocal::GetTextLength() const {
	return textLength;
}

/*
=================
hfDeclLocal::SetText
=================
*/
void hfDeclLocal::SetText( const char *text ) {
	SetTextLocal( text, hfStr::Length( text ) );
}

/*
=================
hfDeclLocal::SetTextLocal
=================
*/
void hfDeclLocal::SetTextLocal( const char *text, const int length ) {

	Mem_Free( textSource );

	checksum = MD5_BlockChecksum( text, length );

#ifdef GET_HUFFMAN_FREQUENCIES
	for( int i = 0; i < length; i++ ) {
		huffmanFrequencies[((const unsigned char *)text)[i]]++;
	}
#endif

#ifdef USE_COMPRESSED_DECLS
	int maxBytesPerCode = ( maxHuffmanBits + 7 ) >> 3;
	byte *compressed = (byte *)_alloca( length * maxBytesPerCode );
	compressedLength = HuffmanCompressText( text, length, compressed, length * maxBytesPerCode );
	textSource = (char *)Mem_Alloc( compressedLength, TAG_DECLTEXT );
	memcpy( textSource, compressed, compressedLength );
#else
	compressedLength = length;
	textSource = (char *) Mem_Alloc( length + 1, TAG_DECLTEXT );
	memcpy( textSource, text, length );
	textSource[length] = '\0';
#endif
	textLength = length;
}

/*
=================
hfDeclLocal::ReplaceSourceFileText
=================
*/
bool hfDeclLocal::ReplaceSourceFileText() {
	int oldFileLength, newFileLength;
	hfFile *file;

	common->Printf( "Writing \'%s\' to \'%s\'...\n", GetName(), GetFileName() );

	if ( sourceFile == &declManagerLocal.implicitDecls ) {
		common->Warning( "Can't save implicit declaration %s.", GetName() );
		return false;
	}

	// get length and allocate buffer to hold the file
	oldFileLength = sourceFile->fileSize;
	newFileLength = oldFileLength - sourceTextLength + textLength;
	hfTempArray<char> buffer( Max( newFileLength, oldFileLength ) );
	memset( buffer.Ptr(), 0, buffer.Size() );

	// read original file
	if ( sourceFile->fileSize ) {

		file = fileSystem->OpenFileRead( GetFileName() );
		if ( !file ) {
			common->Warning( "Couldn't open %s for reading.", GetFileName() );
			return false;
		}

		if ( file->Length() != sourceFile->fileSize || file->Timestamp() != sourceFile->timestamp ) {
			common->Warning( "The file %s has been modified outside of the engine.", GetFileName() );
			return false;
		}

		file->Read( buffer.Ptr(), oldFileLength );
		fileSystem->CloseFile( file );

		if ( MD5_BlockChecksum( buffer.Ptr(), oldFileLength ) != (unsigned int)sourceFile->checksum ) {
			common->Warning( "The file %s has been modified outside of the engine.", GetFileName() );
			return false;
		}
	}

	// insert new text
	char *declText = (char *) _alloca( textLength + 1 );
	GetText( declText );
	memmove( buffer.Ptr() + sourceTextOffset + textLength, buffer.Ptr() + sourceTextOffset + sourceTextLength, oldFileLength - sourceTextOffset - sourceTextLength );
	memcpy( buffer.Ptr() + sourceTextOffset, declText, textLength );

	// write out new file
	file = fileSystem->OpenFileWrite( GetFileName(), "fs_basepath" );
	if ( !file ) {
		common->Warning( "Couldn't open %s for writing.", GetFileName() );
		return false;
	}
	file->Write( buffer.Ptr(), newFileLength );
	fileSystem->CloseFile( file );

	// set new file size, checksum and timestamp
	sourceFile->fileSize = newFileLength;
	sourceFile->checksum = MD5_BlockChecksum( buffer.Ptr(), newFileLength );
	fileSystem->ReadFile( GetFileName(), NULL, &sourceFile->timestamp );

	// move all decls in the same file
	for ( hfDeclLocal *decl = sourceFile->decls; decl; decl = decl->nextInFile ) {
		if (decl->sourceTextOffset > sourceTextOffset) {
			decl->sourceTextOffset += textLength - sourceTextLength;
		}
	}

	// set new size of text in source file
	sourceTextLength = textLength;

	return true;
}

/*
=================
hfDeclLocal::SourceFileChanged
=================
*/
bool hfDeclLocal::SourceFileChanged() const {
	int newLength;
	HF_TIME_T newTimestamp;

	if ( sourceFile->fileSize <= 0 ) {
		return false;
	}

	newLength = fileSystem->ReadFile( GetFileName(), NULL, &newTimestamp );

	if ( newLength != sourceFile->fileSize || newTimestamp != sourceFile->timestamp ) {
		return true;
	}

	return false;
}

/*
=================
hfDeclLocal::MakeDefault
=================
*/
void hfDeclLocal::MakeDefault() {
	static int recursionLevel;
	const char *defaultText;

	declManagerLocal.MediaPrint( "DEFAULTED\n" );
	declState = DS_DEFAULTED;

	AllocateSelf();

	defaultText = self->DefaultDefinition();

	// a parse error inside a DefaultDefinition() string could
	// cause an infinite loop, but normal default definitions could
	// still reference other default definitions, so we can't
	// just dump out on the first recursion
	if ( ++recursionLevel > 100 ) {
		common->FatalError( "hfDecl::MakeDefault: bad DefaultDefinition(): %s", defaultText );
	}

	// always free data before parsing
	self->FreeData();

	// parse
	self->Parse( defaultText, strlen( defaultText ), false );

	// we could still eventually hit the recursion if we have enough Error() calls inside Parse...
	--recursionLevel;
}

/*
=================
hfDeclLocal::SetDefaultText
=================
*/
bool hfDeclLocal::SetDefaultText() {
	return false;
}

/*
=================
hfDeclLocal::DefaultDefinition
=================
*/
const char *hfDeclLocal::DefaultDefinition() const {
	return "{ }";
}

/*
=================
hfDeclLocal::Parse
=================
*/
bool hfDeclLocal::Parse( const char *text, const int textLength, bool allowBinaryVersion ) {
	hfLexer src;

	src.LoadMemory( text, textLength, GetFileName(), GetLineNum() );
	src.SetFlags( DECL_LEXER_FLAGS );
	src.SkipUntilString( "{" );
	src.SkipBracedSection( false );
	return true;
}

/*
=================
hfDeclLocal::FreeData
=================
*/
void hfDeclLocal::FreeData() {
}

/*
=================
hfDeclLocal::List
=================
*/
void hfDeclLocal::List() const {
	common->Printf( "%s\n", GetName() );
}

/*
=================
hfDeclLocal::Print
=================
*/
void hfDeclLocal::Print() const {
}

/*
=================
hfDeclLocal::Reload
=================
*/
void hfDeclLocal::Reload() {
	this->sourceFile->Reload( false );
}

/*
=================
hfDeclLocal::AllocateSelf
=================
*/
void hfDeclLocal::AllocateSelf() {
	if ( self == NULL ) {
		self = declManagerLocal.GetDeclType( (int)type )->allocator();
		self->base = this;
	}
}

/*
=================
hfDeclLocal::ParseLocal
=================
*/
void hfDeclLocal::ParseLocal() {
	bool generatedDefaultText = false;

	AllocateSelf();

	// always free data before parsing
	self->FreeData();

	declManagerLocal.MediaPrint( "parsing %s %s\n", declManagerLocal.declTypes[type]->typeName.c_str(), name.c_str() );

	// if no text source try to generate default text
	if ( textSource == NULL ) {
		generatedDefaultText = self->SetDefaultText();
	}

	// indent for DEFAULTED or media file references
	declManagerLocal.indent++;

	// no text immediately causes a MakeDefault()
	if ( textSource == NULL ) {
		MakeDefault();
		declManagerLocal.indent--;
		return;
	}

	declState = DS_PARSED;

	// parse
	char *declText = (char *) _alloca( ( GetTextLength() + 1 ) * sizeof( char ) );
	GetText( declText );
	self->Parse( declText, GetTextLength(), true );

	// free generated text
	if ( generatedDefaultText ) {
		Mem_Free( textSource );
		textSource = NULL;
		textLength = 0;
	}

	declManagerLocal.indent--;
}

/*
=================
hfDeclLocal::Purge
=================
*/
void hfDeclLocal::Purge() {
	// never purge things that were referenced outside level load,
	// like the console and menu graphics
	if ( parsedOutsideLevelLoad ) {
		return;
	}

	referencedThisLevel = false;
	MakeDefault();

	// the next Find() for this will re-parse the real data
	declState = DS_UNPARSED;
}

/*
=================
hfDeclLocal::EverReferenced
=================
*/
bool hfDeclLocal::EverReferenced() const {
	return everReferenced;
}
