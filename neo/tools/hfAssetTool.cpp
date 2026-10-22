// Offline asset pipeline tool. Standalone executable that links against
// libhflib (hfStr / HF_Asset) so the store format, .mtr parser and GLSL
// sanity checker are shared with the engine and the headless harness.
//
//   assettool pack    <dir> <out.hfpack>   pack every file under <dir>
//   assettool validate <file>              verify store integrity
//   assettool list    <file>               print asset names
//   assettool extract <file> <name>        dump one asset to stdout
//   assettool compile <file.glsl>          GLSL sanity check (glslang if present)
//   assettool mtrs    <dir>                parse .mtr + report texture refs

#include "../hflib/hfprecompiled.h"

#include "../hflib/hfAssetStore.h"
#include "../hflib/hfStr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace HF_Asset;

static int g_entries = 0;
static int g_capacity = 0;
static hfStr * g_names = NULL;
static char ** g_bytes = NULL;
static long * g_sizes = NULL;

static void reserveFiles( int n ) {
	if ( n <= g_capacity ) { return; }
	int nc = ( n + 64 ) & ~63;
	hfStr * newNames = new hfStr[nc];
	char ** newBytes = ( char ** )calloc( nc, sizeof( char * ) );
	long * newSizes = ( long * )calloc( nc, sizeof( long ) );
	for ( int i = 0; i < g_capacity; i++ ) {
		newNames[i] = g_names[i];
		newBytes[i] = g_bytes[i];
		newSizes[i] = g_sizes[i];
	}
	delete[] g_names;
	free( g_bytes );
	free( g_sizes );
	g_names = newNames;
	g_bytes = newBytes;
	g_sizes = newSizes;
	g_capacity = nc;
}

static void addFile( const char * relName, const char * fullPath ) {
	FILE * f = fopen( fullPath, "rb" );
	if ( !f ) { fprintf( stderr, "assettool: cannot open %s\n", fullPath ); return; }
	fseek( f, 0, SEEK_END );
	long size = ftell( f );
	fseek( f, 0, SEEK_SET );
	char * buf = ( char * )malloc( size ? ( size_t )size : 1 );
	if ( size ) { size_t rd = fread( buf, 1, size, f ); (void)rd; }
	fclose( f );
	reserveFiles( g_entries + 1 );
	g_names[g_entries] = relName;
	g_bytes[g_entries] = buf;
	g_sizes[g_entries] = size;
	g_entries++;
}

static void walkDir( const char * dir, const char * base ) {
	DIR * d = opendir( dir );
	if ( !d ) { fprintf( stderr, "assettool: cannot open dir %s\n", dir ); return; }
	struct dirent * e;
	hfStr basePrefix = base;
	while ( ( e = readdir( d ) ) != NULL ) {
		if ( !strcmp( e->d_name, "." ) || !strcmp( e->d_name, ".." ) ) { continue; }
		hfStr full = va( "%s/%s", dir, e->d_name );
		struct stat st;
		if ( stat( full.c_str(), &st ) != 0 ) { continue; }
		if ( S_ISDIR( st.st_mode ) ) {
			walkDir( full.c_str(), base );
		} else {
			hfStr rel = full;
			if ( rel.Find( basePrefix.c_str() ) == 0 ) { rel = rel.Mid( basePrefix.Length() + 1, rel.Length() ); }
			addFile( rel.c_str(), full.c_str() );
		}
	}
	closedir( d );
}

static AssetSource * buildSources( int * countOut ) {
	AssetSource * src = ( AssetSource * )calloc( g_entries, sizeof( AssetSource ) );
	for ( int i = 0; i < g_entries; i++ ) {
		src[i].name = g_names[i].c_str();
		src[i].data = g_bytes[i];
		src[i].size = ( unsigned int )g_sizes[i];
	}
	*countOut = g_entries;
	return src;
}

static int cmdPack( int argc, char * argv[] ) {
	if ( argc < 4 ) { fprintf( stderr, "usage: assettool pack <dir> <out.hfpack>\n" ); return 1; }
	walkDir( argv[2], argv[2] );
	int count = 0;
	AssetSource * src = buildSources( &count );
	PackedStore store = Pack( src, count );
	if ( !store.ok ) { fprintf( stderr, "assettool: pack failed\n" ); return 1; }
	hfStr err;
	if ( !Validate( store.data, store.size, &err ) ) {
		fprintf( stderr, "assettool: pack output invalid: %s\n", err.c_str() );
		FreeStore( store );
		return 1;
	}
	FILE * f = fopen( argv[3], "wb" );
	if ( !f ) { fprintf( stderr, "assettool: cannot write %s\n", argv[3] ); return 1; }
	fwrite( store.data, 1, store.size, f );
	fclose( f );
	printf( "assettool: packed %d assets (%d bytes) -> %s\n", count, store.size, argv[3] );
	FreeStore( store );
	return 0;
}

static int cmdValidate( int argc, char * argv[] ) {
	if ( argc < 3 ) { fprintf( stderr, "usage: assettool validate <file>\n" ); return 1; }
	FILE * f = fopen( argv[2], "rb" );
	if ( !f ) { fprintf( stderr, "assettool: cannot open %s\n", argv[2] ); return 1; }
	fseek( f, 0, SEEK_END ); long size = ftell( f ); fseek( f, 0, SEEK_SET );
	char * buf = ( char * )malloc( size ? ( size_t )size : 1 );
	if ( size ) { (void)fread( buf, 1, size, f ); }
	fclose( f );
	hfStr err;
	bool ok = Validate( ( const byte * )buf, ( int )size, &err );
	printf( "assettool: %s: %s\n", argv[2], ok ? "OK" : ( const char * )err.c_str() );
	free( buf );
	return ok ? 0 : 1;
}

static int cmdList( int argc, char * argv[] ) {
	if ( argc < 3 ) { fprintf( stderr, "usage: assettool list <file>\n" ); return 1; }
	FILE * f = fopen( argv[2], "rb" );
	if ( !f ) { fprintf( stderr, "assettool: cannot open %s\n", argv[2] ); return 1; }
	fseek( f, 0, SEEK_END ); long size = ftell( f ); fseek( f, 0, SEEK_SET );
	char * buf = ( char * )malloc( size ? ( size_t )size : 1 );
	if ( size ) { (void)fread( buf, 1, size, f ); }
	fclose( f );
	hfStr names;
	List( ( const byte * )buf, ( int )size, names );
	// extract back the real names is unavailable (hash-only) — list hashes+sizes
	printf( "%s", names.c_str() );
	free( buf );
	return 0;
}

static int cmdExtract( int argc, char * argv[] ) {
	if ( argc < 4 ) { fprintf( stderr, "usage: assettool extract <file> <name>\n" ); return 1; }
	FILE * f = fopen( argv[2], "rb" );
	if ( !f ) { fprintf( stderr, "assettool: cannot open %s\n", argv[2] ); return 1; }
	fseek( f, 0, SEEK_END ); long size = ftell( f ); fseek( f, 0, SEEK_SET );
	char * buf = ( char * )malloc( size ? ( size_t )size : 1 );
	if ( size ) { (void)fread( buf, 1, size, f ); }
	fclose( f );
	hfStr out;
	AssetEntry meta;
	if ( !Extract( ( const byte * )buf, ( int )size, argv[3], out, &meta ) ) {
		fprintf( stderr, "assettool: '%s' not in store\n", argv[3] );
		free( buf );
		return 1;
	}
	fwrite( out.c_str(), 1, out.Length(), stdout );
	printf( "\n" );
	free( buf );
	return 0;
}

static int cmdCompile( int argc, char * argv[] ) {
	if ( argc < 3 ) { fprintf( stderr, "usage: assettool compile <file.glsl>\n" ); return 1; }
	FILE * f = fopen( argv[2], "rb" );
	if ( !f ) { fprintf( stderr, "assettool: cannot open %s\n", argv[2] ); return 1; }
	fseek( f, 0, SEEK_END ); long size = ftell( f ); fseek( f, 0, SEEK_SET );
	char * buf = ( char * )malloc( size ? ( size_t )size : 1 );
	if ( size ) { (void)fread( buf, 1, size, f ); }
	fclose( f );
	hfStr err;
	bool ok = GlslSanityCheck( buf, &err );
	printf( "assettool: %s: %s%s\n", argv[2], ok ? "syntax-ok" : "SYNTAX FAIL", err.Length() ? va(" (%s)", err.c_str()) : "" );
	free( buf );
	return ok ? 0 : 1;
}

static int cmdMtrs( int argc, char * argv[] ) {
	if ( argc < 3 ) { fprintf( stderr, "usage: assettool mtrs <dir>\n" ); return 1; }
	walkDir( argv[2], argv[2] );
	for ( int i = 0; i < g_entries; i++ ) {
		if ( !strstr( g_names[i].c_str(), ".mtr" ) ) { continue; }
		mtrDef_t defs[64];
		int n = ParseMtr( g_bytes[i], ( int )g_sizes[i], defs, 64 );
		hfStr tx;
		CollectTextures( defs, n, tx );
		printf( "%s: %d defs\n%s", g_names[i].c_str(), n, tx.c_str() );
	}
	return 0;
}

int main( int argc, char * argv[] ) {
	if ( argc < 2 ) {
		fprintf( stderr,
			"assettool - offline asset pipeline\n"
			"  pack    <dir> <out.hfpack>  : pack every file under <dir>\n"
			"  validate <file>             : verify store integrity + adler32\n"
			"  list    <file>              : print asset hashes/sizes\n"
			"  extract <file> <name>       : dump one asset to stdout\n"
			"  compile <file.glsl>         : GLSL sanity check\n"
			"  mtrs    <dir>               : parse .mtr + list referenced textures\n" );
		return 0;
	}
	const char * cmd = argv[1];
	g_entries = 0;
	g_capacity = 0;
	g_names = NULL; g_bytes = NULL; g_sizes = NULL;
	int rc = 0;
	if ( !strcmp( cmd, "pack" ) ) { rc = cmdPack( argc, argv ); }
	else if ( !strcmp( cmd, "validate" ) ) { rc = cmdValidate( argc, argv ); }
	else if ( !strcmp( cmd, "list" ) ) { rc = cmdList( argc, argv ); }
	else if ( !strcmp( cmd, "extract" ) ) { rc = cmdExtract( argc, argv ); }
	else if ( !strcmp( cmd, "compile" ) ) { rc = cmdCompile( argc, argv ); }
	else if ( !strcmp( cmd, "mtrs" ) ) { rc = cmdMtrs( argc, argv ); }
	else { fprintf( stderr, "assettool: unknown command '%s'\n", cmd ); rc = 1; }
	// cleanup
	for ( int i = 0; i < g_capacity; i++ ) { if ( g_bytes[i] ) free( g_bytes[i] ); }
	delete[] g_names;
	free( g_bytes ); free( g_sizes );
	return rc;
}