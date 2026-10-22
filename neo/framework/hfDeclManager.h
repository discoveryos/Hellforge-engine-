#ifndef __DECLMANAGER_H__
#define __DECLMANAGER_H__

/*
===============================================================================

	Declaration Manager

	All "small text" data types, like materials, sound shaders, fx files,
	entity defs, etc. are managed uniformly, allowing reloading, purging,
	listing, printing, etc. All "large text" data types that never have more
	than one declaration in a given file, like maps, models, AAS files, etc.
	are not handled here.

	A decl will never, ever go away once it is created. The manager is
	garranteed to always return the same decl pointer for a decl type/name
	combination. The index of a decl in the per type list also stays the
	same throughout the lifetime of the engine. Although the pointer to
	a decl always stays the same, one should never maintain pointers to
	data inside decls. The data stored in a decl is not garranteed to stay
	the same for more than one engine frame.

	The decl indexes of explicitely defined decls are garrenteed to be
	consistent based on the parsed decl files. However, the indexes of
	implicit decls may be different based on the order in which levels
	are loaded.

	The decl namespaces are separate for each type. Comments for decls go
	above the text definition to keep them associated with the proper decl.

	During decl parsing, errors should never be issued, only warnings
	followed by a call to MakeDefault().

===============================================================================
*/

typedef enum {
	DECL_TABLE				= 0,
	DECL_MATERIAL,
	DECL_SKIN,
	DECL_SOUND,
	DECL_ENTITYDEF,
	DECL_MODELDEF,
	DECL_FX,
	DECL_PARTICLE,
	DECL_AF,
	DECL_PDA,
	DECL_VIDEO,
	DECL_AUDIO,
	DECL_EMAIL,
	DECL_MODELEXPORT,
	DECL_MAPDEF,

	// new decl types can be added here

	DECL_MAX_TYPES			= 32
} declType_t;

typedef enum {
	DS_UNPARSED,
	DS_DEFAULTED,			// set if a parse failed due to an error, or the lack of any source
	DS_PARSED
} declState_t;

const int DECL_LEXER_FLAGS	=	LEXFL_NOSTRINGCONCAT |				// multiple strings seperated by whitespaces are not concatenated
								LEXFL_NOSTRINGESCAPECHARS |			// no escape characters inside strings
								LEXFL_ALLOWPATHNAMES |				// allow path seperators in names
								LEXFL_ALLOWMULTICHARLITERALS |		// allow multi character literals
								LEXFL_ALLOWBACKSLASHSTRINGCONCAT |	// allow multiple strings seperated by '\' to be concatenated
								LEXFL_NOFATALERRORS;				// just set a flag instead of fatal erroring


class hfDeclBase {
public:
	virtual 				~hfDeclBase() {};
	virtual const char *	GetName() const = 0;
	virtual declType_t		GetType() const = 0;
	virtual declState_t		GetState() const = 0;
	virtual bool			IsImplicit() const = 0;
	virtual bool			IsValid() const = 0;
	virtual void			Invalidate() = 0;
	virtual void			Reload() = 0;
	virtual void			EnsureNotPurged() = 0;
	virtual int				Index() const = 0;
	virtual int				GetLineNum() const = 0;
	virtual const char *	GetFileName() const = 0;
	virtual void			GetText( char *text ) const = 0;
	virtual int				GetTextLength() const = 0;
	virtual void			SetText( const char *text ) = 0;
	virtual bool			ReplaceSourceFileText() = 0;
	virtual bool			SourceFileChanged() const = 0;
	virtual void			MakeDefault() = 0;
	virtual bool			EverReferenced() const = 0;
	virtual bool			SetDefaultText() = 0;
	virtual const char *	DefaultDefinition() const = 0;
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion ) = 0;
	virtual void			FreeData() = 0;
	virtual size_t			Size() const = 0;
	virtual void			List() const = 0;
	virtual void			Print() const = 0;
};


class hfDecl {
public:
							// The constructor should initialize variables such that
							// an immediate call to FreeData() does no harm.
							hfDecl() { base = NULL; }
	virtual 				~hfDecl() {};

							// Returns the name of the decl.
	const char *			GetName() const { return base->GetName(); }

							// Returns the decl type.
	declType_t				GetType() const { return base->GetType(); }

							// Returns the decl state which is usefull for finding out if a decl defaulted.
	declState_t				GetState() const { return base->GetState(); }

							// Returns true if the decl was defaulted or the text was created with a call to SetDefaultText.
	bool					IsImplicit() const { return base->IsImplicit(); }

							// The only way non-manager code can have an invalid decl is if the *ByIndex()
							// call was used with forceParse = false to walk the lists to look at names
							// without touching the media.
	bool					IsValid() const { return base->IsValid(); }

							// Sets state back to unparsed.
							// Used by decl editors to undo any changes to the decl.
	void					Invalidate() { base->Invalidate(); }

							// if a pointer might possible be stale from a previous level,
							// call this to have it re-parsed
	void					EnsureNotPurged() { base->EnsureNotPurged(); }

							// Returns the index in the per-type list.
	int						Index() const { return base->Index(); }

							// Returns the line number the decl starts.
	int						GetLineNum() const { return base->GetLineNum(); }

							// Returns the name of the file in which the decl is defined.
	const char *			GetFileName() const { return base->GetFileName(); }

							// Returns the decl text.
	void					GetText( char *text ) const { base->GetText( text ); }

							// Returns the length of the decl text.
	int						GetTextLength() const { return base->GetTextLength(); }

							// Sets new decl text.
	void					SetText( const char *text ) { base->SetText( text ); }

							// Saves out new text for the decl.
							// Used by decl editors to replace the decl text in the source file.
	bool					ReplaceSourceFileText() { return base->ReplaceSourceFileText(); }

							// Returns true if the source file changed since it was loaded and parsed.
	bool					SourceFileChanged() const { return base->SourceFileChanged(); }

							// Frees data and makes the decl a default.
	void					MakeDefault() { base->MakeDefault(); }

							// Returns true if the decl was ever referenced.
	bool					EverReferenced() const { return base->EverReferenced(); }

public:
							// Sets textSource to a default text if necessary.
							// This may be overridden to provide a default definition based on the
							// decl name. For instance materials may default to an implicit definition
							// using a texture with the same name as the decl.
	virtual bool			SetDefaultText() { return base->SetDefaultText(); }

							// Each declaration type must have a default string that it is guaranteed
							// to parse acceptably. When a decl is not explicitly found, is purged, or
							// has an error while parsing, MakeDefault() will do a FreeData(), then a
							// Parse() with DefaultDefinition(). The defaultDefintion should start with
							// an open brace and end with a close brace.
	virtual const char *	DefaultDefinition() const { return base->DefaultDefinition(); }

							// The manager will have already parsed past the type, name and opening brace.
							// All necessary media will be touched before return.
							// The manager will have called FreeData() before issuing a Parse().
							// The subclass can call MakeDefault() internally at any point if
							// there are parse errors.
	virtual bool			Parse( const char *text, const int textLength, bool allowBinaryVersion = false ) { return base->Parse( text, textLength, allowBinaryVersion ); }

							// Frees any pointers held by the subclass. This may be called before
							// any Parse(), so the constructor must have set sane values. The decl will be
							// invalid after issuing this call, but it will always be immediately followed
							// by a Parse()
	virtual void			FreeData() { base->FreeData(); }

							// Returns the size of the decl in memory.
	virtual size_t			Size() const { return base->Size(); }

							// If this isn't overridden, it will just print the decl name.
							// The manager will have printed 7 characters on the line already,
							// containing the reference state and index number.
	virtual void			List() const { base->List(); }

							// The print function will already have dumped the text source
							// and common data, subclasses can override this to dump more
							// explicit data.
	virtual void			Print() const { base->Print(); }

public:
	hfDeclBase *			base;
};


template< class type >
HF_INLINE hfDecl *hfDeclAllocator() {
	return new (TAG_DECL) type;
}


class hfMaterial;
class hfDeclSkin;
class hfSoundShader;

class hfDeclManager {
public:
	virtual					~hfDeclManager() {}

	virtual void			Init() = 0;
	virtual void			Init2() = 0;
	virtual void			Shutdown() = 0;
	virtual void			Reload( bool force ) = 0;

	virtual void			BeginLevelLoad() = 0;
	virtual void			EndLevelLoad() = 0;

							// Registers a new decl type.
	virtual void			RegisterDeclType( const char *typeName, declType_t type, hfDecl *(*allocator)() ) = 0;

							// Registers a new folder with decl files.
	virtual void			RegisterDeclFolder( const char *folder, const char *extension, declType_t defaultType ) = 0;

							// Returns a checksum for all loaded decl text.
	virtual int				GetChecksum() const = 0;

							// Returns the number of decl types.
	virtual int				GetNumDeclTypes() const = 0;

							// Returns the type name for a decl type.
	virtual const char *	GetDeclNameFromType( declType_t type ) const = 0;

							// Returns the decl type for a type name.
	virtual declType_t		GetDeclTypeFromName( const char *typeName ) const = 0;

							// If makeDefault is true, a default decl of appropriate type will be created
							// if an explicit one isn't found. If makeDefault is false, NULL will be returned
							// if the decl wasn't explcitly defined.
	virtual const hfDecl *	FindType( declType_t type, const char *name, bool makeDefault = true ) = 0;

	virtual const hfDecl*	FindDeclWithoutParsing( declType_t type, const char *name, bool makeDefault = true ) = 0;

	virtual void			ReloadFile( const char* filename, bool force ) = 0;

							// Returns the number of decls of the given type.
	virtual int				GetNumDecls( declType_t type ) = 0;

							// The complete lists of decls can be walked to populate editor browsers.
							// If forceParse is set false, you can get the decl to check name / filename / etc.
							// without causing it to parse the source and load media.
	virtual const hfDecl *	DeclByIndex( declType_t type, int index, bool forceParse = true ) = 0;

							// List and print decls.
	virtual void			ListType( const hfCmdArgs &args, declType_t type ) = 0;
	virtual void			PrintType( const hfCmdArgs &args, declType_t type ) = 0;

							// Creates a new default decl of the given type with the given name in
							// the given file used by editors to create a new decls.
	virtual hfDecl *		CreateNewDecl( declType_t type, const char *name, const char *fileName ) = 0;

							// BSM - Added for the material editors rename capabilities
	virtual bool			RenameDecl( declType_t type, const char* oldName, const char* newName ) = 0;

							// When media files are loaded, a reference line can be printed at a
							// proper indentation if decl_show is set
	virtual void			MediaPrint( VERIFY_FORMAT_STRING const char *fmt, ... ) = 0;

	virtual void			WritePrecacheCommands( hfFile *f ) = 0;

									// Convenience functions for specific types.
	virtual	const hfMaterial *		FindMaterial( const char *name, bool makeDefault = true ) = 0;
	virtual const hfDeclSkin *		FindSkin( const char *name, bool makeDefault = true ) = 0;
	virtual const hfSoundShader *	FindSound( const char *name, bool makeDefault = true ) = 0;

	virtual const hfMaterial *		MaterialByIndex( int index, bool forceParse = true ) = 0;
	virtual const hfDeclSkin *		SkinByIndex( int index, bool forceParse = true ) = 0;
	virtual const hfSoundShader *	SoundByIndex( int index, bool forceParse = true ) = 0;

	virtual void					Touch( const hfDecl * decl ) = 0;
};

extern hfDeclManager *		declManager;


template< declType_t type >
HF_INLINE void hfListDecls_f( const hfCmdArgs &args ) {
	declManager->ListType( args, type );
}

template< declType_t type >
HF_INLINE void hfPrintDecls_f( const hfCmdArgs &args ) {
	declManager->PrintType( args, type );
}

#endif /* !__DECLMANAGER_H__ */

