#pragma hdrstop
#include "../hflib/hfprecompiled.h"


#include "hftr_local.h"

// do this with a pointer, in case we want to make the actual manager
// a private virtual subclass
hfImageManager	imageManager;
hfImageManager * globalImages = &imageManager;

hfCVar preLoad_Images( "preLoad_Images", "1", CVAR_SYSTEM | CVAR_BOOL, "preload images during beginlevelload" );

/*
===============
R_ReloadImages_f

Regenerate all images that came directly from files that have changed, so
any saved changes will show up in place.

New r_texturesize/r_texturedepth variables will take effect on reload

reloadImages <all>
===============
*/
void R_ReloadImages_f( const hfCmdArgs &args ) {
	bool all = false;

	if ( args.Argc() == 2 ) {
		if ( !hfStr::Icmp( args.Argv(1), "all" ) ) {
			all = true;
		} else {
			common->Printf( "USAGE: reloadImages <all>\n" );
			return;
		}
	}

	globalImages->ReloadImages( all );
}

typedef struct {
	hfImage	*image;
	int		size;
	int		index;
} sortedImage_t;

/*
=======================
R_QsortImageSizes

=======================
*/
static int R_QsortImageSizes( const void *a, const void *b ) {
	const sortedImage_t	*ea, *eb;

	ea = (sortedImage_t *)a;
	eb = (sortedImage_t *)b;

	if ( ea->size > eb->size ) {
		return -1;
	}
	if ( ea->size < eb->size ) {
		return 1;
	}
	return hfStr::Icmp( ea->image->GetName(), eb->image->GetName() );
}

/*
=======================
R_QsortImageName

=======================
*/
static int R_QsortImageName( const void* a, const void* b ) {
	const sortedImage_t	*ea, *eb;

	ea = (sortedImage_t *)a;
	eb = (sortedImage_t *)b;

	return hfStr::Icmp( ea->image->GetName(), eb->image->GetName() );
}

/*
===============
R_ListImages_f
===============
*/
void R_ListImages_f( const hfCmdArgs &args ) {
	int		i, partialSize;
	hfImage	*image;
	int		totalSize;
	int		count = 0;
	bool	uncompressedOnly = false;
	bool	unloaded = false;
	bool	failed = false;
	bool	sorted = false;
	bool	duplicated = false;
	bool	overSized = false;
	bool	sortByName = false;

	if ( args.Argc() == 1 ) {

	} else if ( args.Argc() == 2 ) {
		if ( hfStr::Icmp( args.Argv( 1 ), "uncompressed" ) == 0 ) {
			uncompressedOnly = true;
		} else if ( hfStr::Icmp( args.Argv( 1 ), "sorted" ) == 0 ) {
			sorted = true;
		} else if ( hfStr::Icmp( args.Argv( 1 ), "namesort" ) == 0 ) {
			sortByName = true;
		} else if ( hfStr::Icmp( args.Argv( 1 ), "unloaded" ) == 0 ) {
			unloaded = true;
		} else if ( hfStr::Icmp( args.Argv( 1 ), "duplicated" ) == 0 ) {
			duplicated = true;
		} else if ( hfStr::Icmp( args.Argv( 1 ), "oversized" ) == 0 ) {
			sorted = true;
			overSized = true;
		} else {
			failed = true;
		}
	} else {
		failed = true;
	}

	if ( failed ) {
		common->Printf( "usage: listImages [ sorted | namesort | unloaded | duplicated | showOverSized ]\n" );
		return;
	}

	const char *header = "       -w-- -h-- filt -fmt-- wrap  size --name-------\n";
	common->Printf( "\n%s", header );

	totalSize = 0;

	sortedImage_t	*sortedArray = (sortedImage_t *)alloca( sizeof( sortedImage_t ) * globalImages->images.Num() );

	for ( i = 0 ; i < globalImages->images.Num() ; i++ ) {
		image = globalImages->images[ i ];

		if ( uncompressedOnly ) {
			if ( image->IsCompressed() ) {
				continue;
			}
		}
		if ( unloaded == image->IsLoaded() ) {
			continue;
		}

		// only print duplicates (from mismatched wrap / clamp, etc)
		if ( duplicated ) {
			int j;
			for ( j = i+1 ; j < globalImages->images.Num() ; j++ ) {
				if ( hfStr::Icmp( image->GetName(), globalImages->images[ j ]->GetName() ) == 0 ) {
					break;
				}
			}
			if ( j == globalImages->images.Num() ) {
				continue;
			}
		}

		if ( sorted || sortByName ) {
			sortedArray[count].image = image;
			sortedArray[count].size = image->StorageSize();
			sortedArray[count].index = i;
		} else {
			common->Printf( "%4i:",	i );
			image->Print();
		}
		totalSize += image->StorageSize();
		count++;
	}

	if ( sorted || sortByName ) {
		if ( sortByName) {
			qsort( sortedArray, count, sizeof( sortedImage_t ), R_QsortImageName );
		} else {
			qsort( sortedArray, count, sizeof( sortedImage_t ), R_QsortImageSizes );
		}
		partialSize = 0;
		for ( i = 0 ; i < count ; i++ ) {
			common->Printf( "%4i:",	sortedArray[i].index );
			sortedArray[i].image->Print();
			partialSize += sortedArray[i].image->StorageSize();
			if ( ( (i+1) % 10 ) == 0 ) {
				common->Printf( "-------- %5.1f of %5.1f megs --------\n", 
					partialSize / (1024*1024.0), totalSize / (1024*1024.0) );
			}
		}
	}

	common->Printf( "%s", header );
	common->Printf( " %i images (%i total)\n", count, globalImages->images.Num() );
	common->Printf( " %5.1f total megabytes of images\n\n\n", totalSize / (1024*1024.0) );
}

/*
==============
AllocImage

Allocates an hfImage, adds it to the list,
copies the name, and adds it to the hash chain.
==============
*/
hfImage *hfImageManager::AllocImage( const char *name ) {
	if (strlen(name) >= MAX_IMAGE_NAME ) {
		common->Error ("hfImageManager::AllocImage: \"%s\" is too long\n", name);
	}

	int hash = hfStr( name ).FileNameHash();

	hfImage * image = new (TAG_IMAGE) hfImage( name );

	imageHash.Add( hash, images.Append( image ) );

	return image;
}

/*
==============
AllocStandaloneImage

Allocates an hfImage,does not add it to the list or hash chain

==============
*/
hfImage *hfImageManager::AllocStandaloneImage( const char *name ) {
	if (strlen(name) >= MAX_IMAGE_NAME ) {
		common->Error ("hfImageManager::AllocImage: \"%s\" is too long\n", name);
	}

	hfImage * image = new (TAG_IMAGE) hfImage( name );

	return image;
}

/*
==================
ImageFromFunction

Images that are procedurally generated are allways specified
with a callback which must work at any time, allowing the OpenGL
system to be completely regenerated if needed.
==================
*/
hfImage *hfImageManager::ImageFromFunction( const char *_name, void (*generatorFunction)( hfImage *image ) ) {

	// strip any .tga file extensions from anywhere in the _name
	hfStr name = _name;
	name.Replace( ".tga", "" );
	name.BackSlashesToSlashes();

	// see if the image already exists
	int hash = name.FileNameHash();
	for ( int i = imageHash.First( hash ); i != -1; i = imageHash.Next( i ) ) {
		hfImage * image = images[i];
		if ( name.Icmp( image->GetName() ) == 0 ) {
			if ( image->generatorFunction != generatorFunction ) {
				common->DPrintf( "WARNING: reused image %s with mixed generators\n", name.c_str() );
			}
			return image;
		}
	}

	// create the image and issue the callback
	hfImage	* image = AllocImage( name );

	image->generatorFunction = generatorFunction;

	// check for precompressed, load is from the front end
	image->referencedOutsideLevelLoad = true;
	image->ActuallyLoadImage( false );

	return image;
}


/*
===============
GetImageWithParameters
==============
*/
hfImage	*hfImageManager::GetImageWithParameters( const char *_name, textureFilter_t filter, textureRepeat_t repeat, textureUsage_t usage, cubeFiles_t cubeMap ) const {
	if ( !_name || !_name[0] || hfStr::Icmp( _name, "default" ) == 0 || hfStr::Icmp( _name, "_default" ) == 0 ) {
		declManager->MediaPrint( "DEFAULTED\n" );
		return globalImages->defaultImage;
	}
	if ( hfStr::Icmpn( _name, "fonts", 5 ) == 0 || hfStr::Icmpn( _name, "newfonts", 8 ) == 0 ) {
		usage = TD_FONT;
	}
	if ( hfStr::Icmpn( _name, "lights", 6 ) == 0 ) {
		usage = TD_LIGHT;
	}
	// strip any .tga file extensions from anywhere in the _name, including image program parameters
	hfStrStatic< MAX_OSPATH > name = _name;
	name.Replace( ".tga", "" );
	name.BackSlashesToSlashes();
	int hash = name.FileNameHash();
	for ( int i = imageHash.First( hash ); i != -1; i = imageHash.Next( i ) ) {
		hfImage	* image = images[i];
		if ( name.Icmp( image->GetName() ) == 0 ) {
			// the built in's, like _white and _flat always match the other options
			if ( name[0] == '_' ) {
				return image;
			}
			if ( image->cubeFiles != cubeMap ) {
				common->Error( "Image '%s' has been referenced with conflicting cube map states", _name );
			}
			if ( image->filter != filter || image->repeat != repeat ) {
				// we might want to have the system reset these parameters on every bind and
				// share the image data
				continue;
			}
			if ( image->usage != usage ) {
				// If an image is used differently then we need 2 copies of it because usage affects the way it's compressed and swizzled
				continue;
			}
			return image;
		}
	}
	return NULL;
}
/*
===============
ImageFromFile

Finds or loads the given image, always returning a valid image pointer.
Loading of the image may be deferred for dynamic loading.
==============
*/
hfImage	*hfImageManager::ImageFromFile( const char *_name, textureFilter_t filter, 
						 textureRepeat_t repeat, textureUsage_t usage, cubeFiles_t cubeMap ) {

	if ( !_name || !_name[0] || hfStr::Icmp( _name, "default" ) == 0 || hfStr::Icmp( _name, "_default" ) == 0 ) {
		declManager->MediaPrint( "DEFAULTED\n" );
		return globalImages->defaultImage;
	}
	if ( hfStr::Icmpn( _name, "fonts", 5 ) == 0 || hfStr::Icmpn( _name, "newfonts", 8 ) == 0 ) {
		usage = TD_FONT;
	}
	if ( hfStr::Icmpn( _name, "lights", 6 ) == 0 ) {
		usage = TD_LIGHT;
	}

	// strip any .tga file extensions from anywhere in the _name, including image program parameters
	hfStrStatic< MAX_OSPATH > name = _name;
	name.Replace( ".tga", "" );
	name.BackSlashesToSlashes();

	//
	// see if the image is already loaded, unless we
	// are in a reloadImages call
	//
	int hash = name.FileNameHash();
	for ( int i = imageHash.First( hash ); i != -1; i = imageHash.Next( i ) ) {
		hfImage	* image = images[i];
		if ( name.Icmp( image->GetName() ) == 0 ) {
			// the built in's, like _white and _flat always match the other options
			if ( name[0] == '_' ) {
				return image;
			}
			if ( image->cubeFiles != cubeMap ) {
				common->Error( "Image '%s' has been referenced with conflicting cube map states", _name );
			}

			if ( image->filter != filter || image->repeat != repeat ) {
				// we might want to have the system reset these parameters on every bind and
				// share the image data
				continue;
			}
			if ( image->usage != usage ) {
				// If an image is used differently then we need 2 copies of it because usage affects the way it's compressed and swizzled
				continue;
			}

			image->usage = usage;
			image->levelLoadReferenced = true;

			if ( ( !insideLevelLoad  || preloadingMapImages ) && !image->IsLoaded() ) {
				image->referencedOutsideLevelLoad = ( !insideLevelLoad && !preloadingMapImages );
				image->ActuallyLoadImage( false );	// load is from front end
				declManager->MediaPrint( "%ix%i %s (reload for mixed referneces)\n", image->GetUploadWidth(), image->GetUploadHeight(), image->GetName() );
			}
			return image;
		}
	}

	//
	// create a new image
	//
	hfImage	* image = AllocImage( name );
	image->cubeFiles = cubeMap;
	image->usage = usage;
	image->filter = filter;
	image->repeat = repeat;

	image->levelLoadReferenced = true;

	// load it if we aren't in a level preload
	if ( !insideLevelLoad || preloadingMapImages ) {
		image->referencedOutsideLevelLoad = ( !insideLevelLoad && !preloadingMapImages );
		image->ActuallyLoadImage( false );	// load is from front end
		declManager->MediaPrint( "%ix%i %s\n", image->GetUploadWidth(), image->GetUploadHeight(), image->GetName() );
	} else {
		declManager->MediaPrint( "%s\n", image->GetName() );
	}

	return image;
}

/*
========================
hfImageManager::ScratchImage
========================
*/
hfImage * hfImageManager::ScratchImage( const char *_name, hfImageOpts *imgOpts, textureFilter_t filter, textureRepeat_t repeat, textureUsage_t usage ) {
	if ( !_name || !_name[0] ) {
		hfLib::FatalError( "hfImageManager::ScratchImage called with empty name" );
	}

	if ( imgOpts == NULL ) {
		hfLib::FatalError( "hfImageManager::ScratchImage called with NULL imgOpts" );
	}

	hfStr name = _name;

	//
	// see if the image is already loaded, unless we
	// are in a reloadImages call
	//
	int hash = name.FileNameHash();
	for ( int i = imageHash.First( hash ); i != -1; i = imageHash.Next( i ) ) {
		hfImage	* image = images[i];
		if ( name.Icmp( image->GetName() ) == 0 ) {
			// the built in's, like _white and _flat always match the other options
			if ( name[0] == '_' ) {
				return image;
			}

			if ( image->filter != filter || image->repeat != repeat ) {
				// we might want to have the system reset these parameters on every bind and
				// share the image data
				continue;
			}
			if ( image->usage != usage ) {
				// If an image is used differently then we need 2 copies of it because usage affects the way it's compressed and swizzled
				continue;
			}

			image->usage = usage;
			image->levelLoadReferenced = true;
			image->referencedOutsideLevelLoad = true;
			return image;
		}
	}

	// clamp is the only repeat mode that makes sense for cube maps, but
	// some platforms let them stay in repeat mode and get border seam issues
	if ( imgOpts->textureType == TT_CUBIC && repeat != TR_CLAMP ) {
		repeat = TR_CLAMP;
	}

	//
	// create a new image
	//
	hfImage* newImage = AllocImage( name );
	if ( newImage != NULL ) {	
		newImage->AllocImage( *imgOpts, filter, repeat );
	}
	return newImage;
}

/*
===============
hfImageManager::GetImage
===============
*/
hfImage *hfImageManager::GetImage( const char *_name ) const {

	if ( !_name || !_name[0] || hfStr::Icmp( _name, "default" ) == 0 || hfStr::Icmp( _name, "_default" ) == 0 ) {
		declManager->MediaPrint( "DEFAULTED\n" );
		return globalImages->defaultImage;
	}

	// strip any .tga file extensions from anywhere in the _name, including image program parameters
	hfStr name = _name;
	name.Replace( ".tga", "" );
	name.BackSlashesToSlashes();

	//
	// look in loaded images
	//
	int hash = name.FileNameHash();
	for ( int i = imageHash.First( hash ); i != -1; i = imageHash.Next( i ) ) {
		hfImage * image = images[i];
		if ( name.Icmp( image->GetName() ) == 0 ) {
			return image;
		}
	}

	return NULL;
}

/*
===============
PurgeAllImages
===============
*/
void hfImageManager::PurgeAllImages() {
	int		i;
	hfImage	*image;

	for ( i = 0; i < images.Num() ; i++ ) {
		image = images[i];
		image->PurgeImage();
	}
}

/*
===============
ReloadImages
===============
*/
void hfImageManager::ReloadImages( bool all ) {
	for ( int i = 0 ; i < globalImages->images.Num() ; i++ ) {
		globalImages->images[ i ]->Reload( all );
	}
}

/*
===============
R_CombineCubeImages_f

Used to combine animations of six separate tga files into
a serials of 6x taller tga files, for preparation to roq compress
===============
*/
void R_CombineCubeImages_f( const hfCmdArgs &args ) {
	if ( args.Argc() != 2 ) {
		common->Printf( "usage: combineCubeImages <baseName>\n" );
		common->Printf( " combines basename[1-6][0001-9999].tga to basenameCM[0001-9999].tga\n" );
		common->Printf( " 1: forward 2:right 3:back 4:left 5:up 6:down\n" );
		return;
	}

	hfStr	baseName = args.Argv( 1 );
	common->SetRefreshOnPrint( true );

	for ( int frameNum = 1 ; frameNum < 10000 ; frameNum++ ) {
		char	filename[MAX_IMAGE_NAME];
		byte	*pics[6];
		int		width = 0, height = 0;
		int		side;
		int		orderRemap[6] = { 1,3,4,2,5,6 };
		for ( side = 0 ; side < 6 ; side++ ) {
			sprintf( filename, "%s%i%04i.tga", baseName.c_str(), orderRemap[side], frameNum );

			common->Printf( "reading %s\n", filename );
			R_LoadImage( filename, &pics[side], &width, &height, NULL, true );

			if ( !pics[side] ) {
				common->Printf( "not found.\n" );
				break;
			}

			// convert from "camera" images to native cube map images
			switch( side ) {
			case 0:	// forward
				R_RotatePic( pics[side], width);
				break;
			case 1:	// back
				R_RotatePic( pics[side], width);
				R_HorizontalFlip( pics[side], width, height );
				R_VerticalFlip( pics[side], width, height );
				break;
			case 2:	// left
				R_VerticalFlip( pics[side], width, height );
				break;
			case 3:	// right
				R_HorizontalFlip( pics[side], width, height );
				break;
			case 4:	// up
				R_RotatePic( pics[side], width);
				break;
			case 5: // down
				R_RotatePic( pics[side], width);
				break;
			}
		}

		if ( side != 6 ) {
			for ( int i = 0 ; i < side ; side++ ) {
				Mem_Free( pics[side] );
			}
			break;
		}

		hfTempArray<byte> buf( width*height*6*4 );
		byte	*combined = (byte *)buf.Ptr();
		for (  side = 0 ; side < 6 ; side++ ) {
			memcpy( combined+width*height*4*side, pics[side], width*height*4 );
			Mem_Free( pics[side] );
		}
		sprintf( filename, "%sCM%04i.tga", baseName.c_str(), frameNum );

		common->Printf( "writing %s\n", filename );
		R_WriteTGA( filename, combined, width, height*6 );
	}
	common->SetRefreshOnPrint( false );
}

/*
===============
UnbindAll
===============
*/
void hfImageManager::UnbindAll() {
	int oldTMU = backEnd.glState.currenttmu;
	for ( int i = 0; i < MAX_PROG_TEXTURE_PARMS; ++i ) {
		backEnd.glState.currenttmu = i;
		BindNull();
	}
	backEnd.glState.currenttmu = oldTMU;
}

/*
===============
BindNull
===============
*/
void hfImageManager::BindNull() {
	RENDERLOG_PRINTF( "BindNull()\n" );

}

/*
===============
Init
===============
*/
void hfImageManager::Init() {

	images.Resize( 1024, 1024 );
	imageHash.ResizeIndex( 1024 );

	CreateIntrinsicImages();

	cmdSystem->AddCommand( "reloadImages", R_ReloadImages_f, CMD_FL_RENDERER, "reloads images" );
	cmdSystem->AddCommand( "listImages", R_ListImages_f, CMD_FL_RENDERER, "lists images" );
	cmdSystem->AddCommand( "combineCubeImages", R_CombineCubeImages_f, CMD_FL_RENDERER, "combines six images for roq compression" );

	// should forceLoadImages be here?
}

/*
===============
Shutdown
===============
*/
void hfImageManager::Shutdown() {
	images.DeleteContents( true );
	imageHash.Clear();

}

/*
====================
hfImageManager::BeginLevelLoad
Frees all images used by the previous level
====================
*/
void hfImageManager::BeginLevelLoad() {
	insideLevelLoad = true;

	for ( int i = 0 ; i < images.Num() ; i++ ) {
		hfImage	*image = images[ i ];

		// generator function images are always kept around
		if ( image->generatorFunction ) {
			continue;
		}

		if ( !image->referencedOutsideLevelLoad && image->IsLoaded() ) {
			image->PurgeImage();
			//hfLib::Printf( "purging %s\n", image->GetName() );
		} else {
			//hfLib::Printf( "not purging %s\n", image->GetName() );
		}

		image->levelLoadReferenced = false;
	}
}


/*
====================
hfImageManager::ExcludePreloadImage
====================
*/
bool hfImageManager::ExcludePreloadImage( const char *name ) {
	hfStrStatic< MAX_OSPATH > imgName = name;
	imgName.ToLower();
	if ( imgName.Find( "newfonts/", false ) >= 0 ) {
		return true;
	}
	if ( imgName.Find( "generated/swf/", false ) >= 0 ) {
		return true;
	}
	if ( imgName.Find( "/loadscreens/", false ) >= 0 ) {
		return true;
	}
	return false;
}

/*
====================
hfImageManager::Preload
====================
*/
void hfImageManager::Preload( const hfPreloadManifest &manifest, const bool & mapPreload ) {
	if ( preLoad_Images.GetBool() && manifest.NumResources() > 0 ) {
		// preload this levels images
		common->Printf( "Preloading images...\n" );
		preloadingMapImages = mapPreload;
		int	start = Sys_Milliseconds();
		int numLoaded = 0;

		//fileSystem->StartPreload( preloadImageFiles );
		for ( int i = 0; i < manifest.NumResources(); i++ ) {
			const preloadEntry_s & p = manifest.GetPreloadByIndex( i );
			if ( p.resType == PRELOAD_IMAGE && !ExcludePreloadImage( p.resourceName ) ) {
				globalImages->ImageFromFile( p.resourceName, ( textureFilter_t )p.imgData.filter, ( textureRepeat_t )p.imgData.repeat, ( textureUsage_t )p.imgData.usage, ( cubeFiles_t )p.imgData.cubeMap );
				numLoaded++;
			}
		}
		//fileSystem->StopPreload();
		int	end = Sys_Milliseconds();
		common->Printf( "%05d images preloaded ( or were already loaded ) in %5.1f seconds\n", numLoaded, ( end - start ) * 0.001 );
		common->Printf( "----------------------------------------\n" );
		preloadingMapImages = false;
	}
}

/*
===============
hfImageManager::LoadLevelImages
===============
*/
int hfImageManager::LoadLevelImages( bool pacifier ) {
	int	loadCount = 0;
	for ( int i = 0 ; i < images.Num() ; i++ ) {
		if ( pacifier ) {
			common->UpdateLevelLoadPacifier();

		}

		hfImage	*image = images[ i ];
		if ( image->generatorFunction ) {
			continue;
		}
		if ( image->levelLoadReferenced && !image->IsLoaded() ) {
			loadCount++;
			image->ActuallyLoadImage( false );
		}
	}
	return loadCount;
}

/*
===============
hfImageManager::EndLevelLoad
===============
*/
void hfImageManager::EndLevelLoad() {
	insideLevelLoad = false;

	common->Printf( "----- hfImageManager::EndLevelLoad -----\n" );
	int start = Sys_Milliseconds();
	int	loadCount = LoadLevelImages( true );

	int	end = Sys_Milliseconds();
	common->Printf( "%5i images loaded in %5.1f seconds\n", loadCount, (end-start) * 0.001 );
	common->Printf( "----------------------------------------\n" );
	//R_ListImages_f( hfCmdArgs( "sorted sorted", false ) );
}

/*
===============
hfImageManager::StartBuild
===============
*/
void hfImageManager::StartBuild() {
}

/*
===============
hfImageManager::FinishBuild
===============
*/
void hfImageManager::FinishBuild( bool removeDups ) {
}

/*
===============
hfImageManager::PrintMemInfo
===============
*/
void hfImageManager::PrintMemInfo( MemInfo_t *mi ) {
	int i, j, total = 0;
	int *sortIndex;
	hfFile *f;

	f = fileSystem->OpenFileWrite( mi->filebase + "_images.txt" );
	if ( !f ) {
		return;
	}

	// sort first
	sortIndex = new (TAG_IMAGE) int[images.Num()];

	for ( i = 0; i < images.Num(); i++ ) {
		sortIndex[i] = i;
	}

	for ( i = 0; i < images.Num() - 1; i++ ) {
		for ( j = i + 1; j < images.Num(); j++ ) {
			if ( images[sortIndex[i]]->StorageSize() < images[sortIndex[j]]->StorageSize() ) {
				int temp = sortIndex[i];
				sortIndex[i] = sortIndex[j];
				sortIndex[j] = temp;
			}
		}
	}

	// print next
	for ( i = 0; i < images.Num(); i++ ) {
		hfImage *im = images[sortIndex[i]];
		int size;

		size = im->StorageSize();
		total += size;

		f->Printf( "%s %3i %s\n", hfStr::FormatNumber( size ).c_str(), im->refCount, im->GetName() );
	}

	delete [] sortIndex;
	mi->imageAssetsTotal = total;

	f->Printf( "\nTotal image bytes allocated: %s\n", hfStr::FormatNumber( total ).c_str() );
	fileSystem->CloseFile( f );
}

