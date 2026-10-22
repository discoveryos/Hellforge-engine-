#pragma hdrstop
#include "../hflib/hfprecompiled.h"
#include "../framework/zlib/hfzlib.h"

/*
========================
hfSWF::Inflate
========================
*/
bool hfSWF::Inflate( const byte * input, int inputSize, byte * output, int outputSize ) {
	struct local_swf_alloc_t {
		static void * zalloc( void * opaque, uint32 items, uint32 size ) {
			return Mem_Alloc( items * size, TAG_SWF );
		}
		static void zfree( void * opaque, void * ptr ) {
			Mem_Free( ptr );
		}
	};
	z_stream stream;
	memset( &stream, 0, sizeof( stream ) );
	stream.next_in = (Bytef *)input;
	stream.avail_in = inputSize;
	stream.next_out = (Bytef *)output;
	stream.avail_out = outputSize;
	stream.zalloc = local_swf_alloc_t::zalloc;
	stream.zfree = local_swf_alloc_t::zfree;
	inflateInit( &stream );
	bool success = ( inflate( &stream, Z_FINISH ) == Z_STREAM_END );
	inflateEnd( &stream );

	return success;
}

