#pragma hdrstop
#include "../hflib/hfprecompiled.h"
#include "hfFont.h"

const char * DEFAULT_FONT = "Arial_Narrow";

static const float old_scale2 = 0.6f;
static const float old_scale1 = 0.3f;

/*
==============================
Old_SelectValueForScale
==============================
*/
HF_INLINE float Old_SelectValueForScale( float scale, float v0, float v1, float v2 ) {
	return ( scale >= old_scale2 ) ? v2 : ( scale >= old_scale1 ) ? v1 : v0;
}

/*
==============================
hfFont::RemapFont
==============================
*/
hfFont * hfFont::RemapFont( const char * baseName ) {
	hfStr cleanName = baseName;

	if ( cleanName == DEFAULT_FONT ) {
		return NULL;
	}

	const char * remapped = hfLocalization::FindString( "#font_" + cleanName );
	if ( remapped != NULL ) {
		return renderSystem->RegisterFont( remapped );
	}

	const char * wildcard = hfLocalization::FindString( "#font_*" );
	if ( wildcard != NULL && cleanName.Icmp( wildcard ) != 0 ) {
		return renderSystem->RegisterFont( wildcard );
	}

	// Note single | so both sides are always executed
	if ( cleanName.ReplaceChar( ' ', '_' ) | cleanName.ReplaceChar( '-', '_' ) ) {
		return renderSystem->RegisterFont( cleanName );
	}

	return NULL;
}

/*
==============================
hfFont::~hfFont
==============================
*/
hfFont::~hfFont() {
	delete fontInfo;
}

/*
==============================
hfFont::hfFont
==============================
*/
hfFont::hfFont( const char * n ) : name( n ) {
	fontInfo = NULL;
	alias = RemapFont( n );

	if ( alias != NULL ) {
		// Make sure we don't have a circular reference
		for ( hfFont * f = alias; f != NULL; f = f->alias ) {
			if ( f == this ) {
				hfLib::FatalError( "Font alias \"%s\" is a circular reference!", n );
			}
		}
		return;
	}

	if ( !LoadFont() ) {
		if ( name.Icmp( DEFAULT_FONT ) == 0 ) {
			hfLib::FatalError( "Could not load default font \"%s\"", DEFAULT_FONT );
		} else {
			hfLib::Warning( "Could not load font %s", n );
			alias = renderSystem->RegisterFont( DEFAULT_FONT );
		}
	}
}

struct oldGlyphInfo_t {
	int					height;			// number of scan lines
	int					top;			// top of glyph in buffer
	int					bottom;			// bottom of glyph in buffer
	int					pitch;			// width for copying
	int					xSkip;			// x adjustment
	int					imageWidth;		// width of actual image
	int					imageHeight;	// height of actual image
	float				s;				// x offset in image where glyph starts
	float				t;				// y offset in image where glyph starts
	float				s2;
	float				t2;
	int					junk;
	char				materialName[32];
};
static const int GLYPHS_PER_FONT = 256;

/*
==============================
LoadOldGlyphData
==============================
*/
bool LoadOldGlyphData( const char * filename, oldGlyphInfo_t glyphInfo[GLYPHS_PER_FONT] ) {
	hfFile * fd = fileSystem->OpenFileRead( filename );
	if ( fd == NULL ) {
		return false;
	}
	fd->Read( glyphInfo, GLYPHS_PER_FONT * sizeof( oldGlyphInfo_t ) );
	for ( int i = 0; i < GLYPHS_PER_FONT; i++ ) {
		hfSwap::Little( glyphInfo[i].height );
		hfSwap::Little( glyphInfo[i].top );
		hfSwap::Little( glyphInfo[i].bottom );
		hfSwap::Little( glyphInfo[i].pitch );
		hfSwap::Little( glyphInfo[i].xSkip );
		hfSwap::Little( glyphInfo[i].imageWidth );
		hfSwap::Little( glyphInfo[i].imageHeight );
		hfSwap::Little( glyphInfo[i].s );
		hfSwap::Little( glyphInfo[i].t );
		hfSwap::Little( glyphInfo[i].s2 );
		hfSwap::Little( glyphInfo[i].t2 );
		assert( glyphInfo[i].imageWidth == glyphInfo[i].pitch );
		assert( glyphInfo[i].imageHeight == glyphInfo[i].height );
		assert( glyphInfo[i].imageWidth == ( glyphInfo[i].s2 - glyphInfo[i].s ) * 256 );
		assert( glyphInfo[i].imageHeight == ( glyphInfo[i].t2 - glyphInfo[i].t ) * 256 );
		assert( glyphInfo[i].junk == 0 );
	}
	delete fd;
	return true;
}

/*
==============================
hfFont::LoadFont
==============================
*/
bool hfFont::LoadFont() {
	hfStr fontName = va( "newfonts/%s/48.dat", GetName() );
	hfFile * fd = fileSystem->OpenFileRead( fontName );
	if ( fd == NULL ) {
		return false;
	}

	const int FONT_INFO_VERSION = 42;
	const int FONT_INFO_MAGIC = ( FONT_INFO_VERSION | ( 'i' << 24 ) | ( 'd' << 16 ) | ( 'f' << 8 ) );

	uint32 version = 0;
	fd->ReadBig( version );
	if ( version != FONT_INFO_MAGIC ) {
		hfLib::Warning( "Wrong version in %s", GetName() );
		delete fd;
		return false;
	}

	fontInfo = new (TAG_FONT) fontInfo_t;

	short pointSize = 0;

	fd->ReadBig( pointSize );
	assert( pointSize == 48 );

	fd->ReadBig( fontInfo->ascender );
	fd->ReadBig( fontInfo->descender );

	fd->ReadBig( fontInfo->numGlyphs );

	fontInfo->glyphData = (glyphInfo_t *)Mem_Alloc( sizeof( glyphInfo_t ) * fontInfo->numGlyphs, TAG_FONT );
	fontInfo->charIndex = (uint32 *)Mem_Alloc( sizeof( uint32 ) * fontInfo->numGlyphs, TAG_FONT );

	fd->Read( fontInfo->glyphData, fontInfo->numGlyphs * sizeof( glyphInfo_t ) );

	for( int i = 0; i < fontInfo->numGlyphs; i++ ) {
		hfSwap::Little( fontInfo->glyphData[i].width );
		hfSwap::Little( fontInfo->glyphData[i].height );
		hfSwap::Little( fontInfo->glyphData[i].top );
		hfSwap::Little( fontInfo->glyphData[i].left );
		hfSwap::Little( fontInfo->glyphData[i].xSkip );
		hfSwap::Little( fontInfo->glyphData[i].s );
		hfSwap::Little( fontInfo->glyphData[i].t );
	}

	fd->Read( fontInfo->charIndex, fontInfo->numGlyphs * sizeof( uint32 ) );
	hfSwap::LittleArray( fontInfo->charIndex, fontInfo->numGlyphs );

	memset( fontInfo->ascii, -1, sizeof( fontInfo->ascii ) );
	for ( int i = 0; i < fontInfo->numGlyphs; i++ ) {
		if ( fontInfo->charIndex[i] < 128 ) {
			fontInfo->ascii[fontInfo->charIndex[i]] = i;
		} else {
			// Since the characters are sorted, as soon as we find a non-ascii character, we can stop
			break;
		}
	}

	hfStr fontTextureName = fontName;
	fontTextureName.SetFileExtension( "tga" );

	fontInfo->material = declManager->FindMaterial( fontTextureName );
	fontInfo->material->SetSort( SS_GUI );

	// Load the old glyph data because we want our new fonts to fit in the old glyph metrics
	int pointSizes[3] = { 12, 24, 48 };
	float scales[3] = { 4.0f, 2.0f, 1.0f };
	for ( int i = 0; i < 3; i++ ) {
		oldGlyphInfo_t oldGlyphInfo[GLYPHS_PER_FONT];
		const char * oldFileName = va( "newfonts/%s/old_%d.dat", GetName(), pointSizes[i] );
		if ( LoadOldGlyphData( oldFileName, oldGlyphInfo ) ) {
			int mh = 0;
			int mw = 0;
			for ( int g = 0; g < GLYPHS_PER_FONT; g++ ) {
				if ( mh < oldGlyphInfo[g].height ) {
					mh = oldGlyphInfo[g].height;
				}
				if ( mw < oldGlyphInfo[g].xSkip ) {
					mw = oldGlyphInfo[g].xSkip;
				}
			}
			fontInfo->oldInfo[i].maxWidth = scales[i] * mw;
			fontInfo->oldInfo[i].maxHeight = scales[i] * mh;
		} else {
			int mh = 0;
			int mw = 0;
			for( int g = 0; g < fontInfo->numGlyphs; g++ ) {
				if ( mh < fontInfo->glyphData[g].height ) {
					mh = fontInfo->glyphData[g].height;
				}
				if ( mw < fontInfo->glyphData[g].xSkip ) {
					mw = fontInfo->glyphData[g].xSkip;
				}
			}
			fontInfo->oldInfo[i].maxWidth = mw;
			fontInfo->oldInfo[i].maxHeight = mh;
		}
	}
	delete fd;
	return true;
}

/*
==============================
hfFont::GetGlyphIndex
==============================
*/
int	hfFont::GetGlyphIndex( uint32 idx ) const {
	if ( idx < 128 ) {
		return fontInfo->ascii[idx];
	}
	if ( fontInfo->numGlyphs == 0 ) {
		return -1;
	}
	if ( fontInfo->charIndex == NULL ) {
		return idx;
	}
	int len = fontInfo->numGlyphs;
	int mid = fontInfo->numGlyphs;
	int offset = 0;
	while ( mid > 0 ) {
		mid = len >> 1;
		if ( fontInfo->charIndex[offset+mid] <= idx ) {
			offset += mid;
		}
		len -= mid;
	}
	return ( fontInfo->charIndex[offset] == idx ) ? offset : -1;
}

/*
==============================
hfFont::GetLineHeight
==============================
*/
float hfFont::GetLineHeight( float scale ) const {
	if ( alias != NULL ) {
		return alias->GetLineHeight( scale );
	}
	if ( fontInfo != NULL ) {
		return scale * Old_SelectValueForScale( scale, fontInfo->oldInfo[0].maxHeight, fontInfo->oldInfo[1].maxHeight, fontInfo->oldInfo[2].maxHeight );
	}
	return 0.0f;
}

/*
==============================
hfFont::GetAscender
==============================
*/
float hfFont::GetAscender( float scale ) const {
	if ( alias != NULL ) {
		return alias->GetAscender( scale );
	}
	if ( fontInfo != NULL ) {
		return scale * fontInfo->ascender;
	}
	return 0.0f;
}

/*
==============================
hfFont::GetMaxCharWidth
==============================
*/
float hfFont::GetMaxCharWidth( float scale ) const {
	if ( alias != NULL ) {
		return alias->GetMaxCharWidth( scale );
	}
	if ( fontInfo != NULL ) {
		return scale * Old_SelectValueForScale( scale, fontInfo->oldInfo[0].maxWidth, fontInfo->oldInfo[1].maxWidth, fontInfo->oldInfo[2].maxWidth );
	}
	return 0.0f;
}

/*
==============================
hfFont::GetGlyphWidth
==============================
*/
float hfFont::GetGlyphWidth( float scale, uint32 idx ) const {
	if ( alias != NULL ) {
		return alias->GetGlyphWidth( scale, idx );
	}
	if ( fontInfo != NULL ) {
		int i = GetGlyphIndex( idx );
		const int asterisk = 42;
		if ( i == -1 && idx != asterisk ) {
			i = GetGlyphIndex( asterisk );
		}
		if ( i >= 0 ) {
			return scale * fontInfo->glyphData[i].xSkip;
		}
	}
	return 0.0f;
}

/*
==============================
hfFont::GetScaledGlyph
==============================
*/
void hfFont::GetScaledGlyph( float scale, uint32 idx, scaledGlyphInfo_t & glyphInfo ) const {
	if ( alias != NULL ) {
		return alias->GetScaledGlyph( scale, idx, glyphInfo );
	}
	if ( fontInfo != NULL ) {
		int i = GetGlyphIndex( idx );
		const int asterisk = 42;
		if ( i == -1 && idx != asterisk ) {
			i = GetGlyphIndex( asterisk );
		}
		if ( i >= 0 ) {
			float invMaterialWidth = 1.0f / fontInfo->material->GetImageWidth();
			float invMaterialHeight = 1.0f / fontInfo->material->GetImageHeight();
			glyphInfo_t & gi = fontInfo->glyphData[i];
			glyphInfo.xSkip = scale * gi.xSkip;
			glyphInfo.top = scale * gi.top;
			glyphInfo.left = scale * gi.left;
			glyphInfo.width = scale * gi.width;
			glyphInfo.height = scale * gi.height;
			glyphInfo.s1 = ( gi.s - 0.5f ) * invMaterialWidth;
			glyphInfo.t1 = ( gi.t - 0.5f ) * invMaterialHeight;
			glyphInfo.s2 = ( gi.s + gi.width + 0.5f ) * invMaterialWidth;
			glyphInfo.t2 = ( gi.t + gi.height + 0.5f ) * invMaterialHeight;
			glyphInfo.material = fontInfo->material;
			return;
		}
	}
	memset( &glyphInfo, 0, sizeof( glyphInfo ) );
}

/*
==============================
hfFont::Touch
==============================
*/
void hfFont::Touch() {
	if ( alias != NULL ) {
		alias->Touch();
	}
	if ( fontInfo != NULL ) {
		const_cast<hfMaterial *>( fontInfo->material )->EnsureNotPurged();
		fontInfo->material->SetSort( SS_GUI );
	}
}
