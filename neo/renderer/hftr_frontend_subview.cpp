#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hftr_local.h"
#include "hfModel_local.h"

/*
==========================================================================================

SUBVIEW SURFACES

==========================================================================================
*/

struct orientation_t {
	hfVec3		origin;
	hfMat3		axis;
};

/*
=================
R_MirrorPoint
=================
*/
static void R_MirrorPoint( const hfVec3 in, orientation_t *surface, orientation_t *camera, hfVec3 &out ) {
	const hfVec3 local = in - surface->origin;

	hfVec3 transformed( 0.0f );
	for ( int i = 0; i < 3; i++ ) {
		const float d = local * surface->axis[i];
		transformed += d * camera->axis[i];
	}

	out = transformed + camera->origin;
}

/*
=================
R_MirrorVector
=================
*/
static void R_MirrorVector( const hfVec3 in, orientation_t *surface, orientation_t *camera, hfVec3 &out ) {
	out.Zero();
	for ( int i = 0; i < 3; i++ ) {
		const float d = in * surface->axis[i];
		out += d * camera->axis[i];
	}
}

/*
=============
R_PlaneForSurface

Returns the plane for the first triangle in the surface
FIXME: check for degenerate triangle?
=============
*/
static void R_PlaneForSurface( const srfTriangles_t *tri, hfPlane &plane ) {
	hfDrawVert * v1 = tri->verts + tri->indexes[0];
	hfDrawVert * v2 = tri->verts + tri->indexes[1];
	hfDrawVert * v3 = tri->verts + tri->indexes[2];
	plane.FromPoints( v1->xyz, v2->xyz, v3->xyz );
}

/*
=========================
R_PreciseCullSurface

Check the surface for visibility on a per-triangle basis
for cases when it is going to be VERY expensive to draw (subviews)

If not culled, also returns the bounding box of the surface in 
Normalized Device Coordinates, so it can be used to crop the scissor rect.

OPTIMIZE: we could also take exact portal passing into consideration
=========================
*/
bool R_PreciseCullSurface( const drawSurf_t *drawSurf, hfBounds &ndcBounds ) {
	const srfTriangles_t * tri = drawSurf->frontEndGeo;

	unsigned int pointOr = 0;
	unsigned int pointAnd = (unsigned int)~0;

	// get an exact bounds of the triangles for scissor cropping
	ndcBounds.Clear();

	const hfJointMat * joints = ( tri->staticModelWithJoints != NULL && r_useGPUSkinning.GetBool() ) ? tri->staticModelWithJoints->jointsInverted : NULL;

	for ( int i = 0; i < tri->numVerts; i++ ) {
		const hfVec3 vXYZ = hfDrawVert::GetSkinnedDrawVertPosition( tri->verts[i], joints );

		hfPlane eye, clip;
		R_TransformModelToClip( vXYZ, drawSurf->space->modelViewMatrix, tr.viewDef->projectionMatrix, eye, clip );

		unsigned int pointFlags = 0;
		for ( int j = 0; j < 3; j++ ) {
			if ( clip[j] >= clip[3] ) {
				pointFlags |= ( 1 << (j*2+0) );
			} else if ( clip[j] <= -clip[3] ) {	// FIXME: the D3D near clip plane is at zero instead of -1
				pointFlags |= ( 1 << (j*2+1) );
			}
		}

		pointAnd &= pointFlags;
		pointOr |= pointFlags;
	}

	// trivially reject
	if ( pointAnd != 0 ) {
		return true;
	}

	// backface and frustum cull
	hfVec3 localViewOrigin;
	R_GlobalPointToLocal( drawSurf->space->modelMatrix, tr.viewDef->renderView.vieworg, localViewOrigin );

	for ( int i = 0; i < tri->numIndexes; i += 3 ) {
		const hfVec3 v1 = hfDrawVert::GetSkinnedDrawVertPosition( tri->verts[ tri->indexes[ i+0 ] ], joints );
		const hfVec3 v2 = hfDrawVert::GetSkinnedDrawVertPosition( tri->verts[ tri->indexes[ i+1 ] ], joints );
		const hfVec3 v3 = hfDrawVert::GetSkinnedDrawVertPosition( tri->verts[ tri->indexes[ i+2 ] ], joints );

		// this is a hack, because R_GlobalPointToLocal doesn't work with the non-normalized
		// axis that we get from the gui view transform.  It doesn't hurt anything, because
		// we know that all gui generated surfaces are front facing
		if ( tr.guiRecursionLevel == 0 ) {
			// we don't care that it isn't normalized,
			// all we want is the sign
			const hfVec3 d1 = v2 - v1;
			const hfVec3 d2 = v3 - v1;
			const hfVec3 normal = d2.Cross( d1 );

			const hfVec3 dir = v1 - localViewOrigin;

			const float dot = normal * dir;
			if ( dot >= 0.0f ) {
				return true;
			}
		}

		// now find the exact screen bounds of the clipped triangle
		hfFixedWinding w;
		w.SetNumPoints( 3 );
		R_LocalPointToGlobal( drawSurf->space->modelMatrix, v1, w[0].ToVec3() );
		R_LocalPointToGlobal( drawSurf->space->modelMatrix, v2, w[1].ToVec3() );
		R_LocalPointToGlobal( drawSurf->space->modelMatrix, v3, w[2].ToVec3() );
		w[0].s = w[0].t = w[1].s = w[1].t = w[2].s = w[2].t = 0.0f;

		for ( int j = 0; j < 4; j++ ) {
			if ( !w.ClipInPlace( -tr.viewDef->frustum[j], 0.1f ) ) {
				break;
			}
		}
		for ( int j = 0; j < w.GetNumPoints(); j++ ) {
			hfVec3 screen;

			R_GlobalToNormalizedDeviceCoordinates( w[j].ToVec3(), screen );
			ndcBounds.AddPoint( screen );
		}
	}

	// if we don't enclose any area, return
	if ( ndcBounds.IsCleared() ) {
		return true;
	}

	return false;
}

/*
========================
R_MirrorViewBySurface
========================
*/
static viewDef_t *R_MirrorViewBySurface( const drawSurf_t *drawSurf ) {
	// copy the viewport size from the original
	viewDef_t * parms = (viewDef_t *)R_FrameAlloc( sizeof( *parms ) );
	*parms = *tr.viewDef;
	parms->renderView.viewID = 0;	// clear to allow player bodies to show up, and suppress view weapons

	parms->isSubview = true;
	parms->isMirror = true;

	// create plane axis for the portal we are seeing
	hfPlane originalPlane, plane;
	R_PlaneForSurface( drawSurf->frontEndGeo, originalPlane );
	R_LocalPlaneToGlobal( drawSurf->space->modelMatrix, originalPlane, plane );

	orientation_t surface;
	surface.origin = plane.Normal() * -plane[3];
	surface.axis[0] = plane.Normal();
	surface.axis[0].NormalVectors( surface.axis[1], surface.axis[2] );
	surface.axis[2] = -surface.axis[2];

	orientation_t camera;
	camera.origin = surface.origin;
	camera.axis[0] = -surface.axis[0];
	camera.axis[1] = surface.axis[1];
	camera.axis[2] = surface.axis[2];

	// set the mirrored origin and axis
	R_MirrorPoint( tr.viewDef->renderView.vieworg, &surface, &camera, parms->renderView.vieworg );

	R_MirrorVector( tr.viewDef->renderView.viewaxis[0], &surface, &camera, parms->renderView.viewaxis[0] );
	R_MirrorVector( tr.viewDef->renderView.viewaxis[1], &surface, &camera, parms->renderView.viewaxis[1] );
	R_MirrorVector( tr.viewDef->renderView.viewaxis[2], &surface, &camera, parms->renderView.viewaxis[2] );

	// make the view origin 16 units away from the center of the surface
	const hfVec3 center = ( drawSurf->frontEndGeo->bounds[0] + drawSurf->frontEndGeo->bounds[1] ) * 0.5f;
	const hfVec3 viewOrigin = center + ( originalPlane.Normal() * 16.0f );

	R_LocalPointToGlobal( drawSurf->space->modelMatrix, viewOrigin, parms->initialViewAreaOrigin );

	// set the mirror clip plane
	parms->numClipPlanes = 1;
	parms->clipPlanes[0] = -camera.axis[0];

	parms->clipPlanes[0][3] = -( camera.origin * parms->clipPlanes[0].Normal() );
	
	return parms;
}

/*
========================
R_XrayViewBySurface
========================
*/
static viewDef_t *R_XrayViewBySurface( const drawSurf_t *drawSurf ) {
	// copy the viewport size from the original
	viewDef_t * parms = (viewDef_t *)R_FrameAlloc( sizeof( *parms ) );
	*parms = *tr.viewDef;
	parms->renderView.viewID = 0;	// clear to allow player bodies to show up, and suppress view weapons

	parms->isSubview = true;
	parms->isXraySubview = true;

	return parms;
}

/*
===============
R_RemoteRender
===============
*/
static void R_RemoteRender( const drawSurf_t *surf, textureStage_t *stage ) {
	// remote views can be reused in a single frame
	if ( stage->dynamicFrameCount == tr.frameCount ) {
		return;
	}

	// if the entity doesn't have a remoteRenderView, do nothing
	if ( !surf->space->entityDef->parms.remoteRenderView ) {
		return;
	}

	int stageWidth = stage->width;
	int stageHeight = stage->height;

	// copy the viewport size from the original
	viewDef_t * parms = (viewDef_t *)R_FrameAlloc( sizeof( *parms ) );
	*parms = *tr.viewDef;

	parms->renderView = *surf->space->entityDef->parms.remoteRenderView;
	parms->renderView.viewID = 0;	// clear to allow player bodies to show up, and suppress view weapons
	parms->initialViewAreaOrigin = parms->renderView.vieworg;
	parms->isSubview = true;
	parms->isMirror = false;


	tr.CropRenderSize( stageWidth, stageHeight );

	tr.GetCroppedViewport( &parms->viewport );

	parms->scissor.x1 = 0;
	parms->scissor.y1 = 0;
	parms->scissor.x2 = parms->viewport.x2 - parms->viewport.x1;
	parms->scissor.y2 = parms->viewport.y2 - parms->viewport.y1;

	parms->superView = tr.viewDef;
	parms->subviewSurface = surf;

	// generate render commands for it
	R_RenderView( parms );

	// copy this rendering to the image
	stage->dynamicFrameCount = tr.frameCount;
	if ( stage->image == NULL ) {
		stage->image = globalImages->scratchImage;
	}

	tr.CaptureRenderToImage( stage->image->GetName(), true );
	tr.UnCrop();
}

/*
=================
R_MirrorRender
=================
*/
void R_MirrorRender( const drawSurf_t *surf, textureStage_t *stage, hfScreenRect scissor ) {
	// remote views can be reused in a single frame
	if ( stage->dynamicFrameCount == tr.frameCount ) {
		return;
	}

	// issue a new view command
	viewDef_t * parms = R_MirrorViewBySurface( surf );
	if ( parms == NULL ) {
		return;
	}

	tr.CropRenderSize( stage->width, stage->height );

	tr.GetCroppedViewport( &parms->viewport );

	parms->scissor.x1 = 0;
	parms->scissor.y1 = 0;
	parms->scissor.x2 = parms->viewport.x2 - parms->viewport.x1;
	parms->scissor.y2 = parms->viewport.y2 - parms->viewport.y1;

	parms->superView = tr.viewDef;
	parms->subviewSurface = surf;

	// triangle culling order changes with mirroring
	parms->isMirror = ( ( (int)parms->isMirror ^ (int)tr.viewDef->isMirror ) != 0 );

	// generate render commands for it
	R_RenderView( parms );

	// copy this rendering to the image
	stage->dynamicFrameCount = tr.frameCount;
	stage->image = globalImages->scratchImage;

	tr.CaptureRenderToImage( stage->image->GetName() );
	tr.UnCrop();
}

/*
=================
R_XrayRender
=================
*/
void R_XrayRender( const drawSurf_t *surf, textureStage_t *stage, hfScreenRect scissor ) {
	// remote views can be reused in a single frame
	if ( stage->dynamicFrameCount == tr.frameCount ) {
		return;
	}

	// issue a new view command
	viewDef_t * parms = R_XrayViewBySurface( surf );
	if ( parms == NULL ) {
		return;
	}

	int stageWidth = stage->width;
	int stageHeight = stage->height;

	tr.CropRenderSize( stageWidth, stageHeight );

	tr.GetCroppedViewport( &parms->viewport );

	parms->scissor.x1 = 0;
	parms->scissor.y1 = 0;
	parms->scissor.x2 = parms->viewport.x2 - parms->viewport.x1;
	parms->scissor.y2 = parms->viewport.y2 - parms->viewport.y1;

	parms->superView = tr.viewDef;
	parms->subviewSurface = surf;

	// triangle culling order changes with mirroring
	parms->isMirror = ( ( (int)parms->isMirror ^ (int)tr.viewDef->isMirror ) != 0 );

	// generate render commands for it
	R_RenderView( parms );

	// copy this rendering to the image
	stage->dynamicFrameCount = tr.frameCount;
	stage->image = globalImages->scratchImage2;

	tr.CaptureRenderToImage( stage->image->GetName(), true );
	tr.UnCrop();
}

/*
==================
R_GenerateSurfaceSubview
==================
*/
bool R_GenerateSurfaceSubview( const drawSurf_t *drawSurf ) {
	// for testing the performance hit
	if ( r_skipSubviews.GetBool() ) {
		return false;
	}

	hfBounds ndcBounds;
	if ( R_PreciseCullSurface( drawSurf, ndcBounds ) ) {
		return false;
	}

	const hfMaterial * shader = drawSurf->material;

	// never recurse through a subview surface that we are
	// already seeing through
	viewDef_t * parms = NULL;
	for ( parms = tr.viewDef; parms != NULL; parms = parms->superView ) {
		if ( parms->subviewSurface != NULL
			&& parms->subviewSurface->frontEndGeo == drawSurf->frontEndGeo
			&& parms->subviewSurface->space->entityDef == drawSurf->space->entityDef ) {
			break;
		}
	}
	if ( parms ) {
		return false;
	}

	// crop the scissor bounds based on the precise cull
	assert( tr.viewDef != NULL );
	hfScreenRect * v = &tr.viewDef->viewport;
	hfScreenRect scissor;
	scissor.x1 = v->x1 + hfMath::Ftoi( ( v->x2 - v->x1 + 1 ) * 0.5f * ( ndcBounds[0][0] + 1.0f ) );
	scissor.y1 = v->y1 + hfMath::Ftoi( ( v->y2 - v->y1 + 1 ) * 0.5f * ( ndcBounds[0][1] + 1.0f ) );
	scissor.x2 = v->x1 + hfMath::Ftoi( ( v->x2 - v->x1 + 1 ) * 0.5f * ( ndcBounds[1][0] + 1.0f ) );
	scissor.y2 = v->y1 + hfMath::Ftoi( ( v->y2 - v->y1 + 1 ) * 0.5f * ( ndcBounds[1][1] + 1.0f ) );

	// nudge a bit for safety
	scissor.Expand();

	scissor.Intersect( tr.viewDef->scissor );

	if ( scissor.IsEmpty() ) {
		// cropped out
		return false;
	}

	// see what kind of subview we are making
	if ( shader->GetSort() != SS_SUBVIEW ) {
		for ( int i = 0; i < shader->GetNumStages(); i++ ) {
			const shaderStage_t	*stage = shader->GetStage( i );
			switch ( stage->texture.dynamic ) {
			case DI_REMOTE_RENDER:
				R_RemoteRender( drawSurf, const_cast<textureStage_t *>(&stage->texture) );
				break;
			case DI_MIRROR_RENDER:
				R_MirrorRender( drawSurf, const_cast<textureStage_t *>(&stage->texture), scissor );
				break;
			case DI_XRAY_RENDER:
				R_XrayRender( drawSurf, const_cast<textureStage_t *>(&stage->texture), scissor );
				break;
			}
		}
		return true;
	}

	// issue a new view command
	parms = R_MirrorViewBySurface( drawSurf );
	if ( parms == NULL ) {
		return false;
	}

	parms->scissor = scissor;
	parms->superView = tr.viewDef;
	parms->subviewSurface = drawSurf;

	// triangle culling order changes with mirroring
	parms->isMirror = ( ( (int)parms->isMirror ^ (int)tr.viewDef->isMirror ) != 0 );

	// generate render commands for it
	R_RenderView( parms );

	return true;
}

/*
================
R_GenerateSubViews

If we need to render another view to complete the current view,
generate it first.

It is important to do this after all drawSurfs for the current
view have been generated, because it may create a subview which
would change tr.viewCount.
================
*/
bool R_GenerateSubViews( const drawSurf_t * const drawSurfs[], const int numDrawSurfs ) {
	SCOPED_PROFILE_EVENT( "R_GenerateSubViews" );

	// for testing the performance hit
	if ( r_skipSubviews.GetBool() ) {
		return false;
	}

	// scan the surfaces until we either find a subview, or determine
	// there are no more subview surfaces.
	bool subviews = false;
	for ( int i = 0; i < numDrawSurfs; i++ ) {
		const drawSurf_t * drawSurf = drawSurfs[i];

		if ( !drawSurf->material->HasSubview() ) {
			continue;
		}

		if ( R_GenerateSurfaceSubview( drawSurf ) ) {
			subviews = true;
		}
	}

	return subviews;
}

