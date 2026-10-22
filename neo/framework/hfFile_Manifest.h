#ifndef __FILE_MANIFEST_H__
#define __FILE_MANIFEST_H__

/*
==============================================================

  File and preload manifests. 

==============================================================
*/

class hfFileManifest {
public:
	hfFileManifest() {
		cacheTable.SetGranularity( 4096 );
		cacheHash.SetGranularity( 4096 );
	}
	~hfFileManifest(){}

	bool LoadManifest( const char *fileName );
	bool LoadManifestFromFile( hfFile *file );
	void WriteManifestFile( const char *fileName );

	int NumFiles() {
		return cacheTable.Num();
	}

	int FindFile( const char *fileName );

	const hfStr & GetFileNameByIndex( int idx ) const;


	const char * GetManifestName() {
		return filename;
	}

	void RemoveAll( const char *filename );
	void AddFile( const char *filename );

	void PopulateList( hfStaticList< hfStr, 16384 > &dest ) {
		dest.Clear();
		for ( int i = 0; i < cacheTable.Num(); i++ ) {
			dest.Append( cacheTable[ i ] );
		}
	}

	void Print() {
		hfLib::Printf( "dump for manifest %s\n", GetManifestName() );
		hfLib::Printf( "---------------------------------------\n" );
		for ( int i = 0; i < NumFiles(); i++ ) {
			const hfStr & name = GetFileNameByIndex( i );
			if ( name.Find( ".idwav", false ) >= 0 ) {
				hfLib::Printf( "%s\n", GetFileNameByIndex( i ).c_str() );
			}
		}
	}

private:
	hfStrList cacheTable;
	hfHashIndex	cacheHash;
	hfStr filename;
};

// image preload 
struct imagePreload_s {
	imagePreload_s() {
		filter = 0;
		repeat = 0;
		usage = 0;
		cubeMap = 0;
	}
	void Write( hfFile *f ) {
		f->WriteBig( filter );
		f->WriteBig( repeat );
		f->WriteBig( usage );
		f->WriteBig( cubeMap );
	}
	void Read( hfFile *f ) {
		f->ReadBig( filter );
		f->ReadBig( repeat );
		f->ReadBig( usage );
		f->ReadBig( cubeMap );
	}
	bool operator==( const imagePreload_s &b ) const { 
		return ( filter == b.filter && repeat == b.repeat && usage == b.usage && cubeMap == b.cubeMap );
	}
	int filter;
	int repeat;
	int usage;
	int cubeMap;
};

enum preloadType_t {
	PRELOAD_IMAGE,
	PRELOAD_MODEL,
	PRELOAD_SAMPLE,
	PRELOAD_ANIM,
	PRELOAD_COLLISION,
	PRELOAD_PARTICLE
};

// preload
struct preloadEntry_s {
	preloadEntry_s() {
		resType = 0;
	}
	bool operator==( const preloadEntry_s &b ) const { 
		bool ret = ( resourceName.Icmp( b.resourceName ) == 0 );
		if ( ret && resType == PRELOAD_IMAGE ) {
			// this should never matter but...
			ret &= ( imgData == b.imgData );
		}
		return ret;
	}
	void Write( hfFile *outFile ) {
		outFile->WriteBig( resType );
		outFile->WriteString( resourceName );
		imgData.Write( outFile );
	}

	void Read( hfFile *inFile ) {
		inFile->ReadBig( resType );
		inFile->ReadString( resourceName );
		imgData.Read( inFile );
	}

	int				resType;		// type
	hfStr			resourceName;	// resource name
	imagePreload_s	imgData;		// image specific data
};

struct preloadSort_t {
	int idx;
	int ofs;
};
class hfSort_Preload : public hfSort_Quick< preloadSort_t, hfSort_Preload > {
public:
	int Compare( const preloadSort_t & a, const preloadSort_t & b ) const { return a.ofs - b.ofs; }
};

class hfPreloadManifest {
public:
	hfPreloadManifest() {
		entries.SetGranularity( 2048 );
	}
	~hfPreloadManifest(){}

	bool LoadManifest( const char *fileName );
	bool LoadManifestFromFile( hfFile *file );

	void WriteManifest( const char *fileName );
	void WriteManifestToFile(  hfFile *outFile ) {
		if ( outFile == NULL ) {
			return;
		}
		filename = outFile->GetName();
		outFile->WriteBig ( ( int )entries.Num() );
		outFile->WriteString( filename );
		for ( int i = 0; i < entries.Num(); i++ ) {
			entries[ i ].Write( outFile );
		}
	}

	int NumResources() const {
		return entries.Num();
	}

	const preloadEntry_s & GetPreloadByIndex( int idx ) const {
		return entries[ idx ];
	}

	const hfStr & GetResourceNameByIndex( int idx ) const {
		return entries[ idx ].resourceName;
	}

	const char * GetManifestName() const {
		return filename;
	}

	void Add( const preloadEntry_s & p ) {
		entries.AddUnique( p );
	}

	void AddSample( const char *_resourceName ) {
		static preloadEntry_s pe;
		pe.resType = PRELOAD_SAMPLE;
		pe.resourceName = _resourceName;
		entries.Append( pe );
	}
	void AddModel( const char *_resourceName ) {
		static preloadEntry_s pe;
		pe.resType = PRELOAD_MODEL;
		pe.resourceName = _resourceName;
		entries.Append( pe );
	}
	void AddParticle( const char *_resourceName ) {
		static preloadEntry_s pe;
		pe.resType = PRELOAD_PARTICLE;
		pe.resourceName = _resourceName;
		entries.Append( pe );
	}
	void AddAnim( const char *_resourceName ) {
		static preloadEntry_s pe;
		pe.resType = PRELOAD_ANIM;
		pe.resourceName = _resourceName;
		entries.Append( pe );
	}
	void AddCollisionModel( const char *_resourceName ) {
		static preloadEntry_s pe;
		pe.resType = PRELOAD_COLLISION;
		pe.resourceName = _resourceName;
		entries.Append( pe );
	}
	void AddImage( const char *_resourceName, int _filter, int _repeat, int _usage, int _cube ) {
		static preloadEntry_s pe;
		pe.resType = PRELOAD_IMAGE;
		pe.resourceName = _resourceName;
		pe.imgData.filter = _filter;
		pe.imgData.repeat = _repeat;
		pe.imgData.usage = _usage;
		pe.imgData.cubeMap = _cube;
		entries.Append( pe );
	}

	void Clear() {
		entries.Clear();
	}

	int FindResource( const char *name ) {
		for ( int i = 0; i < entries.Num(); i++ ) {
			if ( hfStr::Icmp( name, entries[ i ].resourceName ) == 0 ) {
				return i;
			}
		}
		return -1;
	}

	void Print() {
		hfLib::Printf( "dump for preload manifest %s\n", GetManifestName() );
		hfLib::Printf( "---------------------------------------\n" );
		for ( int i = 0; i < NumResources(); i++ ) {
			hfLib::Printf( "%s\n", GetResourceNameByIndex( i ).c_str() );
		}
	}
private:
	hfList< preloadEntry_s > entries;
	hfStr filename;
};


#endif /* !__FILE_MANIFEST_H__ */

