/*
===========================================================================

Hellforge Power 1 Engine - hfscript ahead-of-time images

HfScript_SaveImage / HfScript_LoadImage serialize a compiled program into a
portable, canonical little-endian image.  Loading re-builds the exact
program structure (ops, constants, functions, globals and the string pool)
so it can be handed straight to HfScriptVM::RunProgram.

The image is deliberately free of pointers: string constants store pool
offsets and are re-anchored against the reconstructed pool on load.

===========================================================================
*/
#include "../hflib/hfprecompiled.h"
#include "HfScript.h"

#define HFS_IMAGE_MAGIC		( ( 'H' ) | ( 'F' << 8 ) | ( 'S' << 16 ) | ( '1' << 24 ) )	// "HFS1"
#define HFS_IMAGE_VERSION	1

/*
====================
A small growable byte buffer
====================
*/
struct HfImageWriter {
	int			size;
	int			used;
	byte *		data;

	HfImageWriter() { size = 256; used = 0; data = (byte *)Mem_Alloc( size, TAG_SCRIPT ); }
	~HfImageWriter() { if ( data ) { Mem_Free( data ); data = NULL; } }

	void Reserve( int extra ) {
		if ( used + extra <= size ) {
			return;
		}
		while ( used + extra > size ) {
			size *= 2;
		}
		byte * newData = (byte *)Mem_Alloc( size, TAG_SCRIPT );
		memcpy( newData, data, used );
		Mem_Free( data );
		data = newData;
	}
	void Bytes( const void * src, int n ) { Reserve( n ); memcpy( data + used, src, n ); used += n; }
	void U8( int v ) { byte b = (byte)v; Bytes( &b, 1 ); }
	void U32( unsigned v ) { byte t[ 4 ]; for ( int i = 0; i < 4; i++ ) { t[ i ] = (byte)( v >> ( 8 * i ) ); } Bytes( t, 4 ); }
	void U64( uint64 v ) { byte t[ 8 ]; for ( int i = 0; i < 8; i++ ) { t[ i ] = (byte)( v >> ( 8 * i ) ); } Bytes( t, 8 ); }
	void S64( int64 v ) { U64( (uint64)v ); }
	void Str( const char * s ) {
		int len = s != NULL ? (int)strlen( s ) : 0;
		U32( (unsigned)len );
		if ( len ) { Bytes( s, len ); }
		U8( 0 );
	}
};

/*
====================
Reader with bounds checks
====================
*/
struct HfImageReader {
	const byte *	data;
	int				size;
	int				pos;

	HfImageReader( const void * buffer, int bufferSize ) { data = (const byte *)buffer; size = bufferSize; pos = 0; }

	bool Need( int n ) { return pos >= 0 && n >= 0 && pos + n <= size; }
	bool U32( unsigned * out ) {
		if ( !Need( 4 ) ) { return false; }
		unsigned v = 0;
		for ( int i = 0; i < 4; i++ ) { v |= ( (unsigned)data[ pos + i ] ) << ( 8 * i ); }
		pos += 4;
		*out = v;
		return true;
	}
	bool U64( uint64 * out ) {
		if ( !Need( 8 ) ) { return false; }
		uint64 v = 0;
		for ( int i = 0; i < 8; i++ ) { v |= ( (uint64)data[ pos + i ] ) << ( 8 * i ); }
		pos += 8;
		*out = v;
		return true;
	}
	bool S64( int64 * out ) { uint64 v; if ( !U64( &v ) ) { return false; } *out = (int64)v; return true; }
	bool Raw( void * dst, int n ) { if ( !Need( n ) ) { return false; } memcpy( dst, data + pos, n ); pos += n; return true; }
	bool Str( hfStr * out ) {
		unsigned len;
		if ( !U32( &len ) ) { return false; }
		if ( len > 1 << 20 ) { return false; }
		if ( !Need( (int)len + 1 ) ) { return false; }
		out->Clear();
		if ( len ) { ( *out ) = (const char *)data + pos; }
		pos += (int)len + 1;
		return true;
	}
	bool Skip( int n ) { if ( !Need( n ) ) { return false; } pos += n; return true; }
	int Left() { return size - pos; }
};

/*
====================
HfScript_SaveImage
====================
*/
bool HfScript_SaveImage( const HfScriptProgram * program, void * * outBuffer, int * outSize ) {
	if ( outBuffer == NULL || outSize == NULL || program == NULL || !program->valid ) {
		return false;
	}
	*outBuffer = NULL;
	*outSize = 0;

	HfImageWriter w;
	w.U32( HFS_IMAGE_MAGIC );
	w.U32( HFS_IMAGE_VERSION );
	w.U32( 0 );												// flags
	w.U64( program->numOps );
	w.U64( program->numConstants );
	w.U64( program->numFunctions );
	w.U64( program->numGlobals );
	w.U64( program->poolUsed );
	w.S64( program->entryOp );
	w.S64( program->entryFunction );

	// string pool (the used region, byte-for-byte)
	w.Bytes( program->stringPool, program->poolUsed );

	// ops
	for ( int i = 0; i < program->numOps; i++ ) {
		w.U32( (unsigned)program->ops[ i ].op );
		w.S64( program->ops[ i ].a );
		w.S64( program->ops[ i ].b );
	}

	// constants
	for ( int i = 0; i < program->numConstants; i++ ) {
		const HfScriptValue & c = program->constants[ i ];
		w.U32( (unsigned)c.type );
		uint64 bits = 0;
		if ( c.type == HST_STRING ) {
			// store the pool offset, not the pointer
			if ( c.s == NULL ) {
				bits = 0;
			} else {
				int64 off = c.s - program->stringPool;
				if ( off < 0 || off >= program->poolUsed ) {
					return false;
				}
				bits = (uint64)off;
			}
		} else if ( c.type == HST_FLOAT ) {
			memcpy( &bits, &c.f, sizeof( c.f ) );
		} else {
			bits = (uint64)c.i;			// covers HST_INT, HST_BOOL, and nil padding
		}
		w.U64( bits );
	}

	// functions
	for ( int i = 0; i < program->numFunctions; i++ ) {
		const HfScriptFunction & f = program->functions[ i ];
		w.Str( f.name.c_str() );
		w.S64( f.numParams );
		w.S64( f.opStart );
		w.S64( f.opEnd );
		w.S64( f.totalLocals );
		w.S64( f.line );
	}

	// globals
	for ( int i = 0; i < program->numGlobals; i++ ) {
		const HfScriptGlobal & g = program->globals[ i ];
		w.Str( g.name.c_str() );
		w.S64( g.index );
	}

	byte * copy = (byte *)Mem_Alloc( w.used, TAG_SCRIPT );
	memcpy( copy, w.data, w.used );
	*outBuffer = copy;
	*outSize = w.used;
	return true;
}

/*
====================
HfScript_LoadImage
====================
*/
bool HfScript_LoadImage( HfScriptProgram * program, const void * buffer, int bufferSize ) {
	if ( program == NULL || buffer == NULL || bufferSize < 16 ) {
		return false;
	}

	HfImageReader r( buffer, bufferSize );
	unsigned magic, version, flags;
	if ( !r.U32( &magic ) || magic != HFS_IMAGE_MAGIC ) {
		program->error = "not an hfscript image";
		return false;
	}
	r.U32( &version );
	r.U32( &flags );
	if ( version != HFS_IMAGE_VERSION ) {
		program->error = "unsupported image version";
		return false;
	}

	uint64 numOps, numConstants, numFunctions, numGlobals, poolUsed;
	int64 entryOp, entryFunction;
	if ( !r.U64( &numOps ) || !r.U64( &numConstants ) || !r.U64( &numFunctions )
		|| !r.U64( &numGlobals ) || !r.U64( &poolUsed ) || !r.S64( &entryOp ) || !r.S64( &entryFunction ) ) {
		program->error = "truncated image header";
		return false;
	}
	if ( numOps > ( 1 << 22 ) || numConstants > ( 1 << 20 ) || numFunctions > ( 1 << 20 )
		|| numGlobals > ( 1 << 20 ) || poolUsed > ( 1 << 20 ) ) {
		program->error = "corrupt image counts";
		return false;
	}

	program->Reset();

	// string pool: reconstruct at the same fixed 1 MB base so pool pointers stay valid
	program->poolSize = 1 << 20;
	program->stringPool = (char *)Mem_ClearedAlloc( program->poolSize, TAG_SCRIPT );
	if ( !r.Raw( program->stringPool, (int)poolUsed ) ) {
		program->error = "truncated image pool";
		return false;
	}
	program->poolUsed = (int)poolUsed;

	// ops
	program->AllocOps( (int)numOps );
	program->numOps = (int)numOps;
	for ( int i = 0; i < program->numOps; i++ ) {
		unsigned o;
		int64 a, b;
		if ( !r.U32( &o ) || !r.S64( &a ) || !r.S64( &b ) ) {
			program->error = "truncated image ops";
			return false;
		}
		program->ops[ i ].op = (int32)o;
		program->ops[ i ].a = (int32)a;
		program->ops[ i ].b = (int32)b;
	}

	// constants
	program->constants = (HfScriptValue *)Mem_ClearedAlloc( sizeof( HfScriptValue ) * numConstants, TAG_SCRIPT );
	program->numConstants = (int)numConstants;
	program->constAlloc = (int)numConstants;
	for ( int i = 0; i < (int)numConstants; i++ ) {
		unsigned type;
		uint64 bits;
		if ( !r.U32( &type ) || !r.U64( &bits ) ) {
			program->error = "truncated image constants";
			return false;
		}
		HfScriptValue & cv = program->constants[ i ];
		cv = HfScriptValue::Nil();
		cv.type = (int)type;
		if ( (int)type == HST_STRING ) {
			// re-anchor the pointer into the reconstructed pool
			if ( bits >= (uint64)program->poolUsed ) {
				program->error = "bad image string index";
				return false;
			}
			cv.s = program->stringPool + (int)bits;
		} else if ( (int)type == HST_FLOAT ) {
			memcpy( &cv.f, &bits, sizeof( cv.f ) );
		} else {
			cv.i = (int64)bits;
		}
	}

	// functions
	program->functions = (HfScriptFunction *)Mem_ClearedAlloc( sizeof( HfScriptFunction ) * numFunctions, TAG_SCRIPT );
	program->numFunctions = (int)numFunctions;
	program->funcAlloc = (int)numFunctions;
	for ( int i = 0; i < (int)numFunctions; i++ ) {
		hfStr name;
		int64 p, s, e, t, l;
		if ( !r.Str( &name ) || !r.S64( &p ) || !r.S64( &s ) || !r.S64( &e ) || !r.S64( &t ) || !r.S64( &l ) ) {
			program->error = "truncated image functions";
			return false;
		}
		HfScriptFunction & f = program->functions[ i ];
		f = HfScriptFunction();
		f.name = name;
		f.numParams = (int)p;
		f.opStart = (int)s;
		f.opEnd = (int)e;
		f.totalLocals = (int)t;
		f.line = (int)l;
	}

	// globals
	program->globals = (HfScriptGlobal *)Mem_ClearedAlloc( sizeof( HfScriptGlobal ) * numGlobals, TAG_SCRIPT );
	program->numGlobals = (int)numGlobals;
	program->globalAlloc = (int)numGlobals;
	program->globalValues = (HfScriptValue *)Mem_Alloc( sizeof( HfScriptValue ) * numGlobals, TAG_SCRIPT );
	program->globalValuesSize = (int)numGlobals;
	for ( int i = 0; i < (int)numGlobals; i++ ) {
		hfStr name;
		int64 index;
		if ( !r.Str( &name ) || !r.S64( &index ) ) {
			program->error = "truncated image globals";
			return false;
		}
		HfScriptGlobal & g = program->globals[ i ];
		g = HfScriptGlobal();
		g.name = name;
		g.index = (int)index;
		program->globalValues[ i ] = HfScriptValue::Nil();
	}

	program->entryOp = (int)entryOp;
	program->entryFunction = (int)entryFunction;
	program->valid = true;
	program->sourceName = "<image>";
	return true;
}

/*
====================
HfScript_SaveImageToFile / HfScript_LoadImageFromFile
====================
*/
bool HfScript_SaveImageToFile( const HfScriptProgram * program, const char * path ) {
	void * buffer = NULL;
	int size = 0;
	if ( !HfScript_SaveImage( program, &buffer, &size ) ) {
		return false;
	}
	FILE * f = fopen( path, "wb" );
	if ( f == NULL ) {
		Mem_Free( buffer );
		return false;
	}
	bool ok = fwrite( buffer, 1, size, f ) == (size_t)size;
	fclose( f );
	Mem_Free( buffer );
	return ok;
}

bool HfScript_LoadImageFromFile( HfScriptProgram * program, const char * path ) {
	FILE * f = fopen( path, "rb" );
	if ( f == NULL ) {
		return false;
	}
	fseek( f, 0, SEEK_END );
	long size = ftell( f );
	fseek( f, 0, SEEK_SET );
	if ( size <= 0 ) {
		fclose( f );
		return false;
	}
	void * buffer = Mem_Alloc( size, TAG_SCRIPT );
	bool readOk = fread( buffer, 1, size, f ) == (size_t)size;
	fclose( f );
	if ( !readOk ) {
		Mem_Free( buffer );
		return false;
	}
	bool ok = HfScript_LoadImage( program, buffer, (int)size );
	Mem_Free( buffer );
	return ok;
}

/*
====================
HfScript_IsPythonSource
====================
*/
bool HfScript_IsPythonSource( const char * sourceName ) {
	if ( sourceName == NULL ) {
		return false;
	}
	int len = (int)strlen( sourceName );
	if ( len >= 3 && hfStr::Cmpn( sourceName + len - 3, ".py", 3 ) == 0 ) {
		return true;
	}
	return false;
}