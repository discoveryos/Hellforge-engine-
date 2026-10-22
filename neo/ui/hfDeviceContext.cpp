#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfDeviceContext.h"
#include "../renderer/hfGuiModel.h"

extern hfCVar in_useJoystick;

// bypass rendersystem to directly work on guiModel
extern hfGuiModel * tr_guiModel;

hfVec4 hfDeviceContext::colorPurple;
hfVec4 hfDeviceContext::colorOrange;
hfVec4 hfDeviceContext::colorYellow;
hfVec4 hfDeviceContext::colorGreen;
hfVec4 hfDeviceContext::colorBlue;
hfVec4 hfDeviceContext::colorRed;
hfVec4 hfDeviceContext::colorBlack;
hfVec4 hfDeviceContext::colorWhite;
hfVec4 hfDeviceContext::colorNone;

void hfDeviceContext::Init() {
	xScale = 1.0f;
	yScale = 1.0f;
	xOffset = 0.0f;
	yOffset = 0.0f;
	whiteImage = declManager->FindMaterial("guis/assets/white.tga");
	whiteImage->SetSort( SS_GUI );
	activeFont = renderSystem->RegisterFont( "" );
	colorPurple = hfVec4(1, 0, 1, 1);
	colorOrange = hfVec4(1, 1, 0, 1);
	colorYellow = hfVec4(0, 1, 1, 1);
	colorGreen = hfVec4(0, 1, 0, 1);
	colorBlue = hfVec4(0, 0, 1, 1);
	colorRed = hfVec4(1, 0, 0, 1);
	colorWhite = hfVec4(1, 1, 1, 1);
	colorBlack = hfVec4(0, 0, 0, 1);
	colorNone = hfVec4(0, 0, 0, 0);
	cursorImages[CURSOR_ARROW] = declManager->FindMaterial("ui/assets/guicursor_arrow.tga");	
	cursorImages[CURSOR_HAND] = declManager->FindMaterial("ui/assets/guicursor_hand.tga");
	cursorImages[CURSOR_HAND_JOY1] = declManager->FindMaterial("ui/assets/guicursor_hand_cross.tga");
	cursorImages[CURSOR_HAND_JOY2] = declManager->FindMaterial("ui/assets/guicursor_hand_circle.tga");
	cursorImages[CURSOR_HAND_JOY3] = declManager->FindMaterial("ui/assets/guicursor_hand_square.tga");
	cursorImages[CURSOR_HAND_JOY4] = declManager->FindMaterial("ui/assets/guicursor_hand_triangle.tga");
	cursorImages[CURSOR_HAND_JOY1] = declManager->FindMaterial("ui/assets/guicursor_hand_a.tga");
	cursorImages[CURSOR_HAND_JOY2] = declManager->FindMaterial("ui/assets/guicursor_hand_b.tga");
	cursorImages[CURSOR_HAND_JOY3] = declManager->FindMaterial("ui/assets/guicursor_hand_x.tga");
	cursorImages[CURSOR_HAND_JOY4] = declManager->FindMaterial("ui/assets/guicursor_hand_y.tga");
	scrollBarImages[SCROLLBAR_HBACK] = declManager->FindMaterial("ui/assets/scrollbarh.tga");
	scrollBarImages[SCROLLBAR_VBACK] = declManager->FindMaterial("ui/assets/scrollbarv.tga");
	scrollBarImages[SCROLLBAR_THUMB] = declManager->FindMaterial("ui/assets/scrollbar_thumb.tga");
	scrollBarImages[SCROLLBAR_RIGHT] = declManager->FindMaterial("ui/assets/scrollbar_right.tga");
	scrollBarImages[SCROLLBAR_LEFT] = declManager->FindMaterial("ui/assets/scrollbar_left.tga");
	scrollBarImages[SCROLLBAR_UP] = declManager->FindMaterial("ui/assets/scrollbar_up.tga");
	scrollBarImages[SCROLLBAR_DOWN] = declManager->FindMaterial("ui/assets/scrollbar_down.tga");
	cursorImages[CURSOR_ARROW]->SetSort( SS_GUI );
	cursorImages[CURSOR_HAND]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_HBACK]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_VBACK]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_THUMB]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_RIGHT]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_LEFT]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_UP]->SetSort( SS_GUI );
	scrollBarImages[SCROLLBAR_DOWN]->SetSort( SS_GUI );
	cursor = CURSOR_ARROW;
	enableClipping = true;
	overStrikeMode = true;
	mat.Identity();
	matIsIdentity = true;
	origin.Zero();
	initialized = true;
}

void hfDeviceContext::Shutdown() {
	clipRects.Clear();
	Clear();
}

void hfDeviceContext::Clear() {
	initialized = false;
}

hfDeviceContext::hfDeviceContext() {
	Clear();
}

void hfDeviceContext::SetTransformInfo(const hfVec3 &org, const hfMat3 &m) {
	origin = org;
	mat = m;
	matIsIdentity = mat.IsIdentity();
}

// 
//  added method
void hfDeviceContext::GetTransformInfo(hfVec3& org, hfMat3& m )
{
	m = mat;
	org = origin;
}
// 

void hfDeviceContext::EnableClipping(bool b) { 
	enableClipping = b; 
};

void hfDeviceContext::PopClipRect() {
	if (clipRects.Num()) {
		clipRects.RemoveIndex(clipRects.Num()-1);
	}
}

void hfDeviceContext::PushClipRect(hfRectangle r) {
	clipRects.Append(r);
}

bool hfDeviceContext::ClippedCoords(float *x, float *y, float *w, float *h, float *s1, float *t1, float *s2, float *t2) {

	if ( enableClipping == false || clipRects.Num() == 0 ) {
		return false;
	}

	int c = clipRects.Num();
	while( --c > 0 ) {
		hfRectangle *clipRect = &clipRects[c];
 
		float ox = *x;
		float oy = *y;
		float ow = *w;
		float oh = *h;

		if ( ow <= 0.0f || oh <= 0.0f ) {
			break;
		}

		if (*x < clipRect->x) {
			*w -= clipRect->x - *x;
			*x = clipRect->x;
		} else if (*x > clipRect->x + clipRect->w) {
			*x = *w = *y = *h = 0;
		}
		if (*y < clipRect->y) {
			*h -= clipRect->y - *y;
			*y = clipRect->y;
		} else if (*y > clipRect->y + clipRect->h) {
			*x = *w = *y = *h = 0;
		}
		if (*w > clipRect->w) {
			*w = clipRect->w - *x + clipRect->x;
		} else if (*x + *w > clipRect->x + clipRect->w) {
			*w = clipRect->Right() - *x;
		}
		if (*h > clipRect->h) {
			*h = clipRect->h - *y + clipRect->y;
		} else if (*y + *h > clipRect->y + clipRect->h) {
			*h = clipRect->Bottom() - *y;
		}

		if ( s1 && s2 && t1 && t2 && ow > 0.0f ) {
			float ns1, ns2, nt1, nt2;
			// upper left
			float u = ( *x - ox ) / ow;
			ns1 = *s1 * ( 1.0f - u ) + *s2 * ( u );

			// upper right
			u = ( *x + *w - ox ) / ow;
			ns2 = *s1 * ( 1.0f - u ) + *s2 * ( u );

			// lower left
			u = ( *y - oy ) / oh;
			nt1 = *t1 * ( 1.0f - u ) + *t2 * ( u );

			// lower right
			u = ( *y + *h - oy ) / oh;
			nt2 = *t1 * ( 1.0f - u ) + *t2 * ( u );

			// set values
			*s1 = ns1;
			*s2 = ns2;
			*t1 = nt1;
			*t2 = nt2;
		}
	}

	return (*w == 0 || *h == 0) ? true : false;
}

/*
=============
DrawStretchPic
=============
*/
void hfDeviceContext::DrawWinding( hfWinding & w, const hfMaterial * mat ) {

	hfPlane p;

	p.Normal().Set( 1.0f, 0.0f, 0.0f );
	p.SetDist( 0.0f );
	w.ClipInPlace( p );

	p.Normal().Set( -1.0f, 0.0f, 0.0f );
	p.SetDist( -SCREEN_WIDTH );
	w.ClipInPlace( p );

	p.Normal().Set( 0.0f, 1.0f, 0.0f );
	p.SetDist( 0.0f );
	w.ClipInPlace( p );

	p.Normal().Set( 0.0f, -1.0f, 0.0f );
	p.SetDist( -SCREEN_HEIGHT );
	w.ClipInPlace( p );

	if ( w.GetNumPoints() < 3 ) {
		return;
	}

	int numIndexes = 0;
	triIndex_t tempIndexes[(MAX_POINTS_ON_WINDING-2)*3];
	for ( int j = 2; j < w.GetNumPoints(); j++ ) {
		tempIndexes[numIndexes++] = 0;
		tempIndexes[numIndexes++] = j - 1;
		tempIndexes[numIndexes++] = j;
	}
	assert( numIndexes == ( w.GetNumPoints() - 2 ) * 3 );

	hfDrawVert * verts = renderSystem->AllocTris( w.GetNumPoints(), tempIndexes, numIndexes, mat );
	if ( verts == NULL ) {
		return;
	}
	uint32 currentColor = renderSystem->GetColor();

	for ( int j = 0 ; j < w.GetNumPoints() ; j++ ) {
		verts[j].xyz.x = xOffset + w[j].x * xScale;
		verts[j].xyz.y = yOffset + w[j].y * yScale;
		verts[j].xyz.z = w[j].z;
		verts[j].SetTexCoord( w[j].s, w[j].t );
		verts[j].SetColor( currentColor );
		verts[j].ClearColor2();
		verts[j].SetNormal( 0.0f, 0.0f, 1.0f );
		verts[j].SetTangent( 1.0f, 0.0f, 0.0f );
		verts[j].SetBiTangent( 0.0f, 1.0f, 0.0f );
	}
}

void hfDeviceContext::DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const hfMaterial *shader) {
	if ( matIsIdentity ) {
		renderSystem->DrawStretchPic( xOffset + x * xScale, yOffset + y * yScale, w * xScale, h * yScale, s1, t1, s2, t2, shader );
		return;
	}

	hfFixedWinding winding;
	winding.AddPoint( hfVec5( x, y, 0.0f, s1, t1 ) );
	winding.AddPoint( hfVec5( x+w, y, 0.0f, s2, t1 ) );
	winding.AddPoint( hfVec5( x+w, y+h, 0.0f, s2, t2 ) );
	winding.AddPoint( hfVec5( x, y+h, 0.0f, s1, t2 ) );

	for ( int i = 0; i < winding.GetNumPoints(); i++ ) {
		winding[i].ToVec3() -= origin;
		winding[i].ToVec3() *= mat;
		winding[i].ToVec3() += origin;
	}

	DrawWinding( winding, shader );
}


void hfDeviceContext::DrawMaterial(float x, float y, float w, float h, const hfMaterial *mat, const hfVec4 &color, float scalex, float scaley) {

	renderSystem->SetColor(color);

	float	s0, s1, t0, t1;
// 
//  handle negative scales as well	
	if ( scalex < 0 )
	{
		w *= -1;
		scalex *= -1;
	}
	if ( scaley < 0 )
	{
		h *= -1;
		scaley *= -1;
	}
// 
	if( w < 0 ) {	// flip about vertical
		w  = -w;
		s0 = 1 * scalex;
		s1 = 0;
	}
	else {
		s0 = 0;
		s1 = 1 * scalex;
	}

	if( h < 0 ) {	// flip about horizontal
		h  = -h;
		t0 = 1 * scaley;
		t1 = 0;
	}
	else {
		t0 = 0;
		t1 = 1 * scaley;
	}

	if ( ClippedCoords( &x, &y, &w, &h, &s0, &t0, &s1, &t1 ) ) {
		return;
	}

	DrawStretchPic( x, y, w, h, s0, t0, s1, t1, mat);
}

void hfDeviceContext::DrawMaterialRotated(float x, float y, float w, float h, const hfMaterial *mat, const hfVec4 &color, float scalex, float scaley, float angle) {
	
	renderSystem->SetColor(color);

	float	s0, s1, t0, t1;
	// 
	//  handle negative scales as well	
	if ( scalex < 0 )
	{
		w *= -1;
		scalex *= -1;
	}
	if ( scaley < 0 )
	{
		h *= -1;
		scaley *= -1;
	}
	// 
	if( w < 0 ) {	// flip about vertical
		w  = -w;
		s0 = 1 * scalex;
		s1 = 0;
	}
	else {
		s0 = 0;
		s1 = 1 * scalex;
	}

	if( h < 0 ) {	// flip about horizontal
		h  = -h;
		t0 = 1 * scaley;
		t1 = 0;
	}
	else {
		t0 = 0;
		t1 = 1 * scaley;
	}

	if ( angle == 0.0f && ClippedCoords( &x, &y, &w, &h, &s0, &t0, &s1, &t1 ) ) {
		return;
	}

	DrawStretchPicRotated( x, y, w, h, s0, t0, s1, t1, mat, angle);
}

void hfDeviceContext::DrawStretchPicRotated(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const hfMaterial *shader, float angle) {
	
	hfFixedWinding winding;
	winding.AddPoint( hfVec5( x, y, 0.0f, s1, t1 ) );
	winding.AddPoint( hfVec5( x+w, y, 0.0f, s2, t1 ) );
	winding.AddPoint( hfVec5( x+w, y+h, 0.0f, s2, t2 ) );
	winding.AddPoint( hfVec5( x, y+h, 0.0f, s1, t2 ) );

	for ( int i = 0; i < winding.GetNumPoints(); i++ ) {
		winding[i].ToVec3() -= origin;
		winding[i].ToVec3() *= mat;
		winding[i].ToVec3() += origin;
	}

	//Generate a translation so we can translate to the center of the image rotate and draw
	hfVec3 origTrans;
	origTrans.x = x+(w/2);
	origTrans.y = y+(h/2);
	origTrans.z = 0;


	//Rotate the verts about the z axis before drawing them
	hfMat3 rotz;
	rotz.Identity();
	float sinAng, cosAng;
	hfMath::SinCos( angle, sinAng, cosAng );
	rotz[0][0] = cosAng;
	rotz[0][1] = sinAng;
	rotz[1][0] = -sinAng;
	rotz[1][1] = cosAng;
	for (int i = 0; i < winding.GetNumPoints(); i++) {
		winding[i].ToVec3() -= origTrans;
		winding[i].ToVec3() *= rotz;
		winding[i].ToVec3() += origTrans;
	}
	
	DrawWinding( winding, shader );
}

void hfDeviceContext::DrawFilledRect( float x, float y, float w, float h, const hfVec4 &color) {

	if ( color.w == 0.0f ) {
		return;
	}

	renderSystem->SetColor(color);
	
	if (ClippedCoords(&x, &y, &w, &h, NULL, NULL, NULL, NULL)) {
		return;
	}

	DrawStretchPic( x, y, w, h, 0, 0, 0, 0, whiteImage);
}


void hfDeviceContext::DrawRect( float x, float y, float w, float h, float size, const hfVec4 &color) {

	if ( color.w == 0.0f ) {
		return;
	}

	renderSystem->SetColor(color);
	
	if (ClippedCoords(&x, &y, &w, &h, NULL, NULL, NULL, NULL)) {
		return;
	}

	DrawStretchPic( x, y, size, h, 0, 0, 0, 0, whiteImage );
	DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, whiteImage );
	DrawStretchPic( x, y, w, size, 0, 0, 0, 0, whiteImage );
	DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, whiteImage );
}

void hfDeviceContext::DrawMaterialRect( float x, float y, float w, float h, float size, const hfMaterial *mat, const hfVec4 &color) {

	if ( color.w == 0.0f ) {
		return;
	}

	renderSystem->SetColor(color);
	DrawMaterial( x, y, size, h, mat, color );
	DrawMaterial( x + w - size, y, size, h, mat, color );
	DrawMaterial( x, y, w, size, mat, color );
	DrawMaterial( x, y + h - size, w, size, mat, color );
}


void hfDeviceContext::SetCursor(int n) {
	
	if ( n > CURSOR_ARROW && n < CURSOR_COUNT ) {

		keyBindings_t binds = hfKeyInput::KeyBindingsFromBinding( "_use", true );

		keyNum_t keyNum = K_NONE;
		if ( in_useJoystick.GetBool() ) {
			 keyNum = hfKeyInput::StringToKeyNum( binds.gamepad.c_str() );
		} 

		if ( keyNum != K_NONE ) {
			
			if ( keyNum == K_JOY1 ) {
				cursor = CURSOR_HAND_JOY1;
			} else if ( keyNum == K_JOY2 ) {
				cursor = CURSOR_HAND_JOY2;
			} else if ( keyNum == K_JOY3 ) {
				cursor = CURSOR_HAND_JOY3;
			} else if ( keyNum == K_JOY4 ) {
				cursor = CURSOR_HAND_JOY4;
			}

		} else {
			cursor = CURSOR_HAND;
		}

	} else {
		cursor = CURSOR_ARROW;
	}
}

void hfDeviceContext::DrawCursor(float *x, float *y, float size) {
	if (*x < 0) {
		*x = 0;
	}

	if (*x >= VIRTUAL_WIDTH) {
		*x = VIRTUAL_WIDTH;
	}

	if (*y < 0) {
		*y = 0;
	}

	if (*y >= VIRTUAL_HEIGHT) {
		*y = VIRTUAL_HEIGHT;
	}

	renderSystem->SetColor(colorWhite);
	DrawStretchPic( *x, *y, size, size, 0, 0, 1, 1, cursorImages[cursor]);
}
/*
 =======================================================================================================================
 =======================================================================================================================
 */

void hfDeviceContext::PaintChar( float x, float y, const scaledGlyphInfo_t & glyphInfo ) {
	y -= glyphInfo.top;
	x += glyphInfo.left;

	float w = glyphInfo.width;
	float h = glyphInfo.height;
	float s = glyphInfo.s1;
	float t = glyphInfo.t1;
	float s2 = glyphInfo.s2;
	float t2 = glyphInfo.t2;
	const hfMaterial * hShader = glyphInfo.material;

	if ( ClippedCoords( &x, &y, &w, &h, &s, &t, &s2, &t2) ) {
		return;
	}

	DrawStretchPic(x, y, w, h, s, t, s2, t2, hShader);
}

int hfDeviceContext::DrawText(float x, float y, float scale, hfVec4 color, const char *text, float adjust, int limit, int style, int cursor) {
	int			len;
	hfVec4		newColor;

	hfStr drawText = text;
	int charIndex = 0;

	if ( text && color.w != 0.0f ) {
		renderSystem->SetColor(color);
		memcpy(&newColor[0], &color[0], sizeof(hfVec4));
		len = drawText.Length();
		if (limit > 0 && len > limit) {
			len = limit;
		}

		float prevGlyphSkip = 0.0f;

		while ( charIndex < len ) {
			uint32 textChar = drawText.UTF8Char( charIndex );

			if ( hfStr::IsColor( drawText.c_str() + charIndex ) ) {
				if ( drawText[ charIndex++ ] == C_COLOR_DEFAULT ) {
					newColor = color;
				} else {
					newColor = hfStr::ColorForIndex( charIndex );
					newColor[3] = color[3];
				}
				if (cursor == charIndex-1 || cursor == charIndex) {
					float backup = 0.0f;
					if ( prevGlyphSkip > 0.0f ) {
						backup = ( prevGlyphSkip + adjust) / 5.0f;
					}
					if ( cursor == charIndex-1 ) {
						backup *= 2.0f;
					} else {
						renderSystem->SetColor(newColor);
					}
					DrawEditCursor(x - backup, y, scale);
				}
				renderSystem->SetColor(newColor);
				continue;
			} else {
				scaledGlyphInfo_t glyphInfo;
				activeFont->GetScaledGlyph( scale, textChar, glyphInfo );
				prevGlyphSkip = glyphInfo.xSkip;

				PaintChar( x, y, glyphInfo );

				if (cursor == charIndex-1) {
					DrawEditCursor(x, y, scale);
				}
				x += glyphInfo.xSkip + adjust;
			}
		}
		if (cursor == len) {
			DrawEditCursor(x, y, scale);
		}
	}
	return drawText.Length();
}

void hfDeviceContext::SetSize( float width, float height ) {
	xScale = VIRTUAL_WIDTH / width;
	yScale = VIRTUAL_HEIGHT / height;
}

void hfDeviceContext::SetOffset( float x, float y ) {
	xOffset = x;
	yOffset = y;
}

int hfDeviceContext::CharWidth( const char c, float scale ) {
	return hfMath::Ftoi( activeFont->GetGlyphWidth( scale, c ) );
}

int hfDeviceContext::TextWidth( const char *text, float scale, int limit ) {
	if ( text == NULL ) {
		return 0;
	}

	int i;
	float width = 0;
	if ( limit > 0 ) {
		for ( i = 0; text[i] != '\0' && i < limit; i++ ) {
			if ( hfStr::IsColor( text + i ) ) {
				i++;
			} else {
				width += activeFont->GetGlyphWidth( scale, ((const unsigned char *)text)[i] );
			}
		}
	} else {
		for ( i = 0; text[i] != '\0'; i++ ) {
			if ( hfStr::IsColor( text + i ) ) {
				i++;
			} else {
				width += activeFont->GetGlyphWidth( scale, ((const unsigned char *)text)[i] );
			}
		}
	}
	return hfMath::Ftoi( width );
}

int hfDeviceContext::TextHeight(const char *text, float scale, int limit) {
	return hfMath::Ftoi( activeFont->GetLineHeight( scale ) );
}

int hfDeviceContext::MaxCharWidth(float scale) {
	return hfMath::Ftoi( activeFont->GetMaxCharWidth( scale ) );
}

int hfDeviceContext::MaxCharHeight(float scale) {
	return hfMath::Ftoi( activeFont->GetLineHeight( scale ) );
}

const hfMaterial *hfDeviceContext::GetScrollBarImage(int index) {
	if (index >= SCROLLBAR_HBACK && index < SCROLLBAR_COUNT) {
		return scrollBarImages[index];
	}
	return scrollBarImages[SCROLLBAR_HBACK];
}

// this only supports left aligned text
hfRegion *hfDeviceContext::GetTextRegion(const char *text, float textScale, hfRectangle rectDraw, float xStart, float yStart) {
	return NULL;
}

void hfDeviceContext::DrawEditCursor( float x, float y, float scale ) {
	if ( (int)( hfLib::frameNumber >> 4 ) & 1 ) {
		return;
	}
	char cursorChar = (overStrikeMode) ? '_' : '|';
	scaledGlyphInfo_t glyphInfo;
	activeFont->GetScaledGlyph( scale, cursorChar, glyphInfo );
 	PaintChar( x, y, glyphInfo );
}

int hfDeviceContext::DrawText( const char *text, float textScale, int textAlign, hfVec4 color, hfRectangle rectDraw, bool wrap, int cursor, bool calcOnly, hfList<int> *breaks, int limit ) {
	int			count = 0;
	int			charIndex = 0;
	int			lastBreak = 0;
	float		y = 0.0f;
	float		textWidth = 0.0f;
	float		textWidthAtLastBreak = 0.0f;

	float		charSkip = MaxCharWidth( textScale ) + 1;
	float		lineSkip = MaxCharHeight( textScale );

	bool		lineBreak = false;
	bool		wordBreak = false;

	hfStr drawText = text;
	hfStr textBuffer;

	if (!calcOnly && !(text && *text)) {
		if (cursor == 0) {
			renderSystem->SetColor(color);
			DrawEditCursor(rectDraw.x, lineSkip + rectDraw.y, textScale);
		}
		return hfMath::Ftoi( rectDraw.w / charSkip );
	}

	y = lineSkip + rectDraw.y;

	if ( breaks ) {
		breaks->Append(0);
	}

	while ( charIndex < drawText.Length() ) {
		uint32 textChar = drawText.UTF8Char( charIndex );

		// See if we need to start a new line.
		if ( textChar == '\n' || textChar == '\r' || charIndex == drawText.Length() ) {
			lineBreak = true;
			if ( charIndex < drawText.Length() ) {
				// New line character and we still have more text to read.
				char nextChar = drawText[ charIndex + 1 ];
				if ( ( textChar == '\n' && nextChar == '\r' ) || ( textChar == '\r' && nextChar == '\n' ) ) {
					// Just absorb extra newlines.
					textChar = drawText.UTF8Char( charIndex );
				}
			}
		}

		// Check for escape colors if not then simply get the glyph width.
		if ( textChar == C_COLOR_ESCAPE && charIndex < drawText.Length() ) {
			textBuffer.AppendUTF8Char( textChar );
			textChar = drawText.UTF8Char( charIndex );
		}

		// If the character isn't a new line then add it to the text buffer.
		if ( textChar != '\n' && textChar != '\r' ) {
			textWidth += activeFont->GetGlyphWidth( textScale, textChar );
			textBuffer.AppendUTF8Char( textChar );
		}

		if ( !lineBreak && ( textWidth > rectDraw.w ) ) {
			// The next character will cause us to overflow, if we haven't yet found a suitable
			// break spot, set it to be this character
			if ( textBuffer.Length() > 0 && lastBreak == 0 ) {
				lastBreak = textBuffer.Length();
				textWidthAtLastBreak = textWidth;
			}
			wordBreak = true;
		} else if ( lineBreak || ( wrap && ( textChar == ' ' || textChar == '\t' ) ) ) {
			// The next character is in view, so if we are a break character, store our position
			lastBreak = textBuffer.Length();
			textWidthAtLastBreak = textWidth;
		}

		// We need to go to a new line
		if ( lineBreak || wordBreak ) {
			float x = rectDraw.x;

			if ( textWidthAtLastBreak > 0 ) {
				textWidth = textWidthAtLastBreak;
			}

			// Align text if needed
			if (textAlign == ALIGN_RIGHT) {
				x = rectDraw.x + rectDraw.w - textWidth;
			} else if (textAlign == ALIGN_CENTER) {
				x = rectDraw.x + (rectDraw.w - textWidth) / 2;
			}

			if ( wrap || lastBreak > 0 ) {
				// This is a special case to handle breaking in the middle of a word.
				// if we didn't do this, the cursor would appear on the end of this line
				// and the beginning of the next.
				if ( wordBreak && cursor >= lastBreak && lastBreak == textBuffer.Length() ) {
					cursor++;
				}
			}

			// Draw what's in the current text buffer.
			if (!calcOnly) {
				if ( lastBreak > 0 ) {
					count += DrawText(x, y, textScale, color, textBuffer.Left( lastBreak ).c_str(), 0, 0, 0, cursor);
					textBuffer = textBuffer.Right( textBuffer.Length() - lastBreak );
				} else {
					count += DrawText(x, y, textScale, color, textBuffer.c_str(), 0, 0, 0, cursor);
					textBuffer.Clear();
				}
			}

			if ( cursor < lastBreak ) {
				cursor = -1;
			} else if ( cursor >= 0 ) {
				cursor -= ( lastBreak + 1 );
			}

			// If wrap is disabled return at this point.
			if ( !wrap ) {
				return lastBreak;
			}

			// If we've hit the allowed character limit then break.
			if ( limit && count > limit ) {
				break;
			}

			y += lineSkip + 5;

			if ( !calcOnly && y > rectDraw.Bottom() ) {
				break;
			}

			// If breaks were requested then make a note of this one.
			if (breaks) {
				breaks->Append( drawText.Length() - charIndex );
			}

			// Reset necessary parms for next line.
			lastBreak = 0;
			textWidth = 0;
			textWidthAtLastBreak = 0;
			lineBreak = false;
			wordBreak = false;

			// Reassess the remaining width
			for ( int i = 0; i < textBuffer.Length(); ) {
				if ( textChar != C_COLOR_ESCAPE ) {
					textWidth += activeFont->GetGlyphWidth( textScale, textBuffer.UTF8Char( i ) );
				}
			}

			continue;
		}
	}

	return hfMath::Ftoi( rectDraw.w / charSkip );
}

/*
=============
hfRectangle::String
=============
*/
char *hfRectangle::String() const {
	static	int		index = 0;
	static	char	str[ 8 ][ 48 ];
	char	*s;

	// use an array so that multiple toString's won't collide
	s = str[ index ];
	index = (index + 1)&7;

	sprintf( s, "%.2f %.2f %.2f %.2f", x, y, w, h );

	return s;
}


/*
================================================================================================

OPTIMIZED VERSIONS

================================================================================================
*/

// this is only called for the cursor and debug strings, and it should
// scope properly with push/pop clipRect
void hfDeviceContextOptimized::EnableClipping(bool b) { 
	if ( b == enableClipping ) {
		return;
	}
	enableClipping = b; 
	if ( !enableClipping ) {
		PopClipRect();
	} else {
		// the actual value of the rect is irrelvent
		PushClipRect( hfRectangle( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT ) );

		// allow drawing beyond the normal bounds for debug text
		// this also allows the cursor to draw outside, so we might want
		// to make this exactly the screen bounds, since we aren't likely
		// to ever turn on the gui debug text again...
		clipX1 = -SCREEN_WIDTH;
		clipX2 = SCREEN_WIDTH * 2;
		clipY1 = -SCREEN_HEIGHT;
		clipY2 = SCREEN_HEIGHT * 2;
	}
};


void hfDeviceContextOptimized::PopClipRect() {
	if (clipRects.Num()) {
		clipRects.SetNum( clipRects.Num()-1 );	// don't resize the list, just change num
	}
	if ( clipRects.Num() > 0 ) {
		const hfRectangle & clipRect = clipRects[ clipRects.Num() - 1 ];
		clipX1 = clipRect.x;
		clipY1 = clipRect.y;
		clipX2 = clipRect.x + clipRect.w;
		clipY2 = clipRect.y + clipRect.h;
	} else {
		clipX1 = 0;
		clipY1 = 0;
		clipX2 = SCREEN_WIDTH;
		clipY2 = SCREEN_HEIGHT;
	}
}

static const hfRectangle baseScreenRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT );

void hfDeviceContextOptimized::PushClipRect(hfRectangle r) {
	const hfRectangle & prev = ( clipRects.Num() == 0 ) ? baseScreenRect : clipRects[clipRects.Num()-1];

	// instead of storing the rect, store the intersection of the rect
	// with the previous rect, so ClippedCoords() only has to test against one rect
	hfRectangle intersection = prev;
	intersection.ClipAgainst( r, false );
	clipRects.Append( intersection );

	const hfRectangle & clipRect = clipRects[ clipRects.Num() - 1 ];
	clipX1 = clipRect.x;
	clipY1 = clipRect.y;
	clipX2 = clipRect.x + clipRect.w;
	clipY2 = clipRect.y + clipRect.h;
}

bool hfDeviceContextOptimized::ClippedCoords(float *x, float *y, float *w, float *h, float *s1, float *t1, float *s2, float *t2) {
	const float ox = *x;
	const float oy = *y;
	const float ow = *w;
	const float oh = *h;

	// presume visible first
	if ( ox >= clipX1 && oy >= clipY1 && ox + ow <= clipX2 && oy + oh <= clipY2 ) {
		return false;
	}

	// do clipping
	if ( ox < clipX1 ) {
		*w -= clipX1 - ox;
		*x = clipX1;
	} else if ( ox > clipX2 ) {
		return true;
	}
	if ( oy < clipY1) {
		*h -= clipY1 - oy;
		*y = clipY1;
	} else if ( oy > clipY2) {
		return true;
	}
	if ( *x + *w > clipX2 ) {
		*w = clipX2 - *x;
	}
	if ( *y + *h > clipY2 ) {
		*h = clipY2 - *y;
	}

	if ( *w <= 0 || *h <= 0 ) {
		return true;
	}

	// filled rects won't pass in texcoords
	if ( s1 ) {
		float ns1, ns2, nt1, nt2;
		// upper left
		float u = ( *x - ox ) / ow;
		ns1 = *s1 * ( 1.0f - u ) + *s2 * ( u );

		// upper right
		u = ( *x + *w - ox ) / ow;
		ns2 = *s1 * ( 1.0f - u ) + *s2 * ( u );

		// lower left
		u = ( *y - oy ) / oh;
		nt1 = *t1 * ( 1.0f - u ) + *t2 * ( u );

		// lower right
		u = ( *y + *h - oy ) / oh;
		nt2 = *t1 * ( 1.0f - u ) + *t2 * ( u );

		// set values
		*s1 = ns1;
		*s2 = ns2;
		*t1 = nt1;
		*t2 = nt2;
	}

	// still needs to be drawn
	return false;
}

/*
=============
hfDeviceContextOptimized::DrawText
=============
*/
static triIndex_t quadPicIndexes[6] = { 3, 0, 2, 2, 0, 1 };
int hfDeviceContextOptimized::DrawText(float x, float y, float scale, hfVec4 color, const char *text, float adjust, int limit, int style, int cursor) {
	if ( !matIsIdentity || cursor != -1 ) {
		// fallback to old code
		return hfDeviceContext::DrawText( x, y, scale, color, text, adjust, limit, style, cursor );
	}

	hfStr drawText = text;

	if ( drawText.Length() == 0 ) {
		return 0;
	}
	if ( color.w == 0.0f ) {
		return 0;
	}

	const uint32 currentColor = PackColor( color );
	uint32 currentColorNativeByteOrder = LittleLong( currentColor );

	int len = drawText.Length();
	if (limit > 0 && len > limit) {
		len = limit;
	}
	
	int charIndex = 0;
	while ( charIndex < drawText.Length() ) {
		uint32 textChar = drawText.UTF8Char( charIndex );
		if ( textChar == C_COLOR_ESCAPE ) {
			// I'm not sure if inline text color codes are used anywhere in the game,
			// they may only be needed for multi-color user names
			hfVec4		newColor;
			uint32 colorIndex = drawText.UTF8Char( charIndex );
			if ( colorIndex == C_COLOR_DEFAULT ) {
				newColor = color;
			} else {
				newColor = hfStr::ColorForIndex( colorIndex );
				newColor[3] = color[3];
			}
			renderSystem->SetColor(newColor);
			currentColorNativeByteOrder = LittleLong( PackColor( newColor ) );
			continue;
		}

		scaledGlyphInfo_t glyphInfo;
		activeFont->GetScaledGlyph( scale, textChar, glyphInfo );

		// PaintChar( x, y, glyphInfo );
		float drawY = y - glyphInfo.top;
		float drawX = x + glyphInfo.left;
		float w = glyphInfo.width;
		float h = glyphInfo.height;
		float s = glyphInfo.s1;
		float t = glyphInfo.t1;
		float s2 = glyphInfo.s2;
		float t2 = glyphInfo.t2;

		if ( !ClippedCoords( &drawX, &drawY, &w, &h, &s, &t, &s2, &t2) ) {
			float x1 = xOffset + drawX * xScale;
			float x2 = xOffset + ( drawX + w ) * xScale;
			float y1 = yOffset + drawY * yScale;
			float y2 = yOffset + ( drawY + h ) * yScale;
			hfDrawVert * verts = tr_guiModel->AllocTris( 4, quadPicIndexes, 6, glyphInfo.material, 0, STEREO_DEPTH_TYPE_NONE );
			if ( verts != NULL ) {
				verts[0].xyz[0] = x1;
				verts[0].xyz[1] = y1;
				verts[0].xyz[2] = 0.0f;
				verts[0].SetTexCoord( s, t );
				verts[0].SetNativeOrderColor( currentColorNativeByteOrder );
				verts[0].ClearColor2();

				verts[1].xyz[0] = x2;
				verts[1].xyz[1] = y1;
				verts[1].xyz[2] = 0.0f;
				verts[1].SetTexCoord( s2, t );
				verts[1].SetNativeOrderColor( currentColorNativeByteOrder );
				verts[1].ClearColor2();

				verts[2].xyz[0] = x2;
				verts[2].xyz[1] = y2;
				verts[2].xyz[2] = 0.0f;
				verts[2].SetTexCoord( s2, t2 );
				verts[2].SetNativeOrderColor( currentColorNativeByteOrder );
				verts[2].ClearColor2();

				verts[3].xyz[0] = x1;
				verts[3].xyz[1] = y2;
				verts[3].xyz[2] = 0.0f;
				verts[3].SetTexCoord( s, t2 );
				verts[3].SetNativeOrderColor( currentColorNativeByteOrder );
				verts[3].ClearColor2();
			}
		}

		x += glyphInfo.xSkip + adjust;
	}
	return drawText.Length();
}

