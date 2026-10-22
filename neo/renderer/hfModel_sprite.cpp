#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hftr_local.h"
#include "hfModel_local.h"


/*

A simple sprite model that always faces the view axis.

*/

static const char *sprite_SnapshotName = "_sprite_Snapshot_";

/*
===============
hfRenderModelSprite::IsDynamicModel
===============
*/
dynamicModel_t hfRenderModelSprite::IsDynamicModel() const {
	return DM_CONTINUOUS;
}

/*
===============
hfRenderModelSprite::IsLoaded
===============
*/
bool hfRenderModelSprite::IsLoaded() const {
	return true;
}

/*
===============
hfRenderModelSprite::InstantiateDynamicModel
===============
*/
hfRenderModel *	hfRenderModelSprite::InstantiateDynamicModel( const struct renderEntity_s *renderEntity, const viewDef_t *viewDef, hfRenderModel *cachedModel ) {
	hfRenderModelStatic *staticModel;
	srfTriangles_t *tri;
	modelSurface_t surf;

	if ( cachedModel && !r_useCachedDynamicModels.GetBool() ) {
		delete cachedModel;
		cachedModel = NULL;
	}

	if ( renderEntity == NULL || viewDef == NULL ) {
		delete cachedModel;
		return NULL;
	}

	if ( cachedModel != NULL ) {

		assert( dynamic_cast<hfRenderModelStatic *>( cachedModel ) != NULL );
		assert( hfStr::Icmp( cachedModel->Name(), sprite_SnapshotName ) == 0 );

		staticModel = static_cast<hfRenderModelStatic *>( cachedModel );
		surf = *staticModel->Surface( 0 );
		tri = surf.geometry;

	} else {

		staticModel = new (TAG_MODEL) hfRenderModelStatic;
		staticModel->InitEmpty( sprite_SnapshotName );

		tri = R_AllocStaticTriSurf();
		R_AllocStaticTriSurfVerts( tri, 4 );
		R_AllocStaticTriSurfIndexes( tri, 6 );

		tri->verts[ 0 ].Clear();
		tri->verts[ 0 ].SetNormal( 1.0f, 0.0f, 0.0f );
		tri->verts[ 0 ].SetTangent( 0.0f, 1.0f, 0.0f );
		tri->verts[ 0 ].SetBiTangent( 0.0f, 0.0f, 1.0f );
		tri->verts[ 0 ].SetTexCoord( 0.0f, 0.0f );

		tri->verts[ 1 ].Clear();
		tri->verts[ 1 ].SetNormal( 1.0f, 0.0f, 0.0f );
		tri->verts[ 1 ].SetTangent( 0.0f, 1.0f, 0.0f );
		tri->verts[ 1 ].SetBiTangent( 0.0f, 0.0f, 1.0f );
		tri->verts[ 1 ].SetTexCoord( 1.0f, 0.0f );

		tri->verts[ 2 ].Clear();
		tri->verts[ 2 ].SetNormal( 1.0f, 0.0f, 0.0f );
		tri->verts[ 2 ].SetTangent( 0.0f, 1.0f, 0.0f );
		tri->verts[ 2 ].SetBiTangent( 0.0f, 0.0f, 1.0f );
		tri->verts[ 2 ].SetTexCoord( 1.0f, 1.0f );

		tri->verts[ 3 ].Clear();
		tri->verts[ 3 ].SetNormal( 1.0f, 0.0f, 0.0f );
		tri->verts[ 3 ].SetTangent( 0.0f, 1.0f, 0.0f );
		tri->verts[ 3 ].SetBiTangent( 0.0f, 0.0f, 1.0f );
		tri->verts[ 3 ].SetTexCoord( 0.0f, 1.0f );

		tri->indexes[ 0 ] = 0;
		tri->indexes[ 1 ] = 1;
		tri->indexes[ 2 ] = 3;
		tri->indexes[ 3 ] = 1;
		tri->indexes[ 4 ] = 2;
		tri->indexes[ 5 ] = 3;

		tri->numVerts = 4;
		tri->numIndexes = 6;

		surf.geometry = tri;
		surf.id = 0;
		surf.shader = tr.defaultMaterial;
		staticModel->AddSurface( surf );
	}

	int	red			= hfMath::Ftoi( renderEntity->shaderParms[ SHADERPARM_RED ] * 255.0f );
	int green		= hfMath::Ftoi( renderEntity->shaderParms[ SHADERPARM_GREEN ] * 255.0f );
	int	blue		= hfMath::Ftoi( renderEntity->shaderParms[ SHADERPARM_BLUE ] * 255.0f );
	int	alpha		= hfMath::Ftoi( renderEntity->shaderParms[ SHADERPARM_ALPHA ] * 255.0f );

	hfVec3 right	= hfVec3( 0.0f, renderEntity->shaderParms[ SHADERPARM_SPRITE_WIDTH ] * 0.5f, 0.0f );
	hfVec3 up		= hfVec3( 0.0f, 0.0f, renderEntity->shaderParms[ SHADERPARM_SPRITE_HEIGHT ] * 0.5f );

	tri->verts[ 0 ].xyz = up + right;
	tri->verts[ 0 ].color[ 0 ] = red;
	tri->verts[ 0 ].color[ 1 ] = green;
	tri->verts[ 0 ].color[ 2 ] = blue;
	tri->verts[ 0 ].color[ 3 ] = alpha;

	tri->verts[ 1 ].xyz = up - right;
	tri->verts[ 1 ].color[ 0 ] = red;
	tri->verts[ 1 ].color[ 1 ] = green;
	tri->verts[ 1 ].color[ 2 ] = blue;
	tri->verts[ 1 ].color[ 3 ] = alpha;

	tri->verts[ 2 ].xyz = - right - up;
	tri->verts[ 2 ].color[ 0 ] = red;
	tri->verts[ 2 ].color[ 1 ] = green;
	tri->verts[ 2 ].color[ 2 ] = blue;
	tri->verts[ 2 ].color[ 3 ] = alpha;

	tri->verts[ 3 ].xyz = right - up;
	tri->verts[ 3 ].color[ 0 ] = red;
	tri->verts[ 3 ].color[ 1 ] = green;
	tri->verts[ 3 ].color[ 2 ] = blue;
	tri->verts[ 3 ].color[ 3 ] = alpha;

	R_BoundTriSurf( tri );

	staticModel->bounds = tri->bounds;

	return staticModel;
}

/*
===============
hfRenderModelSprite::Bounds
===============
*/
hfBounds hfRenderModelSprite::Bounds( const struct renderEntity_s *renderEntity ) const {
	hfBounds b;

	b.Zero();
	if ( renderEntity == NULL ) {
		b.ExpandSelf( 8.0f );
	} else {
		b.ExpandSelf( Max( renderEntity->shaderParms[ SHADERPARM_SPRITE_WIDTH ], renderEntity->shaderParms[ SHADERPARM_SPRITE_HEIGHT ] ) * 0.5f );
	}
	return b;
}

