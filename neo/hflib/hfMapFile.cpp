#include "hfprecompiled.h"
#pragma hdrstop


/*
===============
FloatCRC
===============
*/
HF_INLINE unsigned int FloatCRC( float f ) {
	return *(unsigned int *)&f;
}

/*
===============
StringCRC
===============
*/
HF_INLINE unsigned int StringCRC( const char *str ) {
	unsigned int i, crc;
	const unsigned char *ptr;

	crc = 0;
	ptr = reinterpret_cast<const unsigned char*>(str);
	for ( i = 0; str[i]; i++ ) {
		crc ^= str[i] << (i & 3);
	}
	return crc;
}

/*
=================
ComputeAxisBase

WARNING : special case behaviour of atan2(y,x) <-> atan(y/x) might not be the same everywhere when x == 0
rotation by (0,RotY,RotZ) assigns X to normal
=================
*/
static void ComputeAxisBase( const hfVec3 &normal, hfVec3 &texS, hfVec3 &texT ) {
	float RotY, RotZ;
	hfVec3 n;

	// do some cleaning
	n[0] = ( hfMath::Fabs( normal[0] ) < 1e-6f ) ? 0.0f : normal[0];
	n[1] = ( hfMath::Fabs( normal[1] ) < 1e-6f ) ? 0.0f : normal[1];
	n[2] = ( hfMath::Fabs( normal[2] ) < 1e-6f ) ? 0.0f : normal[2];

	RotY = -atan2( n[2], hfMath::Sqrt( n[1] * n[1] + n[0] * n[0]) );
	RotZ = atan2( n[1], n[0] );
	// rotate (0,1,0) and (0,0,1) to compute texS and texT
	texS[0] = -sin(RotZ);
	texS[1] = cos(RotZ);
	texS[2] = 0;
	// the texT vector is along -Z ( T texture coorinates axis )
	texT[0] = -sin(RotY) * cos(RotZ);
	texT[1] = -sin(RotY) * sin(RotZ);
	texT[2] = -cos(RotY);
}

/*
=================
hfMapBrushSide::GetTextureVectors
=================
*/
void hfMapBrushSide::GetTextureVectors( hfVec4 v[2] ) const {
	int i;
	hfVec3 texX, texY;

	ComputeAxisBase( plane.Normal(), texX, texY );
	for ( i = 0; i < 2; i++ ) {
		v[i][0] = texX[0] * texMat[i][0] + texY[0] * texMat[i][1];
		v[i][1] = texX[1] * texMat[i][0] + texY[1] * texMat[i][1];
		v[i][2] = texX[2] * texMat[i][0] + texY[2] * texMat[i][1];
		v[i][3] = texMat[i][2] + ( origin * v[i].ToVec3() );
	}
}

/*
=================
hfMapPatch::Parse
=================
*/
hfMapPatch *hfMapPatch::Parse( hfLexer &src, const hfVec3 &origin, bool patchDef3, float version ) {
	float		info[7];
	hfDrawVert *vert;
	hfToken		token;
	int			i, j;

	if ( !src.ExpectTokenString( "{" ) ) {
		return NULL;
	}

	// read the material (we had an implicit 'textures/' in the old format...)
	if ( !src.ReadToken( &token ) ) {
		src.Error( "hfMapPatch::Parse: unexpected EOF" );
		return NULL;
	}

	// Parse it
	if (patchDef3) {
		if ( !src.Parse1DMatrix( 7, info ) ) {
			src.Error( "hfMapPatch::Parse: unable to Parse patchDef3 info" );
			return NULL;
		}
	} else {
		if ( !src.Parse1DMatrix( 5, info ) ) {
			src.Error( "hfMapPatch::Parse: unable to parse patchDef2 info" );
			return NULL;
		}
	}

	hfMapPatch *patch = new (TAG_IDLIB) hfMapPatch( info[0], info[1] );

	patch->SetSize( info[0], info[1] );
	if ( version < 2.0f ) {
		patch->SetMaterial( "textures/" + token );
	} else {
		patch->SetMaterial( token );
	}

	if ( patchDef3 ) {
		patch->SetHorzSubdivisions( info[2] );
		patch->SetVertSubdivisions( info[3] );
		patch->SetExplicitlySubdivided( true );
	}

	if ( patch->GetWidth() < 0 || patch->GetHeight() < 0 ) {
		src.Error( "hfMapPatch::Parse: bad size" );
		delete patch;
		return NULL;
	}

	// these were written out in the wrong order, IMHO
	if ( !src.ExpectTokenString( "(" ) ) {
		src.Error( "hfMapPatch::Parse: bad patch vertex data" );
		delete patch;
		return NULL;
	}


	for ( j = 0; j < patch->GetWidth(); j++ ) {
		if ( !src.ExpectTokenString( "(" ) ) {
			src.Error( "hfMapPatch::Parse: bad vertex row data" );
			delete patch;
			return NULL;
		}
		for ( i = 0; i < patch->GetHeight(); i++ ) {
			float v[5];

			if ( !src.Parse1DMatrix( 5, v ) ) {
				src.Error( "hfMapPatch::Parse: bad vertex column data" );
				delete patch;
				return NULL;
			}

			vert = &((*patch)[i * patch->GetWidth() + j]);
			vert->xyz[0] = v[0] - origin[0];
			vert->xyz[1] = v[1] - origin[1];
			vert->xyz[2] = v[2] - origin[2];
			vert->SetTexCoord( v[3], v[4] );
		}
		if ( !src.ExpectTokenString( ")" ) ) {
			delete patch;
			src.Error( "hfMapPatch::Parse: unable to parse patch control points" );
			return NULL;
		}
	}

	if ( !src.ExpectTokenString( ")" ) ) {
		src.Error( "hfMapPatch::Parse: unable to parse patch control points, no closure" );
		delete patch;
		return NULL;
	}

	// read any key/value pairs
	while( src.ReadToken( &token ) ) {
		if ( token == "}" ) {
			src.ExpectTokenString( "}" );
			break;
		}
		if ( token.type == TT_STRING ) {
			hfStr key = token;
			src.ExpectTokenType( TT_STRING, 0, &token );
			patch->epairs.Set( key, token );
		}
	}

	return patch;
}

/*
============
hfMapPatch::Write
============
*/
bool hfMapPatch::Write( hfFile *fp, int primitiveNum, const hfVec3 &origin ) const {
	int i, j;
	const hfDrawVert *v;

	if ( GetExplicitlySubdivided() ) {
		fp->WriteFloatString( "// primitive %d\n{\n patchDef3\n {\n", primitiveNum );
		fp->WriteFloatString( "  \"%s\"\n  ( %d %d %d %d 0 0 0 )\n", GetMaterial(), GetWidth(), GetHeight(), GetHorzSubdivisions(), GetVertSubdivisions());
	} else {
		fp->WriteFloatString( "// primitive %d\n{\n patchDef2\n {\n", primitiveNum );
		fp->WriteFloatString( "  \"%s\"\n  ( %d %d 0 0 0 )\n", GetMaterial(), GetWidth(), GetHeight());
	}

	fp->WriteFloatString( "  (\n" );
	hfVec2 st;
	for ( i = 0; i < GetWidth(); i++ ) {
		fp->WriteFloatString( "   ( " );
		for ( j = 0; j < GetHeight(); j++ ) {
			v = &verts[ j * GetWidth() + i ];
			st = v->GetTexCoord();
			fp->WriteFloatString( " ( %f %f %f %f %f )", v->xyz[0] + origin[0],
								v->xyz[1] + origin[1], v->xyz[2] + origin[2], st[0], st[1] );
		}
		fp->WriteFloatString( " )\n" );
	}
	fp->WriteFloatString( "  )\n }\n}\n" );

	return true;
}

/*
===============
hfMapPatch::GetGeometryCRC
===============
*/
unsigned int hfMapPatch::GetGeometryCRC() const {
	int i, j;
	unsigned int crc;

	crc = GetHorzSubdivisions() ^ GetVertSubdivisions();
	for ( i = 0; i < GetWidth(); i++ ) {
		for ( j = 0; j < GetHeight(); j++ ) {
			crc ^= FloatCRC( verts[j * GetWidth() + i].xyz.x );
			crc ^= FloatCRC( verts[j * GetWidth() + i].xyz.y );
			crc ^= FloatCRC( verts[j * GetWidth() + i].xyz.z );
		}
	}

	crc ^= StringCRC( GetMaterial() );

	return crc;
}

/*
=================
hfMapBrush::Parse
=================
*/
hfMapBrush *hfMapBrush::Parse( hfLexer &src, const hfVec3 &origin, bool newFormat, float version ) {
	int i;
	hfVec3 planepts[3];
	hfToken token;
	hfList<hfMapBrushSide*> sides;
	hfMapBrushSide	*side;
	hfDict epairs;

	if ( !src.ExpectTokenString( "{" ) ) {
		return NULL;
	}

	do {
		if ( !src.ReadToken( &token ) ) {
			src.Error( "hfMapBrush::Parse: unexpected EOF" );
			sides.DeleteContents( true );
			return NULL;
		}
		if ( token == "}" ) {
			break;
		}

		// here we may have to jump over brush epairs ( only used in editor )
		do {
			// if token is a brace
			if ( token == "(" ) {
				break;
			}
			// the token should be a key string for a key/value pair
			if ( token.type != TT_STRING ) {
				src.Error( "hfMapBrush::Parse: unexpected %s, expected ( or epair key string", token.c_str() );
				sides.DeleteContents( true );
				return NULL;
			}

			hfStr key = token;

			if ( !src.ReadTokenOnLine( &token ) || token.type != TT_STRING ) {
				src.Error( "hfMapBrush::Parse: expected epair value string not found" );
				sides.DeleteContents( true );
				return NULL;
			}

			epairs.Set( key, token );

			// try to read the next key
			if ( !src.ReadToken( &token ) ) {
				src.Error( "hfMapBrush::Parse: unexpected EOF" );
				sides.DeleteContents( true );
				return NULL;
			}
		} while (1);

		src.UnreadToken( &token );

		side = new (TAG_IDLIB) hfMapBrushSide();
		sides.Append(side);

		if ( newFormat ) {
			if ( !src.Parse1DMatrix( 4, side->plane.ToFloatPtr() ) ) {
				src.Error( "hfMapBrush::Parse: unable to read brush side plane definition" );
				sides.DeleteContents( true );
				return NULL;
			}
		} else {
			// read the three point plane definition
			if (!src.Parse1DMatrix( 3, planepts[0].ToFloatPtr() ) ||
				!src.Parse1DMatrix( 3, planepts[1].ToFloatPtr() ) ||
				!src.Parse1DMatrix( 3, planepts[2].ToFloatPtr() ) ) {
				src.Error( "hfMapBrush::Parse: unable to read brush side plane definition" );
				sides.DeleteContents( true );
				return NULL;
			}

			planepts[0] -= origin;
			planepts[1] -= origin;
			planepts[2] -= origin;

			side->plane.FromPoints( planepts[0], planepts[1], planepts[2] );
		}

		// read the texture matrix
		// this is odd, because the texmat is 2D relative to default planar texture axis
		if ( !src.Parse2DMatrix( 2, 3, side->texMat[0].ToFloatPtr() ) ) {
			src.Error( "hfMapBrush::Parse: unable to read brush side texture matrix" );
			sides.DeleteContents( true );
			return NULL;
		}
		side->origin = origin;
		
		// read the material
		if ( !src.ReadTokenOnLine( &token ) ) {
			src.Error( "hfMapBrush::Parse: unable to read brush side material" );
			sides.DeleteContents( true );
			return NULL;
		}

		// we had an implicit 'textures/' in the old format...
		if ( version < 2.0f ) {
			side->material = "textures/" + token;
		} else {
			side->material = token;
		}

		// Q2 allowed override of default flags and values, but we don't any more
		if ( src.ReadTokenOnLine( &token ) ) {
			if ( src.ReadTokenOnLine( &token ) ) {
				if ( src.ReadTokenOnLine( &token ) ) {
				}
			}
		}
	} while( 1 );

	if ( !src.ExpectTokenString( "}" ) ) {
		sides.DeleteContents( true );
		return NULL;
	}

	hfMapBrush *brush = new (TAG_IDLIB) hfMapBrush();
	for ( i = 0; i < sides.Num(); i++ ) {
		brush->AddSide( sides[i] );
	}

	brush->epairs = epairs;

	return brush;
}

/*
=================
hfMapBrush::ParseQ3
=================
*/
hfMapBrush *hfMapBrush::ParseQ3( hfLexer &src, const hfVec3 &origin ) {
	int i, shift[2], rotate;
	float scale[2];
	hfVec3 planepts[3];
	hfToken token;
	hfList<hfMapBrushSide*> sides;
	hfMapBrushSide	*side;
	hfDict epairs;

	do {
		if ( src.CheckTokenString( "}" ) ) {
			break;
		}

		side = new (TAG_IDLIB) hfMapBrushSide();
		sides.Append( side );

		// read the three point plane definition
		if (!src.Parse1DMatrix( 3, planepts[0].ToFloatPtr() ) ||
			!src.Parse1DMatrix( 3, planepts[1].ToFloatPtr() ) ||
			!src.Parse1DMatrix( 3, planepts[2].ToFloatPtr() ) ) {
			src.Error( "hfMapBrush::ParseQ3: unable to read brush side plane definition" );
			sides.DeleteContents( true );
			return NULL;
		}

		planepts[0] -= origin;
		planepts[1] -= origin;
		planepts[2] -= origin;

		side->plane.FromPoints( planepts[0], planepts[1], planepts[2] );

		// read the material
		if ( !src.ReadTokenOnLine( &token ) ) {
			src.Error( "hfMapBrush::ParseQ3: unable to read brush side material" );
			sides.DeleteContents( true );
			return NULL;
		}

		// we have an implicit 'textures/' in the old format
		side->material = "textures/" + token;

		// read the texture shift, rotate and scale
		shift[0] = src.ParseInt();
		shift[1] = src.ParseInt();
		rotate = src.ParseInt();
		scale[0] = src.ParseFloat();
		scale[1] = src.ParseFloat();
		side->texMat[0] = hfVec3( 0.03125f, 0.0f, 0.0f );
		side->texMat[1] = hfVec3( 0.0f, 0.03125f, 0.0f );
		side->origin = origin;
		
		// Q2 allowed override of default flags and values, but we don't any more
		if ( src.ReadTokenOnLine( &token ) ) {
			if ( src.ReadTokenOnLine( &token ) ) {
				if ( src.ReadTokenOnLine( &token ) ) {
				}
			}
		}
	} while( 1 );

	hfMapBrush *brush = new (TAG_IDLIB) hfMapBrush();
	for ( i = 0; i < sides.Num(); i++ ) {
		brush->AddSide( sides[i] );
	}

	brush->epairs = epairs;

	return brush;
}

/*
============
hfMapBrush::Write
============
*/
bool hfMapBrush::Write( hfFile *fp, int primitiveNum, const hfVec3 &origin ) const {
	int i;
	hfMapBrushSide *side;

	fp->WriteFloatString( "// primitive %d\n{\n brushDef3\n {\n", primitiveNum );

	// write brush epairs
	for ( i = 0; i < epairs.GetNumKeyVals(); i++) {
		fp->WriteFloatString( "  \"%s\" \"%s\"\n", epairs.GetKeyVal(i)->GetKey().c_str(), epairs.GetKeyVal(i)->GetValue().c_str());
	}

	// write brush sides
	for ( i = 0; i < GetNumSides(); i++ ) {
		side = GetSide( i );
		fp->WriteFloatString( "  ( %f %f %f %f ) ", side->plane[0], side->plane[1], side->plane[2], side->plane[3] );
		fp->WriteFloatString( "( ( %f %f %f ) ( %f %f %f ) ) \"%s\" 0 0 0\n",
							side->texMat[0][0], side->texMat[0][1], side->texMat[0][2],
								side->texMat[1][0], side->texMat[1][1], side->texMat[1][2],
									side->material.c_str() );
	}

	fp->WriteFloatString( " }\n}\n" );

	return true;
}

/*
===============
hfMapBrush::GetGeometryCRC
===============
*/
unsigned int hfMapBrush::GetGeometryCRC() const {
	int i, j;
	hfMapBrushSide *mapSide;
	unsigned int crc;

	crc = 0;
	for ( i = 0; i < GetNumSides(); i++ ) {
		mapSide = GetSide(i);
		for ( j = 0; j < 4; j++ ) {
			crc ^= FloatCRC( mapSide->GetPlane()[j] );
		}
		crc ^= StringCRC( mapSide->GetMaterial() );
	}

	return crc;
}

/*
================
hfMapEntity::Parse
================
*/
hfMapEntity *hfMapEntity::Parse( hfLexer &src, bool worldSpawn, float version ) {
	hfToken	token;
	hfMapEntity *mapEnt;
	hfMapPatch *mapPatch;
	hfMapBrush *mapBrush;
	bool worldent;
	hfVec3 origin;
	double v1, v2, v3;

	if ( !src.ReadToken(&token) ) {
		return NULL;
	}

	if ( token != "{" ) {
		src.Error( "hfMapEntity::Parse: { not found, found %s", token.c_str() );
		return NULL;
	}

	mapEnt = new (TAG_IDLIB) hfMapEntity();

	if ( worldSpawn ) {
		mapEnt->primitives.Resize( 1024, 256 );
	}

	origin.Zero();
	worldent = false;
	do {
		if ( !src.ReadToken(&token) ) {
			src.Error( "hfMapEntity::Parse: EOF without closing brace" );
			return NULL;
		}
		if ( token == "}" ) {
			break;
		}

		if ( token == "{" ) {
			// parse a brush or patch
			if ( !src.ReadToken( &token ) ) {
				src.Error( "hfMapEntity::Parse: unexpected EOF" );
				return NULL;
			}

			if ( worldent ) {
				origin.Zero();
			}

			// if is it a brush: brush, brushDef, brushDef2, brushDef3
			if ( token.Icmpn( "brush", 5 ) == 0 ) {
				mapBrush = hfMapBrush::Parse( src, origin, ( !token.Icmp( "brushDef2" ) || !token.Icmp( "brushDef3" ) ), version );
				if ( !mapBrush ) {
					return NULL;
				}
				mapEnt->AddPrimitive( mapBrush );
			}
			// if is it a patch: patchDef2, patchDef3
			else if ( token.Icmpn( "patch", 5 ) == 0 ) {
				mapPatch = hfMapPatch::Parse( src, origin, !token.Icmp( "patchDef3" ), version );
				if ( !mapPatch ) {
					return NULL;
				}
				mapEnt->AddPrimitive( mapPatch );
			}
			// assume it's a brush in Q3 or older style
			else {
				src.UnreadToken( &token );
				mapBrush = hfMapBrush::ParseQ3( src, origin );
				if ( !mapBrush ) {
					return NULL;
				}
				mapEnt->AddPrimitive( mapBrush );
			}
		} else {
			hfStr key, value;

			// parse a key / value pair
			key = token;
			src.ReadTokenOnLine( &token );
			value = token;

			// strip trailing spaces that sometimes get accidentally
			// added in the editor
			value.StripTrailingWhitespace();
			key.StripTrailingWhitespace();

			mapEnt->epairs.Set( key, value );

			if ( !hfStr::Icmp( key, "origin" ) ) {
				// scanf into doubles, then assign, so it is hfVec size independent
				v1 = v2 = v3 = 0;
				sscanf( value, "%lf %lf %lf", &v1, &v2, &v3 );
				origin.x = v1;
				origin.y = v2;
				origin.z = v3;
			}
			else if ( !hfStr::Icmp( key, "classname" ) && !hfStr::Icmp( value, "worldspawn" ) ) {
				worldent = true;
			}
		}
	} while( 1 );

	return mapEnt;
}

/*
============
hfMapEntity::Write
============
*/
bool hfMapEntity::Write( hfFile *fp, int entityNum ) const {
	int i;
	hfMapPrimitive *mapPrim;
	hfVec3 origin;

	fp->WriteFloatString( "// entity %d\n{\n", entityNum );

	// write entity epairs
	for ( i = 0; i < epairs.GetNumKeyVals(); i++) {
		fp->WriteFloatString( "\"%s\" \"%s\"\n", epairs.GetKeyVal(i)->GetKey().c_str(), epairs.GetKeyVal(i)->GetValue().c_str());
	}

	epairs.GetVector( "origin", "0 0 0", origin );

	// write pritimives
	for ( i = 0; i < GetNumPrimitives(); i++ ) {
		mapPrim = GetPrimitive( i );

		switch( mapPrim->GetType() ) {
			case hfMapPrimitive::TYPE_BRUSH:
				static_cast<hfMapBrush*>(mapPrim)->Write( fp, i, origin );
				break;
			case hfMapPrimitive::TYPE_PATCH:
				static_cast<hfMapPatch*>(mapPrim)->Write( fp, i, origin );
				break;
		}
	}

	fp->WriteFloatString( "}\n" );

	return true;
}

/*
===============
hfMapEntity::RemovePrimitiveData
===============
*/
void hfMapEntity::RemovePrimitiveData() {
	primitives.DeleteContents(true);
}

/*
===============
hfMapEntity::GetGeometryCRC
===============
*/
unsigned int hfMapEntity::GetGeometryCRC() const {
	int i;
	unsigned int crc;
	hfMapPrimitive	*mapPrim;

	crc = 0;
	for ( i = 0; i < GetNumPrimitives(); i++ ) {
		mapPrim = GetPrimitive( i );

		switch( mapPrim->GetType() ) {
			case hfMapPrimitive::TYPE_BRUSH:
				crc ^= static_cast<hfMapBrush*>(mapPrim)->GetGeometryCRC();
				break;
			case hfMapPrimitive::TYPE_PATCH:
				crc ^= static_cast<hfMapPatch*>(mapPrim)->GetGeometryCRC();
				break;
		}
	}

	return crc;
}

/*
===============
hfMapFile::Parse
===============
*/
bool hfMapFile::Parse( const char *filename, bool ignoreRegion, bool osPath ) {
	// no string concatenation for epairs and allow path names for materials
	hfLexer src( LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES );
	hfToken token;
	hfStr fullName;
	hfMapEntity *mapEnt;
	int i, j, k;

	name = filename;
	name.StripFileExtension();
	fullName = name;
	hasPrimitiveData = false;

	if ( !ignoreRegion ) {
		// try loading a .reg file first
		fullName.SetFileExtension( "reg" );
		src.LoadFile( fullName, osPath );
	}

	if ( !src.IsLoaded() ) {
		// now try a .map file
		fullName.SetFileExtension( "map" );
		src.LoadFile( fullName, osPath );
		if ( !src.IsLoaded() ) {
			// didn't get anything at all
			return false;
		}
	}

	version = OLD_MAP_VERSION;
	fileTime = src.GetFileTime();
	entities.DeleteContents( true );

	if ( src.CheckTokenString( "Version" ) ) {
		src.ReadTokenOnLine( &token );
		version = token.GetFloatValue();
	}

	while( 1 ) {
		mapEnt = hfMapEntity::Parse( src, ( entities.Num() == 0 ), version );
		if ( !mapEnt ) {
			break;
		}
		entities.Append( mapEnt );
	}

	SetGeometryCRC();

	// if the map has a worldspawn
	if ( entities.Num() ) {

		// "removeEntities" "classname" can be set in the worldspawn to remove all entities with the given classname
		const hfKeyValue *removeEntities = entities[0]->epairs.MatchPrefix( "removeEntities", NULL );
		while ( removeEntities ) {
			RemoveEntities( removeEntities->GetValue() );
			removeEntities = entities[0]->epairs.MatchPrefix( "removeEntities", removeEntities );
		}

		// "overrideMaterial" "material" can be set in the worldspawn to reset all materials
		hfStr material;
		if ( entities[0]->epairs.GetString( "overrideMaterial", "", material ) ) {
			for ( i = 0; i < entities.Num(); i++ ) {
				mapEnt = entities[i];
				for ( j = 0; j < mapEnt->GetNumPrimitives(); j++ ) {
					hfMapPrimitive *mapPrimitive = mapEnt->GetPrimitive( j );
					switch( mapPrimitive->GetType() ) {
						case hfMapPrimitive::TYPE_BRUSH: {
							hfMapBrush *mapBrush = static_cast<hfMapBrush *>(mapPrimitive);
							for ( k = 0; k < mapBrush->GetNumSides(); k++ ) {
								mapBrush->GetSide( k )->SetMaterial( material );
							}
							break;
						}
						case hfMapPrimitive::TYPE_PATCH: {
							static_cast<hfMapPatch *>(mapPrimitive)->SetMaterial( material );
							break;
						}
					}
				}
			}
		}

		// force all entities to have a name key/value pair
		if ( entities[0]->epairs.GetBool( "forceEntityNames" ) ) {
			for ( i = 1; i < entities.Num(); i++ ) {
				mapEnt = entities[i];
				if ( !mapEnt->epairs.FindKey( "name" ) ) {
					mapEnt->epairs.Set( "name", va( "%s%d", mapEnt->epairs.GetString( "classname", "forcedName" ), i ) );
				}
			}
		}

		// move the primitives of any func_group entities to the worldspawn
		if ( entities[0]->epairs.GetBool( "moveFuncGroups" ) ) {
			for ( i = 1; i < entities.Num(); i++ ) {
				mapEnt = entities[i];
				if ( hfStr::Icmp( mapEnt->epairs.GetString( "classname" ), "func_group" ) == 0 ) {
					entities[0]->primitives.Append( mapEnt->primitives );
					mapEnt->primitives.Clear();
				}
			}
		}
	}

	hasPrimitiveData = true;
	return true;
}

/*
============
hfMapFile::Write
============
*/
bool hfMapFile::Write( const char *fileName, const char *ext, bool fromBasePath ) {
	int i;
	hfStr qpath;
	hfFile *fp;

	qpath = fileName;
	qpath.SetFileExtension( ext );

	hfLib::common->Printf( "writing %s...\n", qpath.c_str() );

	if ( fromBasePath ) {
		fp = hfLib::fileSystem->OpenFileWrite( qpath, "fs_basepath" );
	}
	else {
		fp = hfLib::fileSystem->OpenExplicitFileWrite( qpath );
	}

	if ( !fp ) {
		hfLib::common->Warning( "Couldn't open %s\n", qpath.c_str() );
		return false;
	}

	fp->WriteFloatString( "Version %f\n", (float) CURRENT_MAP_VERSION );

	for ( i = 0; i < entities.Num(); i++ ) {
		entities[i]->Write( fp, i );
	}

	hfLib::fileSystem->CloseFile( fp );

	return true;
}

/*
===============
hfMapFile::SetGeometryCRC
===============
*/
void hfMapFile::SetGeometryCRC() {
	int i;

	geometryCRC = 0;
	for ( i = 0; i < entities.Num(); i++ ) {
		geometryCRC ^= entities[i]->GetGeometryCRC();
	}
}

/*
===============
hfMapFile::AddEntity
===============
*/
int hfMapFile::AddEntity( hfMapEntity *mapEnt ) {
	int ret = entities.Append( mapEnt );
	return ret;
}

/*
===============
hfMapFile::FindEntity
===============
*/
hfMapEntity *hfMapFile::FindEntity( const char *name ) {
	for ( int i = 0; i < entities.Num(); i++ ) {
		hfMapEntity *ent = entities[i];
		if ( hfStr::Icmp( ent->epairs.GetString( "name" ), name ) == 0 ) {
			return ent;
		}
	}
	return NULL;
}

/*
===============
hfMapFile::RemoveEntity
===============
*/
void hfMapFile::RemoveEntity( hfMapEntity *mapEnt ) {
	entities.Remove( mapEnt );
	delete mapEnt;
}

/*
===============
hfMapFile::RemoveEntity
===============
*/
void hfMapFile::RemoveEntities( const char *classname ) {
	for ( int i = 0; i < entities.Num(); i++ ) {
		hfMapEntity *ent = entities[i];
		if ( hfStr::Icmp( ent->epairs.GetString( "classname" ), classname ) == 0 ) {
			delete entities[i];
			entities.RemoveIndex( i );
			i--;
		}
	}
}

/*
===============
hfMapFile::RemoveAllEntities
===============
*/
void hfMapFile::RemoveAllEntities() {
	entities.DeleteContents( true );
	hasPrimitiveData = false;
}

/*
===============
hfMapFile::RemovePrimitiveData
===============
*/
void hfMapFile::RemovePrimitiveData() {
	for ( int i = 0; i < entities.Num(); i++ ) {
		hfMapEntity *ent = entities[i];
		ent->RemovePrimitiveData();
	}
	hasPrimitiveData = false;
}

/*
===============
hfMapFile::NeedsReload
===============
*/
bool hfMapFile::NeedsReload() {
	if ( name.Length() ) {
		HF_TIME_T time = FILE_NOT_FOUND_TIMESTAMP;
		if ( hfLib::fileSystem->ReadFile( name, NULL, &time ) > 0 ) {
			return ( time > fileTime );
		}
	}
	return true;
}

