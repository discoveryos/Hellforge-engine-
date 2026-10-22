#ifndef __MODEL_LOCAL_H__
#define __MODEL_LOCAL_H__

/*
===============================================================================

	Static model

===============================================================================
*/

class hfJointMat;
struct deformInfo_t;

class hfRenderModelStatic : public hfRenderModel {
public:
	// the inherited public interface
	static hfRenderModel *		Alloc();

								hfRenderModelStatic();
	virtual						~hfRenderModelStatic();

	virtual void				InitFromFile( const char *fileName );
	virtual bool				LoadBinaryModel( hfFile * file, const HF_TIME_T sourceTimeStamp );
	virtual void				WriteBinaryModel( hfFile * file, HF_TIME_T *_timeStamp = NULL ) const;
	virtual bool				SupportsBinaryModel() { return true; }

	virtual void				PartialInitFromFile( const char *fileName );
	virtual void				PurgeModel();
	virtual void				Reset() {};
	virtual void				LoadModel();
	virtual bool				IsLoaded();
	virtual void				SetLevelLoadReferenced( bool referenced );
	virtual bool				IsLevelLoadReferenced();
	virtual void				TouchData();
	virtual void				InitEmpty( const char *name );
	virtual void				AddSurface( modelSurface_t surface );
	virtual void				FinishSurfaces();
	virtual void				FreeVertexCache();
	virtual const char *		Name() const;
	virtual void				Print() const;
	virtual void				List() const;
	virtual int					Memory() const;
	virtual HF_TIME_T				Timestamp() const;
	virtual int					NumSurfaces() const;
	virtual int					NumBaseSurfaces() const;
	virtual const modelSurface_t *Surface( int surfaceNum ) const;
	virtual srfTriangles_t *	AllocSurfaceTriangles( int numVerts, int numIndexes ) const;
	virtual void				FreeSurfaceTriangles( srfTriangles_t *tris ) const;
	virtual bool				IsStaticWorldModel() const;
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual bool				IsDefaultModel() const;
	virtual bool				IsReloadable() const;
	virtual hfRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const viewDef_t *view, hfRenderModel *cachedModel );
	virtual int					NumJoints() const;
	virtual const hfMD5Joint *	GetJoints() const;
	virtual jointHandle_t		GetJointHandle( const char *name ) const;
	virtual const char *		GetJointName( jointHandle_t handle ) const;
	virtual const hfJointQuat *	GetDefaultPose() const;
	virtual int					NearestJoint( int surfaceNum, int a, int b, int c ) const;
	virtual hfBounds			Bounds( const struct renderEntity_s *ent ) const;
	virtual void				ReadFromDemoFile( class hfDemoFile *f );
	virtual void				WriteToDemoFile( class hfDemoFile *f );
	virtual float				DepthHack() const;

	virtual bool				ModelHasDrawingSurfaces() const { return hasDrawingSurfaces; };
 	virtual bool				ModelHasInteractingSurfaces() const { return hasInteractingSurfaces; };
	virtual bool				ModelHasShadowCastingSurfaces() const { return hasShadowCastingSurfaces; };

	void						MakeDefaultModel();
	
	bool						LoadASE( const char *fileName );
	bool						LoadLWO( const char *fileName );
	bool						LoadMA( const char *filename );

	bool						ConvertASEToModelSurfaces( const struct aseModel_s *ase );
	bool						ConvertLWOToModelSurfaces( const struct st_lwObject *lwo );
	bool						ConvertMAToModelSurfaces (const struct maModel_s *ma );

	struct aseModel_s *			ConvertLWOToASE( const struct st_lwObject *obj, const char *fileName );

	bool						DeleteSurfaceWithId( int id );
	void						DeleteSurfacesWithNegativeId();
	bool						FindSurfaceWithId( int id, int &surfaceNum ) const;

public:
	hfList<modelSurface_t, TAG_MODEL>	surfaces;
	hfBounds					bounds;
	int							overlaysAdded;

	// when an md5 is instantiated, the inverted joints array is stored to allow GPU skinning
	int							numInvertedJoints;
	hfJointMat *				jointsInverted;
	vertCacheHandle_t			jointsInvertedBuffer;

protected:
	int							lastModifiedFrame;
	int							lastArchivedFrame;

	hfStr						name;
	bool						isStaticWorldModel;
	bool						defaulted;
	bool						purged;					// eventually we will have dynamic reloading
	bool						fastLoad;				// don't generate tangents and shadow data
	bool						reloadable;				// if not, reloadModels won't check timestamp
	bool						levelLoadReferenced;	// for determining if it needs to be freed
	bool						hasDrawingSurfaces;
	bool						hasInteractingSurfaces;
	bool						hasShadowCastingSurfaces;
	HF_TIME_T					timeStamp;

	static hfCVar				r_mergeModelSurfaces;	// combine model surfaces with the same material
	static hfCVar				r_slopVertex;			// merge xyz coordinates this far apart
	static hfCVar				r_slopTexCoord;			// merge texture coordinates this far apart
	static hfCVar				r_slopNormal;			// merge normals that dot less than this
};

/*
===============================================================================

	MD5 animated model

===============================================================================
*/

class hfMD5Mesh {
	friend class				hfRenderModelMD5;

public:
								hfMD5Mesh();
								~hfMD5Mesh();

 	void						ParseMesh( hfLexer &parser, int numJoints, const hfJointMat *joints );

	int							NumVerts() const { return numVerts; }
	int							NumTris() const { return numTris; }

	void						UpdateSurface( const struct renderEntity_s *ent, const hfJointMat *joints,
												const hfJointMat *entJointsInverted, modelSurface_t *surf );
	void						CalculateBounds( const hfJointMat * entJoints, hfBounds & bounds ) const;
	int							NearestJoint( int a, int b, int c ) const;

private:
	const hfMaterial *			shader;				// material applied to mesh
	int							numVerts;			// number of vertices
	int							numTris;			// number of triangles
	byte *						meshJoints;			// the joints used by this mesh
	int							numMeshJoints;		// number of mesh joints
	float						maxJointVertDist;	// maximum distance a vertex is separated from a joint
	deformInfo_t *				deformInfo;			// used to create srfTriangles_t from base frames and new vertexes
	int							surfaceNum;			// number of the static surface created for this mesh
};

class hfRenderModelMD5 : public hfRenderModelStatic {
public:
	virtual void				InitFromFile( const char *fileName );
	virtual bool				LoadBinaryModel( hfFile * file, const HF_TIME_T sourceTimeStamp );
	virtual void				WriteBinaryModel( hfFile * file, HF_TIME_T *_timeStamp = NULL ) const;
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual hfBounds			Bounds( const struct renderEntity_s *ent ) const;
	virtual void				Print() const;
	virtual void				List() const;
	virtual void				TouchData();
	virtual void				PurgeModel();
	virtual void				LoadModel();
	virtual int					Memory() const;
	virtual hfRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const viewDef_t *view, hfRenderModel *cachedModel );
	virtual int					NumJoints() const;
	virtual const hfMD5Joint *	GetJoints() const;
	virtual jointHandle_t		GetJointHandle( const char *name ) const;
	virtual const char *		GetJointName( jointHandle_t handle ) const;
	virtual const hfJointQuat *	GetDefaultPose() const;
	virtual int					NearestJoint( int surfaceNum, int a, int b, int c ) const;

	virtual bool				SupportsBinaryModel() { return true; }

private:
	hfList<hfMD5Joint, TAG_MODEL>	joints;
	hfList<hfJointQuat, TAG_MODEL>	defaultPose;
	hfList<hfJointMat, TAG_MODEL>	invertedDefaultPose;
	hfList<hfMD5Mesh, TAG_MODEL>	meshes;

	void						DrawJoints( const renderEntity_t *ent, const viewDef_t *view ) const;
	void						ParseJoint( hfLexer &parser, hfMD5Joint *joint, hfJointQuat *defaultPose );
};

/*
===============================================================================

	MD3 animated model

===============================================================================
*/

struct md3Header_s;
struct md3Surface_s;

class hfRenderModelMD3 : public hfRenderModelStatic {
public:
	virtual void				InitFromFile( const char *fileName );
	virtual bool				SupportsBinaryModel() { return false; }
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual hfRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const viewDef_t *view, hfRenderModel *cachedModel );
	virtual hfBounds			Bounds( const struct renderEntity_s *ent ) const;

private:
	int							index;			// model = tr.models[model->index]
	int							dataSize;		// just for listing purposes
	struct md3Header_s *		md3;			// only if type == MOD_MESH
	int							numLods;

	void						LerpMeshVertexes( srfTriangles_t *tri, const struct md3Surface_s *surf, const float backlerp, const int frame, const int oldframe ) const;
};

/*
===============================================================================

	Liquid model

===============================================================================
*/

class hfRenderModelLiquid : public hfRenderModelStatic {
public:
								hfRenderModelLiquid();

	virtual void				InitFromFile( const char *fileName );
	virtual bool				SupportsBinaryModel() { return false; }
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual hfRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const viewDef_t *view, hfRenderModel *cachedModel );
	virtual hfBounds			Bounds( const struct renderEntity_s *ent ) const;

	virtual void				Reset();
	void						IntersectBounds( const hfBounds &bounds, float displacement );

private:
	modelSurface_t				GenerateSurface( float lerp );
	void						WaterDrop( int x, int y, float *page );
	void						Update();
						
	int							verts_x;
	int							verts_y;
	float						scale_x;
	float						scale_y;
	int							time;
	int							liquid_type;
	int							update_tics;
	int							seed;

	hfRandom					random;
						
	const hfMaterial *			shader;
	deformInfo_t *				deformInfo;		// used to create srfTriangles_t from base frames
											// and new vertexes
						
	float						density;
	float						drop_height;
	int							drop_radius;
	float						drop_delay;

	hfList<float, TAG_MODEL>	pages;
	float *						page1;
	float *						page2;

	hfList<hfDrawVert, TAG_MODEL>	verts;

	int							nextDropTime;

};

/*
===============================================================================

	PRT model

===============================================================================
*/

class hfRenderModelPrt : public hfRenderModelStatic {
public:
								hfRenderModelPrt();

	virtual void				InitFromFile( const char *fileName );
	virtual bool				SupportsBinaryModel() { return false; }
	virtual void				TouchData();
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual hfRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const viewDef_t *view, hfRenderModel *cachedModel );
	virtual hfBounds			Bounds( const struct renderEntity_s *ent ) const;
	virtual float				DepthHack() const;
	virtual int					Memory() const;

	// with the addModels2 arrangement we could have light accepting and
	// shadowing dynamic models, but the original game never did
	virtual bool				ModelHasDrawingSurfaces() const { return true; };
	virtual bool				ModelHasInteractingSurfaces() const { return false; };
	virtual bool				ModelHasShadowCastingSurfaces() const { return false; };

private:
	const hfDeclParticle *		particleSystem;
};

/*
===============================================================================

	Beam model

===============================================================================
*/

class hfRenderModelBeam : public hfRenderModelStatic {
public:
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual bool				SupportsBinaryModel() { return false; }
	virtual bool				IsLoaded() const;
	virtual hfRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const viewDef_t *view, hfRenderModel *cachedModel );
	virtual hfBounds			Bounds( const struct renderEntity_s *ent ) const;

	// with the addModels2 arrangement we could have light accepting and
	// shadowing dynamic models, but the original game never did
	virtual bool				ModelHasDrawingSurfaces() const { return true; };
	virtual bool				ModelHasInteractingSurfaces() const { return false; };
	virtual bool				ModelHasShadowCastingSurfaces() const { return false; };
};

/*
===============================================================================

	Beam model

===============================================================================
*/
#define MAX_TRAIL_PTS	20

struct Trail_t {
	int							lastUpdateTime;
	int							duration;

	hfVec3						pts[MAX_TRAIL_PTS];
	int							numPoints;
};

class hfRenderModelTrail : public hfRenderModelStatic {
	hfList<Trail_t, TAG_MODEL>	trails;
	int							numActive;
	hfBounds					trailBounds;

public:
								hfRenderModelTrail();

	virtual dynamicModel_t		IsDynamicModel() const;
	virtual bool				SupportsBinaryModel() { return false; }
	virtual bool				IsLoaded() const;
	virtual hfRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const viewDef_t *view, hfRenderModel *cachedModel );
	virtual hfBounds			Bounds( const struct renderEntity_s *ent ) const;

	// with the addModels2 arrangement we could have light accepting and
	// shadowing dynamic models, but the original game never did
	virtual bool				ModelHasDrawingSurfaces() const { return true; };
	virtual bool				ModelHasInteractingSurfaces() const { return false; };
	virtual bool				ModelHasShadowCastingSurfaces() const { return false; };

	int							NewTrail( hfVec3 pt, int duration );
	void						UpdateTrail( int index, hfVec3 pt );
	void						DrawTrail( int index, const struct renderEntity_s *ent, srfTriangles_t *tri, float globalAlpha );
};

/*
===============================================================================

	Lightning model

===============================================================================
*/

class hfRenderModelLightning : public hfRenderModelStatic {
public:
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual bool				SupportsBinaryModel() { return false; }
	virtual bool				IsLoaded() const;
	virtual hfRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const viewDef_t *view, hfRenderModel *cachedModel );
	virtual hfBounds			Bounds( const struct renderEntity_s *ent ) const;

	// with the addModels2 arrangement we could have light accepting and
	// shadowing dynamic models, but the original game never did
	virtual bool				ModelHasDrawingSurfaces() const { return true; };
	virtual bool				ModelHasInteractingSurfaces() const { return false; };
	virtual bool				ModelHasShadowCastingSurfaces() const { return false; };
};

/*
================================================================================

	hfRenderModelSprite 

================================================================================
*/
class hfRenderModelSprite : public hfRenderModelStatic {
public:
	virtual	dynamicModel_t		IsDynamicModel() const;
	virtual bool				SupportsBinaryModel() { return false; }
	virtual	bool				IsLoaded() const;
	virtual	hfRenderModel *		InstantiateDynamicModel( const struct renderEntity_s *ent, const viewDef_t *view, hfRenderModel *cachedModel );
	virtual	hfBounds			Bounds( const struct renderEntity_s *ent ) const;

	// with the addModels2 arrangement we could have light accepting and
	// shadowing dynamic models, but the original game never did
	virtual bool				ModelHasDrawingSurfaces() const { return true; };
	virtual bool				ModelHasInteractingSurfaces() const { return false; };
	virtual bool				ModelHasShadowCastingSurfaces() const { return false; };
};

#endif /* !__MODEL_LOCAL_H__ */

