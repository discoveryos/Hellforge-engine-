#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hftr_local.h"

const float hfGuiModel::STEREO_DEPTH_NEAR = 0.0f;
const float hfGuiModel::STEREO_DEPTH_MID  = 0.5f;
const float hfGuiModel::STEREO_DEPTH_FAR  = 1.0f;

/*
================
hfGuiModel::hfGuiModel
================
*/
hfGuiModel::hfGuiModel() {
	// identity color for drawsurf register evaluation
	for ( int i = 0; i < MAX_ENTITY_SHADER_PARMS; i++ ) {
		shaderParms[i] = 1.0f;
	}
}

/*
================
hfGuiModel::Clear

Begins collecting draw commands into surfaces
================
*/
void hfGuiModel::Clear() {
	surfaces.SetNum( 0 );
	AdvanceSurf();
}

/*
================
hfGuiModel::WriteToDemo
================
*/
void hfGuiModel::WriteToDemo( hfDemoFile *demo ) {
}

/*
================
hfGuiModel::ReadFromDemo
================
*/
void hfGuiModel::ReadFromDemo( hfDemoFile *demo ) {
}

/*
================
hfGuiModel::BeginFrame
================
*/
void hfGuiModel::BeginFrame() {
	vertexBlock = vertexCache.AllocVertex( NULL, ALIGN( MAX_VERTS * sizeof( hfDrawVert ), VERTEX_CACHE_ALIGN ) );
	indexBlock = vertexCache.AllocIndex( NULL, ALIGN( MAX_INDEXES * sizeof( triIndex_t ), INDEX_CACHE_ALIGN ) );
	vertexPointer = (hfDrawVert *)vertexCache.MappedVertexBuffer( vertexBlock );
	indexPointer = (triIndex_t *)vertexCache.MappedIndexBuffer( indexBlock );
	numVerts = 0;
	numIndexes = 0;
	Clear();
}

hfCVar	stereoRender_defaultGuiDepth( "stereoRender_defaultGuiDepth", "0", CVAR_RENDERER, "Fraction of separation when not specified" );
/*
================
EmitSurfaces

For full screen GUIs, we can add in per-surface stereoscopic depth effects
================
*/
void hfGuiModel::EmitSurfaces( float modelMatrix[16], float modelViewMatrix[16], 
	bool depthHack, bool allowFullScreenStereoDepth, bool linkAsEntity ) {

	viewEntity_t * guiSpace = (viewEntity_t *)R_ClearedFrameAlloc( sizeof( *guiSpace ), FRAME_ALLOC_VIEW_ENTITY );
	memcpy( guiSpace->modelMatrix, modelMatrix, sizeof( guiSpace->modelMatrix ) );
	memcpy( guiSpace->modelViewMatrix, modelViewMatrix, sizeof( guiSpace->modelViewMatrix ) );
	guiSpace->weaponDepthHack = depthHack;
	guiSpace->isGuiSurface = true;

	// If this is an in-game gui, we need to be able to find the matrix again for head mounted
	// display bypass matrix fixup.
	if ( linkAsEntity ) {
		guiSpace->next = tr.viewDef->viewEntitys;
		tr.viewDef->viewEntitys = guiSpace;
	}

	//---------------------------
	// make a tech5 renderMatrix
	//---------------------------
	hfRenderMatrix viewMat;
	hfRenderMatrix::Transpose( *(hfRenderMatrix *)modelViewMatrix, viewMat );
	hfRenderMatrix::Multiply( tr.viewDef->projectionRenderMatrix, viewMat, guiSpace->mvp );
	if ( depthHack ) {
		hfRenderMatrix::ApplyDepthHack( guiSpace->mvp );
	}

	// to allow 3D-TV effects in the menu system, we define surface flags to set
	// depth fractions between 0=screen and 1=infinity, which directly modulate the
	// screenSeparation parameter for an X offset.
	// The value is stored in the drawSurf sort value, which adjusts the matrix in the
	// backend.
	float defaultStereoDepth = stereoRender_defaultGuiDepth.GetFloat();	// default to at-screen

	// add the surfaces to this view
	for ( int i = 0; i < surfaces.Num(); i++ ) {
		const guiModelSurface_t & guiSurf = surfaces[i];
		if ( guiSurf.numIndexes == 0 ) {
			continue;
		}

		const hfMaterial * shader = guiSurf.material;
		drawSurf_t * drawSurf = (drawSurf_t *)R_FrameAlloc( sizeof( *drawSurf ), FRAME_ALLOC_DRAW_SURFACE );

		drawSurf->numIndexes = guiSurf.numIndexes;
		drawSurf->ambientCache = vertexBlock;
		// build a vertCacheHandle_t that points inside the allocated block
		drawSurf->indexCache = indexBlock + ( (int64)(guiSurf.firstIndex*sizeof(triIndex_t)) << VERTCACHE_OFFSET_SHIFT );
 		drawSurf->shadowCache = 0;
		drawSurf->jointCache = 0;
		drawSurf->frontEndGeo = NULL;
		drawSurf->space = guiSpace;
		drawSurf->material = shader;
		drawSurf->extraGLState = guiSurf.glState;
		drawSurf->scissorRect = tr.viewDef->scissor;
		drawSurf->sort = shader->GetSort();
		drawSurf->renderZFail = 0;
		// process the shader expressions for conditionals / color / texcoords
		const float	*constRegs = shader->ConstantRegisters();
		if ( constRegs ) {
			// shader only uses constant values
			drawSurf->shaderRegisters = constRegs;
		} else {
			float *regs = (float *)R_FrameAlloc( shader->GetNumRegisters() * sizeof( float ), FRAME_ALLOC_SHADER_REGISTER );
			drawSurf->shaderRegisters = regs;
			shader->EvaluateRegisters( regs, shaderParms, tr.viewDef->renderView.shaderParms, tr.viewDef->renderView.time[1] * 0.001f, NULL );
		}
		R_LinkDrawSurfToView( drawSurf, tr.viewDef );
		if ( allowFullScreenStereoDepth ) {
			// override sort with the stereoDepth
			//drawSurf->sort = stereoDepth;

			switch ( guiSurf.stereoType ) {
			case STEREO_DEPTH_TYPE_NEAR: drawSurf->sort = STEREO_DEPTH_NEAR; break;
			case STEREO_DEPTH_TYPE_MID: drawSurf->sort = STEREO_DEPTH_MID; break;
			case STEREO_DEPTH_TYPE_FAR: drawSurf->sort = STEREO_DEPTH_FAR; break;
			case STEREO_DEPTH_TYPE_NONE:
			default:
				drawSurf->sort = defaultStereoDepth;
				break;
			}
		}
	}
}

/*
====================
EmitToCurrentView
====================
*/
void hfGuiModel::EmitToCurrentView( float modelMatrix[16], bool depthHack ) {
	float	modelViewMatrix[16];

	R_MatrixMultiply( modelMatrix, tr.viewDef->worldSpace.modelViewMatrix, modelViewMatrix );

	EmitSurfaces( modelMatrix, modelViewMatrix, depthHack, false /* stereoDepthSort */, true /* link as entity */ );
}


/*
================
hfGuiModel::EmitFullScreen

Creates a view that covers the screen and emit the surfaces
================
*/
void hfGuiModel::EmitFullScreen() {

	if ( surfaces[0].numIndexes == 0 ) {
		return;
	}

	SCOPED_PROFILE_EVENT( "Gui::EmitFullScreen" );

	viewDef_t * viewDef = (viewDef_t *)R_ClearedFrameAlloc( sizeof( *viewDef ), FRAME_ALLOC_VIEW_DEF );
	viewDef->is2Dgui = true;
	tr.GetCroppedViewport( &viewDef->viewport );

	bool stereoEnabled = ( renderSystem->GetStereo3DMode() != STEREO3D_OFF );
	if ( stereoEnabled ) {
		float	GetScreenSeparationForGuis();
		const float screenSeparation = GetScreenSeparationForGuis();

		// this will be negated on the alternate eyes, both rendered each frame
		viewDef->renderView.stereoScreenSeparation = screenSeparation;

		extern hfCVar stereoRender_swapEyes;
		viewDef->renderView.viewEyeBuffer = 0;	// render to both buffers
		if ( stereoRender_swapEyes.GetBool() ) {
			viewDef->renderView.stereoScreenSeparation = -screenSeparation;
		}
	}

	viewDef->scissor.x1 = 0;
	viewDef->scissor.y1 = 0;
	viewDef->scissor.x2 = viewDef->viewport.x2 - viewDef->viewport.x1;
	viewDef->scissor.y2 = viewDef->viewport.y2 - viewDef->viewport.y1;

	viewDef->projectionMatrix[0*4+0] = 2.0f / SCREEN_WIDTH;
	viewDef->projectionMatrix[0*4+1] = 0.0f;
	viewDef->projectionMatrix[0*4+2] = 0.0f;
	viewDef->projectionMatrix[0*4+3] = 0.0f;

	viewDef->projectionMatrix[1*4+0] = 0.0f;
	viewDef->projectionMatrix[1*4+1] = -2.0f / SCREEN_HEIGHT;
	viewDef->projectionMatrix[1*4+2] = 0.0f;
	viewDef->projectionMatrix[1*4+3] = 0.0f;

	viewDef->projectionMatrix[2*4+0] = 0.0f;
	viewDef->projectionMatrix[2*4+1] = 0.0f;
	viewDef->projectionMatrix[2*4+2] = -2.0f;
	viewDef->projectionMatrix[2*4+3] = 0.0f;

	viewDef->projectionMatrix[3*4+0] = -1.0f;
	viewDef->projectionMatrix[3*4+1] = 1.0f;
	viewDef->projectionMatrix[3*4+2] = -1.0f;
	viewDef->projectionMatrix[3*4+3] = 1.0f;

	// make a tech5 renderMatrix for faster culling
	hfRenderMatrix::Transpose( *(hfRenderMatrix *)viewDef->projectionMatrix, viewDef->projectionRenderMatrix );

	viewDef->worldSpace.modelMatrix[0*4+0] = 1.0f;
	viewDef->worldSpace.modelMatrix[1*4+1] = 1.0f;
	viewDef->worldSpace.modelMatrix[2*4+2] = 1.0f;
	viewDef->worldSpace.modelMatrix[3*4+3] = 1.0f;

	viewDef->worldSpace.modelViewMatrix[0*4+0] = 1.0f;
	viewDef->worldSpace.modelViewMatrix[1*4+1] = 1.0f;
	viewDef->worldSpace.modelViewMatrix[2*4+2] = 1.0f;
	viewDef->worldSpace.modelViewMatrix[3*4+3] = 1.0f;

	viewDef->maxDrawSurfs = surfaces.Num();
	viewDef->drawSurfs = (drawSurf_t **)R_FrameAlloc( viewDef->maxDrawSurfs * sizeof( viewDef->drawSurfs[0] ), FRAME_ALLOC_DRAW_SURFACE_POINTER );
	viewDef->numDrawSurfs = 0;

	viewDef_t * oldViewDef = tr.viewDef;
	tr.viewDef = viewDef;

	EmitSurfaces( viewDef->worldSpace.modelMatrix, viewDef->worldSpace.modelViewMatrix, 
		false /* depthHack */ , stereoEnabled /* stereoDepthSort */, false /* link as entity */ );

	tr.viewDef = oldViewDef;

	// add the command to draw this view
	R_AddDrawViewCmd( viewDef, true );
}

/*
=============
AdvanceSurf
=============
*/
void hfGuiModel::AdvanceSurf() {
	guiModelSurface_t	s;

	if ( surfaces.Num() ) {
		s.material = surf->material;
		s.glState = surf->glState;
	} else {
		s.material = tr.defaultMaterial;
		s.glState = 0;
	}

	// advance indexes so the pointer to each surface will be 16 byte aligned
	numIndexes = ALIGN( numIndexes, 8 );

	s.numIndexes = 0;
	s.firstIndex = numIndexes;

	surfaces.Append( s );
	surf = &surfaces[ surfaces.Num() - 1 ];
}

/*
=============
AllocTris
=============
*/
hfDrawVert * hfGuiModel::AllocTris( int vertCount, const triIndex_t * tempIndexes, int indexCount, const hfMaterial * material, const uint64 glState, const stereoDepthType_t stereoType ) {
	if ( material == NULL ) {
		return NULL;
	}
	if ( numIndexes + indexCount > MAX_INDEXES ) {
		static int warningFrame = 0;
		if ( warningFrame != tr.frameCount ) {
			warningFrame = tr.frameCount;
			hfLib::Warning( "hfGuiModel::AllocTris: MAX_INDEXES exceeded" );
		}
		return NULL;
	}
	if ( numVerts + vertCount > MAX_VERTS ) {
		static int warningFrame = 0;
		if ( warningFrame != tr.frameCount ) {
			warningFrame = tr.frameCount;
			hfLib::Warning( "hfGuiModel::AllocTris: MAX_VERTS exceeded" );
		}
		return NULL;
	}

	// break the current surface if we are changing to a new material or we can't
	// fit the data into our allocated block
	if ( material != surf->material || glState != surf->glState || stereoType != surf->stereoType ) {
		if ( surf->numIndexes ) {
			AdvanceSurf();
		}
		surf->material = material;
		surf->glState = glState;
		surf->stereoType = stereoType;
	}

	int startVert = numVerts;
	int startIndex = numIndexes;

	numVerts += vertCount;
	numIndexes += indexCount;

	surf->numIndexes += indexCount;

	if ( ( startIndex & 1 ) || ( indexCount & 1 ) ) {
		// slow for write combined memory!
		// this should be very rare, since quads are always an even index count
		for ( int i = 0; i < indexCount; i++ ) {
			indexPointer[startIndex + i] = startVert + tempIndexes[i];
		}
	} else {
		for ( int i = 0; i < indexCount; i += 2 ) {
			WriteIndexPair( indexPointer + startIndex + i, startVert + tempIndexes[i], startVert + tempIndexes[i+1] );
		}
	}

	return vertexPointer + startVert;
}

