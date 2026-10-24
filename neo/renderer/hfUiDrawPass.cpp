#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfUiDrawPass.h"
#include "hftr_local.h"
#include "../hflib/hfHeadless.h"

HF_Ecs::Registry *	hfUiDrawPass::s_registry = NULL;
HF_Ui::hfUiSystem *	hfUiDrawPass::s_system = NULL;

// Text overlay scratch: the labels are rasterized into an 8-bit coverage
// buffer and uploaded as a single intensity texture.  One upload per frame is
// cheap at UI resolutions and keeps the font path free of engine materials.
static unsigned char *	s_textPixels = NULL;
static int				s_textWidth = 0;
static int				s_textHeight = 0;
static unsigned int		s_textTexture = 0;

static void RasterizeScaled( const char *text, float scale, float x, float y,
							 HF_Ui::hfUiCoverage &out, unsigned char color ) {
	if ( text == NULL ) {
		return;
	}
	int s = ( int )( scale + 0.5f );
	if ( s < 1 ) { s = 1; }
	const int ox = ( int )x;
	const int oy = ( int )y;
	int cx = ox;
	int cy = oy;
	for ( int i = 0; text[ i ] != '\0'; i++ ) {
		const char c = text[ i ];
		if ( c == '\n' ) {
			cx = ox;
			cy += HF_Ui::HF_UI_LINE_HEIGHT * s;
			continue;
		}
		if ( c != ' ' ) {
			const unsigned char *glyph = HF_Ui::hfBitmapFont::Glyph( c );
			for ( int row = 0; row < HF_Ui::HF_UI_GLYPH_H; row++ ) {
				const unsigned char bits = glyph[ row ];
				for ( int col = 0; col < HF_Ui::HF_UI_GLYPH_W; col++ ) {
					if ( bits & ( 1 << ( HF_Ui::HF_UI_GLYPH_W - 1 - col ) ) ) {
						for ( int dy = 0; dy < s; dy++ ) {
							for ( int dx = 0; dx < s; dx++ ) {
								out.Set( cx + col * s + dx, cy + row * s + dy, color );
							}
						}
					}
				}
			}
		}
		cx += HF_Ui::HF_UI_ADVANCE * s;
	}
}

static void BlitCoverage( const HF_Ui::hfUiCoverage &cov ) {
	if ( cov.pixels == NULL || cov.width <= 0 || cov.height <= 0 ) {
		return;
	}
	if ( s_textTexture == 0 ) {
		qglGenTextures( 1, &s_textTexture );
	}

	qglBindTexture( GL_TEXTURE_2D, s_textTexture );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	qglTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	qglPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
	qglTexImage2D( GL_TEXTURE_2D, 0, GL_INTENSITY, cov.width, cov.height, 0,
				   GL_INTENSITY, GL_UNSIGNED_BYTE, cov.pixels );
	qglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	qglEnable( GL_TEXTURE_2D );
	GL_Color( 1, 1, 1, 1 );
	qglBegin( GL_QUADS );
	qglTexCoord2f( 0, 0 ); qglVertex2f( 0, 0 );
	qglTexCoord2f( 1, 0 ); qglVertex2f( ( float )cov.width, 0 );
	qglTexCoord2f( 1, 1 ); qglVertex2f( ( float )cov.width, ( float )cov.height );
	qglTexCoord2f( 0, 1 ); qglVertex2f( 0, ( float )cov.height );
	qglEnd();
	qglDisable( GL_TEXTURE_2D );
	qglBindTexture( GL_TEXTURE_2D, 0 );
}

void hfUiDrawPass::SetSource( HF_Ecs::Registry *registry, HF_Ui::hfUiSystem *uiSystem ) {
	s_registry = registry;
	s_system = uiSystem;
}

void hfUiDrawPass::ClearSource() {
	s_registry = NULL;
	s_system = NULL;
}

bool hfUiDrawPass::HasSource() {
	return s_registry != NULL && s_system != NULL;
}

void hfUiDrawPass::DrawInstalled() {
	if ( !HasSource() ) {
		return;
	}
	// A headless run has no GL context; the backend should not be calling
	// this at all, but the guard keeps a stray call from touching GL.
	if ( HF_Lib::IsHeadless() ) {
		return;
	}
	if ( renderSystem == NULL ) {
		return;
	}
	const int width = renderSystem->GetWidth();
	const int height = renderSystem->GetHeight();
	if ( width <= 0 || height <= 0 ) {
		return;
	}
	// The last 3D view may have left a cropped viewport/scissor behind; the
	// overlay is always full-window in the UI's own coordinate space.
	GL_ViewportAndScissor( 0, 0, width, height );

	if ( s_textWidth != width || s_textHeight != height ) {
		delete[] s_textPixels;
		s_textPixels = new unsigned char[ ( size_t )width * ( size_t )height ];
		s_textWidth = width;
		s_textHeight = height;
	}
	HF_Ui::hfUiCoverage text = { width, height, s_textPixels };
	text.Clear();

	Draw( *s_registry, *s_system, &text );
	BlitCoverage( text );
}

void hfUiDrawPass::Draw( HF_Ecs::Registry &registry, HF_Ui::hfUiSystem &uiSystem, HF_Ui::hfUiCoverage *textCoverage ) {
	int drawCount = uiSystem.GetDrawCount();
	
	// Save GL state
	qglPushAttrib( GL_ALL_ATTRIB_BITS );
	qglPushMatrix();
	
	qglMatrixMode( GL_PROJECTION );
	qglPushMatrix();
	qglLoadIdentity();
	qglOrtho( 0, renderSystem->GetWidth(), renderSystem->GetHeight(), 0, -1, 1 );
	
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity();
	
	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ); qglDisable( GL_DEPTH_TEST );
	GL_Cull( CT_TWO_SIDED );
	qglDisable( GL_TEXTURE_2D );
	
	for ( int d = 0; d < drawCount; d++ ) {
		HF_Ecs::Entity ent = uiSystem.GetDrawEntity( d );
		if ( ent == HF_Ecs::ENTITY_NULL ) continue;
		
		const HF_Ui::hfUiCanvasComponent *canvas = registry.GetComponent< HF_Ui::hfUiCanvasComponent >( ent );
		if ( !canvas || !canvas->visible ) continue;
		
		HF_Ui::hfUiRect rect = uiSystem.GetRect( ent );
		HF_Ui::hfUiRect visible = uiSystem.GetVisible( ent );
		
		if ( visible.Empty() ) continue;
		
		// Panel background
		const HF_Ui::hfUiPanelComponent *panel = registry.GetComponent< HF_Ui::hfUiPanelComponent >( ent );
		if ( panel && panel->a > 0.0f ) {
			GL_Color( panel->r, panel->g, panel->b, panel->a );
			qglBegin( GL_QUADS );
			qglVertex2f( visible.x, visible.y );
			qglVertex2f( visible.x + visible.w, visible.y );
			qglVertex2f( visible.x + visible.w, visible.y + visible.h );
			qglVertex2f( visible.x, visible.y + visible.h );
			qglEnd();
		}
		
		// Button
		const HF_Ui::hfUiButtonComponent *btn = registry.GetComponent< HF_Ui::hfUiButtonComponent >( ent );
		if ( btn && panel == NULL ) { // draw button bg if no panel
			float r,g,b,a;
			if ( btn->state == HF_Ui::HF_UI_BUTTON_DISABLED ) { r=btn->disabledR; g=btn->disabledG; b=btn->disabledB; a=btn->disabledA; }
			else if ( btn->state == HF_Ui::HF_UI_BUTTON_PRESSED ) { r=btn->pressedR; g=btn->pressedG; b=btn->pressedB; a=btn->pressedA; }
			else if ( btn->state == HF_Ui::HF_UI_BUTTON_HOVER ) { r=btn->hoverR; g=btn->hoverG; b=btn->hoverB; a=btn->hoverA; }
			else { r=btn->normalR; g=btn->normalG; b=btn->normalB; a=btn->normalA; }
			GL_Color( r,g,b,a );
			qglBegin( GL_QUADS );
			qglVertex2f( visible.x, visible.y );
			qglVertex2f( visible.x + visible.w, visible.y );
			qglVertex2f( visible.x + visible.w, visible.y + visible.h );
			qglVertex2f( visible.x, visible.y + visible.h );
			qglEnd();
		}
		
		// Toggle/checkbox
		const HF_Ui::hfUiToggleComponent *toggle = registry.GetComponent< HF_Ui::hfUiToggleComponent >( ent );
		if ( toggle ) {
			float bx = visible.x + 4.0f;
			float by = visible.y + (visible.h - toggle->boxSize) * 0.5f;
			GL_Color( toggle->boxR, toggle->boxG, toggle->boxB, toggle->boxA );
			qglBegin( GL_QUADS );
			qglVertex2f( bx, by );
			qglVertex2f( bx + toggle->boxSize, by );
			qglVertex2f( bx + toggle->boxSize, by + toggle->boxSize );
			qglVertex2f( bx, by + toggle->boxSize );
			qglEnd();
			if ( toggle->checked ) {
				float pad = 3.0f;
				GL_Color( toggle->checkR, toggle->checkG, toggle->checkB, toggle->checkA );
				qglBegin( GL_QUADS );
				qglVertex2f( bx + pad, by + pad );
				qglVertex2f( bx + toggle->boxSize - pad, by + pad );
				qglVertex2f( bx + toggle->boxSize - pad, by + toggle->boxSize - pad );
				qglVertex2f( bx + pad, by + toggle->boxSize - pad );
				qglEnd();
			}
		}
		
		// Slider
		const HF_Ui::hfUiSliderComponent *slider = registry.GetComponent< HF_Ui::hfUiSliderComponent >( ent );
		if ( slider ) {
			float t = slider->maxValue - slider->minValue;
			if ( t < 0.0001f ) t = 1.0f;
			float pct = ( slider->value - slider->minValue ) / t;
			if ( pct < 0.0f ) pct = 0.0f;
			if ( pct > 1.0f ) pct = 1.0f;
			
			if ( !slider->vertical ) {
				float barY = visible.y + (visible.h - 4.0f) * 0.5f;
				GL_Color( slider->barR, slider->barG, slider->barB, slider->barA );
				qglBegin( GL_QUADS );
				qglVertex2f( visible.x, barY );
				qglVertex2f( visible.x + visible.w, barY );
				qglVertex2f( visible.x + visible.w, barY + 4.0f );
				qglVertex2f( visible.x, barY + 4.0f );
				qglEnd();
				
				float hx = visible.x + pct * ( visible.w - slider->handleW );
				float hy = visible.y + (visible.h - slider->handleH) * 0.5f;
				GL_Color( slider->handleR, slider->handleG, slider->handleB, slider->handleA );
				qglBegin( GL_QUADS );
				qglVertex2f( hx, hy );
				qglVertex2f( hx + slider->handleW, hy );
				qglVertex2f( hx + slider->handleW, hy + slider->handleH );
				qglVertex2f( hx, hy + slider->handleH );
				qglEnd();
			}
		}
		
		// Text label
		const HF_Ui::hfUiLabelComponent *label = registry.GetComponent< HF_Ui::hfUiLabelComponent >( ent );
		if ( label && label->visible && label->text[0] ) {
			if ( textCoverage ) {
				const float s = label->scale > 0.0f ? label->scale : 1.0f;
				const float tw = label->textWidth > 0.0f ? label->textWidth
								 : ( float )HF_Ui::hfBitmapFont::MeasureText( label->text ) * s;
				const float th = label->textHeight > 0.0f ? label->textHeight
								 : ( float )HF_Ui::HF_UI_LINE_HEIGHT * s;
				float tx = visible.x + 4.0f;
				if ( label->align == HF_Ui::HF_UI_TEXT_CENTER ) {
					tx = visible.x + ( visible.w - tw ) * 0.5f;
				} else if ( label->align == HF_Ui::HF_UI_TEXT_RIGHT ) {
					tx = visible.x + visible.w - tw - 4.0f;
				}
				const float ty = visible.y + ( visible.h - th ) * 0.5f;
				RasterizeScaled( label->text, s, tx, ty, *textCoverage, 255 );
			}
		}
	}
	
	qglMatrixMode( GL_PROJECTION );
	qglPopMatrix();
	qglMatrixMode( GL_MODELVIEW );
	qglPopMatrix();
	qglPopAttrib();
}
