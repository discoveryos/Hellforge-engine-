#ifndef __LIB_H__
#define __LIB_H__

#include <stddef.h>

/*
===============================================================================

	hfLib contains stateless support classes and concrete types. Some classes
	do have static variables, but such variables are initialized once and
	read-only after initialization (they do not maintain a modifiable state).

	The interface pointers hfSys, hfCommon, hfCVarSystem and hfFileSystem
	should be set before using hfLib. The pointers stored here should not
	be used by any part of the engine except for hfLib.

===============================================================================
*/

class hfLib {
private:
	static bool					mainThreadInitialized;
	static HF_TLS				isMainThread;

public:
	static class hfSys *		sys;
	static class hfCommon *		common;
	static class hfCVarSystem *	cvarSystem;
	static class hfFileSystem *	fileSystem;
	static int					frameNumber;

	static void					Init();
	static void					ShutDown();

	// wrapper to hfCommon functions 
	static void					Printf( const char *fmt, ... );
	static void					PrintfIf( const bool test, const char *fmt, ... );
	NO_RETURN static void		Error( const char *fmt, ... );
	NO_RETURN static void		FatalError( const char *fmt, ... );
	static void					Warning( const char *fmt, ... );
	static void					WarningIf( const bool test, const char *fmt, ... );

	// the extra check for mainThreadInitialized is necessary for this to be accurate
	// when called by startup code that happens before hfLib::Init
	static bool					IsMainThread() { return ( 0 == mainThreadInitialized ) || ( 1 == isMainThread ); }
};


/*
===============================================================================

	Types and defines used throughout the engine.

===============================================================================
*/

typedef int						qhandle_t;

class hfFile;
class hfVec3;
class hfVec4;

#ifndef NULL
#define NULL					((void *)0)
#endif

#ifndef BIT
#define BIT( num )				( 1ULL << ( num ) )
#endif

#define	MAX_STRING_CHARS		1024		// max length of a string
#define MAX_PRINT_MSG			16384		// buffer size for our various printf routines

// maximum world size
#define MAX_WORLD_COORD			( 128 * 1024 )
#define MIN_WORLD_COORD			( -128 * 1024 )
#define MAX_WORLD_SIZE			( MAX_WORLD_COORD - MIN_WORLD_COORD )

#define SIZE_KB( x )						( ( (x) + 1023 ) / 1024 )
#define SIZE_MB( x )						( ( ( SIZE_KB( x ) ) + 1023 ) / 1024 )
#define SIZE_GB( x )						( ( ( SIZE_MB( x ) ) + 1023 ) / 1024 )

// basic colors
extern	hfVec4 colorBlack;
extern	hfVec4 colorWhite;
extern	hfVec4 colorRed;
extern	hfVec4 colorGreen;
extern	hfVec4 colorBlue;
extern	hfVec4 colorYellow;
extern	hfVec4 colorMagenta;
extern	hfVec4 colorCyan;
extern	hfVec4 colorOrange;
extern	hfVec4 colorPurple;
extern	hfVec4 colorPink;
extern	hfVec4 colorBrown;
extern	hfVec4 colorLtGrey;
extern	hfVec4 colorMdGrey;
extern	hfVec4 colorDkGrey;

// packs color floats in the range [0,1] into an integer
dword	PackColor( const hfVec3 &color );
void	UnpackColor( const dword color, hfVec3 &unpackedColor );
dword	PackColor( const hfVec4 &color );
void	UnpackColor( const dword color, hfVec4 &unpackedColor );

// little/big endian conversion
short	BigShort( short l );
short	LittleShort( short l );
int		BigLong( int l );
int		LittleLong( int l );
float	BigFloat( float l );
float	LittleFloat( float l );
void	BigRevBytes( void *bp, int elsize, int elcount );
void	LittleRevBytes( void *bp, int elsize, int elcount );
void	LittleBitField( void *bp, int elsize );
void	Swap_Init();

bool	Swap_IsBigEndian();

// for base64
void	SixtetsForInt( byte *out, int src);
int		IntForSixtets( byte *in );

/*
================================================
hfException
================================================
*/
class hfException {
public:
	static const int MAX_ERROR_LEN = 2048;

					hfException( const char *text = "" ) { 
						strncpy( error, text, MAX_ERROR_LEN ); 
					}

	// this really, really should be a const function, but it's referenced too many places to change right now
	const char *	GetError() { 
						return error; 
					}	

protected:
	// if GetError() were correctly const this would be named GetError(), too
	char *		GetErrorBuffer() { 
					return error; 
				}	
	int			GetErrorBufferSize() { 
					return MAX_ERROR_LEN; 
				}

private:
	friend class hfFatalException;
	static char error[MAX_ERROR_LEN];
};

/*
================================================
hfFatalException
================================================
*/
class hfFatalException {
public:
	static const int MAX_ERROR_LEN = 2048;

	hfFatalException( const char *text = "" ) { 
		strncpy( hfException::error, text, MAX_ERROR_LEN ); 
	}

	// this really, really should be a const function, but it's referenced too many places to change right now
	const char *	GetError() { 
		return hfException::error; 
	}	

protected:
	// if GetError() were correctly const this would be named GetError(), too
	char *		GetErrorBuffer() { 
		return hfException::error; 
	}	
	int			GetErrorBufferSize() { 
		return MAX_ERROR_LEN; 
	}
};

/*
================================================
hfNetworkLoadException
================================================
*/
class hfNetworkLoadException : public hfException {
public:
	hfNetworkLoadException( const char * text = "" ) : hfException( text ) { }
};

/*
===============================================================================

	hfLib headers.

===============================================================================
*/

// System
#include "sys/hfsys_assert.h"
#include "sys/hfsys_threading.h"

// memory management and arrays
#include "hfHeap.h"
#include "containers/hfSort.h"
#include "containers/hfList.h"

// math
#include "math/hfSimd.h"
#include "math/hfMath.h"
#include "math/hfRandom.h"
#include "math/hfComplex.h"
#include "math/hfVector.h"
#include "math/hfVecX.h"
#include "math/hfVectorI.h"
#include "math/hfMatrix.h"
#include "math/hfMatX.h"
#include "math/hfAngles.h"
#include "math/hfQuat.h"
#include "math/hfRotation.h"
#include "math/hfPlane.h"
#include "math/hfPluecker.h"
#include "math/hfPolynomial.h"
#include "math/hfExtrapolate.h"
#include "math/hfInterpolate.h"
#include "math/hfCurve.h"
#include "math/hfOde.h"
#include "math/hfLcp.h"

// bounding volumes
#include "bv/hfSphere.h"
#include "bv/hfBounds.h"
#include "bv/hfBox.h"
#include "bv/hfCollide.h"

// geometry
#include "geometry/hfRenderMatrix.h"
#include "geometry/hfJointTransform.h"
#include "geometry/hfDrawVert.h"
#include "geometry/hfWinding.h"
#include "geometry/hfWinding2D.h"
#include "geometry/hfSurface.h"
#include "geometry/hfSurface_Patch.h"
#include "geometry/hfSurface_Polytope.h"
#include "geometry/hfSurface_SweptSpline.h"
#include "geometry/hfTraceModel.h"

// text manipulation
#include "hfStr.h"
#include "hfStrStatic.h"
#include "hfToken.h"
#include "hfLexer.h"
#include "hfParser.h"
#include "hfBase64.h"
#include "hfCmdArgs.h"

// containers
#include "containers/hfArray.h"
#include "containers/hfBTree.h"
#include "containers/hfBinSearch.h"
#include "containers/hfHashIndex.h"
#include "containers/hfHashTable.h"
#include "containers/hfStaticList.h"
#include "containers/hfLinkList.h"
#include "containers/hfHierarchy.h"
#include "containers/hfQueue.h"
#include "containers/hfStack.h"
#include "containers/hfStrList.h"
#include "containers/hfStrPool.h"
#include "containers/hfVectorSet.h"
#include "containers/hfPlaneSet.h"

// hashing
#include "hashing/hfCRC32.h"
#include "hashing/hfMD4.h"
#include "hashing/hfMD5.h"

// misc
#include "hfDict.h"
#include "hfLangDict.h"
#include "hfDataQueue.h"
#include "hfBitMsg.h"
#include "hfMapFile.h"
#include "hfTimer.h"
#include "hfThread.h"
#include "hfHeadless.h"
#include "hfSwap.h"
#include "hfCallback.h"
#include "hfParallelJobList.h"

// entity-component-system + scene / world (general-purpose game layer)
#include "hfEcs.h"
#include "scene/hfWorld.h"
#include "scene/hfLightComponent.h"
#include "scene/hfLightSystem.h"
#include "scene/hfMeshRendererComponent.h"
#include "scene/hfRenderCullSystem.h"
#include "scene/hfSceneSerializer.h"

// typed resource registry over the packed asset store
#include "resource/hfResourceManager.h"

// Phase 5 — content pipeline: JSON parser + glTF subset importer (offline tool)
#include "asset/hfJson.h"
#include "asset/hfGltf.h"
#include "anim/hfAnimationClip.h"
#include "anim/hfAnimationSystem.h"
#include "anim/hfAnimationStateMachine.h"

// Phase 5 — procedural map brushes (CSG) as scene entities
#include "scene/hfCsgComponent.h"
#include "scene/hfCsgSystem.h"

// rigid-body physics (modern-engine conventions)
#include "physics/hfRigidBodyComponent.h"
#include "physics/hfBroadphase.h"
#include "physics/hfPhysicsSystem.h"

// Phase 8 — threaded simulation server (Godot Servers-style split)
#include "server/hfSimulationServer.h"

// Phase 8 — audio server (spatial mix on a worker thread, RID-handled)
#include "audio/hfAudioData.h"
#include "audio/hfAudioSpatial.h"
#include "audio/hfAudioServer.h"

// ui (Phase 9 - canvas controls, layout, CPU text rasterizer)
#include "ui/hfBitmapFont.h"
#include "ui/hfCanvasComponent.h"
#include "ui/hfUiSystem.h"
#include "ui/hfUiEvents.h"
#include "ui/hfUiWidget.h"
#include "ui/hfUiInput.h"

// Phase 10 — networking (ENet-class transport, replication, lag compensation)
#include "net/hfNetComponent.h"
#include "net/hfReplicationSystem.h"

// Phase 11 — editor (reflection, scene tree, document open/edit/save)
#include "editor/hfEditorReflection.h"
#include "editor/hfEditorTree.h"
#include "editor/hfEditorDocument.h"

// Phase 4 — materials + rendering server (Godot-RenderingServer-style split)
#include "render/hfMaterial.h"
#include "render/hfRenderingServer.h"

#include "hfSoftwareCache.h"

#endif	/* !__LIB_H__ */

