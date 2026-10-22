#include "../hflib/hfprecompiled.h"
#pragma hdrstop

/*
================================================================================================

hfResourceContainer

================================================================================================
*/

/*
========================
hfResourceContainer::ReOpen 
========================
*/ 
void hfResourceContainer::ReOpen() {
	delete resourceFile;
	resourceFile = fileSystem->OpenFileRead( fileName );
}

/*
========================
hfResourceContainer::Init 
========================
*/ 
bool hfResourceContainer::Init( const char *_fileName, uint8 containerIndex ) {

	if ( hfStr::Icmp( _fileName, "_ordered.resources" ) == 0 ) {
		resourceFile = fileSystem->OpenFileReadMemory( _fileName );
	} else {
		resourceFile = fileSystem->OpenFileRead( _fileName );
	}

	if ( resourceFile == NULL ) {
		hfLib::Warning( "Unable to open resource file %s", _fileName );
		return false;
	}

	resourceFile->ReadBig( resourceMagic );
	if ( resourceMagic != RESOURCE_FILE_MAGIC ) {
		hfLib::FatalError( "resourceFileMagic != RESOURCE_FILE_MAGIC" );
	}

	fileName = _fileName;

	resourceFile->ReadBig( tableOffset );
	resourceFile->ReadBig( tableLength );
	// read this into a memory buffer with a single read
	char * const buf = (char *)Mem_Alloc( tableLength, TAG_RESOURCE );
	resourceFile->Seek( tableOffset, FS_SEEK_SET );
	resourceFile->Read( buf, tableLength );
	hfFile_Memory memFile( "resourceHeader", (const char *)buf, tableLength );

	// Parse the resourceFile header, which includes every resource used
	// by the game.
	memFile.ReadBig( numFileResources );

	cacheTable.SetNum( numFileResources );

	for ( int i = 0; i < numFileResources; i++ ) {
		hfResourceCacheEntry &rt = cacheTable[ i ];
		rt.Read( &memFile );
		rt.filename.BackSlashesToSlashes();
		rt.filename.ToLower();
		rt.containerIndex = containerIndex;

		const int key = cacheHash.GenerateKey( rt.filename, false );
		bool found = false;
		//for ( int index = cacheHash.GetFirst( key ); index != hfHashIndex::NULL_INDEX; index = cacheHash.GetNext( index ) ) {
		//	hfResourceCacheEntry & rtc = cacheTable[ index ];
		//	if ( hfStr::Icmp( rtc.filename, rt.filename ) == 0 ) {
		//		found = true;
		//		break;
		//	}
		//}
		if ( !found ) {
			//hfLib::Printf( "rez file name: %s\n", rt.filename.c_str() );
			cacheHash.Add( key, i );
		}
	}
	Mem_Free( buf );

	return true;
}


/*
========================
hfResourceContainer::WriteManifestFile 
========================
*/ 
void hfResourceContainer::WriteManifestFile( const char *name, const hfStrList &list ) {
	hfStr filename( name );
	filename.SetFileExtension( "manifest" );
	filename.Insert( "maps/", 0 );
	hfFile *outFile = fileSystem->OpenFileWrite( filename );
	if ( outFile != NULL ) {
		int num = list.Num();
		outFile->WriteBig( num );
		for ( int i = 0; i < num; i++ ) {
			outFile->WriteString( list[ i ] );
		}
		delete outFile;
	}
}

/*
========================
hfResourceContainer::ReadManifestFile 
========================
*/ 
int hfResourceContainer::ReadManifestFile( const char *name, hfStrList &list ) {
	hfFile *inFile = fileSystem->OpenFileRead( name );
	if ( inFile != NULL ) {
		list.SetGranularity( 16384 );
		hfStr str;
		int num;
		list.Clear();
		inFile->ReadBig( num );
		for ( int i = 0; i < num; i++ ) {
			inFile->ReadString( str );
			list.Append( str );
		}
		delete inFile;
	}
	return list.Num();
}


/*
========================
hfResourceContainer::UpdateResourceFile 
========================
*/ 
void hfResourceContainer::UpdateResourceFile( const char *_filename, const hfStrList &_filesToUpdate ) {
	hfFile *outFile = fileSystem->OpenFileWrite( va( "%s.new", _filename ) );
	if ( outFile == NULL ) {
		hfLib::Warning( "Unable to open resource file %s or new output file", _filename );
		return;
	}

	uint32 magic = 0;
	int _tableOffset = 0;
	int _tableLength = 0;
	hfList< hfResourceCacheEntry > entries;
	hfStrList filesToUpdate = _filesToUpdate;

	hfFile *inFile = fileSystem->OpenFileRead( _filename );
	if ( inFile == NULL ) {
		magic = RESOURCE_FILE_MAGIC;

		outFile->WriteBig( magic );
		outFile->WriteBig( _tableOffset );
		outFile->WriteBig( _tableLength );

	} else {
		inFile->ReadBig( magic );
		if ( magic != RESOURCE_FILE_MAGIC ) {
			delete inFile;
			return;
		}

		inFile->ReadBig( _tableOffset );
		inFile->ReadBig( _tableLength );
		// read this into a memory buffer with a single read
		char * const buf = (char *)Mem_Alloc( _tableLength, TAG_RESOURCE );
		inFile->Seek( _tableOffset, FS_SEEK_SET );
		inFile->Read( buf, _tableLength );
		hfFile_Memory memFile( "resourceHeader", (const char *)buf, _tableLength );

		int _numFileResources = 0;
		memFile.ReadBig( _numFileResources );

		outFile->WriteBig( magic );
		outFile->WriteBig( _tableOffset );
		outFile->WriteBig( _tableLength );

		entries.SetNum( _numFileResources );

		for ( int i = 0; i < _numFileResources; i++ ) {
			entries[ i ].Read( &memFile );


			hfLib::Printf( "examining %s\n", entries[ i ].filename.c_str() );
			byte * fileData = NULL;

			for ( int j = filesToUpdate.Num() - 1; j >= 0; j-- ) {
				if ( filesToUpdate[ j ].Icmp( entries[ i ].filename ) == 0 ) {
					hfFile *newFile = fileSystem->OpenFileReadMemory( filesToUpdate[ j ] );
					if ( newFile != NULL ) {
						hfLib::Printf( "Updating %s\n", filesToUpdate[ j ].c_str() );
						entries[ i ].length = newFile->Length();
						fileData = (byte *)Mem_Alloc( entries[ i ].length, TAG_TEMP );
						newFile->Read( fileData, newFile->Length() );
						delete newFile;
					}
					filesToUpdate.RemoveIndex( j );
				}
			}

			if ( fileData == NULL ) {
				inFile->Seek( entries[ i ].offset, FS_SEEK_SET );
				fileData = (byte *)Mem_Alloc( entries[ i ].length, TAG_TEMP );
				inFile->Read( fileData, entries[ i ].length );
			}

			entries[ i ].offset = outFile->Tell();
			outFile->Write( ( void* )fileData, entries[ i ].length );

			Mem_Free( fileData );
		}

		Mem_Free( buf );
	}

	while ( filesToUpdate.Num() > 0 ) {
		hfFile *newFile = fileSystem->OpenFileReadMemory( filesToUpdate[ 0 ] );
		if ( newFile != NULL ) {
			hfLib::Printf( "Appending %s\n", filesToUpdate[ 0 ].c_str() );
			hfResourceCacheEntry rt;
			rt.filename = filesToUpdate[ 0 ];
			rt.length = newFile->Length();
			byte * fileData = (byte *)Mem_Alloc( rt.length, TAG_TEMP );
			newFile->Read( fileData, rt.length );
			int idx = entries.Append( rt );
			if ( idx >= 0 ) {
				entries[ idx ].offset = outFile->Tell();
				outFile->Write( ( void* )fileData, entries[ idx ].length );
			}
			delete newFile;
			Mem_Free( fileData );
		}
		filesToUpdate.RemoveIndex( 0 );
	}

	_tableOffset = outFile->Tell();
	outFile->WriteBig( entries.Num() );

	// write the individual resource entries
	for ( int i = 0; i < entries.Num(); i++ ) {
		entries[ i ].Write( outFile );
	}

	// go back and write the header offsets again, now that we have file offsets and lengths
	_tableLength = outFile->Tell() - _tableOffset;
	outFile->Seek( 0, FS_SEEK_SET );
	outFile->WriteBig( magic );
	outFile->WriteBig( _tableOffset );
	outFile->WriteBig( _tableLength );

	delete outFile;
	delete inFile;
}


/*
========================
hfResourceContainer::ExtractResourceFile 
========================
*/ 
void hfResourceContainer::SetContainerIndex( const int & _idx ) {
	for ( int i = 0; i < cacheTable.Num(); i++ ) {
		cacheTable[ i ].containerIndex = _idx;
	}
}

/*
========================
hfResourceContainer::ExtractResourceFile 
========================
*/ 
void hfResourceContainer::ExtractResourceFile ( const char * _fileName, const char * _outPath, bool _copyWavs ) {
	hfFile *inFile = fileSystem->OpenFileRead( _fileName );

	if ( inFile == NULL ) {
		hfLib::Warning( "Unable to open resource file %s", _fileName );
		return;
	}

	uint32 magic;
	inFile->ReadBig( magic );
	if ( magic != RESOURCE_FILE_MAGIC ) {
		delete inFile;
		return;
	}

	int _tableOffset;
	int _tableLength;
	inFile->ReadBig( _tableOffset );
	inFile->ReadBig( _tableLength );
	// read this into a memory buffer with a single read
	char * const buf = (char *)Mem_Alloc( _tableLength, TAG_RESOURCE );
	inFile->Seek( _tableOffset, FS_SEEK_SET );
	inFile->Read( buf, _tableLength );
	hfFile_Memory memFile( "resourceHeader", (const char *)buf, _tableLength );

	int _numFileResources;
	memFile.ReadBig( _numFileResources );

	for ( int i = 0; i < _numFileResources; i++ ) {
		hfResourceCacheEntry rt;
		rt.Read( &memFile );
		rt.filename.BackSlashesToSlashes();
		rt.filename.ToLower();
		byte *fbuf = NULL;
		if ( _copyWavs && ( rt.filename.Find( ".idwav" ) >= 0 ||  rt.filename.Find( ".idxma" ) >= 0 ||  rt.filename.Find( ".idmsf" ) >= 0 ) ) {
			rt.filename.SetFileExtension( "wav" );
			rt.filename.Replace( "generated/", "" );
			int len = fileSystem->GetFileLength( rt.filename );
			fbuf =  (byte *)Mem_Alloc( len, TAG_RESOURCE );
			fileSystem->ReadFile( rt.filename, (void**)&fbuf, NULL );
		} else {
			inFile->Seek( rt.offset, FS_SEEK_SET );
			fbuf =  (byte *)Mem_Alloc( rt.length, TAG_RESOURCE );
			inFile->Read( fbuf, rt.length );
		}
		hfStr outName = _outPath;
		outName.AppendPath( rt.filename );
		hfFile *outFile = fileSystem->OpenExplicitFileWrite( outName );
		if ( outFile != NULL ) {
			outFile->Write( ( byte* )fbuf, rt.length );
			delete outFile;
		}
		Mem_Free( fbuf );
	}
	delete inFile;
	Mem_Free( buf );
}



/*
========================
hfResourceContainer::Open 
========================
*/ 
void hfResourceContainer::WriteResourceFile( const char *manifestName, const hfStrList &manifest, const bool &_writeManifest ) {

	if ( manifest.Num() == 0 ) {
		return;
	}

	hfLib::Printf( "Writing resource file %s\n", manifestName );

	// build multiple output files at 1GB each
	hfList < hfStrList > outPutFiles;

	hfFileManifest outManifest;
	int64 size = 0;
	hfStrList flist;
	flist.SetGranularity( 16384 );
	for ( int i = 0; i < manifest.Num(); i++ ) {
		flist.Append( manifest[ i ] );
		size += fileSystem->GetFileLength( manifest[ i ] );
		if ( size > 1024 * 1024 * 1024 ) {
			outPutFiles.Append( flist );
			size = 0;
			flist.Clear();
		}
		outManifest.AddFile( manifest[ i ] );
	}

	outPutFiles.Append( flist );

	if ( _writeManifest ) {
		hfStr temp = manifestName;
		temp.Replace( "maps/", "manifests/" );
		temp.StripFileExtension();
		temp.SetFileExtension( "manifest" );
		outManifest.WriteManifestFile( temp );
	}

	for ( int idx = 0; idx < outPutFiles.Num(); idx++ ) {

		hfStrList &fileList = outPutFiles[ idx ];
		if ( fileList.Num() == 0 ) {
			continue;
		}

		hfStr fileName = manifestName;
		if ( idx > 0 ) {
			fileName = va( "%s_%02d", manifestName, idx );
		} 
		fileName.SetFileExtension( "resources" );

		hfFile *resFile = fileSystem->OpenFileWrite( fileName );

		if ( resFile == NULL ) {
			hfLib::Warning( "Cannot open %s for writing.\n", fileName.c_str() );
			return;
		}

		hfLib::Printf( "Writing resource file %s\n", fileName.c_str() );

		int	tableOffset = 0;
		int	tableLength = 0;
		int	tableNewLength = 0;
		uint32	resourceFileMagic = RESOURCE_FILE_MAGIC;

		resFile->WriteBig( resourceFileMagic );
		resFile->WriteBig( tableOffset );
		resFile->WriteBig( tableLength );

		hfList< hfResourceCacheEntry > entries;

		entries.Resize( fileList.Num() );

		for ( int i = 0; i < fileList.Num(); i++ ) {
			hfResourceCacheEntry ent;

			ent.filename = fileList[ i ];
			ent.length = 0;
			ent.offset = 0;

			hfFile *file = fileSystem->OpenFileReadMemory( ent.filename, false );
			hfFile_Memory *fm = dynamic_cast< hfFile_Memory* >( file );
			if ( fm == NULL ) {
				continue;
			}
			// if the entry is uncompressed, align the file pointer to a 16 byte boundary
			// so it will be usable if memory mapped
			ent.length = fm->Length();

			// always get the offset, even if the file will have zero length
			ent.offset = resFile->Tell();

			entries.Append( ent );

			if ( ent.length == 0 ) {
				ent.filename = "";
				delete fm;
				continue;
			}

			resFile->Write( fm->GetDataPtr(), ent.length );

			delete fm;

			// pacifier every ten megs
			if ( ( ent.offset + ent.length ) / 10000000 != ent.offset / 10000000 ) {
				hfLib::Printf( "." );
			}
		}

		hfLib::Printf( "\n" );

		// write the table out now that we have all the files
		tableOffset = resFile->Tell();

		// count how many we are going to write for this platform
		int	numFileResources = entries.Num();

		resFile->WriteBig( numFileResources );

		// write the individual resource entries
		for ( int i = 0; i < entries.Num(); i++ ) {
			entries[ i ].Write( resFile );
			if ( i + 1 == numFileResources ) {
				// we just wrote out the last new entry
				tableNewLength = resFile->Tell() - tableOffset;
			}
		}

		// go back and write the header offsets again, now that we have file offsets and lengths
		tableLength = resFile->Tell() - tableOffset;
		resFile->Seek( 0, FS_SEEK_SET );
		resFile->WriteBig( resourceFileMagic );
		resFile->WriteBig( tableOffset );
		resFile->WriteBig( tableLength );
		delete resFile;
	}
}

