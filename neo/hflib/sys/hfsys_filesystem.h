#ifndef SYS_FILESYSTEM_H
#define SYS_FILESYSTEM_H

void			Sys_Mkdir( const char *path );
bool			Sys_Rmdir( const char *path );
bool			Sys_IsFileWritable( const char *path );

enum sysFolder_t {
	FOLDER_ERROR	= -1,
	FOLDER_NO		= 0,
	FOLDER_YES		= 1
};

// returns FOLDER_YES if the specified path is a folder
sysFolder_t		Sys_IsFolder( const char *path );

// use fs_debug to verbose Sys_ListFiles
// returns -1 if directory was not found (the list is cleared)
int				Sys_ListFiles( const char * directory, const char * extension, hfList<class hfStr> & list );

const char *	Sys_EXEPath();
const char *	Sys_CWD();

const char *	Sys_LaunchPath();

#endif

