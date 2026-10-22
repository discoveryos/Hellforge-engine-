#ifndef __HF_LIGHT_CLUSTER_H__
#define __HF_LIGHT_CLUSTER_H__

#include "hfStr.h"

// Pure CPU-side clustered / tiled light assignment, usable by the engine
// front-end/back-end light loop and the offline harness. No GL, no renderer
// structs: lights are plain floats/labels. Operates in clip space: a light
// whose center lands inside a tile's NDC box (expanded by a z/w-based radius
// margin) is assigned a cluster = slice * tilesX * tilesY + y * tilesX + x.

namespace HF_Cluster {

static const int MAX_TILES_X		= 64;
static const int MAX_TILES_Y		= 40;
static const int MAX_TILES			= MAX_TILES_X * MAX_TILES_Y;
static const int MAX_SLICES			= 16;
static const int MAX_CLUSTERS		= MAX_TILES * MAX_SLICES;
static const int DEFAULT_TILE_SIZE	= 48;
static const int DEFAULT_MAX_LIGHTS	= 8;	// per cluster
static const int MAX_LIGHTS_INPUT	= 1024;

struct LightEntry {
	float		position[3];	// world space
	float		radius;		// world-space influence radius (>= 0)
	unsigned int index;		// caller's light id (written into lists)
	float		priority;	// brightness weight for sort (highest kept first)
};

struct AssignResult {
	unsigned short		offset[MAX_CLUSTERS];			// per-cluster start in indices[]
	unsigned short		count[MAX_CLUSTERS];			// per-cluster count
	unsigned short		indices[ MAX_CLUSTERS * DEFAULT_MAX_LIGHTS ];
	unsigned int		clusterCount;
	unsigned int		totalAssigned;
	unsigned int		culledByCapacity;				// lights rejected everywhere
};

static const int MAX_DEPTH_SLICES = MAX_SLICES;

static const float CLIP_Z_NEAR_DEFAULT = 0.01f;
static const float CLIP_Z_FAR_DEFAULT = 1000.0f;

static inline void ComputeTileGrid( int width, int height, int tileSize, int &tilesX, int &tilesY ) {
	tilesX = width / tileSize;
	tilesY = height / tileSize;
	if ( tilesX < 1 ) { tilesX = 1; }
	if ( tilesY < 1 ) { tilesY = 1; }
	if ( tilesX > MAX_TILES_X ) { tilesX = MAX_TILES_X; }
	if ( tilesY > MAX_TILES_Y ) { tilesY = MAX_TILES_Y; }
}

static inline unsigned int DepthSlice( float clipZ, float nearZ, float farZ ) {
	( void )nearZ;
	( void )farZ;
	// Treats clipZ as NDC depth in [-1, 1]: -1 = near plane, +1 = far plane.
	if ( clipZ > 1.0f ) { clipZ = 1.0f; }
	if ( clipZ < -1.0f ) { clipZ = -1.0f; }
	float t = clipZ * 0.5f + 0.5f;
	// log-style spacing so near slices (where z precision matters) are thin
	t = 1.0f - ( 1.0f - t ) * ( 1.0f - t );	// ease into far slices
	unsigned int s = ( unsigned int )( t * MAX_DEPTH_SLICES );
	if ( s >= MAX_DEPTH_SLICES ) { s = MAX_DEPTH_SLICES - 1; }
	return s;
}

static inline void TransformPointMatrix( const float m[16], const float v[3], float out[4] ) {
	out[0] = m[0] * v[0] + m[4] * v[1] + m[8]  * v[2] + m[12];
	out[1] = m[1] * v[0] + m[5] * v[1] + m[9]  * v[2] + m[13];
	out[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14];
	out[3] = m[3] * v[0] + m[7] * v[1] + m[11] * v[2] + m[15];
}

// Bubble sort a cluster's light indices by priority (descending), trimming
// to at most 'capacity' entries.
static inline void TrimAndSort( unsigned short *list, unsigned short &count, int capacity, const LightEntry *lights ) {
	int n = count < ( unsigned short )capacity ? count : capacity;
	if ( n <= 0 ) { return; }
	// copy the top 'capacity' priorities using insertion of the remainder
	int dst = 0;
	for ( int src = 0; src < count; src++ ) {
		unsigned int li = list[src];
		if ( dst == n && lights[li].priority <= lights[list[dst - 1]].priority ) {
			continue;
		}
		// insert keeping descending order
		int at = dst;
		while ( at > 0 && lights[list[at - 1]].priority < lights[li].priority ) { at--; }
		for ( int k = ( dst < n ? dst : n - 1 ); k > at; k-- ) { list[k] = list[k - 1]; }
		list[at] = ( unsigned short )li;
		if ( dst < n ) { dst++; }
	}
	count = ( unsigned short )dst;
}

static inline unsigned int AssignLights( const float worldToClip[16],
										const int width, const int height, const int tileSize,
										const int numSlices,
										const LightEntry *lights, int numLights,
										int capacity, float marginScale,
										AssignResult &result ) {
	int tilesX, tilesY;
	ComputeTileGrid( width, height, tileSize, tilesX, tilesY );
	if ( capacity <= 0 ) { capacity = DEFAULT_MAX_LIGHTS; }

	result.clusterCount = ( unsigned int )tilesX * tilesY * ( numSlices > 0 ? numSlices : 1 );
	if ( result.clusterCount > MAX_CLUSTERS ) { result.clusterCount = MAX_CLUSTERS; }
	for ( unsigned int c = 0; c < result.clusterCount; c++ ) {
		result.offset[c] = ( unsigned short )( c * capacity );
		result.count[c] = 0;
	}
	result.totalAssigned = 0;
	result.culledByCapacity = 0;

	const float invW = 1.0f / ( float )( width );
	const float invH = 1.0f / ( float )( height );
	const int slices = numSlices > 0 ? numSlices : 1;

	if ( numLights > MAX_LIGHTS_INPUT ) { numLights = MAX_LIGHTS_INPUT; }
	for ( int l = 0; l < numLights; l++ ) {
		const LightEntry &le = lights[l];
		float clip[4];
		TransformPointMatrix( worldToClip, le.position, clip );
		if ( clip[3] == 0.0f ) { continue; }
		float inv = 1.0f / clip[3];
		float ndcX = clip[0] * inv;
		float ndcY = clip[1] * inv;
		float ndcZ = clip[2] * inv;

		// radius → ndc margin (approximate; radius / distance at light center)
		float margin = marginScale * ( le.radius * inv );
		// A light behind the near plane still illuminates the view volume,
		// so only the far side (ndcZ > 1 + margin) culls on z.
		if ( ndcX < -1.0f - margin || ndcX > 1.0f + margin ||
			 ndcY < -1.0f - margin || ndcY > 1.0f + margin ||
			 ndcZ > 1.0f + margin ) {
			continue;
		}
		int tx = ( int )( ( ndcX * 0.5f + 0.5f ) * tilesX );
		int ty = ( int )( ( ndcY * 0.5f + 0.5f ) * tilesY );
		if ( tx < 0 ) { tx = 0; } if ( tx >= tilesX ) { tx = tilesX - 1; }
		if ( ty < 0 ) { ty = 0; } if ( ty >= tilesY ) { ty = tilesY - 1; }
		unsigned int slice = DepthSlice( ndcZ, CLIP_Z_NEAR_DEFAULT, CLIP_Z_FAR_DEFAULT );
		if ( slice >= ( unsigned int )MAX_DEPTH_SLICES ) { slice = MAX_DEPTH_SLICES - 1; }

		unsigned int cluster = slice * ( unsigned int )tilesX * tilesY + ( unsigned int )ty * tilesX + ( unsigned int )tx;
		if ( cluster >= result.clusterCount ) { cluster = result.clusterCount - 1; }

		if ( result.count[cluster] >= capacity ) {
			// might still be brighter than an existing occupant
			TrimAndSort( &result.indices[result.offset[cluster]], result.count[cluster], capacity, lights );
		}
		if ( result.count[cluster] < capacity ) {
			unsigned int at = result.offset[cluster] + result.count[cluster];
			result.indices[at] = le.index;
			result.count[cluster]++;
			TrimAndSort( &result.indices[result.offset[cluster]], result.count[cluster], capacity, lights );
			result.totalAssigned++;
		} else {
			result.culledByCapacity++;
		}
	}
	return result.totalAssigned;
}

static inline void Summarize( const AssignResult &result, int maxClusters, hfStr &out ) {
	out = "";
	int shown = 0;
	for ( unsigned int c = 0; c < result.clusterCount && shown < maxClusters; c++ ) {
		if ( result.count[c] == 0 ) { continue; }
		out += va( "[%d]%d ", c, result.count[c] );
		shown++;
	}
}

} // namespace HF_Cluster

#endif