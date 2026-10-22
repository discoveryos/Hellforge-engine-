#ifndef __BUFFEROBJECT_H__
#define __BUFFEROBJECT_H__

/*
================================================================================================

	Buffer Objects

================================================================================================
*/

class hfIndexBuffer;

enum bufferMapType_t {
	BM_READ,			// map for reading
	BM_WRITE			// map for writing
};

// Returns all targets to virtual memory use instead of buffer object use.
// Call this before doing any conventional buffer reads, like screenshots.
void UnbindBufferObjects();

/*
================================================
hfVertexBuffer
================================================
*/
class hfVertexBuffer {
public:
						hfVertexBuffer();
						~hfVertexBuffer();

	// Allocate or free the buffer.
	bool				AllocBufferObject( const void * data, int allocSize );
	void				FreeBufferObject();

	// Make this buffer a reference to another buffer.
	void				Reference( const hfVertexBuffer & other );
	void				Reference( const hfVertexBuffer & other, int refOffset, int refSize );

	// Copies data to the buffer. 'size' may be less than the originally allocated size.
	void				Update( const void * data, int updateSize ) const;

	void *				MapBuffer( bufferMapType_t mapType ) const;
	hfDrawVert *		MapVertexBuffer( bufferMapType_t mapType ) const { return static_cast< hfDrawVert * >( MapBuffer( mapType ) ); }
	void				UnmapBuffer() const;
	bool				IsMapped() const { return ( size & MAPPED_FLAG ) != 0; }

	int					GetSize() const { return ( size & ~MAPPED_FLAG ); }
	int					GetAllocedSize() const { return ( ( size & ~MAPPED_FLAG ) + 15 ) & ~15; }
	void *				GetAPIObject() const { return apiObject; }
	int					GetOffset() const { return ( offsetInOtherBuffer & ~OWNS_BUFFER_FLAG ); }

private:
	int					size;					// size in bytes
	int					offsetInOtherBuffer;	// offset in bytes
	void *				apiObject;

	// sizeof() confuses typeinfo...
	static const int	MAPPED_FLAG			= 1 << ( 4 /* sizeof( int ) */ * 8 - 1 );
	static const int	OWNS_BUFFER_FLAG	= 1 << ( 4 /* sizeof( int ) */ * 8 - 1 );

private:
	void				ClearWithoutFreeing();
	void				SetMapped() const { const_cast< int & >( size ) |= MAPPED_FLAG; }
	void				SetUnmapped() const { const_cast< int & >( size ) &= ~MAPPED_FLAG; }
	bool				OwnsBuffer() const { return ( ( offsetInOtherBuffer & OWNS_BUFFER_FLAG ) != 0 ); }

	DISALLOW_COPY_AND_ASSIGN( hfVertexBuffer );
};

/*
================================================
hfIndexBuffer
================================================
*/
class hfIndexBuffer {
public:
						hfIndexBuffer();
						~hfIndexBuffer();

	// Allocate or free the buffer.
	bool				AllocBufferObject( const void * data, int allocSize );
	void				FreeBufferObject();

	// Make this buffer a reference to another buffer.
	void				Reference( const hfIndexBuffer & other );
	void				Reference( const hfIndexBuffer & other, int refOffset, int refSize );

	// Copies data to the buffer. 'size' may be less than the originally allocated size.
	void				Update( const void * data, int updateSize ) const;

	void *				MapBuffer( bufferMapType_t mapType ) const;
	triIndex_t *		MapIndexBuffer( bufferMapType_t mapType ) const { return static_cast< triIndex_t * >( MapBuffer( mapType ) ); }
	void				UnmapBuffer() const;
	bool				IsMapped() const { return ( size & MAPPED_FLAG ) != 0; }

	int					GetSize() const { return ( size & ~MAPPED_FLAG ); }
	int					GetAllocedSize() const { return ( ( size & ~MAPPED_FLAG ) + 15 ) & ~15; }
	void *				GetAPIObject() const { return apiObject; }
	int					GetOffset() const { return ( offsetInOtherBuffer & ~OWNS_BUFFER_FLAG ); }

private:
	int					size;					// size in bytes
	int					offsetInOtherBuffer;	// offset in bytes
	void *				apiObject;

	// sizeof() confuses typeinfo...
	static const int	MAPPED_FLAG			= 1 << ( 4 /* sizeof( int ) */ * 8 - 1 );
	static const int	OWNS_BUFFER_FLAG	= 1 << ( 4 /* sizeof( int ) */ * 8 - 1 );

private:
	void				ClearWithoutFreeing();
	void				SetMapped() const { const_cast< int & >( size ) |= MAPPED_FLAG; }
	void				SetUnmapped() const { const_cast< int & >( size ) &= ~MAPPED_FLAG; }
	bool				OwnsBuffer() const { return ( ( offsetInOtherBuffer & OWNS_BUFFER_FLAG ) != 0 ); }

	DISALLOW_COPY_AND_ASSIGN( hfIndexBuffer );
};

/*
================================================
hfJointBuffer

IMPORTANT NOTICE: on the PC, binding to an offset in uniform buffer objects 
is limited to GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, which is 256 on current nvidia cards,
so joint offsets, which are multiples of 48 bytes, must be in multiples of 16 = 768 bytes.
================================================
*/
class hfJointBuffer {
public:
						hfJointBuffer();
						~hfJointBuffer();

	// Allocate or free the buffer.
	bool				AllocBufferObject( const float * joints, int numAllocJoints );
	void				FreeBufferObject();

	// Make this buffer a reference to another buffer.
	void				Reference( const hfJointBuffer & other );
	void				Reference( const hfJointBuffer & other, int jointRefOffset, int numRefJoints );

	// Copies data to the buffer. 'numJoints' may be less than the originally allocated size.
	void				Update( const float * joints, int numUpdateJoints ) const;

	float *				MapBuffer( bufferMapType_t mapType ) const;
	void				UnmapBuffer() const;
	bool				IsMapped() const { return ( numJoints & MAPPED_FLAG ) != 0; }

	int					GetNumJoints() const { return ( numJoints & ~MAPPED_FLAG ); }
	int					GetAllocedSize() const { return ( numJoints & ~MAPPED_FLAG ) * 3 * 4 * sizeof( float ); }
	void *				GetAPIObject() const { return apiObject; }
	int					GetOffset() const { return ( offsetInOtherBuffer & ~OWNS_BUFFER_FLAG ); }

	void				Swap( hfJointBuffer & other );

private:
	int					numJoints;
	int					offsetInOtherBuffer;	// offset in bytes
	void *				apiObject;

	// sizeof() confuses typeinfo...
	static const int	MAPPED_FLAG			= 1 << ( 4 /* sizeof( int ) */ * 8 - 1 );
	static const int	OWNS_BUFFER_FLAG	= 1 << ( 4 /* sizeof( int ) */ * 8 - 1 );

private:
	void				ClearWithoutFreeing();
	void				SetMapped() const { const_cast< int & >( numJoints ) |= MAPPED_FLAG; }
	void				SetUnmapped() const { const_cast< int & >( numJoints ) &= ~MAPPED_FLAG; }
	bool				OwnsBuffer() const { return ( ( offsetInOtherBuffer & OWNS_BUFFER_FLAG ) != 0 ); }

	DISALLOW_COPY_AND_ASSIGN( hfJointBuffer );
};

#endif // !__BUFFEROBJECT_H__

