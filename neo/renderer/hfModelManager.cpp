#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfModel_local.h"
#include "hftr_local.h"	// just for R_FreeWorldInteractions and R_CreateWorldInteractions

hfCVar r_binaryLoadRenderModels( "r_binaryLoadRenderModels", "1", 0, "enable binary load/write of render models" );
hfCVar preload_MapModels( "preload_MapModels", "1", CVAR_SYSTEM | CVAR_BOOL, "preload models during begin or end levelload" );

class hfRenderModelManagerLocal : public hfRenderModelManager {
public:
							hfRenderModelManagerLocal();
	virtual					~hfRenderModelManagerLocal() {}

	virtual void			Init();
	virtual void			Shutdown();
	virtual hfRenderModel *	AllocModel();
	virtual void			FreeModel( hfRenderModel *model );
	virtual hfRenderModel *	FindModel( const char *modelName );
	virtual hfRenderModel *	CheckModel( const char *modelName );
	virtual hfRenderModel *	DefaultModel();
	virtual void			AddModel( hfRenderModel *model );
	virtual void			RemoveModel( hfRenderModel *model );
	virtual void			ReloadModels( bool forceAll = false );
	virtual void			FreeModelVertexCaches();
	virtual void			WritePrecacheCommands( hfFile *file );
	virtual void			BeginLevelLoad();
	virtual void			EndLevelLoad();
	virtual void			Preload( const hfPreloadManifest &manifest );

	virtual	void			PrintMemInfo( MemInfo_t *mi );

private:
	hfList<hfRenderModel*, TAG_MODEL>	models;
	hfHashIndex				hash;
	hfRenderModel *			defaultModel;
	hfRenderModel *			beamModel;
	hfRenderModel *			spriteModel;
	bool					insideLevelLoad;		// don't actually load now

	hfRenderModel *			GetModel( const char *modelName, bool createIfNotFound );

	static void				PrintModel_f( const hfCmdArgs &args );
	static void				ListModels_f( const hfCmdArgs &args );
	static void				ReloadModels_f( const hfCmdArgs &args );
	static void				TouchModel_f( const hfCmdArgs &args );
};


hfRenderModelManagerLocal	localModelManager;
hfRenderModelManager *		renderModelManager = &localModelManager;

/*
==============
hfRenderModelManagerLocal::hfRenderModelManagerLocal
==============
*/
hfRenderModelManagerLocal::hfRenderModelManagerLocal() {
	defaultModel = NULL;
	beamModel = NULL;
	spriteModel = NULL;
	insideLevelLoad = false;
}

/*
==============
hfRenderModelManagerLocal::PrintModel_f
==============
*/
void hfRenderModelManagerLocal::PrintModel_f( const hfCmdArgs &args ) {
	hfRenderModel	*model;

	if ( args.Argc() != 2 ) {
		common->Printf( "usage: printModel <modelName>\n" );
		return;
	}

	model = renderModelManager->CheckModel( args.Argv( 1 ) );
	if ( !model ) {
		common->Printf( "model \"%s\" not found\n", args.Argv( 1 ) );
		return;
	}

	model->Print();
}

/*
==============
hfRenderModelManagerLocal::ListModels_f
==============
*/
void hfRenderModelManagerLocal::ListModels_f( const hfCmdArgs &args ) {
	int		totalMem = 0;
	int		inUse = 0;

	common->Printf( " mem   srf verts tris\n" );
	common->Printf( " ---   --- ----- ----\n" );

	for ( int i = 0; i < localModelManager.models.Num(); i++ ) {
		hfRenderModel	*model = localModelManager.models[i];

		if ( !model->IsLoaded() ) {
			continue;
		}
		model->List();
		totalMem += model->Memory();
		inUse++;
	}

	common->Printf( " ---   --- ----- ----\n" );
	common->Printf( " mem   srf verts tris\n" );

	common->Printf( "%i loaded models\n", inUse );
	common->Printf( "total memory: %4.1fM\n", (float)totalMem / (1024*1024) );
}

/*
==============
hfRenderModelManagerLocal::ReloadModels_f
==============
*/
void hfRenderModelManagerLocal::ReloadModels_f( const hfCmdArgs &args ) {
	if ( hfStr::Icmp( args.Argv(1), "all" ) == 0 ) {
		localModelManager.ReloadModels( true );
	} else {
		localModelManager.ReloadModels( false );
	}
}

/*
==============
hfRenderModelManagerLocal::TouchModel_f

Precache a specific model
==============
*/
void hfRenderModelManagerLocal::TouchModel_f( const hfCmdArgs &args ) {
	const char	*model = args.Argv( 1 );

	if ( !model[0] ) {
		common->Printf( "usage: touchModel <modelName>\n" );
		return;
	}

	common->Printf( "touchModel %s\n", model );
	const bool captureToImage = false;
	common->UpdateScreen( captureToImage );
	hfRenderModel *m = renderModelManager->CheckModel( model );
	if ( !m ) {
		common->Printf( "...not found\n" );
	}
}

/*
=================
hfRenderModelManagerLocal::WritePrecacheCommands
=================
*/
void hfRenderModelManagerLocal::WritePrecacheCommands( hfFile *f ) {
	for ( int i = 0; i < models.Num(); i++ ) {
		hfRenderModel	*model = models[i];

		if ( !model ) {
			continue;
		}
		if ( !model->IsReloadable() ) {
			continue;
		}

		char	str[1024];
		sprintf( str, "touchModel %s\n", model->Name() );
		common->Printf( "%s", str );
		f->Printf( "%s", str );
	}
}

/*
=================
hfRenderModelManagerLocal::Init
=================
*/
void hfRenderModelManagerLocal::Init() {
	cmdSystem->AddCommand( "listModels", ListModels_f, CMD_FL_RENDERER, "lists all models" );
	cmdSystem->AddCommand( "printModel", PrintModel_f, CMD_FL_RENDERER, "prints model info", hfCmdSystem::ArgCompletion_ModelName );
	cmdSystem->AddCommand( "reloadModels", ReloadModels_f, CMD_FL_RENDERER|CMD_FL_CHEAT, "reloads models" );
	cmdSystem->AddCommand( "touchModel", TouchModel_f, CMD_FL_RENDERER, "touches a model", hfCmdSystem::ArgCompletion_ModelName );

	insideLevelLoad = false;

	// create a default model
	hfRenderModelStatic *model = new (TAG_MODEL) hfRenderModelStatic;
	model->InitEmpty( "_DEFAULT" );
	model->MakeDefaultModel();
	model->SetLevelLoadReferenced( true );
	defaultModel = model;
	AddModel( model );

	// create the beam model
	hfRenderModelStatic *beam = new (TAG_MODEL) hfRenderModelBeam;
	beam->InitEmpty( "_BEAM" );
	beam->SetLevelLoadReferenced( true );
	beamModel = beam;
	AddModel( beam );

	hfRenderModelStatic *sprite = new (TAG_MODEL) hfRenderModelSprite;
	sprite->InitEmpty( "_SPRITE" );
	sprite->SetLevelLoadReferenced( true );
	spriteModel = sprite;
	AddModel( sprite );
}

/*
=================
hfRenderModelManagerLocal::Shutdown
=================
*/
void hfRenderModelManagerLocal::Shutdown() {
	models.DeleteContents( true );
	hash.Free();
}

/*
=================
hfRenderModelManagerLocal::GetModel
=================
*/
hfRenderModel *hfRenderModelManagerLocal::GetModel( const char *_modelName, bool createIfNotFound ) {

	if ( !_modelName || !_modelName[0] ) {
		return NULL;
	}

	hfStrStatic< MAX_OSPATH > canonical = _modelName;
	canonical.ToLower();

	hfStrStatic< MAX_OSPATH > extension;
	canonical.ExtractFileExtension( extension );

	// see if it is already present
	int key = hash.GenerateKey( canonical, false );
	for ( int i = hash.First( key ); i != -1; i = hash.Next( i ) ) {
		hfRenderModel *model = models[i];

		if ( canonical.Icmp( model->Name() ) == 0 ) {
			if ( !model->IsLoaded() ) {
				// reload it if it was purged
				hfStr generatedFileName = "generated/rendermodels/";
				generatedFileName.AppendPath( canonical );
				generatedFileName.SetFileExtension( va( "b%s", extension.c_str() ) );
				if ( model->SupportsBinaryModel() && r_binaryLoadRenderModels.GetBool() ) {
					hfFileLocal file( fileSystem->OpenFileReadMemory( generatedFileName ) );
					model->PurgeModel();
					if ( !model->LoadBinaryModel( file, 0 ) ) {
						model->LoadModel();
					}
				} else {
					model->LoadModel();
				}
			} else if ( insideLevelLoad && !model->IsLevelLoadReferenced() ) {
				// we are reusing a model already in memory, but
				// touch all the materials to make sure they stay
				// in memory as well
				model->TouchData();
			}
			model->SetLevelLoadReferenced( true );
			return model;
		}
	}

	// see if we can load it

	// determine which subclass of hfRenderModel to initialize

	hfRenderModel * model = NULL;

	if ( ( extension.Icmp( "ase" ) == 0 ) || ( extension.Icmp( "lwo" ) == 0 ) || ( extension.Icmp( "flt" ) == 0 ) || ( extension.Icmp( "ma" ) == 0 ) ) {
		model = new (TAG_MODEL) hfRenderModelStatic;
	} else if ( extension.Icmp( MD5_MESH_EXT ) == 0 ) {
		model = new (TAG_MODEL) hfRenderModelMD5;
	} else if ( extension.Icmp( "md3" ) == 0 ) {
		model = new (TAG_MODEL) hfRenderModelMD3;
	} else if ( extension.Icmp( "prt" ) == 0  ) {
		model = new (TAG_MODEL) hfRenderModelPrt;
	} else if ( extension.Icmp( "liquid" ) == 0  ) {
		model = new (TAG_MODEL) hfRenderModelLiquid;
	}

	hfStrStatic< MAX_OSPATH > generatedFileName;

	if ( model != NULL ) {

		generatedFileName = "generated/rendermodels/";
		generatedFileName.AppendPath( canonical );
		generatedFileName.SetFileExtension( va( "b%s", extension.c_str() ) );

		// Get the timestamp on the original file, if it's newer than what is stored in binary model, regenerate it
		HF_TIME_T sourceTimeStamp = fileSystem->GetTimestamp( canonical );

		hfFileLocal file( fileSystem->OpenFileReadMemory( generatedFileName ) );

		if ( !model->SupportsBinaryModel() || !r_binaryLoadRenderModels.GetBool() ) {
			model->InitFromFile( canonical );
		} else {
			if ( !model->LoadBinaryModel( file, sourceTimeStamp ) ) {
				model->InitFromFile( canonical );

				hfFileLocal outputFile( fileSystem->OpenFileWrite( generatedFileName, "fs_basepath" ) );
				hfLib::Printf( "Writing %s\n", generatedFileName.c_str() );
				model->WriteBinaryModel( outputFile );
			} /* else {
				hfLib::Printf( "loaded binary model %s from file %s\n", model->Name(), generatedFileName.c_str() );
			} */
		} 
	}

	// Not one of the known formats
	if ( model == NULL ) {

		if ( extension.Length() ) {
			common->Warning( "unknown model type '%s'", canonical.c_str() );
		}

		if ( !createIfNotFound ) {
			return NULL;
		}

		hfRenderModelStatic	*smodel = new (TAG_MODEL) hfRenderModelStatic;
		smodel->InitEmpty( canonical );
		smodel->MakeDefaultModel();

		model = smodel;
	}

	if ( cvarSystem->GetCVarBool( "fs_buildresources" ) ) {
		fileSystem->AddModelPreload( canonical );
	}
	model->SetLevelLoadReferenced( true );

	if ( !createIfNotFound && model->IsDefaultModel() ) {
		delete model;
		model = NULL;

		return NULL;
	}

	if ( cvarSystem->GetCVarBool( "fs_buildgame" ) ) {
		fileSystem->AddModelPreload( model->Name() );
	}

	AddModel( model );

	return model;
}

/*
=================
hfRenderModelManagerLocal::AllocModel
=================
*/
hfRenderModel *hfRenderModelManagerLocal::AllocModel() {
	return new (TAG_MODEL) hfRenderModelStatic();
}

/*
=================
hfRenderModelManagerLocal::FreeModel
=================
*/
void hfRenderModelManagerLocal::FreeModel( hfRenderModel *model ) {
	if ( !model ) {
		return;
	}
	if ( !dynamic_cast<hfRenderModelStatic *>( model ) ) {
		common->Error( "hfRenderModelManager::FreeModel: model '%s' is not a static model", model->Name() );
		return;
	}
	if ( model == defaultModel ) {
		common->Error( "hfRenderModelManager::FreeModel: can't free the default model" );
		return;
	}
	if ( model == beamModel ) {
		common->Error( "hfRenderModelManager::FreeModel: can't free the beam model" );
		return;
	}
	if ( model == spriteModel ) { 
		common->Error( "hfRenderModelManager::FreeModel: can't free the sprite model" );
		return;
	}

	R_CheckForEntityDefsUsingModel( model );

	delete model;
}

/*
=================
hfRenderModelManagerLocal::FindModel
=================
*/
hfRenderModel *hfRenderModelManagerLocal::FindModel( const char *modelName ) {
	return GetModel( modelName, true );
}

/*
=================
hfRenderModelManagerLocal::CheckModel
=================
*/
hfRenderModel *hfRenderModelManagerLocal::CheckModel( const char *modelName ) {
	return GetModel( modelName, false );
}

/*
=================
hfRenderModelManagerLocal::DefaultModel
=================
*/
hfRenderModel *hfRenderModelManagerLocal::DefaultModel() {
	return defaultModel;
}

/*
=================
hfRenderModelManagerLocal::AddModel
=================
*/
void hfRenderModelManagerLocal::AddModel( hfRenderModel *model ) {
	hash.Add( hash.GenerateKey( model->Name(), false ), models.Append( model ) );
}

/*
=================
hfRenderModelManagerLocal::RemoveModel
=================
*/
void hfRenderModelManagerLocal::RemoveModel( hfRenderModel *model ) {
	int index = models.FindIndex( model );
	if ( index != -1 ) {
		hash.RemoveIndex( hash.GenerateKey( model->Name(), false ), index );
		models.RemoveIndex( index );
	}
}

/*
=================
hfRenderModelManagerLocal::ReloadModels
=================
*/
void hfRenderModelManagerLocal::ReloadModels( bool forceAll ) {
	if ( forceAll ) {
		common->Printf( "Reloading all model files...\n" );
	} else {
		common->Printf( "Checking for changed model files...\n" );
	}

	R_FreeDerivedData();

	// skip the default model at index 0
	for ( int i = 1; i < models.Num(); i++ ) {
		hfRenderModel	*model = models[i];

		// we may want to allow world model reloading in the future, but we don't now
		if ( !model->IsReloadable() ) {
			continue;
		}

		if ( !forceAll ) {
			// check timestamp
			HF_TIME_T current;

			fileSystem->ReadFile( model->Name(), NULL, &current );
			if ( current <= model->Timestamp() ) {
				continue;
			}
		}

		common->DPrintf( "reloading %s.\n", model->Name() );

		model->LoadModel();
	}

	// we must force the world to regenerate, because models may
	// have changed size, making their references invalid
	R_ReCreateWorldReferences();
}

/*
=================
hfRenderModelManagerLocal::FreeModelVertexCaches
=================
*/
void hfRenderModelManagerLocal::FreeModelVertexCaches() {
	for ( int i = 0; i < models.Num(); i++ ) {
		hfRenderModel *model = models[i];
		model->FreeVertexCache();
	}
}

/*
=================
hfRenderModelManagerLocal::BeginLevelLoad
=================
*/
void hfRenderModelManagerLocal::BeginLevelLoad() {
	insideLevelLoad = true;

	for ( int i = 0; i < models.Num(); i++ ) {
		hfRenderModel *model = models[i];

		// always reload all models 
		if ( model->IsReloadable() ) {
			R_CheckForEntityDefsUsingModel( model );
			model->PurgeModel();
		}

		model->SetLevelLoadReferenced( false );
	}

	vertexCache.FreeStaticData();
}

/*
=================
hfRenderModelManagerLocal::Preload
=================
*/
void hfRenderModelManagerLocal::Preload( const hfPreloadManifest &manifest  ) {
	if ( preload_MapModels.GetBool() ) {
		// preload this levels images
		int	start = Sys_Milliseconds();
		int numLoaded = 0;
		hfList< preloadSort_t > preloadSort;
		preloadSort.Resize( manifest.NumResources() );
		for ( int i = 0; i < manifest.NumResources(); i++ ) {
			const preloadEntry_s & p = manifest.GetPreloadByIndex( i );
			hfResourceCacheEntry rc;
			hfStrStatic< MAX_OSPATH > filename;
			if ( p.resType == PRELOAD_MODEL ) {
				filename = "generated/rendermodels/";
				filename += p.resourceName;
				hfStrStatic< 16 > ext;
				filename.ExtractFileExtension( ext );
				filename.SetFileExtension( va( "b%s", ext.c_str() ) );
			}
			if ( p.resType == PRELOAD_PARTICLE ) {
				filename = "generated/particles/";
				filename += p.resourceName;
				filename += ".bprt";
			}
			if ( !filename.IsEmpty() ) {
				if ( fileSystem->GetResourceCacheEntry( filename, rc ) ) {
					preloadSort_t ps = {};
					ps.idx = i;
					ps.ofs = rc.offset;
					preloadSort.Append( ps );
				}
			}
		}
		
		preloadSort.SortWithTemplate( hfSort_Preload() );

		for ( int i = 0; i < preloadSort.Num(); i++ ) {
			const preloadSort_t & ps = preloadSort[ i ];
			const preloadEntry_s & p = manifest.GetPreloadByIndex( ps.idx );
			if ( p.resType == PRELOAD_MODEL ) {
				hfRenderModel * model = FindModel( p.resourceName );
				if ( model != NULL ) {
					model->SetLevelLoadReferenced( true );
				}
			} else if ( p.resType == PRELOAD_PARTICLE ) {
				declManager->FindType( DECL_PARTICLE, p.resourceName );
			}
			numLoaded++;
		}

		int	end = Sys_Milliseconds();
		common->Printf( "%05d models preloaded ( or were already loaded ) in %5.1f seconds\n", numLoaded, ( end - start ) * 0.001 );
		common->Printf( "----------------------------------------\n" );
	}
}



/*
=================
hfRenderModelManagerLocal::EndLevelLoad
=================
*/
void hfRenderModelManagerLocal::EndLevelLoad() {
	common->Printf( "----- hfRenderModelManagerLocal::EndLevelLoad -----\n" );

	int start = Sys_Milliseconds();

	insideLevelLoad = false;
	int	purgeCount = 0;
	int	keepCount = 0;
	int	loadCount = 0;

	// purge any models not touched
	for ( int i = 0; i < models.Num(); i++ ) {
		hfRenderModel *model = models[i];

		if ( !model->IsLevelLoadReferenced() && model->IsLoaded() && model->IsReloadable() ) {

//			common->Printf( "purging %s\n", model->Name() );

			purgeCount++;

			R_CheckForEntityDefsUsingModel( model );

			model->PurgeModel();

		} else {

//			common->Printf( "keeping %s\n", model->Name() );

			keepCount++;
		}

		common->UpdateLevelLoadPacifier();
	}

	// load any new ones
	for ( int i = 0; i < models.Num(); i++ ) {
		common->UpdateLevelLoadPacifier();


		hfRenderModel *model = models[i];

		if ( model->IsLevelLoadReferenced() && !model->IsLoaded() && model->IsReloadable() ) {
			loadCount++;
			model->LoadModel();
		}
	}

	// create static vertex/index buffers for all models
	for ( int i = 0; i < models.Num(); i++ ) {
		common->UpdateLevelLoadPacifier();


		hfRenderModel *model = models[i];
		if ( model->IsLoaded() ) {
			for ( int j = 0; j < model->NumSurfaces(); j++ ) {
				R_CreateStaticBuffersForTri( *(model->Surface( j )->geometry) );
			}
		}
	}


	// _D3XP added this
	int	end = Sys_Milliseconds();
	common->Printf( "%5i models purged from previous level, ", purgeCount );
	common->Printf( "%5i models kept.\n", keepCount );
	if ( loadCount ) {
		common->Printf( "%5i new models loaded in %5.1f seconds\n", loadCount, (end-start) * 0.001 );
	}
	common->Printf( "---------------------------------------------------\n" );
}

/*
=================
hfRenderModelManagerLocal::PrintMemInfo
=================
*/
void hfRenderModelManagerLocal::PrintMemInfo( MemInfo_t *mi ) {
	int i, j, totalMem = 0;
	int *sortIndex;
	hfFile *f;

	f = fileSystem->OpenFileWrite( mi->filebase + "_models.txt" );
	if ( !f ) {
		return;
	}

	// sort first
	sortIndex = new (TAG_MODEL) int[ localModelManager.models.Num()];

	for ( i = 0; i <  localModelManager.models.Num(); i++ ) {
		sortIndex[i] = i;
	}

	for ( i = 0; i <  localModelManager.models.Num() - 1; i++ ) {
		for ( j = i + 1; j <  localModelManager.models.Num(); j++ ) {
			if (  localModelManager.models[sortIndex[i]]->Memory() <  localModelManager.models[sortIndex[j]]->Memory() ) {
				int temp = sortIndex[i];
				sortIndex[i] = sortIndex[j];
				sortIndex[j] = temp;
			}
		}
	}

	// print next
	for ( int i = 0; i < localModelManager.models.Num(); i++ ) {
		hfRenderModel	*model = localModelManager.models[sortIndex[i]];
		int mem;

		if ( !model->IsLoaded() ) {
			continue;
		}

		mem = model->Memory();
		totalMem += mem;
		f->Printf( "%s %s\n", hfStr::FormatNumber( mem ).c_str(), model->Name() );
	}

	delete [] sortIndex;
	mi->modelAssetsTotal = totalMem;

	f->Printf( "\nTotal model bytes allocated: %s\n", hfStr::FormatNumber( totalMem ).c_str() );
	fileSystem->CloseFile( f );
}

