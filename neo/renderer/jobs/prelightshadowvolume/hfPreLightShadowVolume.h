#ifndef __PRELIGHTSHADOWVOLUME_H__
#define __PRELIGHTSHADOWVOLUME_H__

/*
================================================================================================

Pre-Light Shadow Volume Setup

A pre-light shadow is cast from static world geometry touching a static light.
A pre-light shadow volume does not extend to infinity but is capped at the light
boundaries which means the end caps always need to be rendered.

A pre-light shadow volume is created at map compile time and this job determines whether or
not the shadow volume needs to be rendered with Z-Fail.

================================================================================================
*/

/*
================================================
preLightShadowVolumeParms_t
================================================
*/
struct preLightShadowVolumeParms_t {
	// input
	const hfShadowVert *			verts;					// streamed in from main memory
	int								numVerts;
	const triIndex_t *				indexes;				// streamed in from main memory
	int								numIndexes;
	hfBounds						triangleBounds;
	hfRenderMatrix					triangleMVP;
	hfVec3							localLightOrigin;
	hfVec3							localViewOrigin;
	float							zNear;
	float							lightZMin;
	float							lightZMax;
	bool							forceShadowCaps;
	bool							useShadowPreciseInsideTest;
	bool							useShadowDepthBounds;
	// temp
	byte *							tempCullBits;			// temp buffer in SPU local memory
	// output
	int *							numShadowIndices;		// streamed out to main memory
	int *							renderZFail;			// streamed out to main memory
	float *							shadowZMin;				// streamed out to main memory
	float *							shadowZMax;				// streamed out to main memory
	volatile shadowVolumeState_t *	shadowVolumeState;		// streamed out to main memory
	// next in chain on view light
	preLightShadowVolumeParms_t *	next;
	int								pad;
};


void PreLightShadowVolumeJob( const preLightShadowVolumeParms_t * parms );
void PreLightShadowVolume_SetupSPURSHeader( CellSpursJob128 * job, const preLightShadowVolumeParms_t * parms );

#endif // !__PRELIGHTSHADOWVOLUME_H__

