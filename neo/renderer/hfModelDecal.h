#ifndef __MODELDECAL_H__
#define __MODELDECAL_H__

/*
===============================================================================

	Decals are lightweight primitives for bullet / blood marks on static
	geometry. Decals with common materials will be merged together, but
	additional decals will be allocated as needed. The material should not
	be one that receives lighting, because no interactions are generated
	for these lightweight surfaces.

	FIXME:	Decals on models in portalled off areas do not get freed
			until the area becomes visible again.

===============================================================================
*/

static const int NUM_DECAL_BOUNDING_PLANES	= 6;
#ifdef HF_PC
static const int MAX_DEFERRED_DECALS		= 8;
static const int DEFFERED_DECAL_TIMEOUT		= 1000;	// don't create a decal if it wasn't visible within the first second
static const int MAX_DECALS					= 64;
#else
static const int MAX_DEFERRED_DECALS		= 4;
static const int DEFFERED_DECAL_TIMEOUT		= 200;	// don't create a decal if it wasn't visible within the first 200 milliseconds
static const int MAX_DECALS					= 32;
#endif
static const int MAX_DECAL_VERTS			= 3 + NUM_DECAL_BOUNDING_PLANES + 3 + 6;	// 3 triangle verts clipped NUM_DECAL_BOUNDING_PLANES + 3 times (plus 6 for safety)
static const int MAX_DECAL_INDEXES			= ( MAX_DECAL_VERTS - 2 ) * 3;

compile_time_assert( CONST_ISPOWEROFTWO( MAX_DECALS ) );
// the max indices must be a multiple of 2 for copying indices to write-combined memory
compile_time_assert( ( ( MAX_DECAL_INDEXES * sizeof( triIndex_t ) ) & 15 ) == 0 );

struct decalProjectionParms_t {
	hfPlane					boundingPlanes[NUM_DECAL_BOUNDING_PLANES];
	hfPlane					fadePlanes[2];
	hfPlane					textureAxis[2];
	hfVec3					projectionOrigin;
	hfBounds				projectionBounds;
	const hfMaterial *		material;
	float					fadeDepth;
	int						startTime;
	bool					parallel;
	bool					force;
};

ALIGNTYPE16 struct decal_t {
	ALIGNTYPE16 hfDrawVert	verts[MAX_DECAL_VERTS];
	ALIGNTYPE16 triIndex_t	indexes[MAX_DECAL_INDEXES];
	float					vertDepthFade[MAX_DECAL_VERTS];
	int						numVerts;
	int						numIndexes;
	int						startTime;
	const hfMaterial *		material;
};

class hfRenderModelDecal {
public:
								hfRenderModelDecal();
								~hfRenderModelDecal();

								// Creates decal projection parameters.
	static bool					CreateProjectionParms( decalProjectionParms_t &parms, const hfFixedWinding &winding, const hfVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const hfMaterial *material, const int startTime );

								// Transform the projection parameters from global space to local.
	static void					GlobalProjectionParmsToLocal( decalProjectionParms_t &localParms, const decalProjectionParms_t &globalParms, const hfVec3 &origin, const hfMat3 &axis );

								// clear the model for reuse
	void						ReUse();

								// Save the parameters for the renderer front-end to actually create the decal.
	void						AddDeferredDecal( const decalProjectionParms_t & localParms );

								// Creates a decal on the given model.
	void						CreateDeferredDecals( const hfRenderModel *model );

								// Remove decals that are completely faded away.
	void						RemoveFadedDecals( int time );

	unsigned int				GetNumDecalDrawSurfs();
	struct drawSurf_t *			CreateDecalDrawSurf( const struct viewEntity_t *space, unsigned int index );

	void						ReadFromDemoFile( class hfDemoFile *f );
	void						WriteToDemoFile( class hfDemoFile *f ) const;

private:
	decal_t						decals[MAX_DECALS];
	unsigned int				firstDecal;
	unsigned int				nextDecal;

	decalProjectionParms_t		deferredDecals[MAX_DEFERRED_DECALS];
	unsigned int				firstDeferredDecal;
	unsigned int				nextDeferredDecal;

	const hfMaterial *			decalMaterials[MAX_DECALS];
	unsigned int				numDecalMaterials;

	void						CreateDecalFromWinding( const hfWinding &w, const hfMaterial *decalMaterial, const hfPlane fadePlanes[2], float fadeDepth, int startTime );
	void						CreateDecal( const hfRenderModel *model, const decalProjectionParms_t &localParms );
};

#endif /* !__MODELDECAL_H__ */

