#pragma hdrstop
#include "../hflib/hfprecompiled.h"

//===============================================================
//
//	memory allocation all in one place
//
//===============================================================

#undef new

/*
==================
Mem_Alloc16
==================
*/
void * Mem_Alloc16( const int size, const memTag_t tag ) {
	if ( !size ) {
		return NULL;
	}
	const int paddedSize = ( size + 15 ) & ~15;
#if defined(_WIN32)
	return _aligned_malloc( paddedSize, 16 );
#else
	void * mem = NULL;
	if ( posix_memalign( &mem, 16, paddedSize ) != 0 ) {
		return NULL;
	}
	return mem;
#endif
}

/*
==================
Mem_Free16
==================
*/
void Mem_Free16( void *ptr ) {
	if ( ptr == NULL ) {
		return;
	}
#if defined(_WIN32)
	_aligned_free( ptr );
#else
	free( ptr );
#endif
}

/*
==================
Mem_ClearedAlloc
==================
*/
void * Mem_ClearedAlloc( const int size, const memTag_t tag ) {
	void * mem = Mem_Alloc( size, tag );
	SIMDProcessor->Memset( mem, 0, size );
	return mem;
}

/*
==================
Mem_CopyString
==================
*/
char *Mem_CopyString( const char *in ) {
	char * out = (char *)Mem_Alloc( strlen(in) + 1, TAG_STRING );
	strcpy( out, in );
	return out;
}


