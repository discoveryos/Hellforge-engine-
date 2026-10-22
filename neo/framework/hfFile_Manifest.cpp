#include "../hflib/hfprecompiled.h"
#pragma hdrstop


/*
================================================================================================

hfPreloadManifest

================================================================================================
*/

/*
========================
hfPreloadManifest::LoadManifest 
========================
*/ 
bool hfPreloadManifest::LoadManifest( const char *fileName ) {
	hfFile * inFile = fileSystem->OpenFileReadMemory( fileName );
	if ( inFile != NULL ) {
		int numEntries;
		inFile->ReadBig( numEntries );
		inFile->ReadString( filename );
		entries.SetNum( numEntries );
		for ( int i = 0; i < numEntries; i++ ) {
			entries[ i ].Read( inFile );
		}
delete inFile;
		return true;
	}
	return false;
}

/*
================================================================================================

hfFileManifest

================================================================================================
*/
/*
========================
hfFileManifest::LoadManifest 
========================
*/ 
bool hfFileManifest::LoadManifest( const char *_fileName ) {
	hfFile *file = fileSystem->OpenFileRead( _fileName , false );
	if ( file != NULL ) {
		return LoadManifestFromFile( file );
	}
	return false;
}

/*
========================
hfFileManifest::LoadManifestFromFile 

// this will delete the file when finished
========================
*/ 
bool hfFileManifest::LoadManifestFromFile( hfFile *file ) {
	if ( file == NULL ) {
		return false;
	}
	filename = file->GetName();
	hfStr str;
	int num;
	file->ReadBig( num );
	cacheTable.SetNum( num );
	for ( int i = 0; i < num; i++ ) {
		file->ReadString( cacheTable[ i ] );
		//if ( FindFile( cacheTable[ i ].filename ) == NULL ) {
			// we only care about the first usage
			const int key = cacheHash.GenerateKey( cacheTable[ i ], false );
			cacheHash.Add( key, i );
		//}
	}
	delete file;
	return true;
}

/*
========================
hfFileManifest::WriteManifestFile 
========================
*/ 
void hfFileManifest::WriteManifestFile( const char *fileName ) {
	hfFile *file = fileSystem->OpenFileWrite( fileName );
	if ( file == NULL ) {
		return;
	}
	hfStr str;
	int num = cacheTable.Num();
	file->WriteBig( num );
	for ( int i = 0; i < num; i++ ) {
		file->WriteString( cacheTable[ i ] );
	}
	delete file;
}

/*
========================
hfPreloadManifest::WriteManifestFile 
========================
*/ 
void hfPreloadManifest::WriteManifest( const char *fileName ) {
	hfFile *file = fileSystem->OpenFileWrite( fileName, "fs_savepath" );
	if ( file != NULL ) {
		WriteManifestToFile( file );
		delete file;
	}
}

/*
========================
hfFileManifest::FindFile 
========================
*/ 
int hfFileManifest::FindFile( const char *fileName ) {
	const int key =cacheHash.GenerateKey( fileName, false );
	for ( int index = cacheHash.GetFirst( key ); index != hfHashIndex::NULL_INDEX; index = cacheHash.GetNext( index ) ) {
		if ( hfStr::Icmp( cacheTable[ index ], fileName ) == 0 ) {
			return index;
		}
	}
	return -1;
}

/*
========================
hfFileManifest::RemoveAll 
========================
*/ 
void hfFileManifest::RemoveAll( const char * _fileName ) {
	for ( int i = 0; i < cacheTable.Num(); i++ ) {
		if ( cacheTable[ i ].Icmp( _fileName ) == 0 ) {
			const int key =cacheHash.GenerateKey( cacheTable[ i ], false );
			cacheTable.RemoveIndex( i );
			cacheHash.RemoveIndex( key, i );
			i--;
		}
	}
}


/*
========================
hfFileManifest::GetFileNameByIndex 
========================
*/ 
const hfStr & hfFileManifest::GetFileNameByIndex( int idx ) const {
	return cacheTable[ idx ];
}



/*
=========================
hfFileManifest::AddFile 
=========================
*/ 
void hfFileManifest::AddFile( const char *fileName ) {
	//if ( FindFile( fileName ) == NULL ) {
		// we only care about the first usage
		const int key = cacheHash.GenerateKey( fileName, false );
		int idx = cacheTable.Append( fileName );
		cacheHash.Add( key, idx );
	//}
}




