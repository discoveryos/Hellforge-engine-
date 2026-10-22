#pragma hdrstop
#include "../hflib/hfprecompiled.h"


#include "hfAASFile.h"
#include "hfAASFile_local.h"

/*
===============================================================================

	AAS File Manager

===============================================================================
*/

class hfAASFileManagerLocal : public hfAASFileManager {
public:
	virtual						~hfAASFileManagerLocal() {}

	virtual hfAASFile *			LoadAAS( const char *fileName, unsigned int mapFileCRC );
	virtual void				FreeAAS( hfAASFile *file );
};

hfAASFileManagerLocal			AASFileManagerLocal;
hfAASFileManager *				AASFileManager = &AASFileManagerLocal;


/*
================
hfAASFileManagerLocal::LoadAAS
================
*/
hfAASFile *hfAASFileManagerLocal::LoadAAS( const char *fileName, unsigned int mapFileCRC ) {
	hfAASFileLocal *file = new (TAG_AAS) hfAASFileLocal();
	if ( !file->Load( fileName, mapFileCRC ) ) {
		delete file;
		return NULL;
	}
	return file;
}

/*
================
hfAASFileManagerLocal::FreeAAS
================
*/
void hfAASFileManagerLocal::FreeAAS( hfAASFile *file ) {
	delete file;
}

