#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfUiDrawPass.h"
#include "hftr_local.h"
#include "../hflib/hfHeadless.h"

HF_Ecs::Registry *	hfUiDrawPass::s_registry = NULL;
HF_Ui::hfUiSystem *	hfUiDrawPass::s_system = NULL;
HfArena *			hfUiDrawPass::s_arena = NULL;
bool					hfUiDrawPass::s_gameplayOverlayEnabled = false;
bool					hfUiDrawPass::s_colorblindMode = false;

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

// Rounded/nine-patch panel approximation.  A rounded box is assembled from
// the centre cell, four edge cells and four inset corner cells.  Keeping the
// construction as quads means it works on the same GL path as the rest of the
// overlay (and stays a no-op in headless mode).
static void DrawQuad( float x, float y, float w, float h ) {
	if ( w <= 0.0f || h <= 0.0f ) {
		return;
	}
	qglBegin( GL_QUADS );
	qglVertex2f( x, y );
	qglVertex2f( x + w, y );
	qglVertex2f( x + w, y + h );
	qglVertex2f( x, y + h );
	qglEnd();
}

static void DrawRoundedFill( float x, float y, float w, float h, float radius,
							 float r, float g, float b, float a ) {
	if ( w <= 0.0f || h <= 0.0f || a <= 0.0f ) {
		return;
	}
	GL_Color( r, g, b, a );
	if ( radius <= 0.5f ) {
		DrawQuad( x, y, w, h );
		return;
	}
	if ( radius > w * 0.5f ) { radius = w * 0.5f; }
	if ( radius > h * 0.5f ) { radius = h * 0.5f; }

	const float innerX = x + radius;
	const float innerY = y + radius;
	const float innerW = w - radius * 2.0f;
	const float innerH = h - radius * 2.0f;

	DrawQuad( innerX, innerY, innerW, innerH );				// centre
	DrawQuad( innerX, y, innerW, radius );					// top
	DrawQuad( innerX, y + h - radius, innerW, radius );		// bottom
	DrawQuad( x, innerY, radius, innerH );					// left
	DrawQuad( x + w - radius, innerY, radius, innerH );		// right

	const float c = radius * 0.45f;
	DrawQuad( x + radius - c, y + radius - c, c * 2.0f, c * 2.0f );
	DrawQuad( x + w - radius - c, y + radius - c, c * 2.0f, c * 2.0f );
	DrawQuad( x + radius - c, y + h - radius - c, c * 2.0f, c * 2.0f );
	DrawQuad( x + w - radius - c, y + h - radius - c, c * 2.0f, c * 2.0f );
}

// Minimal arena overlay: the deterministic sim's ±120 world box is mapped to
// a centred square of the UI viewport.  This is intentionally plain quads —
// it needs no assets and is a no-op in headless mode.  Ownership is explicit:
// it draws only when the host installed an arena AND enabled it for PLAYING.
static void EnemyColor( int kind, bool cb, float &r, float &g, float &b ) {
	if ( cb ) {
		switch ( kind ) {
			case HF_ARENA_ENEMY_SPITTER:	r = 0.20f; g = 0.75f; b = 0.95f; break;
			case HF_ARENA_ENEMY_BRUTE:		r = 0.60f; g = 0.45f; b = 0.95f; break;
			case HF_ARENA_ENEMY_BOSS:		r = 0.95f; g = 0.60f; b = 0.10f; break;
			default:						r = 0.90f; g = 0.75f; b = 0.20f; break;
		}
		return;
	}
	switch ( kind ) {
		case HF_ARENA_ENEMY_SPITTER:	r = 0.35f; g = 0.75f; b = 0.95f; break;
		case HF_ARENA_ENEMY_BRUTE:		r = 0.70f; g = 0.35f; b = 0.85f; break;
		case HF_ARENA_ENEMY_BOSS:		r = 1.00f; g = 0.25f; b = 0.55f; break;
		default:						r = 0.85f; g = 0.20f; b = 0.20f; break;
	}
}

static void PickupColor( int kind, bool cb, float &r, float &g, float &b ) {
	if ( cb ) {
		switch ( kind ) {
			case HF_ARENA_PICKUP_HEALTH:	r = 0.20f; g = 0.85f; b = 0.95f; break;
			case HF_ARENA_PICKUP_SCORE:		r = 0.95f; g = 0.80f; b = 0.20f; break;
			default:						r = 0.80f; g = 0.80f; b = 0.95f; break;
		}
		return;
	}
	switch ( kind ) {
		case HF_ARENA_PICKUP_HEALTH:	r = 0.30f; g = 0.95f; b = 0.45f; break;
		case HF_ARENA_PICKUP_SCORE:		r = 0.95f; g = 0.55f; b = 0.95f; break;
		default:						r = 0.95f; g = 0.85f; b = 0.25f; break;
	}
}

static void DrawArenaOverlay( const HfArena &arena, float viewportW, float viewportH ) {
	const bool cb = hfUiDrawPass::GetColorblindMode();
	const float side = viewportW < viewportH ? viewportW : viewportH;
	const float scale = side / 280.0f;		// ±120 world units plus margin
	float shakeX = 0.0f, shakeY = 0.0f;
	HfArena::HfArenaFxState::ShakeOffset( arena.fx.shakeTicks, arena.fx.shakeTicks, 18, shakeX, shakeY );
	const float ox = viewportW * 0.5f + shakeX;
	const float oy = viewportH * 0.5f + shakeY;

	// Arena bounds.
	GL_Color( 0.25f, 0.27f, 0.34f, 1.0f );
	DrawQuad( ox - 120.0f * scale, oy - 120.0f * scale, 240.0f * scale, 240.0f * scale );

	// Boss telegraph: pulse on boss-wave warning events.
	for ( int i = 0; i < arena.fx.eventCount; i++ ) {
		const HfArena::HfArenaFxEvent &ev = arena.fx.events[ i ];
		if ( ev.life <= 0 || ev.type != HfArena::FX_BOSS_WARNING ) continue;
		const float t = ( float )ev.life / ( float )ev.maxLife;
		const float r = ( 1.0f - t ) * 80.0f * scale;
		GL_Color( cb ? 0.95f : 1.0f, cb ? 0.70f : 0.25f, cb ? 0.10f : 0.25f, t * 0.25f );
		DrawQuad( ox - r, oy - r, r * 2.0f, 3.0f );
		DrawQuad( ox - r, oy + r - 3.0f, r * 2.0f, 3.0f );
		DrawQuad( ox - r, oy - r, 3.0f, r * 2.0f );
		DrawQuad( ox + r - 3.0f, oy - r, 3.0f, r * 2.0f );
	}

	// Player, then a short facing pip so aim direction is visible.
	GL_Color( 1.0f, 0.75f, 0.15f, 1.0f );
	DrawQuad( ox + arena.playerX * scale - 3.0f, oy + arena.playerY * scale - 3.0f, 6.0f, 6.0f );
	DrawQuad( ox + ( arena.playerX + arena.facingX * 7.0f ) * scale - 1.5f,
			  oy + ( arena.playerY + arena.facingY * 7.0f ) * scale - 1.5f, 3.0f, 3.0f );

	// Dodge i-frames are drawn as a larger translucent shell.
	if ( arena.dodgeIFrames > 0 ) {
		GL_Color( 0.35f, 0.85f, 1.0f, 0.45f );
		DrawQuad( ox + arena.playerX * scale - 5.0f, oy + arena.playerY * scale - 5.0f, 10.0f, 10.0f );
	}

	for ( int i = 0; i < arena.enemyCount; i++ ) {
		const HfArenaEnemy &e = arena.enemies[ i ];
		float r, g, b;
		EnemyColor( e.kind, cb, r, g, b );
		GL_Color( r, g, b, 1.0f );
		const float size = ( e.kind == HF_ARENA_ENEMY_BOSS ) ? 9.0f
			: ( e.kind == HF_ARENA_ENEMY_BRUTE ) ? 6.0f : 5.0f;
		DrawQuad( ox + e.x * scale - size * 0.5f, oy + e.y * scale - size * 0.5f, size, size );
	}

	for ( int i = 0; i < arena.bulletCount; i++ ) {
		const HfArenaBullet &b = arena.bullets[ i ];
		if ( b.team == HF_ARENA_BULLET_ENEMY ) {
			GL_Color( cb ? 0.95f : 1.0f, cb ? 0.55f : 0.45f, cb ? 0.10f : 0.75f, 1.0f );
		} else {
			GL_Color( 0.95f, 0.95f, 0.65f, 1.0f );
		}
		DrawQuad( ox + b.x * scale - 1.0f, oy + b.y * scale - 1.0f, 2.0f, 2.0f );
	}

	for ( int i = 0; i < arena.pickupCount; i++ ) {
		const HfArenaPickup &p = arena.pickups[ i ];
		float r, g, b;
		PickupColor( p.kind, cb, r, g, b );
		GL_Color( r, g, b, 1.0f );
		DrawQuad( ox + p.x * scale - 2.0f, oy + p.y * scale - 2.0f, 4.0f, 4.0f );
	}

	// FX events: muzzle flashes, hits, pickups, dodge/wave markers.
	for ( int i = 0; i < arena.fx.eventCount; i++ ) {
		const HfArena::HfArenaFxEvent &ev = arena.fx.events[ i ];
		if ( ev.life <= 0 ) continue;
		const float t = ( float )ev.life / ( float )ev.maxLife;
		const float px = ox + ev.x * scale;
		const float py = oy + ev.y * scale;
		if ( ev.type == HfArena::FX_MUZZLE ) {
			GL_Color( 1.0f, 0.95f, 0.65f, t * 0.9f );
			DrawQuad( px - 2.0f, py - 2.0f, 4.0f, 4.0f );
		} else if ( ev.type == HfArena::FX_HIT ) {
			GL_Color( 1.0f, cb ? 0.80f : 0.55f, cb ? 0.20f : 0.25f, t * 0.8f );
			DrawQuad( px - 2.5f, py - 0.5f, 5.0f, 1.0f );
			DrawQuad( px - 0.5f, py - 2.5f, 1.0f, 5.0f );
		} else if ( ev.type == HfArena::FX_PICKUP ) {
			GL_Color( cb ? 0.20f : 0.30f, cb ? 0.85f : 0.95f, cb ? 0.95f : 0.45f, t * 0.5f );
			DrawQuad( px - 4.0f, py - 4.0f, 8.0f, 8.0f );
		} else if ( ev.type == HfArena::FX_DODGE ) {
			GL_Color( 0.35f, 0.85f, 1.0f, t * 0.35f );
			DrawQuad( px - 6.0f, py - 6.0f, 12.0f, 12.0f );
		} else if ( ev.type == HfArena::FX_WAVE ) {
			GL_Color( 1.0f, 1.0f, 1.0f, t * 0.15f );
			DrawQuad( ox - 120.0f * scale, oy - 120.0f * scale, 240.0f * scale, 240.0f * scale );
		}
	}

	// Particles.
	for ( int i = 0; i < arena.fx.particleCount; i++ ) {
		const HfArena::HfArenaFxParticle &p = arena.fx.particles[ i ];
		if ( p.life <= 0 ) continue;
		const float t = ( float )p.life / ( float )p.maxLife;
		float r = 1.0f, g = 1.0f, b = 1.0f;
		if ( p.kind == HfArena::FX_HURT ) { r = 1.0f; g = cb ? 0.75f : 0.30f; b = cb ? 0.20f : 0.30f; }
		else if ( p.kind == HfArena::FX_DODGE ) { r = 0.35f; g = 0.85f; b = 1.0f; }
		else if ( p.kind == HfArena::FX_PICKUP || p.kind == HF_ARENA_PICKUP_HEALTH || p.kind == HF_ARENA_PICKUP_AMMO || p.kind == HF_ARENA_PICKUP_SCORE ) { PickupColor( p.kind == HfArena::FX_PICKUP ? HF_ARENA_PICKUP_AMMO : p.kind, cb, r, g, b ); }
		else { EnemyColor( p.kind, cb, r, g, b ); }
		GL_Color( r, g, b, t * 0.8f );
		DrawQuad( ox + p.x * scale - 1.5f, oy + p.y * scale - 1.5f, 3.0f, 3.0f );
	}

	// Damage vignette.
	const float vig = HfArena::HfArenaFxState::VignetteAlpha( arena.fx.damageFlash );
	if ( vig > 0.0f ) {
		GL_Color( cb ? 0.90f : 0.80f, cb ? 0.55f : 0.10f, cb ? 0.10f : 0.10f, vig * 0.30f );
		DrawQuad( 0.0f, 0.0f, viewportW, 24.0f );
		DrawQuad( 0.0f, viewportH - 24.0f, viewportW, 24.0f );
		DrawQuad( 0.0f, 0.0f, 24.0f, viewportH );
		DrawQuad( viewportW - 24.0f, 0.0f, 24.0f, viewportH );
	}
}

void hfUiDrawPass::SetSource( HF_Ecs::Registry *registry, HF_Ui::hfUiSystem *uiSystem ) {
	s_registry = registry;
	s_system = uiSystem;
}

void hfUiDrawPass::ClearSource() {
	s_registry = NULL;
	s_system = NULL;
	s_arena = NULL;
	s_gameplayOverlayEnabled = false;
}

bool hfUiDrawPass::HasSource() {
	return s_registry != NULL && s_system != NULL;
}

bool hfUiDrawPass::ShouldDrawGameplayOverlay() {
	return GameplayOverlayGate( s_gameplayOverlayEnabled, s_arena != NULL, HasSource() );
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

	// Gameplay layer first so the HUD drawn from the registry stays on top.
	if ( ShouldDrawGameplayOverlay() ) {
		DrawArenaOverlay( *s_arena, ( float )renderSystem->GetWidth(), ( float )renderSystem->GetHeight() );
	}
	
	for ( int d = 0; d < drawCount; d++ ) {
		HF_Ecs::Entity ent = uiSystem.GetDrawEntity( d );
		if ( ent == HF_Ecs::ENTITY_NULL ) continue;
		
		const HF_Ui::hfUiCanvasComponent *canvas = registry.GetComponent< HF_Ui::hfUiCanvasComponent >( ent );
		if ( !canvas || !canvas->visible ) continue;
		
		HF_Ui::hfUiRect rect = uiSystem.GetRect( ent );
		HF_Ui::hfUiRect visible = uiSystem.GetVisible( ent );
		
		if ( visible.Empty() ) continue;
		
		// Panel background.  borderWidth/cornerRadius turn the flat quad into
		// a nine-patch-style rounded panel without any texture resources.
		const HF_Ui::hfUiPanelComponent *panel = registry.GetComponent< HF_Ui::hfUiPanelComponent >( ent );
		if ( panel && panel->a > 0.0f ) {
			const float border = panel->borderWidth > 0.0f ? panel->borderWidth : 0.0f;
			if ( border > 0.0f && panel->borderA > 0.0f ) {
				DrawRoundedFill( visible.x, visible.y, visible.w, visible.h, panel->cornerRadius,
								 panel->borderR, panel->borderG, panel->borderB, panel->borderA );
				float inset = border;
				const float maxInset = ( visible.w < visible.h ? visible.w : visible.h ) * 0.5f;
				if ( inset > maxInset ) { inset = maxInset; }
				const float innerRadius = panel->cornerRadius > inset ? panel->cornerRadius - inset : 0.0f;
				DrawRoundedFill( visible.x + inset, visible.y + inset,
								 visible.w - inset * 2.0f, visible.h - inset * 2.0f,
								 innerRadius, panel->r, panel->g, panel->b, panel->a );
			} else {
				DrawRoundedFill( visible.x, visible.y, visible.w, visible.h, panel->cornerRadius,
								 panel->r, panel->g, panel->b, panel->a );
			}
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
		
		// Focus ring.  Kept in hfUiFocusComponent by hfUiInput::ApplyFocus so
		// the draw pass stays a pure consumer of widget state.
		const HF_Ui::hfUiFocusComponent *focus = registry.GetComponent< HF_Ui::hfUiFocusComponent >( ent );
		if ( focus != NULL && focus->focused ) {
			const float t = focus->thickness > 0.0f ? focus->thickness : 1.0f;
			GL_Color( focus->r, focus->g, focus->b, focus->a );
			qglBegin( GL_QUADS );
			// top
			qglVertex2f( visible.x, visible.y );
			qglVertex2f( visible.x + visible.w, visible.y );
			qglVertex2f( visible.x + visible.w, visible.y + t );
			qglVertex2f( visible.x, visible.y + t );
			// bottom
			qglVertex2f( visible.x, visible.y + visible.h - t );
			qglVertex2f( visible.x + visible.w, visible.y + visible.h - t );
			qglVertex2f( visible.x + visible.w, visible.y + visible.h );
			qglVertex2f( visible.x, visible.y + visible.h );
			// left
			qglVertex2f( visible.x, visible.y );
			qglVertex2f( visible.x + t, visible.y );
			qglVertex2f( visible.x + t, visible.y + visible.h );
			qglVertex2f( visible.x, visible.y + visible.h );
			// right
			qglVertex2f( visible.x + visible.w - t, visible.y );
			qglVertex2f( visible.x + visible.w, visible.y );
			qglVertex2f( visible.x + visible.w, visible.y + visible.h );
			qglVertex2f( visible.x + visible.w - t, visible.y + visible.h );
			qglEnd();
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
