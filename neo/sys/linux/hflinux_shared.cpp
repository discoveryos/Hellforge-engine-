#include "../../hflib/hfprecompiled.h"
#include "hflinux_local.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <glob.h>
#include <errno.h>

/*
================
Sys_Milliseconds
================
*/
int Sys_Milliseconds() {
	static double secbase = 0;
	struct timeval tp;
	gettimeofday( &tp, NULL );
	if ( secbase == 0 ) {
		secbase = tp.tv_sec;
		return tp.tv_usec / 1000;
	}
	return (int)( ( tp.tv_sec - secbase ) * 1000 + tp.tv_usec / 1000 );
}

/*
================
Sys_Microseconds
================
*/
uint64 Sys_Microseconds() {
	struct timeval tp;
	gettimeofday( &tp, NULL );
	return (uint64)tp.tv_sec * 1000000 + tp.tv_usec;
}

/*
================
Sys_GetSystemRam
================
*/
int Sys_GetSystemRam() {
	long pages = sysconf( _SC_PHYS_PAGES );
	long pageSize = sysconf( _SC_PAGE_SIZE );
	if ( pages <= 0 || pageSize <= 0 ) {
		return 512;
	}
	return (int)( ( pages * pageSize ) / ( 1024 * 1024 ) );
}

/*
================
Sys_GetDriveFreeSpaceInBytes
================
*/
int64 Sys_GetDriveFreeSpaceInBytes( const char * path ) {
	struct statfs statbuf;
	if ( statfs( path, &statbuf ) == -1 ) {
		return 0;
	}
	return (int64)statbuf.f_bavail * statbuf.f_bsize;
}

/*
================
Sys_GetDriveFreeSpace
================
*/
int Sys_GetDriveFreeSpace( const char * path ) {
	return (int)( Sys_GetDriveFreeSpaceInBytes( path ) / ( 1024 * 1024 ) );
}

/*
================
Sys_GetVideoRam
================
*/
int Sys_GetVideoRam() {
	// Linux has no simple portable API; default to 256MB
	return 256;
}

/*
================
Sys_GetCurrentMemoryStatus
================
*/
void Sys_GetCurrentMemoryStatus( sysMemoryStats_t &stats ) {
	memset( &stats, 0, sizeof( stats ) );
	stats.totalPhysical = Sys_GetSystemRam();
	stats.availPhysical = stats.totalPhysical;
}

/*
================
Sys_GetExeLaunchMemoryStatus
================
*/
void Sys_GetExeLaunchMemoryStatus( sysMemoryStats_t &stats ) {
	memset( &stats, 0, sizeof( stats ) );
	stats.totalPhysical = Sys_GetSystemRam();
	stats.availPhysical = stats.totalPhysical;
}

/*
================
Sys_LockMemory
================
*/
bool Sys_LockMemory( void *ptr, int bytes ) {
	return ( mlock( ptr, bytes ) == 0 );
}

/*
================
Sys_UnlockMemory
================
*/
bool Sys_UnlockMemory( void *ptr, int bytes ) {
	return ( munlock( ptr, bytes ) == 0 );
}

/*
================
Sys_SetPhysicalWorkMemory
================
*/
void Sys_SetPhysicalWorkMemory( int minBytes, int maxBytes ) {
	// no-op on Linux
}

/*
================
Sys_GetCurrentUser
================
*/
char *Sys_GetCurrentUser() {
	static char name[256];
	struct passwd * pw = getpwuid( getuid() );
	if ( pw != NULL ) {
		hfStr::snPrintf( name, sizeof( name ), "%s", pw->pw_name );
	} else {
		hfStr::snPrintf( name, sizeof( name ), "player" );
	}
	return name;
}

/*
================
Sys_GetCallStack
================
*/
void Sys_GetCallStack( address_t *callStack, const int callStackSize ) {
	// Not fully implemented on Linux; leave entries zeroed.
	for ( int i = 0; i < callStackSize; i++ ) {
		callStack[i] = 0;
	}
}

/*
================
Sys_GetCallStackStr
================
*/
const char *Sys_GetCallStackStr( const address_t *callStack, const int callStackSize ) {
	return "";
}

/*
================
Sys_GetCallStackCurStr
================
*/
const char *Sys_GetCallStackCurStr( int depth ) {
	return "";
}

/*
================
Sys_GetCallStackCurAddressStr
================
*/
const char *Sys_GetCallStackCurAddressStr( int depth ) {
	return "";
}

/*
================
Sys_ShutdownSymbols
================
*/
void Sys_ShutdownSymbols() {
}

/*
================
Sys_Mkdir
================
*/
void Sys_Mkdir( const char *path ) {
	mkdir( path, 0777 );
}

/*
================
Sys_FileTimeStamp
================
*/
HF_TIME_T Sys_FileTimeStamp( hfFileHandle fp ) {
	struct stat st;
	int fd = fileno( fp );
	if ( fstat( fd, &st ) == -1 ) {
		return -1;
	}
	return (HF_TIME_T)st.st_mtime;
}

/*
================
Sys_Rmdir
================
*/
bool Sys_Rmdir( const char *path ) {
	return rmdir( path ) == 0;
}

/*
================
Sys_IsFileWritable
================
*/
bool Sys_IsFileWritable( const char *path ) {
	return access( path, W_OK ) == 0;
}

/*
================
Sys_IsFolder
================
*/
sysFolder_t Sys_IsFolder( const char *path ) {
	struct stat st;
	if ( stat( path, &st ) == -1 ) {
		return FOLDER_ERROR;
	}
	return S_ISDIR( st.st_mode ) ? FOLDER_YES : FOLDER_NO;
}

/*
================
Sys_Cwd
================
*/
const char *Sys_Cwd() {
	static char cwd[1024];
	if ( getcwd( cwd, sizeof( cwd ) ) == NULL ) {
		return "";
	}
	return cwd;
}

/*
================
Sys_EXEPath
================
*/
const char *Sys_EXEPath() {
	static char path[1024];
	ssize_t len = readlink( "/proc/self/exe", path, sizeof( path ) - 1 );
	if ( len > 0 ) {
		path[len] = '\0';
		return path;
	}
	return "hellforge";
}

/*
================
Sys_DefaultBasePath
================
*/
const char *Sys_DefaultBasePath() {
	return Sys_Cwd();
}

/*
================
Sys_DefaultSavePath
================
*/
const char *Sys_DefaultSavePath() {
	const char *home = getenv( "HOME" );
	static char path[1024];
	if ( home != NULL ) {
		hfStr::snPrintf( path, sizeof( path ), "%s/.local/share/hellforge", home );
	} else {
		return Sys_DefaultBasePath();
	}
	Sys_Mkdir( path );
	return path;
}

/*
================
Sys_ListFiles
================
*/
int Sys_ListFiles( const char *directory, const char *extension, hfStrList &list ) {
	DIR * dir = opendir( directory );
	if ( dir == NULL ) {
		return -1;
	}

	list.Clear();

	int extLen = extension != NULL ? strlen( extension ) : 0;

	struct dirent * entry;
	while ( ( entry = readdir( dir ) ) != NULL ) {
		if ( entry->d_name[0] == '.' ) {
			continue;
		}
		const char * name = entry->d_name;
		if ( extLen > 0 ) {
			int nameLen = (int)strlen( name );
			if ( nameLen < extLen || hfStr::Icmpn( name + nameLen - extLen, extension, extLen ) != 0 ) {
				continue;
			}
		}
		list.Append( name );
	}
	closedir( dir );

	// sort (case insensitive)
	for ( int i = 1; i < list.Num(); i++ ) {
		for ( int j = i; j > 0 && hfStr::Icmp( list[j], list[j - 1] ) < 0; j-- ) {
			hfStr tmp = list[j];
			list[j] = list[j - 1];
			list[j - 1] = tmp;
		}
	}
	return list.Num();
}

const char *Sys_LaunchPath() {
	return Sys_EXEPath();
}

