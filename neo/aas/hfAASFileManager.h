#ifndef __AASFILEMANAGER_H__
#define __AASFILEMANAGER_H__

/*
===============================================================================

	AAS File Manager

===============================================================================
*/

class hfAASFileManager {
public:
	virtual						~hfAASFileManager() {}

	virtual hfAASFile *			LoadAAS( const char *fileName, unsigned int mapFileCRC ) = 0;
	virtual void				FreeAAS( hfAASFile *file ) = 0;
};

extern hfAASFileManager *		AASFileManager;

#endif /* !__AASFILEMANAGER_H__ */

