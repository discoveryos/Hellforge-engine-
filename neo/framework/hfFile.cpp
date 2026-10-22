#include "../hflib/hfprecompiled.h"
#pragma hdrstop

#include "hfUnzip.h"

/*
=================
FS_WriteFloatString
=================
*/
int FS_WriteFloatString( char *buf, const char *fmt, va_list argPtr ) {
	long i;
	unsigned long u;
	double f;
	char *str;
	int index;
	hfStr tmp, format;

	index = 0;

	while( *fmt ) {
		switch( *fmt ) {
			case '%':
				format = "";
				format += *fmt++;
				while ( (*fmt >= '0' && *fmt <= '9') ||
						*fmt == '.' || *fmt == '-' || *fmt == '+' || *fmt == '#') {
					format += *fmt++;
				}
				format += *fmt;
				switch( *fmt ) {
					case 'f':
					case 'e':
					case 'E':
					case 'g':
					case 'G':
						f = va_arg( argPtr, double );
						if ( format.Length() <= 2 ) {
							// high precision floating point number without trailing zeros
							sprintf( tmp, "%1.10f", f );
							tmp.StripTrailing( '0' );
							tmp.StripTrailing( '.' );
							index += sprintf( buf+index, "%s", tmp.c_str() );
						}
						else {
							index += sprintf( buf+index, format.c_str(), f );
						}
						break;
					case 'd':
					case 'i':
						i = va_arg( argPtr, long );
						index += sprintf( buf+index, format.c_str(), i );
						break;
					case 'u':
						u = va_arg( argPtr, unsigned long );
						index += sprintf( buf+index, format.c_str(), u );
						break;
					case 'o':
						u = va_arg( argPtr, unsigned long );
						index += sprintf( buf+index, format.c_str(), u );
						break;
					case 'x':
						u = va_arg( argPtr, unsigned long );
						index += sprintf( buf+index, format.c_str(), u );
						break;
					case 'X':
						u = va_arg( argPtr, unsigned long );
						index += sprintf( buf+index, format.c_str(), u );
						break;
					case 'c':
						i = va_arg( argPtr, long );
						index += sprintf( buf+index, format.c_str(), (char) i );
						break;
					case 's':
						str = va_arg( argPtr, char * );
						index += sprintf( buf+index, format.c_str(), str );
						break;
					case '%':
						index += sprintf( buf+index, format.c_str() ); //-V618
						break;
					default:
						common->Error( "FS_WriteFloatString: invalid format %s", format.c_str() );
						break;
				}
				fmt++;
				break;
			case '\\':
				fmt++;
				switch( *fmt ) {
					case 't':
						index += sprintf( buf+index, "\t" );
						break;
					case 'v':
						index += sprintf( buf+index, "\v" );
						break;
					case 'n':
						index += sprintf( buf+index, "\n" );
						break;
					case '\\':
						index += sprintf( buf+index, "\\" );
						break;
					default:
						common->Error( "FS_WriteFloatString: unknown escape character \'%c\'", *fmt );
						break;
				}
				fmt++;
				break;
			default:
				index += sprintf( buf+index, "%c", *fmt );
				fmt++;
				break;
		}
	}

	return index;
}

/*
=================================================================================

hfFile

=================================================================================
*/

/*
=================
hfFile::GetName
=================
*/
const char *hfFile::GetName() const {
	return "";
}

/*
=================
hfFile::GetFullPath
=================
*/
const char *hfFile::GetFullPath() const {
	return "";
}

/*
=================
hfFile::Read
=================
*/
int hfFile::Read( void *buffer, int len ) {
	common->FatalError( "hfFile::Read: cannot read from hfFile" );
	return 0;
}

/*
=================
hfFile::Write
=================
*/
int hfFile::Write( const void *buffer, int len ) {
	common->FatalError( "hfFile::Write: cannot write to hfFile" );
	return 0;
}

/*
=================
hfFile::Length
=================
*/
int hfFile::Length() const {
	return 0;
}

/*
=================
hfFile::Timestamp
=================
*/
HF_TIME_T hfFile::Timestamp() const {
	return 0;
}

/*
=================
hfFile::Tell
=================
*/
int hfFile::Tell() const {
	return 0;
}

/*
=================
hfFile::ForceFlush
=================
*/
void hfFile::ForceFlush() {
}

/*
=================
hfFile::Flush
=================
*/
void hfFile::Flush() {
}

/*
=================
hfFile::Seek
=================
*/
int hfFile::Seek( long offset, fsOrigin_t origin ) {
	return -1;
}

/*
=================
hfFile::Rewind
=================
*/
void hfFile::Rewind() {
	Seek( 0, FS_SEEK_SET );
}

/*
=================
hfFile::Printf
=================
*/
int hfFile::Printf( const char *fmt, ... ) {
	char buf[MAX_PRINT_MSG];
	int length;
	va_list argptr;

	va_start( argptr, fmt );
	length = hfStr::vsnPrintf( buf, MAX_PRINT_MSG-1, fmt, argptr );
	va_end( argptr );

	// so notepad formats the lines correctly
  	hfStr	work( buf );
 	work.Replace( "\n", "\r\n" );
  
  	return Write( work.c_str(), work.Length() );
}

/*
=================
hfFile::VPrintf
=================
*/
int hfFile::VPrintf( const char *fmt, va_list args ) {
	char buf[MAX_PRINT_MSG];
	int length;

	length = hfStr::vsnPrintf( buf, MAX_PRINT_MSG-1, fmt, args );
	return Write( buf, length );
}

/*
=================
hfFile::WriteFloatString
=================
*/
int hfFile::WriteFloatString( const char *fmt, ... ) {
	char buf[MAX_PRINT_MSG];
	int len;
	va_list argPtr;

	va_start( argPtr, fmt );
	len = FS_WriteFloatString( buf, fmt, argPtr );
	va_end( argPtr );

	return Write( buf, len );
}

/*
 =================
 hfFile::ReadInt
 =================
 */
int hfFile::ReadInt( int &value ) {
	int result = Read( &value, sizeof( value ) );
	value = LittleLong(value);
	return result;
}

/*
 =================
 hfFile::ReadUnsignedInt
 =================
 */
int hfFile::ReadUnsignedInt( unsigned int &value ) {
	int result = Read( &value, sizeof( value ) );
	value = LittleLong(value);
	return result;
}

/*
 =================
 hfFile::ReadShort
 =================
 */
int hfFile::ReadShort( short &value ) {
	int result = Read( &value, sizeof( value ) );
	value = LittleShort(value);
	return result;
}

/*
 =================
 hfFile::ReadUnsignedShort
 =================
 */
int hfFile::ReadUnsignedShort( unsigned short &value ) {
	int result = Read( &value, sizeof( value ) );
	value = LittleShort(value);
	return result;
}

/*
 =================
 hfFile::ReadChar
 =================
 */
int hfFile::ReadChar( char &value ) {
	return Read( &value, sizeof( value ) );
}

/*
 =================
 hfFile::ReadUnsignedChar
 =================
 */
int hfFile::ReadUnsignedChar( unsigned char &value ) {
	return Read( &value, sizeof( value ) );
}

/*
 =================
 hfFile::ReadFloat
 =================
 */
int hfFile::ReadFloat( float &value ) {
	int result = Read( &value, sizeof( value ) );
	value = LittleFloat(value);
	return result;
}

/*
 =================
 hfFile::ReadBool
 =================
 */
int hfFile::ReadBool( bool &value ) {
	unsigned char c;
	int result = ReadUnsignedChar( c );
	value = c ? true : false;
	return result;
}

/*
 =================
 hfFile::ReadString
 =================
 */
int hfFile::ReadString( hfStr &string ) {
	int len;
	int result = 0;
	
	ReadInt( len );
	if ( len >= 0 ) {
		string.Fill( ' ', len );
		result = Read( &string[ 0 ], len );
	}
	return result;
}

/*
 =================
 hfFile::ReadVec2
 =================
 */
int hfFile::ReadVec2( hfVec2 &vec ) {
	int result = Read( &vec, sizeof( vec ) );
	LittleRevBytes( &vec, sizeof(float), sizeof(vec)/sizeof(float) );
	return result;
}

/*
 =================
 hfFile::ReadVec3
 =================
 */
int hfFile::ReadVec3( hfVec3 &vec ) {
	int result = Read( &vec, sizeof( vec ) );
	LittleRevBytes( &vec, sizeof(float), sizeof(vec)/sizeof(float) );
	return result;
}

/*
 =================
 hfFile::ReadVec4
 =================
 */
int hfFile::ReadVec4( hfVec4 &vec ) {
	int result = Read( &vec, sizeof( vec ) );
	LittleRevBytes( &vec, sizeof(float), sizeof(vec)/sizeof(float) );
	return result;
}

/*
 =================
 hfFile::ReadVec6
 =================
 */
int hfFile::ReadVec6( hfVec6 &vec ) {
	int result = Read( &vec, sizeof( vec ) );
	LittleRevBytes( &vec, sizeof(float), sizeof(vec)/sizeof(float) );
	return result;
}

/*
 =================
 hfFile::ReadMat3
 =================
 */
int hfFile::ReadMat3( hfMat3 &mat ) {
	int result = Read( &mat, sizeof( mat ) );
	LittleRevBytes( &mat, sizeof(float), sizeof(mat)/sizeof(float) );
	return result;
}

/*
 =================
 hfFile::WriteInt
 =================
 */
int hfFile::WriteInt( const int value ) {
	int v = LittleLong(value);
	return Write( &v, sizeof( v ) );
}

/*
 =================
 hfFile::WriteUnsignedInt
 =================
 */
int hfFile::WriteUnsignedInt( const unsigned int value ) {
	unsigned int v = LittleLong(value);
	return Write( &v, sizeof( v ) );
}

/*
 =================
 hfFile::WriteShort
 =================
 */
int hfFile::WriteShort( const short value ) {
	short v = LittleShort(value);
	return Write( &v, sizeof( v ) );
}

/*
 =================
 hfFile::WriteUnsignedShort
 =================
 */
int hfFile::WriteUnsignedShort( const unsigned short value ) {
	unsigned short v = LittleShort(value);
	return Write( &v, sizeof( v ) );
}

/*
 =================
 hfFile::WriteChar
 =================
 */
int hfFile::WriteChar( const char value ) {
	return Write( &value, sizeof( value ) );
}

/*
 =================
 hfFile::WriteUnsignedChar
 =================
 */
int hfFile::WriteUnsignedChar( const unsigned char value ) {
	return Write( &value, sizeof( value ) );
}

/*
 =================
 hfFile::WriteFloat
 =================
 */
int hfFile::WriteFloat( const float value ) {
	float v = LittleFloat(value);
	return Write( &v, sizeof( v ) );
}

/*
 =================
 hfFile::WriteBool
 =================
 */
int hfFile::WriteBool( const bool value ) {
	unsigned char c = value;
	return WriteUnsignedChar( c );
}

/*
 =================
 hfFile::WriteString
 =================
 */
int hfFile::WriteString( const char *value ) {
	int len = strlen( value );
	WriteInt( len );
    return Write( value, len );
}

/*
 =================
 hfFile::WriteVec2
 =================
 */
int hfFile::WriteVec2( const hfVec2 &vec ) {
	hfVec2 v = vec;
	LittleRevBytes( &v, sizeof(float), sizeof(v)/sizeof(float) );
	return Write( &v, sizeof( v ) );
}

/*
 =================
 hfFile::WriteVec3
 =================
 */
int hfFile::WriteVec3( const hfVec3 &vec ) {
	hfVec3 v = vec;
	LittleRevBytes( &v, sizeof(float), sizeof(v)/sizeof(float) );
	return Write( &v, sizeof( v ) );
}

/*
 =================
 hfFile::WriteVec4
 =================
 */
int hfFile::WriteVec4( const hfVec4 &vec ) {
	hfVec4 v = vec;
	LittleRevBytes( &v, sizeof(float), sizeof(v)/sizeof(float) );
	return Write( &v, sizeof( v ) );
}

/*
 =================
 hfFile::WriteVec6
 =================
 */
int hfFile::WriteVec6( const hfVec6 &vec ) {
	hfVec6 v = vec;
	LittleRevBytes( &v, sizeof(float), sizeof(v)/sizeof(float) );
	return Write( &v, sizeof( v ) );
}

/*
 =================
 hfFile::WriteMat3
 =================
 */
int hfFile::WriteMat3( const hfMat3 &mat ) {
	hfMat3 v = mat;
	LittleRevBytes(&v, sizeof(float), sizeof(v)/sizeof(float) );
	return Write( &v, sizeof( v ) );
}

/*
=================================================================================

hfFile_Memory

=================================================================================
*/


/*
=================
hfFile_Memory::hfFile_Memory
=================
*/
hfFile_Memory::hfFile_Memory() {
	name = "*unknown*";
	maxSize = 0;
	fileSize = 0;
	allocated = 0;
	granularity = 16384;

	mode = ( 1 << FS_WRITE );
	filePtr = NULL;
	curPtr = NULL;
}

/*
=================
hfFile_Memory::hfFile_Memory
=================
*/
hfFile_Memory::hfFile_Memory( const char *name ) {
	this->name = name;
	maxSize = 0;
	fileSize = 0;
	allocated = 0;
	granularity = 16384;

	mode = ( 1 << FS_WRITE );
	filePtr = NULL;
	curPtr = NULL;
}

/*
=================
hfFile_Memory::hfFile_Memory
=================
*/
hfFile_Memory::hfFile_Memory( const char *name, char *data, int length ) {
	this->name = name;
	maxSize = length;
	fileSize = 0;
	allocated = length;
	granularity = 16384;

	mode = ( 1 << FS_WRITE );
	filePtr = data;
	curPtr = data;
}

/*
=================
hfFile_Memory::hfFile_Memory
=================
*/
hfFile_Memory::hfFile_Memory( const char *name, const char *data, int length ) {
	this->name = name;
	maxSize = 0;
	fileSize = length;
	allocated = 0;
	granularity = 16384;

	mode = ( 1 << FS_READ );
	filePtr = const_cast<char *>(data);
	curPtr = const_cast<char *>(data);
}

/*
=================
hfFile_Memory::TakeDataOwnership

this also makes the file read only
=================
*/
void hfFile_Memory::TakeDataOwnership() {
	if ( filePtr != NULL && fileSize > 0 ) {
		maxSize = 0;
		mode = ( 1 << FS_READ );
		allocated = fileSize;
	}
}

/*
=================
hfFile_Memory::~hfFile_Memory
=================
*/
hfFile_Memory::~hfFile_Memory() {
	if ( filePtr && allocated > 0 && maxSize == 0 ) {
		Mem_Free( filePtr );
	}
}

/*
=================
hfFile_Memory::Read
=================
*/
int hfFile_Memory::Read( void *buffer, int len ) {

	if ( !( mode & ( 1 << FS_READ ) ) ) {
		common->FatalError( "hfFile_Memory::Read: %s not opened in read mode", name.c_str() );
		return 0;
	}

	if ( curPtr + len > filePtr + fileSize ) {
		len = filePtr + fileSize - curPtr;
	}
	memcpy( buffer, curPtr, len );
	curPtr += len;
	return len;
}

hfCVar memcpyImpl( "memcpyImpl", "0", 0, "Which implementation of memcpy to use for hfFile_Memory::Write() [0/1 - standard (1 eliminates branch misprediction), 2 - auto-vectorized]" );
void * memcpy2( void * __restrict b, const void * __restrict a, size_t n ) {
	char * s1 = (char *)b;
	const char * s2 = (const char *)a;
	for ( ; 0 < n; --n ) {
		*s1++ = *s2++;
	}
	return b;
}

/*
=================
hfFile_Memory::Write
=================
*/
hfHashTableT< int, int > histogram;
CONSOLE_COMMAND( outputHistogram, "", 0 ) {
	for ( int i = 0; i < histogram.Num(); i++ ) {
		int key;
		histogram.GetIndexKey( i, key );
		int * value = histogram.GetIndex( i );

		hfLib::Printf( "%d\t%d\n", key, *value );
	}
}

CONSOLE_COMMAND( clearHistogram, "", 0 ) {
	histogram.Clear();
}

int hfFile_Memory::Write( const void *buffer, int len ) {
	if ( len == 0 ) {
		// ~4% falls into this case for some reason...
		return 0;
	}

	if ( !( mode & ( 1 << FS_WRITE ) ) ) {
		common->FatalError( "hfFile_Memory::Write: %s not opened in write mode", name.c_str() );
		return 0;
	}

	int alloc = curPtr + len + 1 - filePtr - allocated; // need room for len+1
	if ( alloc > 0 ) {
		if ( maxSize != 0 ) {
			common->Error( "hfFile_Memory::Write: exceeded maximum size %d", maxSize );
			return 0;
		}
		int extra = granularity * ( 1 + alloc / granularity );
		char *newPtr = (char *) Mem_Alloc( allocated + extra, TAG_IDFILE );
		if ( allocated ) {
			memcpy( newPtr, filePtr, allocated );
		}
		allocated += extra;
		curPtr = newPtr + ( curPtr - filePtr );		
		if ( filePtr ) {
			Mem_Free( filePtr );
		}
		filePtr = newPtr;
	}

	//memcpy( curPtr, buffer, len );
	memcpy2( curPtr, buffer, len );

#if 0
	if ( memcpyImpl.GetInteger() == 0 ) {
		memcpy( curPtr, buffer, len );
	} else if ( memcpyImpl.GetInteger() == 1 ) {
		memcpy( curPtr, buffer, len );
	} else if ( memcpyImpl.GetInteger() == 2 ) {
		memcpy2( curPtr, buffer, len );
	}
#endif

#if 0
	int * value;
	if ( histogram.Get( len, &value ) && value != NULL ) {
		(*value)++;
	} else {
		histogram.Set( len, 1 );
	}
#endif

	curPtr += len;
	fileSize += len;
	filePtr[ fileSize ] = 0; // len + 1
	return len;
}

/*
=================
hfFile_Memory::Length
=================
*/
int hfFile_Memory::Length() const {
	return fileSize;
}

/*
========================
hfFile_Memory::SetLength
========================
*/
void hfFile_Memory::SetLength( size_t len ) {
	PreAllocate( len );
	fileSize = len;
}

/*
========================
hfFile_Memory::PreAllocate
========================
*/
void hfFile_Memory::PreAllocate( size_t len ) {
	if ( len > allocated ) {
		if ( maxSize != 0 ) {
			hfLib::Error( "hfFile_Memory::SetLength: exceeded maximum size %d", maxSize );
		}
		char * newPtr = (char *)Mem_Alloc( len, TAG_IDFILE );
		if ( allocated > 0 ) {
			memcpy( newPtr, filePtr, allocated );
		}
		allocated = len;
		curPtr = newPtr + ( curPtr - filePtr );		
		if ( filePtr != NULL ) {
			Mem_Free( filePtr );
		}
		filePtr = newPtr;
	}
}

/*
=================
hfFile_Memory::Timestamp
=================
*/
HF_TIME_T hfFile_Memory::Timestamp() const {
	return 0;
}

/*
=================
hfFile_Memory::Tell
=================
*/
int hfFile_Memory::Tell() const {
	return ( curPtr - filePtr );
}

/*
=================
hfFile_Memory::ForceFlush
=================
*/
void hfFile_Memory::ForceFlush() {
}

/*
=================
hfFile_Memory::Flush
=================
*/
void hfFile_Memory::Flush() {
}

/*
=================
hfFile_Memory::Seek

  returns zero on success and -1 on failure
=================
*/
int hfFile_Memory::Seek( long offset, fsOrigin_t origin ) {

	switch( origin ) {
		case FS_SEEK_CUR: {
			curPtr += offset;
			break;
		}
		case FS_SEEK_END: {
			curPtr = filePtr + fileSize - offset;
			break;
		}
		case FS_SEEK_SET: {
			curPtr = filePtr + offset;
			break;
		}
		default: {
			common->FatalError( "hfFile_Memory::Seek: bad origin for %s\n", name.c_str() );
			return -1;
		}
	}
	if ( curPtr < filePtr ) {
		curPtr = filePtr;
		return -1;
	}
	if ( curPtr > filePtr + fileSize ) {
		curPtr = filePtr + fileSize;
		return -1;
	}
	return 0;
}

/*
========================
hfFile_Memory::SetMaxLength 
========================
*/
void hfFile_Memory::SetMaxLength( size_t len ) {
	size_t oldLength = fileSize;

	SetLength( len );

	maxSize = len;
	fileSize = oldLength;
}

/*
=================
hfFile_Memory::MakeReadOnly
=================
*/
void hfFile_Memory::MakeReadOnly() {
	mode = ( 1 << FS_READ );
	Rewind();
}

/*
========================
hfFile_Memory::MakeWritable
========================
*/
void hfFile_Memory::MakeWritable() {
	mode = ( 1 << FS_WRITE );
	Rewind();
}

/*
=================
hfFile_Memory::Clear
=================
*/
void hfFile_Memory::Clear( bool freeMemory ) {
	fileSize = 0;
	granularity = 16384;
	if ( freeMemory ) {
		allocated = 0;
		Mem_Free( filePtr );
		filePtr = NULL;
		curPtr = NULL;
	} else {
		curPtr = filePtr;
	}
}

/*
=================
hfFile_Memory::SetData
=================
*/
void hfFile_Memory::SetData( const char *data, int length ) {
	maxSize = 0;
	fileSize = length;
	allocated = 0;
	granularity = 16384;

	mode = ( 1 << FS_READ );
	filePtr = const_cast<char *>(data);
	curPtr = const_cast<char *>(data);
}

/*
========================
hfFile_Memory::TruncateData
========================
*/
void hfFile_Memory::TruncateData( size_t len ) {
	if ( len > allocated ) {
		hfLib::Error( "hfFile_Memory::TruncateData: len (%d) exceeded allocated size (%d)", len, allocated );
	} else {
		fileSize = len;
	}
}

/*
=================================================================================

hfFile_BitMsg

=================================================================================
*/

/*
=================
hfFile_BitMsg::hfFile_BitMsg
=================
*/
hfFile_BitMsg::hfFile_BitMsg( hfBitMsg &msg ) {
	name = "*unknown*";
	mode = ( 1 << FS_WRITE );
	this->msg = &msg;
}

/*
=================
hfFile_BitMsg::hfFile_BitMsg
=================
*/
hfFile_BitMsg::hfFile_BitMsg( const hfBitMsg &msg ) {
	name = "*unknown*";
	mode = ( 1 << FS_READ );
	this->msg = const_cast<hfBitMsg *>(&msg);
}

/*
=================
hfFile_BitMsg::~hfFile_BitMsg
=================
*/
hfFile_BitMsg::~hfFile_BitMsg() {
}

/*
=================
hfFile_BitMsg::Read
=================
*/
int hfFile_BitMsg::Read( void *buffer, int len ) {

	if ( !( mode & ( 1 << FS_READ ) ) ) {
		common->FatalError( "hfFile_BitMsg::Read: %s not opened in read mode", name.c_str() );
		return 0;
	}

	return msg->ReadData( buffer, len );
}

/*
=================
hfFile_BitMsg::Write
=================
*/
int hfFile_BitMsg::Write( const void *buffer, int len ) {

	if ( !( mode & ( 1 << FS_WRITE ) ) ) {
		common->FatalError( "hfFile_Memory::Write: %s not opened in write mode", name.c_str() );
		return 0;
	}

	msg->WriteData( buffer, len );
	return len;
}

/*
=================
hfFile_BitMsg::Length
=================
*/
int hfFile_BitMsg::Length() const {
	return msg->GetSize();
}

/*
=================
hfFile_BitMsg::Timestamp
=================
*/
HF_TIME_T hfFile_BitMsg::Timestamp() const {
	return 0;
}

/*
=================
hfFile_BitMsg::Tell
=================
*/
int hfFile_BitMsg::Tell() const {
	if ( mode == FS_READ ) {
		return msg->GetReadCount();
	} else {
		return msg->GetSize();
	}
}

/*
=================
hfFile_BitMsg::ForceFlush
=================
*/
void hfFile_BitMsg::ForceFlush() {
}

/*
=================
hfFile_BitMsg::Flush
=================
*/
void hfFile_BitMsg::Flush() {
}

/*
=================
hfFile_BitMsg::Seek

  returns zero on success and -1 on failure
=================
*/
int hfFile_BitMsg::Seek( long offset, fsOrigin_t origin ) {
	return -1;
}


/*
=================================================================================

hfFile_Permanent

=================================================================================
*/

/*
=================
hfFile_Permanent::hfFile_Permanent
=================
*/
hfFile_Permanent::hfFile_Permanent() {
	name = "invalid";
	o = NULL;
	mode = 0;
	fileSize = 0;
	handleSync = false;
}

/*
=================
hfFile_Permanent::~hfFile_Permanent
=================
*/
hfFile_Permanent::~hfFile_Permanent() {
	if ( o ) {
#if defined(_WIN32)
		CloseHandle( o );
#else
		fclose( o );
#endif
	}
}

/*
=================
hfFile_Permanent::Read

Properly handles partial reads
=================
*/
int hfFile_Permanent::Read( void *buffer, int len ) {
	int		block, remaining;
	int		read;
	byte *	buf;
	int		tries;

	if ( !(mode & ( 1 << FS_READ ) ) ) {
		common->FatalError( "hfFile_Permanent::Read: %s not opened in read mode", name.c_str() );
		return 0;
	}

	if ( !o ) {
		return 0;
	}

	buf = (byte *)buffer;

	remaining = len;
	tries = 0;
	while( remaining ) {
		block = remaining;
#if defined(_WIN32)
		DWORD bytesRead;
		if ( !ReadFile( o, buf, block, &bytesRead, NULL ) ) {
			hfLib::Warning( "hfFile_Permanent::Read failed with %d from %s", GetLastError(), name.c_str() );
		}
		read = bytesRead;
#else
		read = (int)fread( buf, 1, block, o );
		if ( ferror( o ) ) {
			hfLib::Warning( "hfFile_Permanent::Read failed from %s", name.c_str() );
		}
#endif
		if ( read == 0 ) {
			// we might have been trying to read from a CD, which
			// sometimes returns a 0 read on windows
			if ( !tries ) {
				tries = 1;
			}
			else {
				return len-remaining;
			}
		}

		if ( read == -1 ) {
			common->FatalError( "hfFile_Permanent::Read: -1 bytes read from %s", name.c_str() );
		}

		remaining -= read;
		buf += read;
	}
	return len;
}

/*
=================
hfFile_Permanent::Write

Properly handles partial writes
=================
*/
int hfFile_Permanent::Write( const void *buffer, int len ) {
	int		block, remaining;
	int		written;
	byte *	buf;
	int		tries;

	if ( !( mode & ( 1 << FS_WRITE ) ) ) {
		common->FatalError( "hfFile_Permanent::Write: %s not opened in write mode", name.c_str() );
		return 0;
	}

	if ( !o ) {
		return 0;
	}

	buf = (byte *)buffer;

	remaining = len;
	tries = 0;
	while( remaining ) {
		block = remaining;
#if defined(_WIN32)
		DWORD bytesWritten;
		WriteFile( o, buf, block, &bytesWritten, NULL );
		written = bytesWritten;
#else
		written = (int)fwrite( buf, 1, block, o );
#endif
		if ( written == 0 ) {
			if ( !tries ) {
				tries = 1;
			}
			else {
				common->Printf( "hfFile_Permanent::Write: 0 bytes written to %s\n", name.c_str() );
				return 0;
			}
		}

		if ( written == -1 ) {
			common->Printf( "hfFile_Permanent::Write: -1 bytes written to %s\n", name.c_str() );
			return 0;
		}

		remaining -= written;
		buf += written;
		fileSize += written;
	}
	if ( handleSync ) {
		Flush();
	}
	return len;
}

/*
=================
hfFile_Permanent::ForceFlush
=================
*/
void hfFile_Permanent::ForceFlush() {
#if defined(_WIN32)
	FlushFileBuffers( o );
#else
	fflush( o );
#endif
}

/*
=================
hfFile_Permanent::Flush
=================
*/
void hfFile_Permanent::Flush() {
#if defined(_WIN32)
	FlushFileBuffers( o );
#else
	fflush( o );
#endif
}

/*
=================
hfFile_Permanent::Tell
=================
*/
int hfFile_Permanent::Tell() const {
#if defined(_WIN32)
	return SetFilePointer( o, 0, NULL, FILE_CURRENT );
#else
	return ftell( o );
#endif
}

/*
================
hfFile_Permanent::Length
================
*/
int hfFile_Permanent::Length() const {
	return fileSize;
}

/*
================
hfFile_Permanent::Timestamp
================
*/
HF_TIME_T hfFile_Permanent::Timestamp() const {
	HF_TIME_T ts = Sys_FileTimeStamp( o );
	return ts;
}

/*
=================
hfFile_Permanent::Seek

  returns zero on success and -1 on failure
=================
*/
int hfFile_Permanent::Seek( long offset, fsOrigin_t origin ) {
#if defined(_WIN32)
	int retVal = INVALID_SET_FILE_POINTER;
	switch( origin ) {
		case FS_SEEK_CUR: retVal = SetFilePointer( o, offset, NULL, FILE_CURRENT ); break;
		case FS_SEEK_END: retVal = SetFilePointer( o, offset, NULL, FILE_END ); break;
		case FS_SEEK_SET: retVal = SetFilePointer( o, offset, NULL, FILE_BEGIN ); break;
	}
	return ( retVal == INVALID_SET_FILE_POINTER ) ? -1 : 0;
#else
	int originPosix = SEEK_SET;
	switch( origin ) {
		case FS_SEEK_CUR: originPosix = SEEK_CUR; break;
		case FS_SEEK_END: originPosix = SEEK_END; break;
		case FS_SEEK_SET: originPosix = SEEK_SET; break;
	}
	return ( fseek( o, offset, originPosix ) == 0 ) ? 0 : -1;
#endif
}

#if 1
/*
=================================================================================

hfFile_Cached

=================================================================================
*/

/*
=================
hfFile_Cached::hfFile_Cached
=================
*/
hfFile_Cached::hfFile_Cached() : hfFile_Permanent() {
	internalFilePos = 0;
	bufferedStartOffset = 0;
	bufferedEndOffset = 0;
	buffered = NULL;
}

/*
=================
hfFile_Cached::~hfFile_Cached
=================
*/
hfFile_Cached::~hfFile_Cached() {
	Mem_Free( buffered );
}

/*
=================
hfFile_ReadBuffered::BufferData

Buffer a section of the file
=================
*/
void hfFile_Cached::CacheData( uint64 offset, uint64 length ) {
	Mem_Free( buffered );
	bufferedStartOffset = offset;
	bufferedEndOffset = offset + length;
	buffered = ( byte* )Mem_Alloc( length, TAG_RESOURCE );
	if ( buffered == NULL ) {
		return;
	}
	int internalFilePos = hfFile_Permanent::Tell();
	hfFile_Permanent::Seek( offset, FS_SEEK_SET );
	hfFile_Permanent::Read( buffered, length );
	hfFile_Permanent::Seek( internalFilePos, FS_SEEK_SET );
}

/*
=================
hfFile_ReadBuffered::Read

=================
*/
int hfFile_Cached::Read( void *buffer, int len ) {
	if ( internalFilePos >= bufferedStartOffset && internalFilePos + len < bufferedEndOffset ) {
		// this is in the buffer
		memcpy( buffer, (void*)&buffered[ internalFilePos - bufferedStartOffset ], len );
		internalFilePos += len;
		return len;
	}
	int read = hfFile_Permanent::Read( buffer, len );
	if ( read != -1 ) {
		internalFilePos += ( int64 )read;
	}
	return read;
}



/*
=================
hfFile_Cached::Tell
=================
*/
int hfFile_Cached::Tell() const {
	return internalFilePos;
}

/*
=================
hfFile_Cached::Seek

  returns zero on success and -1 on failure
=================
*/
int hfFile_Cached::Seek( long offset, fsOrigin_t origin ) {
	if ( origin == FS_SEEK_SET && offset >= bufferedStartOffset && offset < bufferedEndOffset ) {
		// don't do anything to the actual file ptr, just update or internal position
		internalFilePos = offset;
		return 0;
	}

	int retVal = hfFile_Permanent::Seek( offset, origin );
	internalFilePos = hfFile_Permanent::Tell();
	return retVal;
}
#endif

/*
=================================================================================

hfFile_InZip

=================================================================================
*/

/*
=================
hfFile_InZip::hfFile_InZip
=================
*/
hfFile_InZip::hfFile_InZip() {
	name = "invalid";
	zipFilePos = 0;
	fileSize = 0;
	memset( &z, 0, sizeof( z ) );
}

/*
=================
hfFile_InZip::~hfFile_InZip
=================
*/
hfFile_InZip::~hfFile_InZip() {
	unzCloseCurrentFile( z );
	unzClose( z );
}

/*
=================
hfFile_InZip::Read

Properly handles partial reads
=================
*/
int hfFile_InZip::Read( void *buffer, int len ) {
	int l = unzReadCurrentFile( z, buffer, len );
	return l;
}

/*
=================
hfFile_InZip::Write
=================
*/
int hfFile_InZip::Write( const void *buffer, int len ) {
	common->FatalError( "hfFile_InZip::Write: cannot write to the zipped file %s", name.c_str() );
	return 0;
}

/*
=================
hfFile_InZip::ForceFlush
=================
*/
void hfFile_InZip::ForceFlush() {
	common->FatalError( "hfFile_InZip::ForceFlush: cannot flush the zipped file %s", name.c_str() );
}

/*
=================
hfFile_InZip::Flush
=================
*/
void hfFile_InZip::Flush() {
	common->FatalError( "hfFile_InZip::Flush: cannot flush the zipped file %s", name.c_str() );
}

/*
=================
hfFile_InZip::Tell
=================
*/
int hfFile_InZip::Tell() const {
	return unztell( z );
}

/*
================
hfFile_InZip::Length
================
*/
int hfFile_InZip::Length() const {
	return fileSize;
}

/*
================
hfFile_InZip::Timestamp
================
*/
HF_TIME_T hfFile_InZip::Timestamp() const {
	return 0;
}

/*
=================
hfFile_InZip::Seek

  returns zero on success and -1 on failure
=================
*/
#define ZIP_SEEK_BUF_SIZE	(1<<15)

int hfFile_InZip::Seek( long offset, fsOrigin_t origin ) {
	int res, i;
	char *buf;

	switch( origin ) {
		case FS_SEEK_END: {
			offset = fileSize - offset;
		}
		case FS_SEEK_SET: {
			// set the file position in the zip file (also sets the current file info)
			unzSetCurrentFileInfoPosition( z, zipFilePos );
			unzOpenCurrentFile( z );
			if ( offset <= 0 ) {
				return 0;
			}
		}
		case FS_SEEK_CUR: {
			buf = (char *) _alloca16( ZIP_SEEK_BUF_SIZE );
			for ( i = 0; i < ( offset - ZIP_SEEK_BUF_SIZE ); i += ZIP_SEEK_BUF_SIZE ) {
				res = unzReadCurrentFile( z, buf, ZIP_SEEK_BUF_SIZE );
				if ( res < ZIP_SEEK_BUF_SIZE ) {
					return -1;
				}
			}
			res = i + unzReadCurrentFile( z, buf, offset - i );
			return ( res == offset ) ? 0 : -1;
		}
		default: {
			common->FatalError( "hfFile_InZip::Seek: bad origin for %s\n", name.c_str() );
			break;
		}
	}
	return -1;
}

#if 1

/*
=================================================================================

hfFile_InnerResource

=================================================================================
*/

/*
=================
hfFile_InnerResource::hfFile_InnerResource
=================
*/
hfFile_InnerResource::hfFile_InnerResource( const char *_name, hfFile *rezFile, int _offset, int _len ) {
	name = _name;
	offset = _offset;
	length = _len;
	resourceFile = rezFile;
	internalFilePos = 0;
	resourceBuffer = NULL;
}

/*
=================
hfFile_InnerResource::~hfFile_InnerResource
=================
*/
hfFile_InnerResource::~hfFile_InnerResource() {
	if ( resourceBuffer != NULL ) {
		fileSystem->FreeResourceBuffer();
	}
}

/*
=================
hfFile_InnerResource::Read

Properly handles partial reads
=================
*/
int hfFile_InnerResource::Read( void *buffer, int len ) {
	if ( resourceFile == NULL ) {
		return 0;
	}

	if ( internalFilePos + len > length ) {
		len = length - internalFilePos;
	}

	int read = 0; //fileSystem->ReadFromBGL( resourceFile, (byte*)buffer, offset + internalFilePos, len );

	if ( read != len ) {
		if ( resourceBuffer != NULL ) {
			memcpy( buffer, &resourceBuffer[ internalFilePos ], len );
			read = len;
		} else {
			read = fileSystem->ReadFromBGL( resourceFile, buffer, offset + internalFilePos, len );
		}
	}

	internalFilePos += read;

	return read;
}

/*
=================
hfFile_InnerResource::Tell
=================
*/
int hfFile_InnerResource::Tell() const {
	return internalFilePos;
}


/*
=================
hfFile_InnerResource::Seek

  returns zero on success and -1 on failure
=================
*/

int hfFile_InnerResource::Seek( long offset, fsOrigin_t origin ) {
	switch( origin ) {
		case FS_SEEK_END: {
			internalFilePos = length - offset - 1;
			return 0;
		}
		case FS_SEEK_SET: {
			internalFilePos = offset;
			if ( internalFilePos >= 0 && internalFilePos < length ) {
				return 0;
			}
			return -1;
		}
		case FS_SEEK_CUR: {
			internalFilePos += offset;
			if ( internalFilePos >= 0 && internalFilePos < length ) {
				return 0;
			}
			return -1;
		}
		default: {
			common->FatalError( "hfFile_InnerResource::Seek: bad origin for %s\n", name.c_str() );
			break;
		}
	}
	return -1;
}
#endif

/*
================================================================================================

hfFileLocal

================================================================================================
*/

/*
========================
hfFileLocal::~hfFileLocal

Destructor that will destroy (close) the managed file when this wrapper class goes out of scope.
========================
*/
hfFileLocal::~hfFileLocal() {
	if ( file != NULL ) {
		delete file;
		file = NULL;
	}
}

static const char * testEndianNessFilename = "temp.bin";
struct testEndianNess_t {
	testEndianNess_t() {
		a = 0x12345678;
		b = 0x12345678;
		c = 3.0f;
		d = -4.0f;
		e = "test";
		f = hfVec3( 1.0f, 2.0f, -3.0f );
		g = false;
		h = true;
		for ( int index = 0; index < sizeof( i ); index++ ) {
			i[index] = 0x37;
		}
	}
	bool operator==( testEndianNess_t & test ) const {
		return a == test.a &&
			b == test.b &&
			c == test.c &&
			d == test.d &&
			e == test.e &&
			f == test.f &&
			g == test.g &&
			h == test.h &&
			( memcmp( i, test.i, sizeof( i ) ) == 0 );
	}
	int				a;
	unsigned int	b;
	float			c;
	float			d;
	hfStr			e;
	hfVec3			f;
	bool			g;
	bool			h;
	byte			i[10];
};
CONSOLE_COMMAND( testEndianNessWrite, "Tests the read/write compatibility between platforms", 0 ) {
	hfFileLocal file( fileSystem->OpenFileWrite( testEndianNessFilename ) );
	if ( file == NULL ) {
		hfLib::Printf( "Couldn't open the %s testfile.\n", testEndianNessFilename );
		return;
	}

	testEndianNess_t testData;

	file->WriteBig( testData.a );
	file->WriteBig( testData.b );
	file->WriteFloat( testData.c );
	file->WriteFloat( testData.d );
	file->WriteString( testData.e );
	file->WriteVec3( testData.f );
	file->WriteBig( testData.g );
	file->WriteBig( testData.h );
	file->Write( testData.i, sizeof( testData.i )/ sizeof( testData.i[0] ) );
}

CONSOLE_COMMAND( testEndianNessRead, "Tests the read/write compatibility between platforms", 0 ) {
	hfFileLocal file( fileSystem->OpenFileRead( testEndianNessFilename ) );
	if ( file == NULL ) {
		hfLib::Printf( "Couldn't find the %s testfile.\n", testEndianNessFilename );
		return;
	}

	testEndianNess_t srcData;
	testEndianNess_t testData;

	memset( &testData, 0, sizeof( testData ) );

	file->ReadBig( testData.a );
	file->ReadBig( testData.b );
	file->ReadFloat( testData.c );
	file->ReadFloat( testData.d );
	file->ReadString( testData.e );
	file->ReadVec3( testData.f );
	file->ReadBig( testData.g );
	file->ReadBig( testData.h );
	file->Read( testData.i, sizeof( testData.i )/ sizeof( testData.i[0] ) );

	assert( srcData == testData );
}

CONSOLE_COMMAND( testEndianNessReset, "Tests the read/write compatibility between platforms", 0 ) {
	fileSystem->RemoveFile( testEndianNessFilename );
}



