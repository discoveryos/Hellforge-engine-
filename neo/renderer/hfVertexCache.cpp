#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hftr_local.h"

hfVertexCache vertexCache;

hfCVar r_showVertexCache( "r_showVertexCache", "0", CVAR_RENDERER | CVAR_BOOL, "Print stats about the vertex cache every frame" );
hfCVar r_showVertexCacheTimings( "r_showVertexCache", "0", CVAR_RENDERER | CVAR_BOOL, "Print stats about the vertex cache every frame" );


/*
==============
ClearGeoBufferSet
==============
*/
static void ClearGeoBufferSet( geoBufferSet_t &gbs ) {
	gbs.indexMemUsed.SetValue( 0 );
	gbs.vertexMemUsed.SetValue( 0 );
	gbs.jointMemUsed.SetValue( 0 );
	gbs.allocations = 0;
}

/*
==============
MapGeoBufferSet
==============
*/
static void MapGeoBufferSet( geoBufferSet_t &gbs ) {
	if ( gbs.mappedVertexBase == NULL ) {
		gbs.mappedVertexBase = (byte *)gbs.vertexBuffer.MapBuffer( BM_WRITE );
	}
	if ( gbs.mappedIndexBase == NULL ) {
		gbs.mappedIndexBase = (byte *)gbs.indexBuffer.MapBuffer( BM_WRITE );
	}
	if ( gbs.mappedJointBase == NULL && gbs.jointBuffer.GetAllocedSize() != 0 ) {
		gbs.mappedJointBase = (byte *)gbs.jointBuffer.MapBuffer( BM_WRITE );
	}
}

/*
==============
UnmapGeoBufferSet
==============
*/
static void UnmapGeoBufferSet( geoBufferSet_t &gbs ) {
	if ( gbs.mappedVertexBase != NULL ) {
		gbs.vertexBuffer.UnmapBuffer();
		gbs.mappedVertexBase = NULL;
	}
	if ( gbs.mappedIndexBase != NULL ) {
		gbs.indexBuffer.UnmapBuffer();
		gbs.mappedIndexBase = NULL;
	}
	if ( gbs.mappedJointBase != NULL ) {
		gbs.jointBuffer.UnmapBuffer();
		gbs.mappedJointBase = NULL;
	}
}

/*
==============
AllocGeoBufferSet
==============
*/
static void AllocGeoBufferSet( geoBufferSet_t &gbs, const int vertexBytes, const int indexBytes, const int jointBytes ) {
	gbs.vertexBuffer.AllocBufferObject( NULL, vertexBytes );
	gbs.indexBuffer.AllocBufferObject( NULL, indexBytes );
	if ( jointBytes != 0 ) {
		gbs.jointBuffer.AllocBufferObject( NULL, jointBytes / sizeof( hfJointMat ) );
	}
	ClearGeoBufferSet( gbs );
}

/*
==============
hfVertexCache::Init
==============
*/
void hfVertexCache::Init( bool restart ) {
	currentFrame = 0;
	listNum = 0;

	mostUsedVertex = 0;
	mostUsedIndex = 0;
	mostUsedJoint = 0;

	for ( int i = 0; i < VERTCACHE_NUM_FRAMES; i++ ) {
		AllocGeoBufferSet( frameData[i], VERTCACHE_VERTEX_MEMORY_PER_FRAME, VERTCACHE_INDEX_MEMORY_PER_FRAME, VERTCACHE_JOINT_MEMORY_PER_FRAME );
	}
	AllocGeoBufferSet( staticData, STATIC_VERTEX_MEMORY, STATIC_INDEX_MEMORY, 0 );

	MapGeoBufferSet( frameData[listNum] );
}

/*
==============
hfVertexCache::Shutdown
==============
*/
void hfVertexCache::Shutdown() {
	for ( int i = 0; i < VERTCACHE_NUM_FRAMES; i++ ) {
		frameData[i].vertexBuffer.FreeBufferObject();
		frameData[i].indexBuffer.FreeBufferObject();
		frameData[i].jointBuffer.FreeBufferObject();
	}
}

/*
==============
hfVertexCache::PurgeAll
==============
*/
void hfVertexCache::PurgeAll() {
	Shutdown();
	Init( true );
}

/*
==============
hfVertexCache::FreeStaticData

call on loading a new map
==============
*/
void hfVertexCache::FreeStaticData() {
	ClearGeoBufferSet( staticData );
	mostUsedVertex = 0;
	mostUsedIndex = 0;
	mostUsedJoint = 0;
}

/*
==============
hfVertexCache::ActuallyAlloc
==============
*/
vertCacheHandle_t hfVertexCache::ActuallyAlloc( geoBufferSet_t & vcs, const void * data, int bytes, cacheType_t type ) {
	if ( bytes == 0 ) {
		return (vertCacheHandle_t)0;
	}

	assert( ( ((UINT_PTR)(data)) & 15 ) == 0 );
	assert( ( bytes & 15 ) == 0 );

	// thread safe interlocked adds
	byte ** base = NULL;
	int	endPos = 0;
	if ( type == CACHE_INDEX ) {
		base = &vcs.mappedIndexBase;
		endPos = vcs.indexMemUsed.Add( bytes );
		if ( endPos > vcs.indexBuffer.GetAllocedSize() ) {
			hfLib::Error( "Out of index cache" );
		}
	} else if ( type == CACHE_VERTEX ) {
		base = &vcs.mappedVertexBase;
		endPos = vcs.vertexMemUsed.Add( bytes );
		if ( endPos > vcs.vertexBuffer.GetAllocedSize() ) {
			hfLib::Error( "Out of vertex cache" );
		}
	} else if ( type == CACHE_JOINT ) {
		base = &vcs.mappedJointBase;
		endPos = vcs.jointMemUsed.Add( bytes );
		if ( endPos > vcs.jointBuffer.GetAllocedSize() ) {
			hfLib::Error( "Out of joint buffer cache" );
		}
	} else {
		assert( false );
	}

	vcs.allocations++;

	int offset = endPos - bytes;

	// Actually perform the data transfer
	if ( data != NULL ) {
		MapGeoBufferSet( vcs );
		CopyBuffer( *base + offset, (const byte *)data, bytes );
	}

	vertCacheHandle_t handle =	( (uint64)(currentFrame & VERTCACHE_FRAME_MASK ) << VERTCACHE_FRAME_SHIFT ) |
								( (uint64)(offset & VERTCACHE_OFFSET_MASK ) << VERTCACHE_OFFSET_SHIFT ) |
								( (uint64)(bytes & VERTCACHE_SIZE_MASK ) << VERTCACHE_SIZE_SHIFT );
	if ( &vcs == &staticData ) {
		handle |= VERTCACHE_STATIC;
	}
	return handle;
}

/*
==============
hfVertexCache::GetVertexBuffer
==============
*/
bool hfVertexCache::GetVertexBuffer( vertCacheHandle_t handle, hfVertexBuffer * vb ) {
	const int isStatic = handle & VERTCACHE_STATIC;
	const uint64 size = (int)( handle >> VERTCACHE_SIZE_SHIFT ) & VERTCACHE_SIZE_MASK;
	const uint64 offset = (int)( handle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	const uint64 frameNum = (int)( handle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
	if ( isStatic ) {
		vb->Reference( staticData.vertexBuffer, offset, size );
		return true;
	}
	if ( frameNum != ( ( currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) ) {
		return false;
	}
	vb->Reference( frameData[drawListNum].vertexBuffer, offset, size );
	return true;
}

/*
==============
hfVertexCache::GetIndexBuffer
==============
*/
bool hfVertexCache::GetIndexBuffer( vertCacheHandle_t handle, hfIndexBuffer * ib ) {
	const int isStatic = handle & VERTCACHE_STATIC;
	const uint64 size = (int)( handle >> VERTCACHE_SIZE_SHIFT ) & VERTCACHE_SIZE_MASK;
	const uint64 offset = (int)( handle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	const uint64 frameNum = (int)( handle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
	if ( isStatic ) {
		ib->Reference( staticData.indexBuffer, offset, size );
		return true;
	}
	if ( frameNum != ( ( currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) ) {
		return false;
	}
	ib->Reference( frameData[drawListNum].indexBuffer, offset, size );
	return true;
}

/*
==============
hfVertexCache::GetJointBuffer
==============
*/
bool hfVertexCache::GetJointBuffer( vertCacheHandle_t handle, hfJointBuffer * jb ) {
	const int isStatic = handle & VERTCACHE_STATIC;
	const uint64 numBytes = (int)( handle >> VERTCACHE_SIZE_SHIFT ) & VERTCACHE_SIZE_MASK;
	const uint64 jointOffset = (int)( handle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;
	const uint64 frameNum = (int)( handle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
	const uint64 numJoints = numBytes / sizeof( hfJointMat );
	if ( isStatic ) {
		jb->Reference( staticData.jointBuffer, jointOffset, numJoints );
		return true;
	}
	if ( frameNum != ( ( currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) ) {
		return false;
	}
	jb->Reference( frameData[drawListNum].jointBuffer, jointOffset, numJoints );
	return true;
}

/*
==============
hfVertexCache::BeginBackEnd
==============
*/
void hfVertexCache::BeginBackEnd() {
	mostUsedVertex = Max( mostUsedVertex, frameData[listNum].vertexMemUsed.GetValue() );
	mostUsedIndex = Max( mostUsedIndex, frameData[listNum].indexMemUsed.GetValue() );
	mostUsedJoint = Max( mostUsedJoint, frameData[listNum].jointMemUsed.GetValue() );

	if ( r_showVertexCache.GetBool() ) {
		hfLib::Printf( "%08d: %d allocations, %dkB vertex, %dkB index, %kB joint : %dkB vertex, %dkB index, %kB joint\n", 
			currentFrame, frameData[listNum].allocations,
			frameData[listNum].vertexMemUsed.GetValue() / 1024,
			frameData[listNum].indexMemUsed.GetValue() / 1024,
			frameData[listNum].jointMemUsed.GetValue() / 1024,
			mostUsedVertex / 1024,
			mostUsedIndex / 1024,
			mostUsedJoint / 1024 );
	}

	// unmap the current frame so the GPU can read it
	const int startUnmap = Sys_Milliseconds();
	UnmapGeoBufferSet( frameData[listNum] );
	UnmapGeoBufferSet( staticData );
	const int endUnmap = Sys_Milliseconds();
	if ( endUnmap - startUnmap > 1 ) {
		hfLib::PrintfIf( r_showVertexCacheTimings.GetBool(), "hfVertexCache::unmap took %i msec\n", endUnmap - startUnmap );
	}
	drawListNum = listNum;

	// prepare the next frame for writing to by the CPU
	currentFrame++;

	listNum = currentFrame % VERTCACHE_NUM_FRAMES;
	const int startMap = Sys_Milliseconds();
	MapGeoBufferSet( frameData[listNum] );
	const int endMap = Sys_Milliseconds();
	if ( endMap - startMap > 1 ) {
		hfLib::PrintfIf( r_showVertexCacheTimings.GetBool(), "hfVertexCache::map took %i msec\n", endMap - startMap );
	}

	ClearGeoBufferSet( frameData[listNum] );


#if 0
	const int startBind = Sys_Milliseconds();
	qglBindBufferARB( GL_ARRAY_BUFFER_ARB, (GLuint)frameData[drawListNum].vertexBuffer.GetAPIObject() );
	qglBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, (GLuint)frameData[drawListNum].indexBuffer.GetAPIObject() );
	const int endBind = Sys_Milliseconds();
	if ( endBind - startBind > 1 ) {
		hfLib::Printf( "hfVertexCache::bind took %i msec\n", endBind - startBind );
	}
#endif

}


