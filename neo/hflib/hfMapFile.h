#ifndef __MAPFILE_H__
#define __MAPFILE_H__

/*
===============================================================================

	Reads or writes the contents of .map files into a standard internal
	format, which can then be moved into private formats for collision
	detection, map processing, or editor use.

	No validation (duplicate planes, null area brushes, etc) is performed.
	There are no limits to the number of any of the elements in maps.
	The order of entities, brushes, and sides is maintained.

===============================================================================
*/

const int OLD_MAP_VERSION					= 1;
const int CURRENT_MAP_VERSION				= 2;
const int DEFAULT_CURVE_SUBDIVISION			= 4;
const float DEFAULT_CURVE_MAX_ERROR			= 4.0f;
const float DEFAULT_CURVE_MAX_ERROR_CD		= 24.0f;
const float DEFAULT_CURVE_MAX_LENGTH		= -1.0f;
const float DEFAULT_CURVE_MAX_LENGTH_CD		= -1.0f;


class hfMapPrimitive {
public:
	enum { TYPE_INVALID = -1, TYPE_BRUSH, TYPE_PATCH };

	hfDict					epairs;

							hfMapPrimitive() { type = TYPE_INVALID; }
	virtual					~hfMapPrimitive() { }
	int						GetType() const { return type; }

protected:
	int						type;
};


class hfMapBrushSide {
	friend class hfMapBrush;

public:
							hfMapBrushSide();
							~hfMapBrushSide() { }
	const char *			GetMaterial() const { return material; }
	void					SetMaterial( const char *p ) { material = p; }
	const hfPlane &			GetPlane() const { return plane; }
	void					SetPlane( const hfPlane &p ) { plane = p; }
	void					SetTextureMatrix( const hfVec3 mat[2] ) { texMat[0] = mat[0]; texMat[1] = mat[1]; }
	void					GetTextureMatrix( hfVec3 &mat1, hfVec3 &mat2 ) { mat1 = texMat[0]; mat2 = texMat[1]; }
	void					GetTextureVectors( hfVec4 v[2] ) const;

protected:
	hfStr					material;
	hfPlane					plane;
	hfVec3					texMat[2];
	hfVec3					origin;
};

HF_INLINE hfMapBrushSide::hfMapBrushSide() {
	plane.Zero();
	texMat[0].Zero();
	texMat[1].Zero();
	origin.Zero();
}


class hfMapBrush : public hfMapPrimitive {
public:
							hfMapBrush() { type = TYPE_BRUSH; sides.Resize( 8, 4 ); }
							~hfMapBrush() { sides.DeleteContents( true ); }
	static hfMapBrush *		Parse( hfLexer &src, const hfVec3 &origin, bool newFormat = true, float version = CURRENT_MAP_VERSION );
	static hfMapBrush *		ParseQ3( hfLexer &src, const hfVec3 &origin );
	bool					Write( hfFile *fp, int primitiveNum, const hfVec3 &origin ) const;
	int						GetNumSides() const { return sides.Num(); }
	int						AddSide( hfMapBrushSide *side ) { return sides.Append( side ); }
	hfMapBrushSide *		GetSide( int i ) const { return sides[i]; }
	unsigned int			GetGeometryCRC() const;

protected:
	int						numSides;
	hfList<hfMapBrushSide*, TAG_IDLIB_LIST_MAP> sides;
};


class hfMapPatch : public hfMapPrimitive, public hfSurface_Patch {
public:
							hfMapPatch();
							hfMapPatch( int maxPatchWidth, int maxPatchHeight );
							~hfMapPatch() { }
	static hfMapPatch *		Parse( hfLexer &src, const hfVec3 &origin, bool patchDef3 = true, float version = CURRENT_MAP_VERSION );
	bool					Write( hfFile *fp, int primitiveNum, const hfVec3 &origin ) const;
	const char *			GetMaterial() const { return material; }
	void					SetMaterial( const char *p ) { material = p; }
	int						GetHorzSubdivisions() const { return horzSubdivisions; }
	int						GetVertSubdivisions() const { return vertSubdivisions; }
	bool					GetExplicitlySubdivided() const { return explicitSubdivisions; }
	void					SetHorzSubdivisions( int n ) { horzSubdivisions = n; }
	void					SetVertSubdivisions( int n ) { vertSubdivisions = n; }
	void					SetExplicitlySubdivided( bool b ) { explicitSubdivisions = b; }
	unsigned int			GetGeometryCRC() const;

protected:
	hfStr					material;
	int						horzSubdivisions;
	int						vertSubdivisions;
	bool					explicitSubdivisions;
};

HF_INLINE hfMapPatch::hfMapPatch() {
	type = TYPE_PATCH;
	horzSubdivisions = vertSubdivisions = 0;
	explicitSubdivisions = false;
	width = height = 0;
	maxWidth = maxHeight = 0;
	expanded = false;
}

HF_INLINE hfMapPatch::hfMapPatch( int maxPatchWidth, int maxPatchHeight ) {
	type = TYPE_PATCH;
	horzSubdivisions = vertSubdivisions = 0;
	explicitSubdivisions = false;
	width = height = 0;
	maxWidth = maxPatchWidth;
	maxHeight = maxPatchHeight;
	verts.SetNum( maxWidth * maxHeight );
	expanded = false;
}


class hfMapEntity {
	friend class			hfMapFile;

public:
	hfDict					epairs;

public:
							hfMapEntity() { epairs.SetHashSize( 64 ); }
							~hfMapEntity() { primitives.DeleteContents( true ); }
	static hfMapEntity *	Parse( hfLexer &src, bool worldSpawn = false, float version = CURRENT_MAP_VERSION );
	bool					Write( hfFile *fp, int entityNum ) const;
	int						GetNumPrimitives() const { return primitives.Num(); }
	hfMapPrimitive *		GetPrimitive( int i ) const { return primitives[i]; }
	void					AddPrimitive( hfMapPrimitive *p ) { primitives.Append( p ); }
	unsigned int			GetGeometryCRC() const;
	void					RemovePrimitiveData();

protected:
	hfList<hfMapPrimitive*, TAG_IDLIB_LIST_MAP>	primitives;
};


class hfMapFile {
public:
							hfMapFile();
							~hfMapFile() { entities.DeleteContents( true ); }

							// filename does not require an extension
							// normally this will use a .reg file instead of a .map file if it exists,
							// which is what the game and dmap want, but the editor will want to always
							// load a .map file
	bool					Parse( const char *filename, bool ignoreRegion = false, bool osPath = false );
	bool					Write( const char *fileName, const char *ext, bool fromBasePath = true );
							// get the number of entities in the map
	int						GetNumEntities() const { return entities.Num(); }
							// get the specified entity
	hfMapEntity *			GetEntity( int i ) const { return entities[i]; }
							// get the name without file extension
	const char *			GetName() const { return name; }
							// get the file time
	HF_TIME_T					GetFileTime() const { return fileTime; }
							// get CRC for the map geometry
							// texture coordinates and entity key/value pairs are not taken into account
	unsigned int			GetGeometryCRC() const { return geometryCRC; }
							// returns true if the file on disk changed
	bool					NeedsReload();

	int						AddEntity( hfMapEntity *mapentity );
	hfMapEntity *			FindEntity( const char *name );
	void					RemoveEntity( hfMapEntity *mapEnt );
	void					RemoveEntities( const char *classname );
	void					RemoveAllEntities();
	void					RemovePrimitiveData();
	bool					HasPrimitiveData() { return hasPrimitiveData; }

protected:
	float					version;
	HF_TIME_T					fileTime;
	unsigned int			geometryCRC;
	hfList<hfMapEntity *, TAG_IDLIB_LIST_MAP>	entities;
	hfStr					name;
	bool					hasPrimitiveData;

private:
	void					SetGeometryCRC();
};

HF_INLINE hfMapFile::hfMapFile() {
	version = CURRENT_MAP_VERSION;
	fileTime = 0;
	geometryCRC = 0;
	entities.Resize( 1024, 256 );
	hasPrimitiveData = false;
}

#endif /* !__MAPFILE_H__ */

