#pragma hdrstop
#include "../hflib/hfprecompiled.h"
#include "hftr_local.h"

hfCVar r_showBuffers( "r_showBuffers", "0", CVAR_INTEGER, "" );


//static const GLenum bufferUsage = GL_STATIC_DRAW_ARB;
static const GLenum bufferUsage = GL_DYNAMIC_DRAW_ARB;

/*
==================
IsWriteCombined
==================
*/
#if defined(_WIN32)
bool IsWriteCombined( void * base ) {
	MEMORY_BASIC_INFORMATION info;
	SIZE_T size = VirtualQueryEx( GetCurrentProcess(), base, &info, sizeof( info ) );
	if ( size == 0 ) {
		DWORD error = GetLastError();
		error = error;
		return false;
	}
	bool isWriteCombined = ( ( info.AllocationProtect & PAGE_WRITECOMBINE ) != 0 );
	return isWriteCombined;
}
#endif



/*
================================================================================================

	Buffer Objects

================================================================================================
*/

/*
========================
UnbindBufferObjects
========================
*/
void UnbindBufferObjects() {
	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, 0 );
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, 0 );
}

#ifdef HF_WIN_X86_SSE2_INTRIN

void CopyBuffer( byte * dst, const byte * src, int numBytes ) {
	assert_16_byte_aligned( dst );
	assert_16_byte_aligned( src );

	int i = 0;
	for ( ; i + 128 <= numBytes; i += 128 ) {
		__m128i d0 = _mm_load_si128( (__m128i *)&src[i + 0*16] );
		__m128i d1 = _mm_load_si128( (__m128i *)&src[i + 1*16] );
		__m128i d2 = _mm_load_si128( (__m128i *)&src[i + 2*16] );
		__m128i d3 = _mm_load_si128( (__m128i *)&src[i + 3*16] );
		__m128i d4 = _mm_load_si128( (__m128i *)&src[i + 4*16] );
		__m128i d5 = _mm_load_si128( (__m128i *)&src[i + 5*16] );
		__m128i d6 = _mm_load_si128( (__m128i *)&src[i + 6*16] );
		__m128i d7 = _mm_load_si128( (__m128i *)&src[i + 7*16] );
		_mm_stream_si128( (__m128i *)&dst[i + 0*16], d0 );
		_mm_stream_si128( (__m128i *)&dst[i + 1*16], d1 );
		_mm_stream_si128( (__m128i *)&dst[i + 2*16], d2 );
		_mm_stream_si128( (__m128i *)&dst[i + 3*16], d3 );
		_mm_stream_si128( (__m128i *)&dst[i + 4*16], d4 );
		_mm_stream_si128( (__m128i *)&dst[i + 5*16], d5 );
		_mm_stream_si128( (__m128i *)&dst[i + 6*16], d6 );
		_mm_stream_si128( (__m128i *)&dst[i + 7*16], d7 );
	}
	for ( ; i + 16 <= numBytes; i += 16 ) {
		__m128i d = _mm_load_si128( (__m128i *)&src[i] );
		_mm_stream_si128( (__m128i *)&dst[i], d );
	}
	for ( ; i + 4 <= numBytes; i += 4 ) {
		*(uint32 *)&dst[i] = *(const uint32 *)&src[i];
	}
	for ( ; i < numBytes; i++ ) {
		dst[i] = src[i];
	}
	_mm_sfence();
}

#else

void CopyBuffer( byte * dst, const byte * src, int numBytes ) {
	assert_16_byte_aligned( dst );
	assert_16_byte_aligned( src );
	memcpy( dst, src, numBytes );
}

#endif

/*
================================================================================================

	hfVertexBuffer

================================================================================================
*/

/*
========================
hfVertexBuffer::hfVertexBuffer
========================
*/
hfVertexBuffer::hfVertexBuffer() {
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
	SetUnmapped();
}

/*
========================
hfVertexBuffer::~hfVertexBuffer
========================
*/
hfVertexBuffer::~hfVertexBuffer() {
	FreeBufferObject();
}

/*
========================
hfVertexBuffer::AllocBufferObject
========================
*/
bool hfVertexBuffer::AllocBufferObject( const void * data, int allocSize ) {
	assert( apiObject == NULL );
	assert_16_byte_aligned( data );

	if ( allocSize <= 0 ) {
		hfLib::Error( "hfVertexBuffer::AllocBufferObject: allocSize = %i", allocSize );
	}

	size = allocSize;

	bool allocationFailed = false;

	int numBytes = GetAllocedSize();


	// clear out any previous error
	qglGetError();

	GLuint bufferObject = 0xFFFF;
	qglGenBuffersARB( 1, & bufferObject );
	if ( bufferObject == 0xFFFF ) {
		hfLib::FatalError( "hfVertexBuffer::AllocBufferObject: failed" );
	}
	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, bufferObject );

	// these are rewritten every frame
	qglBufferDataARB( GL_ARRAY_BUFFER_ARB, numBytes, NULL, bufferUsage );
	apiObject = reinterpret_cast< void * >( bufferObject );

	GLenum err = qglGetError();
	if ( err == GL_OUT_OF_MEMORY ) {
		hfLib::Warning( "hfVertexBuffer::AllocBufferObject: allocation failed" );
		allocationFailed = true;
	}


	if ( r_showBuffers.GetBool() ) {
		hfLib::Printf( "vertex buffer alloc %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize() );
	}

	// copy the data
	if ( data != NULL ) {
		Update( data, allocSize );
	}

	return !allocationFailed;
}

/*
========================
hfVertexBuffer::FreeBufferObject
========================
*/
void hfVertexBuffer::FreeBufferObject() {
	if ( IsMapped() ) {
		UnmapBuffer();
	}

	// if this is a sub-allocation inside a larger buffer, don't actually free anything.
	if ( OwnsBuffer() == false ) {
		ClearWithoutFreeing();
		return;
	}

	if ( apiObject == NULL ) {
		return;
	}

	if ( r_showBuffers.GetBool() ) {
		hfLib::Printf( "vertex buffer free %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize() );
	}

	GLuint bufferObject = ( GLuint )( reinterpret_cast< uintptr_t >( apiObject ) );
	qglDeleteBuffersARB( 1, & bufferObject );

	ClearWithoutFreeing();
}

/*
========================
hfVertexBuffer::Reference
========================
*/
void hfVertexBuffer::Reference( const hfVertexBuffer & other ) {
	assert( IsMapped() == false );
	//assert( other.IsMapped() == false );	// this happens when building hfTriangles while at the same time setting up hfDrawVerts
	assert( other.GetAPIObject() != NULL );
	assert( other.GetSize() > 0 );

	FreeBufferObject();
	size = other.GetSize();						// this strips the MAPPED_FLAG
	offsetInOtherBuffer = other.GetOffset();	// this strips the OWNS_BUFFER_FLAG
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
hfVertexBuffer::Reference
========================
*/
void hfVertexBuffer::Reference( const hfVertexBuffer & other, int refOffset, int refSize ) {
	assert( IsMapped() == false );
	//assert( other.IsMapped() == false );	// this happens when building hfTriangles while at the same time setting up hfDrawVerts
	assert( other.GetAPIObject() != NULL );
	assert( refOffset >= 0 );
	assert( refSize >= 0 );
	assert( refOffset + refSize <= other.GetSize() );

	FreeBufferObject();
	size = refSize;
	offsetInOtherBuffer = other.GetOffset() + refOffset;
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
hfVertexBuffer::Update
========================
*/
void hfVertexBuffer::Update( const void * data, int updateSize ) const {
	assert( apiObject != NULL );
	assert( IsMapped() == false );
	assert_16_byte_aligned( data );
	assert( ( GetOffset() & 15 ) == 0 );

	if ( updateSize > size ) {
		hfLib::FatalError( "hfVertexBuffer::Update: size overrun, %i > %i\n", updateSize, GetSize() );
	}

	int numBytes = ( updateSize + 15 ) & ~15;

	GLuint bufferObject = ( GLuint )( reinterpret_cast< uintptr_t >( apiObject ) );
	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, bufferObject );
	qglBufferSubDataARB( GL_ARRAY_BUFFER_ARB, GetOffset(), (GLsizeiptrARB)numBytes, data );
/*
	void * buffer = MapBuffer( BM_WRITE );
	CopyBuffer( (byte *)buffer + GetOffset(), (byte *)data, numBytes );
	UnmapBuffer();
*/
}

/*
========================
hfVertexBuffer::MapBuffer
========================
*/
void * hfVertexBuffer::MapBuffer( bufferMapType_t mapType ) const {
	assert( apiObject != NULL );
	assert( IsMapped() == false );

	void * buffer = NULL;

	GLuint bufferObject = ( GLuint )( reinterpret_cast< uintptr_t >( apiObject ) );
	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, bufferObject );
	if ( mapType == BM_READ ) {
		//buffer = qglMapBufferARB( GL_ARRAY_BUFFER_ARB, GL_READ_ONLY_ARB );
		buffer = qglMapBufferRange( GL_ARRAY_BUFFER_ARB, 0, GetAllocedSize(), GL_MAP_READ_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT );
		if ( buffer != NULL ) {
			buffer = (byte *)buffer + GetOffset();
		}
	} else if ( mapType == BM_WRITE ) {
		//buffer = qglMapBufferARB( GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB );
		buffer = qglMapBufferRange( GL_ARRAY_BUFFER_ARB, 0, GetAllocedSize(), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT );
		if ( buffer != NULL ) {
			buffer = (byte *)buffer + GetOffset();
		}
#if defined(_WIN32)
		assert( IsWriteCombined( buffer ) );
#endif
	} else {
		assert( false );
	}

	SetMapped();

	if ( buffer == NULL ) {
		hfLib::FatalError( "hfVertexBuffer::MapBuffer: failed" );
	}
	return buffer;
}

/*
========================
hfVertexBuffer::UnmapBuffer
========================
*/
void hfVertexBuffer::UnmapBuffer() const {
	assert( apiObject != NULL );
	assert( IsMapped() );

	GLuint bufferObject = ( GLuint )( reinterpret_cast< uintptr_t >( apiObject ) );
	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, bufferObject );
	if ( !qglUnmapBufferARB( GL_ARRAY_BUFFER_ARB ) ) {
		hfLib::Printf( "hfVertexBuffer::UnmapBuffer failed\n" );
	}

	SetUnmapped();
}

/*
========================
hfVertexBuffer::ClearWithoutFreeing
========================
*/
void hfVertexBuffer::ClearWithoutFreeing() {
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
}

/*
================================================================================================

	hfIndexBuffer

================================================================================================
*/

/*
========================
hfIndexBuffer::hfIndexBuffer
========================
*/
hfIndexBuffer::hfIndexBuffer() {
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
	SetUnmapped();
}

/*
========================
hfIndexBuffer::~hfIndexBuffer
========================
*/
hfIndexBuffer::~hfIndexBuffer() {
	FreeBufferObject();
}

/*
========================
hfIndexBuffer::AllocBufferObject
========================
*/
bool hfIndexBuffer::AllocBufferObject( const void * data, int allocSize ) {
	assert( apiObject == NULL );
	assert_16_byte_aligned( data );

	if ( allocSize <= 0 ) {
		hfLib::Error( "hfIndexBuffer::AllocBufferObject: allocSize = %i", allocSize );
	}

	size = allocSize;

	bool allocationFailed = false;

	int numBytes = GetAllocedSize();


	// clear out any previous error
	qglGetError();

	GLuint bufferObject = 0xFFFF;
	qglGenBuffersARB( 1, & bufferObject );
	if ( bufferObject == 0xFFFF ) {
		GLenum error = qglGetError();
		hfLib::FatalError( "hfIndexBuffer::AllocBufferObject: failed - GL_Error %d", error );
	}
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, bufferObject );

	// these are rewritten every frame
	qglBufferDataARB( GL_ELEMENT_ARRAY_BUFFER_ARB, numBytes, NULL, bufferUsage );
	apiObject = reinterpret_cast< void * >( bufferObject );

	GLenum err = qglGetError();
	if ( err == GL_OUT_OF_MEMORY ) {
		hfLib::Warning( "hfIndexBuffer:AllocBufferObject: allocation failed" );
		allocationFailed = true;
	}


	if ( r_showBuffers.GetBool() ) {
		hfLib::Printf( "index buffer alloc %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize() );
	}

	// copy the data
	if ( data != NULL ) {
		Update( data, allocSize );
	}

	return !allocationFailed;
}

/*
========================
hfIndexBuffer::FreeBufferObject
========================
*/
void hfIndexBuffer::FreeBufferObject() {
	if ( IsMapped() ) {
		UnmapBuffer();
	}

	// if this is a sub-allocation inside a larger buffer, don't actually free anything.
	if ( OwnsBuffer() == false ) {
		ClearWithoutFreeing();
		return;
	}

	if ( apiObject == NULL ) {
		return;
	}

	if ( r_showBuffers.GetBool() ) {
		hfLib::Printf( "index buffer free %p, api %p (%i bytes)\n", this, GetAPIObject(), GetSize() );
	}

	GLuint bufferObject = ( GLuint )( reinterpret_cast< uintptr_t >( apiObject ) );
	qglDeleteBuffersARB( 1, & bufferObject );

	ClearWithoutFreeing();
}

/*
========================
hfIndexBuffer::Reference
========================
*/
void hfIndexBuffer::Reference( const hfIndexBuffer & other ) {
	assert( IsMapped() == false );
	//assert( other.IsMapped() == false );	// this happens when building hfTriangles while at the same time setting up triIndex_t
	assert( other.GetAPIObject() != NULL );
	assert( other.GetSize() > 0 );

	FreeBufferObject();
	size = other.GetSize();						// this strips the MAPPED_FLAG
	offsetInOtherBuffer = other.GetOffset();	// this strips the OWNS_BUFFER_FLAG
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
hfIndexBuffer::Reference
========================
*/
void hfIndexBuffer::Reference( const hfIndexBuffer & other, int refOffset, int refSize ) {
	assert( IsMapped() == false );
	//assert( other.IsMapped() == false );	// this happens when building hfTriangles while at the same time setting up triIndex_t
	assert( other.GetAPIObject() != NULL );
	assert( refOffset >= 0 );
	assert( refSize >= 0 );
	assert( refOffset + refSize <= other.GetSize() );

	FreeBufferObject();
	size = refSize;
	offsetInOtherBuffer = other.GetOffset() + refOffset;
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
hfIndexBuffer::Update
========================
*/
void hfIndexBuffer::Update( const void * data, int updateSize ) const {

	assert( apiObject != NULL );
	assert( IsMapped() == false );
	assert_16_byte_aligned( data );
	assert( ( GetOffset() & 15 ) == 0 );

	if ( updateSize > size ) {
		hfLib::FatalError( "hfIndexBuffer::Update: size overrun, %i > %i\n", updateSize, GetSize() );
	}

	int numBytes = ( updateSize + 15 ) & ~15;

	GLuint bufferObject = ( GLuint )( reinterpret_cast< uintptr_t >( apiObject ) );
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, bufferObject );
	qglBufferSubDataARB( GL_ELEMENT_ARRAY_BUFFER_ARB, GetOffset(), (GLsizeiptrARB)numBytes, data );
/*
	void * buffer = MapBuffer( BM_WRITE );
	CopyBuffer( (byte *)buffer + GetOffset(), (byte *)data, numBytes );
	UnmapBuffer();
*/
}

/*
========================
hfIndexBuffer::MapBuffer
========================
*/
void * hfIndexBuffer::MapBuffer( bufferMapType_t mapType ) const {

	assert( apiObject != NULL );
	assert( IsMapped() == false );

	void * buffer = NULL;

	GLuint bufferObject = ( GLuint )( reinterpret_cast< uintptr_t >( apiObject ) );
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, bufferObject );
	if ( mapType == BM_READ ) {
		//buffer = qglMapBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, GL_READ_ONLY_ARB );
		buffer = qglMapBufferRange( GL_ELEMENT_ARRAY_BUFFER_ARB, 0, GetAllocedSize(), GL_MAP_READ_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT );
		if ( buffer != NULL ) {
			buffer = (byte *)buffer + GetOffset();
		}
	} else if ( mapType == BM_WRITE ) {
		//buffer = qglMapBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB );
		buffer = qglMapBufferRange( GL_ELEMENT_ARRAY_BUFFER_ARB, 0, GetAllocedSize(), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT );
		if ( buffer != NULL ) {
			buffer = (byte *)buffer + GetOffset();
		}
#if defined(_WIN32)
		assert( IsWriteCombined( buffer ) );
#endif
	} else {
		assert( false );
	}

	SetMapped();

	if ( buffer == NULL ) {
		hfLib::FatalError( "hfIndexBuffer::MapBuffer: failed" );
	}
	return buffer;
}

/*
========================
hfIndexBuffer::UnmapBuffer
========================
*/
void hfIndexBuffer::UnmapBuffer() const {
	assert( apiObject != NULL );
	assert( IsMapped() );

	GLuint bufferObject = ( GLuint )( reinterpret_cast< uintptr_t >( apiObject ) );
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, bufferObject );
	if ( !qglUnmapBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB ) ) {
		hfLib::Printf( "hfIndexBuffer::UnmapBuffer failed\n" );
	}

	SetUnmapped();
}

/*
========================
hfIndexBuffer::ClearWithoutFreeing
========================
*/
void hfIndexBuffer::ClearWithoutFreeing() {
	size = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
}

/*
================================================================================================

	hfJointBuffer

================================================================================================
*/

/*
========================
hfJointBuffer::hfJointBuffer
========================
*/
hfJointBuffer::hfJointBuffer() {
	numJoints = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
	SetUnmapped();
}

/*
========================
hfJointBuffer::~hfJointBuffer
========================
*/
hfJointBuffer::~hfJointBuffer() {
	FreeBufferObject();
}

/*
========================
hfJointBuffer::AllocBufferObject
========================
*/
bool hfJointBuffer::AllocBufferObject( const float * joints, int numAllocJoints ) {
	assert( apiObject == NULL );
	assert_16_byte_aligned( joints );

	if ( numAllocJoints <= 0 ) {
		hfLib::Error( "hfJointBuffer::AllocBufferObject: joints = %i", numAllocJoints );
	}

	numJoints = numAllocJoints;

	bool allocationFailed = false;

	const int numBytes = GetAllocedSize();

	GLuint buffer = 0;
	qglGenBuffersARB( 1, &buffer );
	qglBindBufferARB( GL_UNIFORM_BUFFER, buffer );
	qglBufferDataARB( GL_UNIFORM_BUFFER, numBytes, NULL, GL_STREAM_DRAW_ARB );
	qglBindBufferARB( GL_UNIFORM_BUFFER, 0);
	apiObject = reinterpret_cast< void * >( buffer );

	if ( r_showBuffers.GetBool() ) {
		hfLib::Printf( "joint buffer alloc %p, api %p (%i joints)\n", this, GetAPIObject(), GetNumJoints() );
	}

	// copy the data
	if ( joints != NULL ) {
		Update( joints, numAllocJoints );
	}

	return !allocationFailed;
}

/*
========================
hfJointBuffer::FreeBufferObject
========================
*/
void hfJointBuffer::FreeBufferObject() {
	if ( IsMapped() ) {
		UnmapBuffer();
	}

	// if this is a sub-allocation inside a larger buffer, don't actually free anything.
	if ( OwnsBuffer() == false ) {
		ClearWithoutFreeing();
		return;
	}

	if ( apiObject == NULL ) {
		return;
	}

	if ( r_showBuffers.GetBool() ) {
		hfLib::Printf( "joint buffer free %p, api %p (%i joints)\n", this, GetAPIObject(), GetNumJoints() );
	}

	GLuint buffer = ( GLuint )( reinterpret_cast< uintptr_t >( apiObject ) );
	qglBindBufferARB( GL_UNIFORM_BUFFER, 0 );
	qglDeleteBuffersARB( 1, & buffer );

	ClearWithoutFreeing();
}

/*
========================
hfJointBuffer::Reference
========================
*/
void hfJointBuffer::Reference( const hfJointBuffer & other ) {
	assert( IsMapped() == false );
	assert( other.IsMapped() == false );
	assert( other.GetAPIObject() != NULL );
	assert( other.GetNumJoints() > 0 );

	FreeBufferObject();
	numJoints = other.GetNumJoints();			// this strips the MAPPED_FLAG
	offsetInOtherBuffer = other.GetOffset();	// this strips the OWNS_BUFFER_FLAG
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
hfJointBuffer::Reference
========================
*/
void hfJointBuffer::Reference( const hfJointBuffer & other, int jointRefOffset, int numRefJoints ) {
	assert( IsMapped() == false );
	assert( other.IsMapped() == false );
	assert( other.GetAPIObject() != NULL );
	assert( jointRefOffset >= 0 );
	assert( numRefJoints >= 0 );
	assert( jointRefOffset + numRefJoints * sizeof( hfJointMat ) <= other.GetNumJoints() * sizeof( hfJointMat ) );
	assert_16_byte_aligned( numRefJoints * 3 * 4 * sizeof( float ) );

	FreeBufferObject();
	numJoints = numRefJoints;
	offsetInOtherBuffer = other.GetOffset() + jointRefOffset;
	apiObject = other.apiObject;
	assert( OwnsBuffer() == false );
}

/*
========================
hfJointBuffer::Update
========================
*/
void hfJointBuffer::Update( const float * joints, int numUpdateJoints ) const {
	assert( apiObject != NULL );
	assert( IsMapped() == false );
	assert_16_byte_aligned( joints );
	assert( ( GetOffset() & 15 ) == 0 );

	if ( numUpdateJoints > numJoints ) {
		hfLib::FatalError( "hfJointBuffer::Update: size overrun, %i > %i\n", numUpdateJoints, numJoints );
	}

	const int numBytes = numUpdateJoints * 3 * 4 * sizeof( float );

	qglBindBufferARB( GL_UNIFORM_BUFFER, ( GLuint )( reinterpret_cast< uintptr_t >( apiObject ) ) );
	qglBufferSubDataARB( GL_UNIFORM_BUFFER, GetOffset(), (GLsizeiptrARB)numBytes, joints );
}

/*
========================
hfJointBuffer::MapBuffer
========================
*/
float * hfJointBuffer::MapBuffer( bufferMapType_t mapType ) const {
	assert( IsMapped() == false );
	assert( mapType == BM_WRITE );
	assert( apiObject != NULL );

	int numBytes = GetAllocedSize();

	void * buffer = NULL;

	qglBindBufferARB( GL_UNIFORM_BUFFER, ( GLuint )( reinterpret_cast< uintptr_t >( apiObject ) ) );
	numBytes = numBytes;
	assert( GetOffset() == 0 );
	//buffer = qglMapBufferARB( GL_UNIFORM_BUFFER, GL_WRITE_ONLY_ARB );
	buffer = qglMapBufferRange( GL_UNIFORM_BUFFER, 0, GetAllocedSize(), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT );
	if ( buffer != NULL ) {
		buffer = (byte *)buffer + GetOffset();
	}

	SetMapped();

	if ( buffer == NULL ) {
		hfLib::FatalError( "hfJointBuffer::MapBuffer: failed" );
	}
	return (float *) buffer;
}

/*
========================
hfJointBuffer::UnmapBuffer
========================
*/
void hfJointBuffer::UnmapBuffer() const {
	assert( apiObject != NULL );
	assert( IsMapped() );

	qglBindBufferARB( GL_UNIFORM_BUFFER, ( GLuint )( reinterpret_cast< uintptr_t >( apiObject ) ) );
	if ( !qglUnmapBufferARB( GL_UNIFORM_BUFFER ) ) {
		hfLib::Printf( "hfJointBuffer::UnmapBuffer failed\n" );
	}

	SetUnmapped();
}

/*
========================
hfJointBuffer::ClearWithoutFreeing
========================
*/
void hfJointBuffer::ClearWithoutFreeing() {
	numJoints = 0;
	offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	apiObject = NULL;
}

/*
========================
hfJointBuffer::Swap
========================
*/
void hfJointBuffer::Swap( hfJointBuffer & other ) {
	// Make sure the ownership of the buffer is not transferred to an unintended place.
	assert( other.OwnsBuffer() == OwnsBuffer() );

	SwapValues( other.numJoints, numJoints );
	SwapValues( other.offsetInOtherBuffer, offsetInOtherBuffer );
	SwapValues( other.apiObject, apiObject );
}

