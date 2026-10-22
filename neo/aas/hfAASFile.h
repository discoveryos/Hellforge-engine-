#ifndef __AASFILE_H__
#define __AASFILE_H__

/*
===============================================================================

	AAS File

===============================================================================
*/

#define AAS_FILEID					"DewmAAS"
#define AAS_FILEVERSION				"1.07"

// travel flags
#define TFL_INVALID					BIT(0)		// not valid
#define TFL_WALK					BIT(1)		// walking
#define TFL_CROUCH					BIT(2)		// crouching
#define TFL_WALKOFFLEDGE			BIT(3)		// walking of a ledge
#define TFL_BARRIERJUMP				BIT(4)		// jumping onto a barrier
#define TFL_JUMP					BIT(5)		// jumping
#define TFL_LADDER					BIT(6)		// climbing a ladder
#define TFL_SWIM					BIT(7)		// swimming
#define TFL_WATERJUMP				BIT(8)		// jump out of the water
#define TFL_TELEPORT				BIT(9)		// teleportation
#define TFL_ELEVATOR				BIT(10)		// travel by elevator
#define TFL_FLY						BIT(11)		// fly
#define TFL_SPECIAL					BIT(12)		// special
#define TFL_WATER					BIT(21)		// travel through water
#define TFL_AIR						BIT(22)		// travel through air

// face flags
#define FACE_SOLID					BIT(0)		// solid at the other side
#define FACE_LADDER					BIT(1)		// ladder surface
#define FACE_FLOOR					BIT(2)		// standing on floor when on this face
#define FACE_LIQUID					BIT(3)		// face seperating two areas with liquid
#define FACE_LIQUIDSURFACE			BIT(4)		// face seperating liquid and air

// area flags
#define AREA_FLOOR					BIT(0)		// AI can stand on the floor in this area
#define AREA_GAP					BIT(1)		// area has a gap
#define AREA_LEDGE					BIT(2)		// if entered the AI bbox partly floats above a ledge
#define AREA_LADDER					BIT(3)		// area contains one or more ladder faces
#define AREA_LIQUID					BIT(4)		// area contains a liquid
#define AREA_CROUCH					BIT(5)		// AI cannot walk but can only crouch in this area
#define AREA_REACHABLE_WALK			BIT(6)		// area is reachable by walking or swimming
#define AREA_REACHABLE_FLY			BIT(7)		// area is reachable by flying

// area contents flags
#define AREACONTENTS_SOLID			BIT(0)		// solid, not a valid area
#define AREACONTENTS_WATER			BIT(1)		// area contains water
#define AREACONTENTS_CLUSTERPORTAL	BIT(2)		// area is a cluster portal
#define AREACONTENTS_OBSTACLE		BIT(3)		// area contains (part of) a dynamic obstacle
#define AREACONTENTS_TELEPORTER		BIT(4)		// area contains (part of) a teleporter trigger

// bits for different bboxes
#define AREACONTENTS_BBOX_BIT		24

#define MAX_REACH_PER_AREA			256
#define MAX_AAS_TREE_DEPTH			128

#define MAX_AAS_BOUNDING_BOXES		4

// reachability to another area
class hfReachability {
public:
	int							travelType;			// type of travel required to get to the area
	short						toAreaNum;			// number of the reachable area
	short						fromAreaNum;		// number of area the reachability starts
	hfVec3						start;				// start point of inter area movement
	hfVec3						end;				// end point of inter area movement
	int							edgeNum;			// edge crossed by this reachability
	unsigned short				travelTime;			// travel time of the inter area movement
	byte						number;				// reachability number within the fromAreaNum (must be < 256)
	byte						disableCount;		// number of times this reachability has been disabled
	hfReachability *			next;				// next reachability in list
	hfReachability *			rev_next;			// next reachability in reversed list
	unsigned short *			areaTravelTimes;	// travel times within the fromAreaNum from reachabilities that lead towards this area
public:
	void						CopyBase( hfReachability &reach );
};

class hfReachability_Walk : public hfReachability {
};

class hfReachability_BarrierJump : public hfReachability {
};

class hfReachability_WaterJump : public hfReachability {
};

class hfReachability_WalkOffLedge : public hfReachability {
};

class hfReachability_Swim : public hfReachability {
};

class hfReachability_Fly : public hfReachability {
};

class hfReachability_Special : public hfReachability {
public:
	hfDict						dict;
};

// index
typedef int aasIndex_t;

// vertex
typedef hfVec3 aasVertex_t;

// edge
typedef struct aasEdge_s {
	int							vertexNum[2];		// numbers of the vertexes of this edge
} aasEdge_t;

// area boundary face
typedef struct aasFace_s {
	unsigned short				planeNum;			// number of the plane this face is on
	unsigned short				flags;				// face flags
	int							numEdges;			// number of edges in the boundary of the face
	int							firstEdge;			// first edge in the edge index
	short						areas[2];			// area at the front and back of this face
} aasFace_t;

// area with a boundary of faces
typedef struct aasArea_s {
	int							numFaces;			// number of faces used for the boundary of the area
	int							firstFace;			// first face in the face index used for the boundary of the area
	hfBounds					bounds;				// bounds of the area
	hfVec3						center;				// center of the area an AI can move towards
	unsigned short				flags;				// several area flags
	unsigned short				contents;			// contents of the area
	short						cluster;			// cluster the area belongs to, if negative it's a portal
	short						clusterAreaNum;		// number of the area in the cluster
	int							travelFlags;		// travel flags for traveling through this area
	hfReachability *			reach;				// reachabilities that start from this area
	hfReachability *			rev_reach;			// reachabilities that lead to this area
} aasArea_t;

// nodes of the bsp tree
typedef struct aasNode_s {
	unsigned short				planeNum;			// number of the plane that splits the subspace at this node
	int							children[2];		// child nodes, zero is solid, negative is -(area number)
} aasNode_t;

// cluster portal
typedef struct aasPortal_s {
	short						areaNum;			// number of the area that is the actual portal
	short						clusters[2];		// number of cluster at the front and back of the portal
	short						clusterAreaNum[2];	// number of this portal area in the front and back cluster
	unsigned short				maxAreaTravelTime;	// maximum travel time through the portal area
} aasPortal_t;

// cluster
typedef struct aasCluster_s {
	int							numAreas;			// number of areas in the cluster
	int							numReachableAreas;	// number of areas with reachabilities
	int							numPortals;			// number of cluster portals
	int							firstPortal;		// first cluster portal in the index
} aasCluster_t;

// trace through the world
typedef struct aasTrace_s {
								// parameters
	int							flags;				// areas with these flags block the trace
	int							travelFlags;		// areas with these travel flags block the trace
	int							maxAreas;			// size of the 'areas' array
	int							getOutOfSolid;		// trace out of solid if the trace starts in solid
								// output
	float						fraction;			// fraction of trace completed
	hfVec3						endpos;				// end position of trace
	int							planeNum;			// plane hit
	int							lastAreaNum;		// number of last area the trace went through
	int							blockingAreaNum;	// area that could not be entered
	int							numAreas;			// number of areas the trace went through
	int *						areas;				// array to store areas the trace went through
	hfVec3 *					points;				// points where the trace entered each new area
								aasTrace_s() { areas = NULL; points = NULL; getOutOfSolid = false; flags = travelFlags = maxAreas = 0; }
} aasTrace_t;

// settings
class hfAASSettings {
public:
								// collision settings
	int							numBoundingBoxes;
	hfBounds					boundingBoxes[MAX_AAS_BOUNDING_BOXES];
	bool						usePatches;
	bool						writeBrushMap;
	bool						playerFlood;
	bool						noOptimize;
	bool						allowSwimReachabilities;
	bool						allowFlyReachabilities;
	hfStr						fileExtension;
								// physics settings
	hfVec3						gravity;
	hfVec3						gravityDir;
	hfVec3						invGravityDir;
	float						gravityValue;
	float						maxStepHeight;
	float						maxBarrierHeight;
	float						maxWaterJumpHeight;
	float						maxFallHeight;
	float						minFloorCos;
								// fixed travel times
	int							tt_barrierJump;
	int							tt_startCrouching;
	int							tt_waterJump;
	int							tt_startWalkOffLedge;

public:
								hfAASSettings();

	bool						FromFile( const hfStr &fileName );
	bool						FromParser( hfLexer &src );
	bool						FromDict( const char *name, const hfDict *dict );
	bool						WriteToFile( hfFile *fp ) const;
	bool						ValidForBounds( const hfBounds &bounds ) const;
	bool						ValidEntity( const char *classname ) const;

private:
	bool						ParseBool( hfLexer &src, bool &b );
	bool						ParseInt( hfLexer &src, int &i );
	bool						ParseFloat( hfLexer &src, float &f );
	bool						ParseVector( hfLexer &src, hfVec3 &vec );
	bool						ParseBBoxes( hfLexer &src );
};


/*

-	when a node child is a solid leaf the node child number is zero
-	two adjacent areas (sharing a plane at opposite sides) share a face
	this face is a portal between the areas
-	when an area uses a face from the faceindex with a positive index
	then the face plane normal points into the area
-	the face edges are stored counter clockwise using the edgeindex
-	two adjacent convex areas (sharing a face) only share One face
	this is a simple result of the areas being convex
-	the areas can't have a mixture of ground and gap faces
	other mixtures of faces in one area are allowed
-	areas with the AREACONTENTS_CLUSTERPORTAL in the settings have
	the cluster number set to the negative portal number
-	edge zero is a dummy
-	face zero is a dummy
-	area zero is a dummy
-	node zero is a dummy
-	portal zero is a dummy
-	cluster zero is a dummy

*/


class hfAASFile {
public:
	virtual 					~hfAASFile() {}

	const char *				GetName() const { return name.c_str(); }
	unsigned int				GetCRC() const { return crc; }

	int							GetNumPlanes() const { return planeList.Num(); }
	const hfPlane &				GetPlane( int index ) const { return planeList[index]; }
	int							GetNumVertices() const { return vertices.Num(); }
	const aasVertex_t &			GetVertex( int index ) const { return vertices[index]; }
	int							GetNumEdges() const { return edges.Num(); }
	const aasEdge_t &			GetEdge( int index ) const { return edges[index]; }
	int							GetNumEdgeIndexes() const { return edgeIndex.Num(); }
	const aasIndex_t &			GetEdgeIndex( int index ) const { return edgeIndex[index]; }
	int							GetNumFaces() const { return faces.Num(); }
	const aasFace_t &			GetFace( int index ) const { return faces[index]; }
	int							GetNumFaceIndexes() const { return faceIndex.Num(); }
	const aasIndex_t &			GetFaceIndex( int index ) const { return faceIndex[index]; }
	int							GetNumAreas() const { return areas.Num(); }
	const aasArea_t &			GetArea( int index ) { return areas[index]; }
	int							GetNumNodes() const { return nodes.Num(); }
	const aasNode_t &			GetNode( int index ) const { return nodes[index]; }
	int							GetNumPortals() const { return portals.Num(); }
	const aasPortal_t &			GetPortal( int index ) { return portals[index]; }
	int							GetNumPortalIndexes() const { return portalIndex.Num(); }
	const aasIndex_t &			GetPortalIndex( int index ) const { return portalIndex[index]; }
	int							GetNumClusters() const { return clusters.Num(); }
	const aasCluster_t &		GetCluster( int index ) const { return clusters[index]; }

	const hfAASSettings &		GetSettings() const { return settings; }

	void						SetPortalMaxTravelTime( int index, int time ) { portals[index].maxAreaTravelTime = time; }
	void						SetAreaTravelFlag( int index, int flag ) { areas[index].travelFlags |= flag; }
	void						RemoveAreaTravelFlag( int index, int flag ) { areas[index].travelFlags &= ~flag; }

	virtual hfVec3				EdgeCenter( int edgeNum ) const = 0;
	virtual hfVec3				FaceCenter( int faceNum ) const = 0;
	virtual hfVec3				AreaCenter( int areaNum ) const = 0;

	virtual hfBounds			EdgeBounds( int edgeNum ) const = 0;
	virtual hfBounds			FaceBounds( int faceNum ) const = 0;
	virtual hfBounds			AreaBounds( int areaNum ) const = 0;

	virtual int					PointAreaNum( const hfVec3 &origin ) const = 0;
	virtual int					PointReachableAreaNum( const hfVec3 &origin, const hfBounds &searchBounds, const int areaFlags, const int excludeTravelFlags ) const = 0;
	virtual int					BoundsReachableAreaNum( const hfBounds &bounds, const int areaFlags, const int excludeTravelFlags ) const = 0;
	virtual void				PushPointIntoAreaNum( int areaNum, hfVec3 &point ) const = 0;
	virtual bool				Trace( aasTrace_t &trace, const hfVec3 &start, const hfVec3 &end ) const = 0;
	virtual void				PrintInfo() const = 0;

protected:
	hfStr						name;
	unsigned int				crc;

	hfPlaneSet					planeList;
	hfList<aasVertex_t, TAG_AAS>			vertices;
	hfList<aasEdge_t, TAG_AAS>			edges;
	hfList<aasIndex_t, TAG_AAS>			edgeIndex;
	hfList<aasFace_t, TAG_AAS>			faces;
	hfList<aasIndex_t, TAG_AAS>			faceIndex;
	hfList<aasArea_t, TAG_AAS>			areas;
	hfList<aasNode_t, TAG_AAS>			nodes;
	hfList<aasPortal_t, TAG_AAS>			portals;
	hfList<aasIndex_t, TAG_AAS>			portalIndex;
	hfList<aasCluster_t, TAG_AAS>		clusters;
	hfAASSettings				settings;
};

#endif /* !__AASFILE_H__ */

