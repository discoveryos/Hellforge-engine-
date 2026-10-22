#ifndef __RENDERWORLDLOCAL_H__
#define __RENDERWORLDLOCAL_H__

#include "hfBoundsTrack.h"

// assume any lightDef or entityDef index above this is an internal error
const int LUDICROUS_INDEX	= 10000;


typedef struct portal_s {
	int						intoArea;		// area this portal leads to
	hfWinding *				w;				// winding points have counter clockwise ordering seen this area
	hfPlane					plane;			// view must be on the positive side of the plane to cross
	struct portal_s *		next;			// next portal of the area
	struct doublePortal_s *	doublePortal;
} portal_t;


typedef struct doublePortal_s {
	struct portal_s	*		portals[2];
	int						blockingBits;	// PS_BLOCK_VIEW, PS_BLOCK_AIR, etc, set by doors that shut them off

	// A portal will be considered closed if it is past the
	// fog-out point in a fog volume.  We only support a single
	// fog volume over each portal.
	hfRenderLightLocal *		fogLight;
	struct doublePortal_s *	nextFoggedPortal;
} doublePortal_t;


typedef struct portalArea_s {
	int				areaNum;
	int				connectedAreaNum[NUM_PORTAL_ATTRIBUTES];	// if two areas have matching connectedAreaNum, they are
									// not separated by a portal with the apropriate PS_BLOCK_* blockingBits
	int				viewCount;		// set by R_FindViewLightsAndEntities
	portal_t *		portals;		// never changes after load
	areaReference_t	entityRefs;		// head/tail of doubly linked list, may change
	areaReference_t	lightRefs;		// head/tail of doubly linked list, may change
} portalArea_t;


static const int	CHILDREN_HAVE_MULTIPLE_AREAS = -2;
static const int	AREANUM_SOLID = -1;
typedef struct {
	hfPlane			plane;
	int				children[2];		// negative numbers are (-1 - areaNumber), 0 = solid
	int				commonChildrenArea;	// if all children are either solid or a single area,
										// this is the area number, else CHILDREN_HAVE_MULTIPLE_AREAS
} areaNode_t;

struct reusableDecal_t {
	qhandle_t				entityHandle;
	int						lastStartTime;
	hfRenderModelDecal *	decals;
};

struct reusableOverlay_t {
	qhandle_t				entityHandle;
	int						lastStartTime;
	hfRenderModelOverlay *	overlays;
};

struct portalStack_t;

class hfRenderWorldLocal : public hfRenderWorld {
public:
							hfRenderWorldLocal();
	virtual					~hfRenderWorldLocal();

	virtual	bool			InitFromMap( const char *mapName );
	virtual void			ResetLocalRenderModels();				// Fixes a crash when switching expansion packs

	virtual	qhandle_t		AddEntityDef( const renderEntity_t *re );
	virtual	void			UpdateEntityDef( qhandle_t entityHandle, const renderEntity_t *re );
	virtual	void			FreeEntityDef( qhandle_t entityHandle );
	virtual const renderEntity_t *GetRenderEntity( qhandle_t entityHandle ) const;

	virtual	qhandle_t		AddLightDef( const renderLight_t *rlight );
	virtual	void			UpdateLightDef( qhandle_t lightHandle, const renderLight_t *rlight );
	virtual	void			FreeLightDef( qhandle_t lightHandle );
	virtual const renderLight_t *GetRenderLight( qhandle_t lightHandle ) const;

	virtual bool			CheckAreaForPortalSky( int areaNum );

	virtual	void			GenerateAllInteractions();
	virtual void			RegenerateWorld();

	virtual void			ProjectDecalOntoWorld( const hfFixedWinding &winding, const hfVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const hfMaterial *material, const int startTime );
	virtual void			ProjectDecal( qhandle_t entityHandle, const hfFixedWinding &winding, const hfVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const hfMaterial *material, const int startTime );
	virtual void			ProjectOverlay( qhandle_t entityHandle, const hfPlane localTextureAxis[2], const hfMaterial *material, const int startTime );
	virtual void			RemoveDecals( qhandle_t entityHandle );

	virtual void			SetRenderView( const renderView_t *renderView );
	virtual	void			RenderScene( const renderView_t *renderView );

	virtual	int				NumAreas() const;
	virtual int				PointInArea( const hfVec3 &point ) const;
	virtual int				BoundsInAreas( const hfBounds &bounds, int *areas, int maxAreas ) const;
	virtual	int				NumPortalsInArea( int areaNum );
	virtual exitPortal_t	GetPortal( int areaNum, int portalNum );

	virtual	guiPoint_t		GuiTrace( qhandle_t entityHandle, const hfVec3 start, const hfVec3 end ) const;
	virtual bool			ModelTrace( modelTrace_t &trace, qhandle_t entityHandle, const hfVec3 &start, const hfVec3 &end, const float radius ) const;
	virtual bool			Trace( modelTrace_t &trace, const hfVec3 &start, const hfVec3 &end, const float radius, bool skipDynamic = true, bool skipPlayer = false ) const;
	virtual bool			FastWorldTrace( modelTrace_t &trace, const hfVec3 &start, const hfVec3 &end ) const;

	virtual void			DebugClearLines( int time );
	virtual void			DebugLine( const hfVec4 &color, const hfVec3 &start, const hfVec3 &end, const int lifetime = 0, const bool depthTest = false );
	virtual void			DebugArrow( const hfVec4 &color, const hfVec3 &start, const hfVec3 &end, int size, const int lifetime = 0 );
	virtual void			DebugWinding( const hfVec4 &color, const hfWinding &w, const hfVec3 &origin, const hfMat3 &axis, const int lifetime = 0, const bool depthTest = false );
	virtual void			DebugCircle( const hfVec4 &color, const hfVec3 &origin, const hfVec3 &dir, const float radius, const int numSteps, const int lifetime = 0, const bool depthTest = false );
	virtual void			DebugSphere( const hfVec4 &color, const hfSphere &sphere, const int lifetime = 0, bool depthTest = false );
	virtual void			DebugBounds( const hfVec4 &color, const hfBounds &bounds, const hfVec3 &org = vec3_origin, const int lifetime = 0 );
	virtual void			DebugBox( const hfVec4 &color, const hfBox &box, const int lifetime = 0 );
	virtual void			DebugCone( const hfVec4 &color, const hfVec3 &apex, const hfVec3 &dir, float radius1, float radius2, const int lifetime = 0 );
	virtual void			DebugScreenRect( const hfVec4 &color, const hfScreenRect &rect, const viewDef_t *viewDef, const int lifetime = 0 );
	virtual void			DebugAxis( const hfVec3 &origin, const hfMat3 &axis );

	virtual void			DebugClearPolygons( int time );
	virtual void			DebugPolygon( const hfVec4 &color, const hfWinding &winding, const int lifeTime = 0, const bool depthTest = false );

	virtual void			DrawText( const char *text, const hfVec3 &origin, float scale, const hfVec4 &color, const hfMat3 &viewAxis, const int align = 1, const int lifetime = 0, bool depthTest = false );

	//-----------------------

	hfStr					mapName;				// ie: maps/tim_dm2.proc, written to demoFile
	HF_TIME_T				mapTimeStamp;			// for fast reloads of the same level

	areaNode_t *			areaNodes;
	int						numAreaNodes;

	portalArea_t *			portalAreas;
	int						numPortalAreas;
	int						connectedAreaNum;		// incremented every time a door portal state changes

	hfScreenRect *			areaScreenRect;

	doublePortal_t *		doublePortals;
	int						numInterAreaPortals;

	hfList<hfRenderModel *, TAG_MODEL>	localModels;

	hfList<hfRenderEntityLocal*, TAG_ENTITY>	entityDefs;
	hfList<hfRenderLightLocal*, TAG_LIGHT>		lightDefs;

	hfBlockAlloc<areaReference_t, 1024> areaReferenceAllocator;
	hfBlockAlloc<hfInteraction, 256>	interactionAllocator;

#ifdef HF_PC
	static const int MAX_DECAL_SURFACES = 32;
#else
	static const int MAX_DECAL_SURFACES = 16;
#endif
	hfArray<reusableDecal_t, MAX_DECAL_SURFACES>	decals;
	hfArray<reusableOverlay_t, MAX_DECAL_SURFACES>	overlays;

	// all light / entity interactions are referenced here for fast lookup without
	// having to crawl the doubly linked lists.  EnntityDefs are sequential for better
	// cache access, because the table is accessed by light in hfRenderWorldLocal::CreateLightDefInteractions()
	// Growing this table is time consuming, so we add a pad value to the number
	// of entityDefs and lightDefs
	hfInteraction **		interactionTable;
	int						interactionTableWidth;		// entityDefs
	int						interactionTableHeight;		// lightDefs

	bool					generateAllInteractionsCalled;

	//-----------------------
	// hfRenderWorld_load.cpp

	hfRenderModel *			ParseModel( hfLexer *src, const char *mapName, HF_TIME_T mapTimeStamp, hfFile *fileOut );
	hfRenderModel *			ParseShadowModel( hfLexer *src, hfFile *fileOut );
	void					SetupAreaRefs();
	void					ParseInterAreaPortals( hfLexer *src, hfFile *fileOut );
	void					ParseNodes( hfLexer *src, hfFile *fileOut );
	int						CommonChildrenArea_r( areaNode_t *node );
	void					FreeWorld();
	void					ClearWorld();
	void					FreeDefs();
	void					TouchWorldModels();
	void					AddWorldModelEntities();
	void					ClearPortalStates();
	void					ReadBinaryAreaPortals( hfFile *file );
	void					ReadBinaryNodes( hfFile *file );
	hfRenderModel *			ReadBinaryModel( hfFile *file );
	hfRenderModel *			ReadBinaryShadowModel( hfFile *file );

	//--------------------------
	// hfRenderWorld_portals.cpp

	bool					CullEntityByPortals( const hfRenderEntityLocal *entity, const portalStack_t *ps );
	void					AddAreaViewEntities( int areaNum, const portalStack_t *ps );
	bool					CullLightByPortals( const hfRenderLightLocal *light, const portalStack_t *ps );
	void					AddAreaViewLights( int areaNum, const portalStack_t *ps );
	void					AddAreaToView( int areaNum, const portalStack_t *ps );
	hfScreenRect			ScreenRectFromWinding( const hfWinding *w, const viewEntity_t *space );
	bool					PortalIsFoggedOut( const portal_t *p );
	void					FloodViewThroughArea_r( const hfVec3 & origin, int areaNum, const portalStack_t *ps );
	void					FlowViewThroughPortals( const hfVec3 & origin, int numPlanes, const hfPlane *planes );
	void					BuildConnectedAreas_r( int areaNum );
	void					BuildConnectedAreas();
	void					FindViewLightsAndEntities();

	void					FloodLightThroughArea_r( hfRenderLightLocal *light, int areaNum, const portalStack_t *ps );
	void					FlowLightThroughPortals( hfRenderLightLocal *light );

	int						NumPortals() const;
	qhandle_t				FindPortal( const hfBounds &b ) const;
	void					SetPortalState( qhandle_t portal, int blockingBits );
	int						GetPortalState( qhandle_t portal );
	bool					AreasAreConnected( int areaNum1, int areaNum2, portalConnection_t connection ) const;
	void					FloodConnectedAreas( portalArea_t *area, int portalAttributeIndex );
	hfScreenRect &			GetAreaScreenRect( int areaNum ) const { return areaScreenRect[areaNum]; }
	void					ShowPortals();

	//--------------------------
	// hfRenderWorld_demo.cpp

	void					StartWritingDemo( hfDemoFile *demo );
	void					StopWritingDemo();
	bool					ProcessDemoCommand( hfDemoFile *readDemo, renderView_t *demoRenderView, int *demoTimeOffset );

	void					WriteLoadMap();
	void					WriteRenderView( const renderView_t *renderView );
	void					WriteVisibleDefs( const viewDef_t *viewDef );
	void					WriteFreeLight( qhandle_t handle );
	void					WriteFreeEntity( qhandle_t handle );
	void					WriteRenderLight( qhandle_t handle, const renderLight_t *light );
	void					WriteRenderEntity( qhandle_t handle, const renderEntity_t *ent );
	void					ReadRenderEntity();
	void					ReadRenderLight();
	

	//--------------------------
	// hfRenderWorld.cpp

	void					ResizeInteractionTable();

	void					AddEntityRefToArea( hfRenderEntityLocal *def, portalArea_t *area );
	void					AddLightRefToArea( hfRenderLightLocal *light, portalArea_t *area );

	void					RecurseProcBSP_r( modelTrace_t *results, int parentNodeNum, int nodeNum, float p1f, float p2f, const hfVec3 &p1, const hfVec3 &p2 ) const;
	void					BoundsInAreas_r( int nodeNum, const hfBounds &bounds, int *areas, int *numAreas, int maxAreas ) const;

	float					DrawTextLength( const char *text, float scale, int len = 0 );

	void					FreeInteractions();

	void					PushFrustumIntoTree_r( hfRenderEntityLocal *def, hfRenderLightLocal *light, const frustumCorners_t & corners, int nodeNum );
	void					PushFrustumIntoTree( hfRenderEntityLocal *def, hfRenderLightLocal *light, const hfRenderMatrix & frustumTransform, const hfBounds & frustumBounds );

	hfRenderModelDecal *	AllocDecal( qhandle_t newEntityHandle, int startTime );
	hfRenderModelOverlay *	AllocOverlay( qhandle_t newEntityHandle, int startTime );

	//-------------------------------
	// tr_light.c
	void					CreateLightDefInteractions( hfRenderLightLocal * const ldef, const int renderViewID );
};

// if an entity / light combination has been evaluated and found to not genrate any surfaces or shadows,
// the constant INTERACTION_EMPTY will be stored in the interaction table, int contrasts to NULL, which
// means that the combination has not yet been tested for having surfaces.
static hfInteraction * const INTERACTION_EMPTY = (hfInteraction *)1;

void R_ListRenderLightDefs_f( const hfCmdArgs &args );
void R_ListRenderEntityDefs_f( const hfCmdArgs &args );

#endif /* !__RENDERWORLDLOCAL_H__ */

