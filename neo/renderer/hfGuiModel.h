struct guiModelSurface_t {
	const hfMaterial *	material;
	uint64				glState;
	int					firstIndex;
	int					numIndexes;
	stereoDepthType_t		stereoType;
};

class hfRenderMatrix;

class hfGuiModel {
public:
	hfGuiModel();

	void	Clear();

	void	WriteToDemo( hfDemoFile * demo );
	void	ReadFromDemo( hfDemoFile * demo );
	
	// allocates memory for verts and indexes in frame-temporary buffer memory
	void	BeginFrame();

	void	EmitToCurrentView( float modelMatrix[16], bool depthHack );
	void	EmitFullScreen();

	// the returned pointer will be in write-combined memory, so only make contiguous
	// 32 bit writes and never read from it.
	hfDrawVert * AllocTris( int numVerts, const triIndex_t * indexes, int numIndexes, const hfMaterial * material, 
							const uint64 glState, const stereoDepthType_t stereoType );

	//---------------------------
private:
	void	AdvanceSurf();
	void	EmitSurfaces( float modelMatrix[16], float modelViewMatrix[16], 
		bool depthHack, bool allowFullScreenStereoDepth, bool linkAsEntity );

	guiModelSurface_t *			surf;

	float						shaderParms[ MAX_ENTITY_SHADER_PARMS ];

	static const float STEREO_DEPTH_NEAR;
	static const float STEREO_DEPTH_MID;
	static const float STEREO_DEPTH_FAR;

	// if we exceed these limits we stop rendering GUI surfaces
	static const int MAX_INDEXES = ( 20000 * 6 );
	static const int MAX_VERTS	 = ( 20000 * 4 );

	vertCacheHandle_t			vertexBlock;
	vertCacheHandle_t			indexBlock;
	hfDrawVert *				vertexPointer;
	triIndex_t *				indexPointer;

	int		numVerts;
	int		numIndexes;

	hfList<guiModelSurface_t, TAG_MODEL>	surfaces;
};


