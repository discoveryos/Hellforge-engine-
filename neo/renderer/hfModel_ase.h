#ifndef __MODEL_ASE_H__
#define __MODEL_ASE_H__

/*
===============================================================================

	ASE loader. (3D Studio Max ASCII Export)

===============================================================================
*/

typedef struct {
	int						vertexNum[3];
	int						tVertexNum[3];
	hfVec3					faceNormal;
	hfVec3					vertexNormals[3];
	byte					vertexColors[3][4];
} aseFace_t;

typedef struct {
	int						timeValue;

	int						numVertexes;
	int						numTVertexes;
	int						numCVertexes;
	int						numFaces;
	int						numTVFaces;
	int						numCVFaces;

	hfVec3					transform[4];			// applied to normals

	bool					colorsParsed;
	bool					normalsParsed;
	hfVec3 *				vertexes;
	hfVec2 *				tvertexes;
	hfVec3 *				cvertexes;
	aseFace_t *				faces;
} aseMesh_t;

typedef struct {
	char					name[128];
	float					uOffset, vOffset;		// max lets you offset by material without changing texCoords
	float					uTiling, vTiling;		// multiply tex coords by this
	float					angle;					// in clockwise radians
} aseMaterial_t;

typedef struct {
	char					name[128];
	int						materialRef;

	aseMesh_t				mesh;

	// frames are only present with animations
	hfList<aseMesh_t*, TAG_MODEL>		frames;			// aseMesh_t
} aseObject_t;

typedef struct aseModel_s {
	HF_TIME_T					timeStamp;
	hfList<aseMaterial_t *, TAG_MODEL>	materials;
	hfList<aseObject_t *, TAG_MODEL>	objects;
} aseModel_t;


aseModel_t *ASE_Load( const char *fileName );
void		ASE_Free( aseModel_t *ase );

#endif /* !__MODEL_ASE_H__ */

