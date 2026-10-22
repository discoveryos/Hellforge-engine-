#ifndef __MODELMANAGER_H__
#define __MODELMANAGER_H__

/*
===============================================================================

	Model Manager

	Temporarily created models do not need to be added to the model manager.

===============================================================================
*/

class hfRenderModelManager {
public:
	virtual					~hfRenderModelManager() {}

	// registers console commands and clears the list
	virtual	void			Init() = 0;

	// frees all the models
	virtual	void			Shutdown() = 0;

	// called only by renderer::BeginLevelLoad
	virtual void			BeginLevelLoad() = 0;

	// called only by renderer::EndLevelLoad
	virtual void			EndLevelLoad() = 0;

	// called only by renderer::Preload
	virtual void			Preload( const hfPreloadManifest &manifest ) = 0;

	// allocates a new empty render model.
	virtual hfRenderModel *	AllocModel() = 0;

	// frees a render model
	virtual void			FreeModel( hfRenderModel *model ) = 0;

	// returns NULL if modelName is NULL or an empty string, otherwise
	// it will create a default model if not loadable
	virtual	hfRenderModel *	FindModel( const char *modelName ) = 0;

	// returns NULL if not loadable
	virtual	hfRenderModel *	CheckModel( const char *modelName ) = 0;

	// returns the default cube model
	virtual	hfRenderModel *	DefaultModel() = 0;

	// world map parsing will add all the inline models with this call
	virtual	void			AddModel( hfRenderModel *model ) = 0;

	// when a world map unloads, it removes its internal models from the list
	// before freeing them.
	// There may be an issue with multiple renderWorlds that share data...
	virtual	void			RemoveModel( hfRenderModel *model ) = 0;

	// the reloadModels console command calls this, but it can
	// also be explicitly invoked
	virtual	void			ReloadModels( bool forceAll = false ) = 0;

	// write "touchModel <model>" commands for each non-world-map model
	virtual	void			WritePrecacheCommands( hfFile *f ) = 0;

	// called during vid_restart
	virtual	void			FreeModelVertexCaches() = 0;

	// print memory info
	virtual	void			PrintMemInfo( MemInfo_t *mi ) = 0;
};

// this will be statically pointed at a private implementation
extern	hfRenderModelManager	*renderModelManager;

#endif /* !__MODELMANAGER_H__ */

