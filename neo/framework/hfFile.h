#ifndef __FILE_H__
#define __FILE_H__

/*
==============================================================

  File Streams.

==============================================================
*/

// mode parm for Seek
typedef enum {
	FS_SEEK_CUR,
	FS_SEEK_END,
	FS_SEEK_SET
} fsOrigin_t;

class hfFileSystemLocal;


class hfFile {
public:
	virtual					~hfFile() {};
							// Get the name of the file.
	virtual const char *	GetName() const;
							// Get the full file path.
	virtual const char *	GetFullPath() const;
							// Read data from the file to the buffer.
	virtual int				Read( void *buffer, int len );
							// Write data from the buffer to the file.
	virtual int				Write( const void *buffer, int len );
							// Returns the length of the file.
	virtual int				Length() const;
							// Return a time value for reload operations.
	virtual HF_TIME_T		Timestamp() const;
							// Returns offset in file.
	virtual int				Tell() const;
							// Forces flush on files being writting to.
	virtual void			ForceFlush();
							// Causes any buffered data to be written to the file.
	virtual void			Flush();
							// Seek on a file.
	virtual int				Seek( long offset, fsOrigin_t origin );
							// Go back to the beginning of the file.
	virtual void			Rewind();
							// Like fprintf.
	virtual int				Printf( VERIFY_FORMAT_STRING const char *fmt, ... );
							// Like fprintf but with argument pointer
	virtual int				VPrintf( const char *fmt, va_list arg );
							// Write a string with high precision floating point numbers to the file.
	virtual int				WriteFloatString( VERIFY_FORMAT_STRING const char *fmt, ... );
	
	// Endian portable alternatives to Read(...)
	virtual int				ReadInt( int &value );
	virtual int				ReadUnsignedInt( unsigned int &value );
	virtual int				ReadShort( short &value );
	virtual int				ReadUnsignedShort( unsigned short &value );
	virtual int				ReadChar( char &value );
	virtual int				ReadUnsignedChar( unsigned char &value );
	virtual int				ReadFloat( float &value );
	virtual int				ReadBool( bool &value );
	virtual int				ReadString( hfStr &string );
	virtual int				ReadVec2( hfVec2 &vec );
	virtual int				ReadVec3( hfVec3 &vec );
	virtual int				ReadVec4( hfVec4 &vec );
	virtual int				ReadVec6( hfVec6 &vec );
	virtual int				ReadMat3( hfMat3 &mat );
	
	// Endian portable alternatives to Write(...)
	virtual int				WriteInt( const int value );
	virtual int				WriteUnsignedInt( const unsigned int value );
	virtual int				WriteShort( const short value );
	virtual int				WriteUnsignedShort( unsigned short value );
	virtual int				WriteChar( const char value );
	virtual int				WriteUnsignedChar( const unsigned char value );
	virtual int				WriteFloat( const float value );
	virtual int				WriteBool( const bool value );
	virtual int				WriteString( const char *string );
	virtual int				WriteVec2( const hfVec2 &vec );
	virtual int				WriteVec3( const hfVec3 &vec );
	virtual int				WriteVec4( const hfVec4 &vec );
	virtual int				WriteVec6( const hfVec6 &vec );
	virtual int				WriteMat3( const hfMat3 &mat );

	template<class type> HF_INLINE size_t ReadBig( type &c ) {
		size_t r = Read( &c, sizeof( c ) );
		hfSwap::Big( c );
		return r;
	}

	template<class type> HF_INLINE size_t ReadBigArray( type *c, int count ) {
		size_t r = Read( c, sizeof( c[0] ) * count );
		hfSwap::BigArray( c, count );
		return r;
	}

	template<class type> HF_INLINE size_t WriteBig( const type &c ) {
		type b = c;
		hfSwap::Big( b );
		return Write( &b, sizeof( b ) );
	}

	template<class type> HF_INLINE size_t WriteBigArray( const type *c, int count ) {
		size_t r = 0;
		for ( int i = 0; i < count; i++ ) {
			r += WriteBig( c[i] );
		}
		return r;
	}
};

/*
================================================
hfFile_Memory 
================================================
*/
class hfFile_Memory : public hfFile {
	friend class			hfFileSystemLocal;

public:
							hfFile_Memory();	// file for writing without name
							hfFile_Memory( const char *name );	// file for writing
							hfFile_Memory( const char *name, char *data, int length );	// file for writing
							hfFile_Memory( const char *name, const char *data, int length );	// file for reading
	virtual					~hfFile_Memory();

	virtual const char *	GetName() const { return name.c_str(); }
	virtual const char *	GetFullPath() const { return name.c_str(); }
	virtual int				Read( void *buffer, int len );
	virtual int				Write( const void *buffer, int len );
	virtual int				Length() const;
	virtual void			SetLength( size_t len );
	virtual HF_TIME_T		Timestamp() const;
	virtual int				Tell() const;
	virtual void			ForceFlush();
	virtual void			Flush();
	virtual int				Seek( long offset, fsOrigin_t origin );

	// Set the given length and don't allow the file to grow.
	void					SetMaxLength( size_t len );
							// changes memory file to read only
	void					MakeReadOnly();
	// Change the file to be writable
	void					MakeWritable();
							// clear the file
	virtual void			Clear( bool freeMemory = true );
							// set data for reading
	void					SetData( const char *data, int length );
							// returns const pointer to the memory buffer
	const char *			GetDataPtr() const { return filePtr; }
							// returns pointer to the memory buffer
	char *					GetDataPtr() { return filePtr; }
							// set the file granularity
	void					SetGranularity( int g ) { assert( g > 0 ); granularity = g; }
	void					PreAllocate( size_t len );

	// Doesn't change how much is allocated, but allows you to set the size of the file to smaller than it should be.
	// Useful for stripping off a checksum at the end of the file
	void					TruncateData( size_t len );

	void					TakeDataOwnership();

	size_t					GetMaxLength() { return maxSize; }
	size_t					GetAllocated() { return allocated; }

protected:
	hfStr					name;			// name of the file
private:
	int						mode;			// open mode
	size_t					maxSize;		// maximum size of file
	size_t					fileSize;		// size of the file
	size_t					allocated;		// allocated size
	int						granularity;	// file granularity
	char *					filePtr;		// buffer holding the file data
	char *					curPtr;			// current read/write pointer
};


class hfFile_BitMsg : public hfFile {
	friend class			hfFileSystemLocal;

public:
							hfFile_BitMsg( hfBitMsg &msg );
							hfFile_BitMsg( const hfBitMsg &msg );
	virtual					~hfFile_BitMsg();

	virtual const char *	GetName() const { return name.c_str(); }
	virtual const char *	GetFullPath() const { return name.c_str(); }
	virtual int				Read( void *buffer, int len );
	virtual int				Write( const void *buffer, int len );
	virtual int				Length() const;
	virtual HF_TIME_T		Timestamp() const;
	virtual int				Tell() const;
	virtual void			ForceFlush();
	virtual void			Flush();
	virtual int				Seek( long offset, fsOrigin_t origin );

private:
	hfStr					name;			// name of the file
	int						mode;			// open mode
	hfBitMsg *				msg;
};


class hfFile_Permanent : public hfFile {
	friend class			hfFileSystemLocal;

public:
							hfFile_Permanent();
	virtual					~hfFile_Permanent();

	virtual const char *	GetName() const { return name.c_str(); }
	virtual const char *	GetFullPath() const { return fullPath.c_str(); }
	virtual int				Read( void *buffer, int len );
	virtual int				Write( const void *buffer, int len );
	virtual int				Length() const;
	virtual HF_TIME_T		Timestamp() const;
	virtual int				Tell() const;
	virtual void			ForceFlush();
	virtual void			Flush();
	virtual int				Seek( long offset, fsOrigin_t origin );

	// returns file pointer
	hfFileHandle			GetFilePtr() { return o; }

private:
	hfStr					name;			// relative path of the file - relative path
	hfStr					fullPath;		// full file path - OS path
	int						mode;			// open mode
	int						fileSize;		// size of the file
	hfFileHandle			o;				// file handle
	bool					handleSync;		// true if written data is immediately flushed
};

class hfFile_Cached : public hfFile_Permanent {
	friend class			hfFileSystemLocal;
public:
	hfFile_Cached();
	virtual					~hfFile_Cached();

	void					CacheData( uint64 offset, uint64 length );

	virtual int				Read( void *buffer, int len );

	virtual int				Tell() const;
	virtual int				Seek( long offset, fsOrigin_t origin );

private:
	uint64				internalFilePos;
	uint64				bufferedStartOffset;
	uint64				bufferedEndOffset;
	byte *				buffered;
};


class hfFile_InZip : public hfFile {
	friend class			hfFileSystemLocal;

public:
							hfFile_InZip();
	virtual					~hfFile_InZip();

	virtual const char *	GetName() const { return name.c_str(); }
	virtual const char *	GetFullPath() const { return fullPath.c_str(); }
	virtual int				Read( void *buffer, int len );
	virtual int				Write( const void *buffer, int len );
	virtual int				Length() const;
	virtual HF_TIME_T		Timestamp() const;
	virtual int				Tell() const;
	virtual void			ForceFlush();
	virtual void			Flush();
	virtual int				Seek( long offset, fsOrigin_t origin );

private:
	hfStr					name;			// name of the file in the pak
	hfStr					fullPath;		// full file path including pak file name
	int						zipFilePos;		// zip file info position in pak
	int						fileSize;		// size of the file
	void *					z;				// unzip info
};

#if 1
class hfFile_InnerResource : public hfFile {
	friend class			hfFileSystemLocal;

public:
							hfFile_InnerResource( const char *_name, hfFile *rezFile, int _offset, int _len );
	virtual					~hfFile_InnerResource();

	virtual const char *	GetName() const { return name.c_str(); }
	virtual const char *	GetFullPath() const { return name.c_str(); }
	virtual int				Read( void *buffer, int len );
	virtual int				Write( const void *buffer, int len ) { assert( false ); return 0; }
	virtual int				Length() const { return length; }
	virtual HF_TIME_T		Timestamp() const { return 0; }
	virtual int				Tell() const;
	virtual int				Seek( long offset, fsOrigin_t origin );
	void					SetResourceBuffer( byte * buf ) {
		resourceBuffer = buf;
		internalFilePos = 0;
	}

private:
	hfStr				name;				// name of the file in the pak
	int					offset;				// offset in the resource file
	int					length;				// size
	hfFile *			resourceFile;		// actual file
	int					internalFilePos;	// seek offset
	byte *				resourceBuffer;		// if using the temp save memory
};
#endif
/*
================================================
hfFileLocal is a FileStream wrapper that automatically closes a file when the 
class variable goes out of scope. Note that the pointer passed in to the constructor can be for 
any type of File Stream that ultimately inherits from hfFile, and that this is not actually a 
SmartPointer, as it does not keep a reference count. 
================================================
*/
class hfFileLocal {
public:
	// Constructor that accepts and stores the file pointer.
	hfFileLocal( hfFile *_file )	: file( _file ) {
	}

	// Destructor that will destroy (close) the file when this wrapper class goes out of scope.
	~hfFileLocal();

	// Cast to a file pointer.
	operator hfFile * () const {
		return file;
	}

	// Member access operator for treating the wrapper as if it were the file, itself.
	hfFile * operator -> () const {
		return file;
	}

protected:
	hfFile *file;	// The managed file pointer.
};



#endif /* !__FILE_H__ */

