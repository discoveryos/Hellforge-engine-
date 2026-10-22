#include "../hflib/hfprecompiled.h"
#pragma hdrstop

hfCVar hfDemoFile::com_logDemos( "com_logDemos", "0", CVAR_SYSTEM | CVAR_BOOL, "Write demo.log with debug information in it" );
hfCVar hfDemoFile::com_compressDemos( "com_compressDemos", "1", CVAR_SYSTEM | CVAR_INTEGER | CVAR_ARCHIVE, "Compression scheme for demo files\n0: None    (Fast, large files)\n1: LZW     (Fast to compress, Fast to decompress, medium/small files)\n2: LZSS    (Slow to compress, Fast to decompress, small files)\n3: Huffman (Fast to compress, Slow to decompress, medium files)\nSee also: The 'CompressDemo' command" );
hfCVar hfDemoFile::com_preloadDemos( "com_preloadDemos", "0", CVAR_SYSTEM | CVAR_BOOL | CVAR_ARCHIVE, "Load the whole demo in to RAM before running it" );

#define DEMO_MAGIC GAME_NAME " RDEMO"

/*
================
hfDemoFile::hfDemoFile
================
*/
hfDemoFile::hfDemoFile() {
	f = NULL;
	fLog = NULL;
	log = false;
	fileImage = NULL;
	compressor = NULL;
	writing = false;
}

/*
================
hfDemoFile::~hfDemoFile
================
*/
hfDemoFile::~hfDemoFile() {
	Close();
}

/*
================
hfDemoFile::AllocCompressor
================
*/
hfCompressor *hfDemoFile::AllocCompressor( int type ) {
	switch ( type ) {
	case 0: return hfCompressor::AllocNoCompression();
	default:
	case 1: return hfCompressor::AllocLZW();
	case 2: return hfCompressor::AllocLZSS();
	case 3: return hfCompressor::AllocHuffman();
	}
}

/*
================
hfDemoFile::OpenForReading
================
*/
bool hfDemoFile::OpenForReading( const char *fileName ) {
	static const int magicLen = sizeof(DEMO_MAGIC) / sizeof(DEMO_MAGIC[0]);
	char magicBuffer[magicLen];
	int compression;
	int fileLength;

	Close();

	f = fileSystem->OpenFileRead( fileName );
	if ( !f ) {
		return false;
	}

	fileLength = f->Length();

	if ( com_preloadDemos.GetBool() ) {
		fileImage = (byte *)Mem_Alloc( fileLength, TAG_CRAP );
		f->Read( fileImage, fileLength );
		fileSystem->CloseFile( f );
		f = new (TAG_SYSTEM) hfFile_Memory( va( "preloaded(%s)", fileName ), (const char *)fileImage, fileLength );
	}

	if ( com_logDemos.GetBool() ) {
		fLog = fileSystem->OpenFileWrite( "demoread.log" );
	}

	writing = false;

	f->Read(magicBuffer, magicLen);
	if ( memcmp(magicBuffer, DEMO_MAGIC, magicLen) == 0 ) {
		f->ReadInt( compression );
	} else {
		// Ideally we would error out if the magic string isn't there,
		// but for backwards compatibility we are going to assume it's just an uncompressed demo file
		compression = 0;
		f->Rewind();
	}

	compressor = AllocCompressor( compression );
	compressor->Init( f, false, 8 );

	return true;
}

/*
================
hfDemoFile::SetLog
================
*/
void hfDemoFile::SetLog(bool b, const char *p) {
	log = b;
	if (p) {
		logStr = p;
	}
}

/*
================
hfDemoFile::Log
================
*/
void hfDemoFile::Log(const char *p) {
	if ( fLog && p && *p ) {
		fLog->Write( p, strlen(p) );
	}
}

/*
================
hfDemoFile::OpenForWriting
================
*/
bool hfDemoFile::OpenForWriting( const char *fileName ) {
	Close();

	f = fileSystem->OpenFileWrite( fileName );
	if ( f == NULL ) {
		return false;
	}

	if ( com_logDemos.GetBool() ) {
		fLog = fileSystem->OpenFileWrite( "demowrite.log" );
	}

	writing = true;

	f->Write(DEMO_MAGIC, sizeof(DEMO_MAGIC));
	f->WriteInt( com_compressDemos.GetInteger() );
	f->Flush();

	compressor = AllocCompressor( com_compressDemos.GetInteger() );
	compressor->Init( f, true, 8 );

	return true;
}

/*
================
hfDemoFile::Close
================
*/
void hfDemoFile::Close() {
	if ( writing && compressor ) {
		compressor->FinishCompress();
	}

	if ( f ) {
		fileSystem->CloseFile( f );
		f = NULL;
	}
	if ( fLog ) {
		fileSystem->CloseFile( fLog );
		fLog = NULL;
	}
	if ( fileImage ) {
		Mem_Free( fileImage );
		fileImage = NULL;
	}
	if ( compressor ) {
		delete compressor;
		compressor = NULL;
	}

	demoStrings.DeleteContents( true );
}

/*
================
hfDemoFile::ReadHashString
================
*/
const char *hfDemoFile::ReadHashString() {
	int		index;

	if ( log && fLog ) {
		const char *text = va( "%s > Reading hash string\n", logStr.c_str() );
		fLog->Write( text, strlen( text ) );
	} 

	ReadInt( index );

	if ( index == -1 ) {
		// read a new string for the table
		hfStr	*str = new (TAG_SYSTEM) hfStr;
		
		hfStr data;
		ReadString( data );
		*str = data;
		
		demoStrings.Append( str );

		return *str;
	}

	if ( index < -1 || index >= demoStrings.Num() ) {
		Close();
		common->Error( "demo hash index out of range" );
	}

	return demoStrings[index]->c_str();
}

/*
================
hfDemoFile::WriteHashString
================
*/
void hfDemoFile::WriteHashString( const char *str ) {
	if ( log && fLog ) {
		const char *text = va( "%s > Writing hash string\n", logStr.c_str() );
		fLog->Write( text, strlen( text ) );
	}
	// see if it is already in the has table
	for ( int i = 0 ; i < demoStrings.Num() ; i++ ) {
		if ( !strcmp( demoStrings[i]->c_str(), str ) ) {
			WriteInt( i );
			return;
		}
	}

	// add it to our table and the demo table
	hfStr	*copy = new (TAG_SYSTEM) hfStr( str );
//common->Printf( "hash:%i = %s\n", demoStrings.Num(), str );
	demoStrings.Append( copy );
	int cmd = -1;	
	WriteInt( cmd );
	WriteString( str );
}

/*
================
hfDemoFile::ReadDict
================
*/
void hfDemoFile::ReadDict( hfDict &dict ) {
	int i, c;
	hfStr key, val;

	dict.Clear();
	ReadInt( c );
	for ( i = 0; i < c; i++ ) {
		key = ReadHashString();
		val = ReadHashString();
		dict.Set( key, val );
	}
}

/*
================
hfDemoFile::WriteDict
================
*/
void hfDemoFile::WriteDict( const hfDict &dict ) {
	int i, c;

	c = dict.GetNumKeyVals();
	WriteInt( c );
	for ( i = 0; i < c; i++ ) {
		WriteHashString( dict.GetKeyVal( i )->GetKey() );
		WriteHashString( dict.GetKeyVal( i )->GetValue() );
	}
}

/*
 ================
 hfDemoFile::Read
 ================
 */
int hfDemoFile::Read( void *buffer, int len ) {
	int read = compressor->Read( buffer, len );
	if ( read == 0 && len >= 4 ) {
		*(demoSystem_t *)buffer = DS_FINISHED;
	}
	return read;
}

/*
 ================
 hfDemoFile::Write
 ================
 */
int hfDemoFile::Write( const void *buffer, int len ) {
	return compressor->Write( buffer, len );
}





