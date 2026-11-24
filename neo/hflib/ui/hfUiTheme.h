#ifndef __HF_UI_THEME_H__
#define __HF_UI_THEME_H__

/*
===============================================================================

	Phase 17 — hfUiTheme.

	A theme is the data a UI is skinned from: a palette plus a couple of
	geometry knobs.  It is deliberately plain data with a text format, because
	a theme is user content — it has to survive a save/load round trip, be
	diffable, and be applied at boot before the first frame.

	Format (one key per line, `#` starts a comment, unknown keys are ignored):

		panel              = 0.12,0.13,0.18,1.0
		button_normal      = 0.20,0.20,0.25,1.0
		panel_border_width = 1.0
		corner_radius      = 4.0
		font_scale         = 1.15
		focus              = 0.95,0.55,0.15,1.0
		focus_width        = 2.0

	Colors are r,g,b,a floats; the geometry keys are floats.

	Apply() walks the registry and rewrites every widget component in place.
	That is the whole point of a theme: a reskin touches no widget code, no
	layout and no event wiring.  Root panels (parent == ENTITY_NULL) take the
	`background` color so a screen backdrop can differ from a dialog panel.

===============================================================================
*/

#include "../hfprecompiled.h"
#include "../hfEcs.h"
#include "hfCanvasComponent.h"
#include "hfUiWidget.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace HF_Ui {

struct hfUiThemeColor {
	float	r, g, b, a;

	HF_INLINE hfUiThemeColor() : r( 0 ), g( 0 ), b( 0 ), a( 1 ) {}
	HF_INLINE hfUiThemeColor( float R, float G, float B, float A = 1.0f ) : r( R ), g( G ), b( B ), a( A ) {}
};

class hfUiTheme {
public:
	hfUiThemeColor	background;
	hfUiThemeColor	panel;
	hfUiThemeColor	panelBorder;
	hfUiThemeColor	text;
	hfUiThemeColor	textDim;
	hfUiThemeColor	accent;

	hfUiThemeColor	buttonNormal;
	hfUiThemeColor	buttonHover;
	hfUiThemeColor	buttonPressed;
	hfUiThemeColor	buttonDisabled;

	hfUiThemeColor	sliderBar;
	hfUiThemeColor	sliderHandle;
	hfUiThemeColor	toggleBox;
	hfUiThemeColor	toggleCheck;
	hfUiThemeColor	focus;

	float			panelBorderWidth;
	float			cornerRadius;
	float			fontScale;
	float			focusWidth;

	hfUiTheme() {
		SetDefaults();
	}

	void SetDefaults() {
		background			= hfUiThemeColor( 0.06f, 0.07f, 0.10f, 1.00f );
		panel				= hfUiThemeColor( 0.12f, 0.13f, 0.18f, 1.00f );
		panelBorder			= hfUiThemeColor( 0.30f, 0.32f, 0.40f, 1.00f );
		text				= hfUiThemeColor( 0.95f, 0.95f, 0.97f, 1.00f );
		textDim				= hfUiThemeColor( 0.55f, 0.58f, 0.65f, 1.00f );
		accent				= hfUiThemeColor( 0.95f, 0.55f, 0.15f, 1.00f );

		buttonNormal		= hfUiThemeColor( 0.20f, 0.20f, 0.25f, 1.00f );
		buttonHover			= hfUiThemeColor( 0.30f, 0.30f, 0.35f, 1.00f );
		buttonPressed		= hfUiThemeColor( 0.15f, 0.15f, 0.20f, 1.00f );
		buttonDisabled		= hfUiThemeColor( 0.15f, 0.15f, 0.15f, 0.50f );

		sliderBar			= hfUiThemeColor( 0.15f, 0.15f, 0.20f, 1.00f );
		sliderHandle		= hfUiThemeColor( 0.80f, 0.80f, 0.80f, 1.00f );
		toggleBox			= hfUiThemeColor( 0.25f, 0.25f, 0.30f, 1.00f );
		toggleCheck			= hfUiThemeColor( 0.20f, 0.80f, 0.40f, 1.00f );
		focus				= hfUiThemeColor( 0.95f, 0.55f, 0.15f, 1.00f );

		panelBorderWidth	= 0.0f;
		cornerRadius		= 0.0f;
		fontScale			= 1.0f;
		focusWidth			= 2.0f;
	}

	// --- text format ------------------------------------------------------

	bool Parse( const char *text ) {
		if ( text == NULL ) { return false; }
		int recognized = 0;
		const char *p = text;
		while ( *p != '\0' ) {
			const char *lineEnd = p;
			while ( *lineEnd != '\0' && *lineEnd != '\n' ) { lineEnd++; }

			char line[ 256 ];
			int len = ( int )( lineEnd - p );
			if ( len > ( int )sizeof( line ) - 1 ) { len = ( int )sizeof( line ) - 1; }
			memcpy( line, p, len );
			line[ len ] = '\0';
			p = ( *lineEnd == '\n' ) ? lineEnd + 1 : lineEnd;

			char *comment = strchr( line, '#' );
			if ( comment != NULL ) { *comment = '\0'; }

			char *eq = strchr( line, '=' );
			if ( eq == NULL ) { continue; }
			*eq = '\0';
			char *key = Trim( line );
			char *value = Trim( eq + 1 );
			if ( *key == '\0' || *value == '\0' ) { continue; }

			if ( ParseColor( key, value ) ) { recognized++; continue; }
			if ( ParseFloat( key, value ) ) { recognized++; continue; }
		}
		return recognized > 0;
	}

	int Serialize( char *out, int maxSize ) const {
		if ( out == NULL || maxSize <= 0 ) { return 0; }
		out[ 0 ] = '\0';
		AppendColor( out, maxSize, "background", background );
		AppendColor( out, maxSize, "panel", panel );
		AppendColor( out, maxSize, "panel_border", panelBorder );
		AppendColor( out, maxSize, "text", text );
		AppendColor( out, maxSize, "text_dim", textDim );
		AppendColor( out, maxSize, "accent", accent );
		AppendColor( out, maxSize, "button_normal", buttonNormal );
		AppendColor( out, maxSize, "button_hover", buttonHover );
		AppendColor( out, maxSize, "button_pressed", buttonPressed );
		AppendColor( out, maxSize, "button_disabled", buttonDisabled );
		AppendColor( out, maxSize, "slider_bar", sliderBar );
		AppendColor( out, maxSize, "slider_handle", sliderHandle );
		AppendColor( out, maxSize, "toggle_box", toggleBox );
		AppendColor( out, maxSize, "toggle_check", toggleCheck );
		AppendColor( out, maxSize, "focus", focus );
		AppendFloat( out, maxSize, "panel_border_width", panelBorderWidth );
		AppendFloat( out, maxSize, "corner_radius", cornerRadius );
		AppendFloat( out, maxSize, "font_scale", fontScale );
		AppendFloat( out, maxSize, "focus_width", focusWidth );
		return ( int )strlen( out );
	}

	bool LoadFile( const char *path ) {
		if ( path == NULL ) { return false; }
		FILE *f = fopen( path, "rb" );
		if ( f == NULL ) { return false; }
		char buffer[ 4096 ];
		const size_t n = fread( buffer, 1, sizeof( buffer ) - 1, f );
		fclose( f );
		buffer[ n ] = '\0';
		return Parse( buffer );
	}

	bool SaveFile( const char *path ) const {
		if ( path == NULL ) { return false; }
		char buffer[ 4096 ];
		Serialize( buffer, sizeof( buffer ) );
		FILE *f = fopen( path, "wb" );
		if ( f == NULL ) { return false; }
		fwrite( buffer, 1, strlen( buffer ), f );
		fclose( f );
		return true;
	}

	// --- reskin -----------------------------------------------------------

	int Apply( HF_Ecs::Registry &registry ) const {
		int touched = 0;

		registry.Each< hfUiPanelComponent >( [ & ]( HF_Ecs::Entity entity, hfUiPanelComponent &p ) {
			const hfUiCanvasComponent *canvas = registry.GetComponent< hfUiCanvasComponent >( entity );
			const bool isRoot = ( canvas == NULL || canvas->parent == HF_Ecs::ENTITY_NULL );
			const hfUiThemeColor &c = isRoot ? background : panel;
			p.r = c.r; p.g = c.g; p.b = c.b; p.a = c.a;
			p.borderR = panelBorder.r;
			p.borderG = panelBorder.g;
			p.borderB = panelBorder.b;
			p.borderA = panelBorder.a;
			p.borderWidth = panelBorderWidth;
			p.cornerRadius = cornerRadius;
			touched++;
		} );

		registry.Each< hfUiFocusComponent >( [ & ]( HF_Ecs::Entity entity, hfUiFocusComponent &f ) {
			( void )entity;
			f.r = focus.r; f.g = focus.g; f.b = focus.b; f.a = focus.a;
			f.thickness = focusWidth > 0.0f ? focusWidth : 1.0f;
			touched++;
		} );

		registry.Each< hfUiButtonComponent >( [ & ]( HF_Ecs::Entity entity, hfUiButtonComponent &b ) {
			( void )entity;
			b.normalR = buttonNormal.r;   b.normalG = buttonNormal.g;   b.normalB = buttonNormal.b;   b.normalA = buttonNormal.a;
			b.hoverR = buttonHover.r;     b.hoverG = buttonHover.g;     b.hoverB = buttonHover.b;     b.hoverA = buttonHover.a;
			b.pressedR = buttonPressed.r; b.pressedG = buttonPressed.g; b.pressedB = buttonPressed.b; b.pressedA = buttonPressed.a;
			b.disabledR = buttonDisabled.r; b.disabledG = buttonDisabled.g; b.disabledB = buttonDisabled.b; b.disabledA = buttonDisabled.a;
			touched++;
		} );

		registry.Each< hfUiSliderComponent >( [ & ]( HF_Ecs::Entity entity, hfUiSliderComponent &s ) {
			( void )entity;
			s.barR = sliderBar.r; s.barG = sliderBar.g; s.barB = sliderBar.b; s.barA = sliderBar.a;
			s.handleR = sliderHandle.r; s.handleG = sliderHandle.g; s.handleB = sliderHandle.b; s.handleA = sliderHandle.a;
			touched++;
		} );

		registry.Each< hfUiToggleComponent >( [ & ]( HF_Ecs::Entity entity, hfUiToggleComponent &t ) {
			( void )entity;
			t.boxR = toggleBox.r; t.boxG = toggleBox.g; t.boxB = toggleBox.b; t.boxA = toggleBox.a;
			t.checkR = toggleCheck.r; t.checkG = toggleCheck.g; t.checkB = toggleCheck.b; t.checkA = toggleCheck.a;
			touched++;
		} );

		registry.Each< hfUiLabelComponent >( [ & ]( HF_Ecs::Entity entity, hfUiLabelComponent &l ) {
			( void )entity;
			// Large labels are headings: use the author's base scale so a
			// global font-scale change does not silently reclassify them.
			const hfUiThemeColor &c = ( l.baseScale >= 2.0f ) ? accent : text;
			l.r = c.r; l.g = c.g; l.b = c.b; l.a = c.a;
			const float base = l.baseScale > 0.0f ? l.baseScale : l.scale;
			l.scale = base * ( fontScale > 0.0f ? fontScale : 1.0f );
			touched++;
		} );

		return touched;
	}

private:
	static char *Trim( char *s ) {
		while ( *s == ' ' || *s == '\t' ) { s++; }
		int len = ( int )strlen( s );
		while ( len > 0 && ( s[ len - 1 ] == ' ' || s[ len - 1 ] == '\t' || s[ len - 1 ] == '\r' ) ) {
			s[ --len ] = '\0';
		}
		return s;
	}

	static void AppendColor( char *out, int maxSize, const char *key, const hfUiThemeColor &c ) {
		const int used = ( int )strlen( out );
		if ( used >= maxSize - 1 ) { return; }
		snprintf( out + used, ( size_t )( maxSize - used ), "%s = %g,%g,%g,%g\n", key, c.r, c.g, c.b, c.a );
	}

	static void AppendFloat( char *out, int maxSize, const char *key, float value ) {
		const int used = ( int )strlen( out );
		if ( used >= maxSize - 1 ) { return; }
		snprintf( out + used, ( size_t )( maxSize - used ), "%s = %g\n", key, value );
	}

	static bool ParseColorValue( const char *value, hfUiThemeColor &out ) {
		float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
		if ( sscanf( value, "%f , %f , %f , %f", &r, &g, &b, &a ) < 3 ) {
			return false;
		}
		out.r = r; out.g = g; out.b = b; out.a = a;
		return true;
	}

	bool ParseColor( const char *key, const char *value ) {
		hfUiThemeColor *target = NULL;
		if ( strcmp( key, "background" ) == 0 ) { target = &background; }
		else if ( strcmp( key, "panel" ) == 0 ) { target = &panel; }
		else if ( strcmp( key, "panel_border" ) == 0 ) { target = &panelBorder; }
		else if ( strcmp( key, "text" ) == 0 ) { target = &text; }
		else if ( strcmp( key, "text_dim" ) == 0 ) { target = &textDim; }
		else if ( strcmp( key, "accent" ) == 0 ) { target = &accent; }
		else if ( strcmp( key, "button_normal" ) == 0 ) { target = &buttonNormal; }
		else if ( strcmp( key, "button_hover" ) == 0 ) { target = &buttonHover; }
		else if ( strcmp( key, "button_pressed" ) == 0 ) { target = &buttonPressed; }
		else if ( strcmp( key, "button_disabled" ) == 0 ) { target = &buttonDisabled; }
		else if ( strcmp( key, "slider_bar" ) == 0 ) { target = &sliderBar; }
		else if ( strcmp( key, "slider_handle" ) == 0 ) { target = &sliderHandle; }
		else if ( strcmp( key, "toggle_box" ) == 0 ) { target = &toggleBox; }
		else if ( strcmp( key, "toggle_check" ) == 0 ) { target = &toggleCheck; }
		else if ( strcmp( key, "focus" ) == 0 ) { target = &focus; }
		if ( target == NULL ) { return false; }
		return ParseColorValue( value, *target );
	}

	bool ParseFloat( const char *key, const char *value ) {
		if ( strcmp( key, "panel_border_width" ) == 0 ) {
			panelBorderWidth = ( float )atof( value );
			return true;
		}
		if ( strcmp( key, "corner_radius" ) == 0 ) {
			cornerRadius = ( float )atof( value );
			return true;
		}
		if ( strcmp( key, "font_scale" ) == 0 ) {
			fontScale = ( float )atof( value );
			return true;
		}
		if ( strcmp( key, "focus_width" ) == 0 ) {
			focusWidth = ( float )atof( value );
			return true;
		}
		return false;
	}
};

} // namespace HF_Ui

#endif
