#ifndef __HF_UI_CANVAS_COMPONENT_H__
#define __HF_UI_CANVAS_COMPONENT_H__

/*
===============================================================================

	Phase 9 — canvas components.

	The data half of the Control/Canvas-class UI. A control is a rectangle
	placed inside its parent by an anchor (Unity/Godot style): the anchors
	say which fractions of the parent the rect is tied to, and the offsets
	say how far from those anchors it sits. A control that also has a layout
	mode (row / column / grid) imposes a flow on its children instead of
	letting them anchor individually, with scroll and clipping applied to the
	children as a group.

	All header-only and registry-based, so the layout runs headlessly and is
	validated by the harness. The companion hfUiSystem.h does the solving.

===============================================================================
*/

#include "../hfprecompiled.h"
#include "../hfEcs.h"

namespace HF_Ui {

// --- geometry -------------------------------------------------------------

struct hfUiRect {
	float	x, y, w, h;

	HF_INLINE hfUiRect() : x( 0 ), y( 0 ), w( 0 ), h( 0 ) {}
	HF_INLINE hfUiRect( float X, float Y, float W, float H ) : x( X ), y( Y ), w( W ), h( H ) {}

	HF_INLINE float	Right() const { return x + w; }
	HF_INLINE float	Bottom() const { return y + h; }
	HF_INLINE bool	Empty() const { return w <= 0.0f || h <= 0.0f; }
	HF_INLINE bool	Contains( float px, float py ) const {
		return px >= x && px <= x + w && py >= y && py <= y + h;
	}
	HF_INLINE hfUiRect Intersect( const hfUiRect &o ) const {
		float nx = x > o.x ? x : o.x;
		float ny = y > o.y ? y : o.y;
		float nr = Right()  < o.Right()  ? Right()  : o.Right();
		float nb = Bottom() < o.Bottom() ? Bottom() : o.Bottom();
		if ( nr < nx ) { nr = nx; }
		if ( nb < ny ) { nb = ny; }
		return hfUiRect( nx, ny, nr - nx, nb - ny );
	}
	HF_INLINE hfUiRect Offset( float dx, float dy ) const {
		return hfUiRect( x + dx, y + dy, w, h );
	}
};

struct hfUiMargin {
	float	left, top, right, bottom;

	HF_INLINE hfUiMargin() : left( 0.0f ), top( 0.0f ), right( 0.0f ), bottom( 0.0f ) {}
	HF_INLINE static hfUiMargin All( float v ) {
		hfUiMargin m; m.left = m.top = m.right = m.bottom = v; return m;
	}
};

// --- enums ----------------------------------------------------------------

enum hfUiLayoutMode {
	HF_UI_LAYOUT_NONE = 0,
	HF_UI_LAYOUT_ROW,
	HF_UI_LAYOUT_COLUMN,
	HF_UI_LAYOUT_GRID
};

enum hfUiAnchorPreset {
	HF_UI_ANCHOR_TOP_LEFT = 0,
	HF_UI_ANCHOR_TOP_CENTER,
	HF_UI_ANCHOR_TOP_RIGHT,
	HF_UI_ANCHOR_MIDDLE_LEFT,
	HF_UI_ANCHOR_CENTER,
	HF_UI_ANCHOR_MIDDLE_RIGHT,
	HF_UI_ANCHOR_BOTTOM_LEFT,
	HF_UI_ANCHOR_BOTTOM_CENTER,
	HF_UI_ANCHOR_BOTTOM_RIGHT,
	HF_UI_ANCHOR_LEFT_WIDE,
	HF_UI_ANCHOR_TOP_WIDE,
	HF_UI_ANCHOR_RIGHT_WIDE,
	HF_UI_ANCHOR_BOTTOM_WIDE,
	HF_UI_ANCHOR_FULL_RECT,
	HF_UI_ANCHOR_PRESET_COUNT
};

enum hfUiTextAlign {
	HF_UI_TEXT_LEFT = 0,
	HF_UI_TEXT_CENTER,
	HF_UI_TEXT_RIGHT
};

// --- anchor ---------------------------------------------------------------

struct hfUiAnchor {
	hfVec2	anchorMin;		// fraction of the parent rect the (left, top) edge is tied to
	hfVec2	anchorMax;		// fraction of the parent rect the (right, bottom) edge is tied to
	hfVec2	offsetMin;		// pixel offset added to the anchorMin corner
	hfVec2	offsetMax;		// pixel offset added to the anchorMax corner

	HF_INLINE hfUiAnchor() {
		anchorMin.Set( 0.0f, 0.0f );
		anchorMax.Set( 0.0f, 0.0f );
		offsetMin.Set( 0.0f, 0.0f );
		offsetMax.Set( 0.0f, 0.0f );
	}

	HF_INLINE void SetPreset( int preset ) {
		anchorMin.Set( 0.0f, 0.0f );
		anchorMax.Set( 0.0f, 0.0f );
		offsetMin.Set( 0.0f, 0.0f );
		offsetMax.Set( 0.0f, 0.0f );
		switch ( preset ) {
			case HF_UI_ANCHOR_TOP_CENTER:	 anchorMin.Set( 0.5f, 0.0f ); anchorMax.Set( 0.5f, 0.0f ); break;
			case HF_UI_ANCHOR_TOP_RIGHT:	 anchorMin.Set( 1.0f, 0.0f ); anchorMax.Set( 1.0f, 0.0f ); break;
			case HF_UI_ANCHOR_MIDDLE_LEFT:	 anchorMin.Set( 0.0f, 0.5f ); anchorMax.Set( 0.0f, 0.5f ); break;
			case HF_UI_ANCHOR_CENTER:	 anchorMin.Set( 0.5f, 0.5f ); anchorMax.Set( 0.5f, 0.5f ); break;
			case HF_UI_ANCHOR_MIDDLE_RIGHT:	anchorMin.Set( 1.0f, 0.5f ); anchorMax.Set( 1.0f, 0.5f ); break;
			case HF_UI_ANCHOR_BOTTOM_LEFT:	anchorMin.Set( 0.0f, 1.0f ); anchorMax.Set( 0.0f, 1.0f ); break;
			case HF_UI_ANCHOR_BOTTOM_CENTER:anchorMin.Set( 0.5f, 1.0f ); anchorMax.Set( 0.5f, 1.0f ); break;
			case HF_UI_ANCHOR_BOTTOM_RIGHT:	anchorMin.Set( 1.0f, 1.0f ); anchorMax.Set( 1.0f, 1.0f ); break;
			case HF_UI_ANCHOR_LEFT_WIDE:	anchorMin.Set( 0.0f, 0.0f ); anchorMax.Set( 0.0f, 1.0f ); break;
			case HF_UI_ANCHOR_TOP_WIDE:		anchorMin.Set( 0.0f, 0.0f ); anchorMax.Set( 1.0f, 0.0f ); break;
			case HF_UI_ANCHOR_RIGHT_WIDE:	anchorMin.Set( 1.0f, 0.0f ); anchorMax.Set( 1.0f, 1.0f ); break;
			case HF_UI_ANCHOR_BOTTOM_WIDE:	anchorMin.Set( 0.0f, 1.0f ); anchorMax.Set( 1.0f, 1.0f ); break;
			case HF_UI_ANCHOR_FULL_RECT:	anchorMin.Set( 0.0f, 0.0f ); anchorMax.Set( 1.0f, 1.0f ); break;
			case HF_UI_ANCHOR_TOP_LEFT:
			default:						break;
		}
	}

	// Convenience: a fixed-size rect whose top-left sits (x, y) from the parent's top-left.
	HF_INLINE void SetFixed( float x, float y, float w, float h ) {
		anchorMin.Set( 0.0f, 0.0f );
		anchorMax.Set( 0.0f, 0.0f );
		offsetMin.Set( x, y );
		offsetMax.Set( x + w, y + h );
	}

	// Resolve the rect inside a parent rect. When anchorMin == anchorMax the
	// rect has size (offsetMax - offsetMin); otherwise the offsets are insets
	// from the two anchored corners (a stretched rect).
	HF_INLINE hfUiRect Resolve( const hfUiRect &parent ) const {
		float left   = parent.x + parent.w * anchorMin.x + offsetMin.x;
		float top    = parent.y + parent.h * anchorMin.y + offsetMin.y;
		float right  = parent.x + parent.w * anchorMax.x + offsetMax.x;
		float bottom = parent.y + parent.h * anchorMax.y + offsetMax.y;
		return hfUiRect( left, top, right - left, bottom - top );
	}
};

// --- canvas component (an element, optionally a container) ----------------

struct hfUiCanvasComponent {
	HF_Ecs::Entity	parent;			// ENTITY_NULL for a root control
	hfUiAnchor		anchor;
	hfUiLayoutMode	layout;
	hfUiMargin		padding;
	float			spacing;		// gap between laid-out children
	int				gridColumns;	// cell count per row for HF_UI_LAYOUT_GRID
	float			scrollX;		// content offset applied to children
	float			scrollY;
	hfVec2			minSize;		// size requested of a parent container
	bool			clipChildren;	// clip children to this rect
	bool			visible;
	bool			interactive;	// participates in hit-testing
	int				order;			// sibling order inside a container (ties: entity)
	int				zIndex;			// draw/hit priority, higher is on top

	HF_INLINE hfUiCanvasComponent() {
		parent       = HF_Ecs::ENTITY_NULL;
		layout       = HF_UI_LAYOUT_NONE;
		spacing      = 0.0f;
		gridColumns  = 1;
		scrollX      = 0.0f;
		scrollY      = 0.0f;
		minSize.Set( 0.0f, 0.0f );
		clipChildren = false;
		visible      = true;
		interactive  = false;
		order        = 0;
		zIndex       = 0;
	}
};

// --- label component ------------------------------------------------------

struct hfUiLabelComponent {
	char	text[ 128 ];
	float	scale;			// pixel scale multiplier applied to font metrics
	float	r, g, b, a;
	int		align;
	bool	wrap;
	bool	visible;

	// resolved every layout pass (see hfUiSystem):
	int		lineCount;
	float	textWidth;
	float	textHeight;

	HF_INLINE hfUiLabelComponent() {
		text[ 0 ] = '\0';
		scale = 1.0f;
		r = g = b = a = 1.0f;
		align = HF_UI_TEXT_LEFT;
		wrap = false;
		visible = true;
		lineCount = 0;
		textWidth = 0.0f;
		textHeight = 0.0f;
	}

	HF_INLINE void SetText( const char *s ) {
		int i = 0;
		if ( s ) {
			for ( ; s[ i ] != '\0' && i < ( int )sizeof( text ) - 1; i++ ) { text[ i ] = s[ i ]; }
		}
		text[ i ] = '\0';
	}

	HF_INLINE int Length() const {
		int i = 0;
		while ( text[ i ] != '\0' ) { i++; }
		return i;
	}
};

} // namespace HF_Ui

#endif