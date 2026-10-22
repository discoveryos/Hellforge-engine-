#ifndef __HF_UI_SYSTEM_H__
#define __HF_UI_SYSTEM_H__

/*
===============================================================================

	Phase 9 — hfUiSystem.

	Resolves the canvas component graph into concrete screen rectangles every
	layout pass:

	  1. gather every hfUiCanvasComponent and its depth in the parent chain
	  2. resolve each control's rect from its parent (anchor) — roots anchor in
	     the viewport — then subtract the parent's scroll
	  3. containers (row / column / grid) place their children in flow order
	  4. clip each control to its parent's clip region (and to its own rect
	     when clipChildren is set)
	  5. measure / wrap labels against their resolved width
	  6. build a draw order (zIndex, then depth, then entity) and answer
	     hit-tests against it

	It touches only the registry, so it is GPU-free and runs in the harness.
	A renderer later consumes GetDraw*() / GetRect() to emit quads.

===============================================================================
*/

#include "../hfprecompiled.h"
#include "hfCanvasComponent.h"
#include "hfBitmapFont.h"
#include "../scene/hfSystem.h"

namespace HF_Ui {

static const int HF_UI_MAX_CONTROLS = 512;
static const int HF_UI_MAX_WRAP_LINES = 64;

struct hfUiControlState {
	HF_Ecs::Entity	entity;
	hfUiRect		rect;			// resolved screen bounds
	hfUiRect		clip;			// region this control clips its children to
	hfUiRect		visible;		// rect ∩ parent clip (what is actually shown)
	int				depth;			// distance from a root control
	int				zIndex;			// draw / hit priority (higher on top)
	bool			rectSet;		// true once a container has placed this control
	bool			interactive;
	bool			visibleFlag;

	HF_INLINE hfUiControlState() {
		entity = HF_Ecs::ENTITY_NULL;
		depth = 0;
		zIndex = 0;
		rectSet = false;
		interactive = false;
		visibleFlag = true;
	}
};

class hfUiSystem : public HF_Scene::hfSystem {
public:
	hfUiSystem() : m_count( 0 ), m_drawCount( 0 ) {
		m_viewport = hfUiRect( 0.0f, 0.0f, 1280.0f, 720.0f );
	}

	virtual const char *	GetName() const { return "UiSystem"; }
	virtual int				GetPriority() const { return 90; }
	virtual void			OnRegister( HF_Ecs::Registry &registry ) { ( void )registry; }

	HF_INLINE void			SetViewport( float w, float h ) { m_viewport = hfUiRect( 0.0f, 0.0f, w, h ); }
	HF_INLINE const hfUiRect &	GetViewport() const { return m_viewport; }

	virtual void			Update( HF_Ecs::Registry &registry, float dt ) {
		( void )dt;
		Gather( registry );
		SortByDepth();
		ResolveLayout( registry );
		MeasureText( registry );
		BuildDrawOrder();
	}

	// --- queries used by tests and by a renderer --------------------------

	HF_INLINE int			GetControlCount() const { return m_count; }
	HF_INLINE int			GetDrawCount() const { return m_drawCount; }

	HF_INLINE int			FindControl( HF_Ecs::Entity e ) const {
		for ( int i = 0; i < m_count; i++ ) { if ( m_controls[ i ].entity == e ) { return i; } }
		return -1;
	}

	HF_INLINE hfUiRect		GetRect( HF_Ecs::Entity e ) const {
		int i = FindControl( e );
		return i >= 0 ? m_controls[ i ].rect : hfUiRect();
	}
	HF_INLINE hfUiRect		GetClip( HF_Ecs::Entity e ) const {
		int i = FindControl( e );
		return i >= 0 ? m_controls[ i ].clip : hfUiRect();
	}
	HF_INLINE hfUiRect		GetVisible( HF_Ecs::Entity e ) const {
		int i = FindControl( e );
		return i >= 0 ? m_controls[ i ].visible : hfUiRect();
	}
	HF_INLINE HF_Ecs::Entity	GetDrawEntity( int i ) const {
		if ( i < 0 || i >= m_drawCount ) { return HF_Ecs::ENTITY_NULL; }
		return m_controls[ m_draw[ i ] ].entity;
	}

	// Top-most interactive control containing the point, or ENTITY_NULL.
	HF_INLINE HF_Ecs::Entity	HitTest( float x, float y ) const {
		for ( int d = m_drawCount - 1; d >= 0; d-- ) {
			const hfUiControlState &s = m_controls[ m_draw[ d ] ];
			if ( !s.visibleFlag || !s.interactive ) { continue; }
			if ( s.visible.Contains( x, y ) ) { return s.entity; }
		}
		return HF_Ecs::ENTITY_NULL;
	}

private:
	void					Gather( HF_Ecs::Registry &registry ) {
		m_count = 0;
		registry.Each< hfUiCanvasComponent >(
			[ & ]( HF_Ecs::Entity entity, hfUiCanvasComponent &c ) {
				if ( m_count >= HF_UI_MAX_CONTROLS ) { return; }
				hfUiControlState &s = m_controls[ m_count++ ];
				s.entity = entity;
				s.rectSet = false;
				s.interactive = c.interactive;
				s.visibleFlag = c.visible;
				s.zIndex = c.zIndex;
				s.depth = 0;
				HF_Ecs::Entity p = c.parent;
				int guard = 0;
				while ( p != HF_Ecs::ENTITY_NULL && guard < HF_UI_MAX_CONTROLS ) {
					hfUiCanvasComponent *pc = registry.GetComponent< hfUiCanvasComponent >( p );
					if ( !pc ) { break; }
					s.depth++;
					p = pc->parent;
					guard++;
				}
			} );
	}

	void					SortByDepth() {
		for ( int i = 1; i < m_count; i++ ) {
			hfUiControlState tmp = m_controls[ i ];
			int j = i - 1;
			while ( j >= 0 && m_controls[ j ].depth > tmp.depth ) {
				m_controls[ j + 1 ] = m_controls[ j ];
				j--;
			}
			m_controls[ j + 1 ] = tmp;
		}
	}

	void					ResolveLayout( HF_Ecs::Registry &registry ) {
		for ( int i = 0; i < m_count; i++ ) {
			hfUiControlState &s = m_controls[ i ];
			hfUiCanvasComponent *c = registry.GetComponent< hfUiCanvasComponent >( s.entity );
			if ( !c ) { continue; }

			hfUiCanvasComponent *pc = NULL;
			int parentIdx = -1;
			if ( c->parent != HF_Ecs::ENTITY_NULL ) {
				pc = registry.GetComponent< hfUiCanvasComponent >( c->parent );
				parentIdx = FindControl( c->parent );
			}
			hfUiRect parentRect = pc ? m_controls[ parentIdx ].rect : m_viewport;
			hfUiRect parentClip = pc ? m_controls[ parentIdx ].clip : m_viewport;

			if ( !s.rectSet ) {
				s.rect = c->anchor.Resolve( parentRect );
			}
			if ( pc ) {
				s.rect.x -= pc->scrollX;
				s.rect.y -= pc->scrollY;
			}
			s.visible = s.rect.Intersect( parentClip );
			s.clip = c->clipChildren ? parentClip.Intersect( s.rect ) : parentClip;

			if ( c->layout != HF_UI_LAYOUT_NONE ) {
				PlaceChildren( registry, i, c );
			}
		}
	}

	void					PlaceChildren( HF_Ecs::Registry &registry, int parentIdx, hfUiCanvasComponent *container ) {
		hfUiControlState &ps = m_controls[ parentIdx ];
		hfUiRect content(
			ps.rect.x + container->padding.left,
			ps.rect.y + container->padding.top,
			ps.rect.w - container->padding.left - container->padding.right,
			ps.rect.h - container->padding.top - container->padding.bottom );
		if ( content.w < 0.0f ) { content.w = 0.0f; }
		if ( content.h < 0.0f ) { content.h = 0.0f; }

		int child[ HF_UI_MAX_CONTROLS ];
		int n = 0;
		for ( int i = 0; i < m_count && n < HF_UI_MAX_CONTROLS; i++ ) {
			if ( m_controls[ i ].entity == ps.entity ) { continue; }
			hfUiCanvasComponent *cc = registry.GetComponent< hfUiCanvasComponent >( m_controls[ i ].entity );
			if ( cc && cc->parent == ps.entity ) { child[ n++ ] = i; }
		}
		// insertion sort by (order, entity) for deterministic flow
		for ( int a = 1; a < n; a++ ) {
			int key = child[ a ];
			hfUiCanvasComponent *kc = registry.GetComponent< hfUiCanvasComponent >( m_controls[ key ].entity );
			int b = a - 1;
			while ( b >= 0 ) {
				hfUiCanvasComponent *bc = registry.GetComponent< hfUiCanvasComponent >( m_controls[ child[ b ] ].entity );
				bool greater = bc->order > kc->order ||
					( bc->order == kc->order && m_controls[ child[ b ] ].entity > m_controls[ key ].entity );
				if ( !greater ) { break; }
				child[ b + 1 ] = child[ b ];
				b--;
			}
			child[ b + 1 ] = key;
		}

		if ( container->layout == HF_UI_LAYOUT_ROW ) {
			float cx = content.x;
			for ( int k = 0; k < n; k++ ) {
				hfUiCanvasComponent *cc = registry.GetComponent< hfUiCanvasComponent >( m_controls[ child[ k ] ].entity );
				float w = cc->minSize.x > 0.0f ? cc->minSize.x : 0.0f;
				float h = cc->minSize.y > 0.0f ? cc->minSize.y : content.h;
				SetChildRect( child[ k ], hfUiRect( cx, content.y, w, h ) );
				cx += w + container->spacing;
			}
		} else if ( container->layout == HF_UI_LAYOUT_COLUMN ) {
			float cy = content.y;
			for ( int k = 0; k < n; k++ ) {
				hfUiCanvasComponent *cc = registry.GetComponent< hfUiCanvasComponent >( m_controls[ child[ k ] ].entity );
				float w = cc->minSize.x > 0.0f ? cc->minSize.x : content.w;
				float h = cc->minSize.y > 0.0f ? cc->minSize.y : 0.0f;
				SetChildRect( child[ k ], hfUiRect( content.x, cy, w, h ) );
				cy += h + container->spacing;
			}
		} else if ( container->layout == HF_UI_LAYOUT_GRID ) {
			int cols = container->gridColumns > 0 ? container->gridColumns : 1;
			float cellW = ( content.w - container->spacing * ( float )( cols - 1 ) ) / ( float )cols;
			if ( cellW < 0.0f ) { cellW = 0.0f; }

			// per-row height = tallest child in that row
			float rowH[ HF_UI_MAX_CONTROLS ];
			int rows = ( n + cols - 1 ) / cols;
			for ( int r = 0; r < rows; r++ ) { rowH[ r ] = 0.0f; }
			for ( int k = 0; k < n; k++ ) {
				hfUiCanvasComponent *cc = registry.GetComponent< hfUiCanvasComponent >( m_controls[ child[ k ] ].entity );
				int r = k / cols;
				if ( cc->minSize.y > rowH[ r ] ) { rowH[ r ] = cc->minSize.y; }
			}
			float y = content.y;
			for ( int r = 0; r < rows; r++ ) {
				for ( int cIdx = 0; cIdx < cols; cIdx++ ) {
					int k = r * cols + cIdx;
					if ( k >= n ) { break; }
					float x = content.x + ( float )cIdx * ( cellW + container->spacing );
					SetChildRect( child[ k ], hfUiRect( x, y, cellW, rowH[ r ] ) );
				}
				y += rowH[ r ] + container->spacing;
			}
		}
	}

	HF_INLINE void			SetChildRect( int idx, const hfUiRect &r ) {
		m_controls[ idx ].rect = r;
		m_controls[ idx ].rectSet = true;
	}

	void					MeasureText( HF_Ecs::Registry &registry ) {
		registry.Each< hfUiLabelComponent >(
			[ & ]( HF_Ecs::Entity entity, hfUiLabelComponent &lab ) {
				if ( !lab.visible ) { lab.lineCount = 0; lab.textWidth = 0.0f; lab.textHeight = 0.0f; return; }
				int idx = FindControl( entity );
				if ( idx < 0 ) { lab.lineCount = 0; lab.textWidth = 0.0f; lab.textHeight = 0.0f; return; }
				float s = lab.scale > 0.0f ? lab.scale : 1.0f;
				if ( lab.wrap ) {
					int maxW = ( int )( m_controls[ idx ].rect.w / s );
					if ( maxW < 1 ) { maxW = 1; }
					hfUiTextLine lines[ HF_UI_MAX_WRAP_LINES ];
					int lc = hfBitmapFont::WrapText( lab.text, maxW, lines, HF_UI_MAX_WRAP_LINES );
					lab.lineCount = lc;
					int widest = 0;
					for ( int i = 0; i < lc && i < HF_UI_MAX_WRAP_LINES; i++ ) {
						if ( lines[ i ].width > widest ) { widest = lines[ i ].width; }
					}
					lab.textWidth = ( float )widest * s;
				} else {
					lab.textWidth = ( float )hfBitmapFont::MeasureText( lab.text ) * s;
					int lc = 1;
					for ( int i = 0; lab.text[ i ] != '\0'; i++ ) { if ( lab.text[ i ] == '\n' ) { lc++; } }
					lab.lineCount = lc;
				}
				lab.textHeight = ( float )lab.lineCount * ( float )HF_UI_LINE_HEIGHT * s;
			} );
	}

	void					BuildDrawOrder() {
		m_drawCount = m_count;
		for ( int i = 0; i < m_count; i++ ) { m_draw[ i ] = i; }
		// stable insertion sort: zIndex asc, then depth asc, then entity asc
		for ( int a = 1; a < m_drawCount; a++ ) {
			int key = m_draw[ a ];
			const hfUiControlState &k = m_controls[ key ];
			int b = a - 1;
			while ( b >= 0 ) {
				const hfUiControlState &o = m_controls[ m_draw[ b ] ];
				bool greater = o.zIndex > k.zIndex ||
					( o.zIndex == k.zIndex && o.depth > k.depth ) ||
					( o.zIndex == k.zIndex && o.depth == k.depth && o.entity > k.entity );
				if ( !greater ) { break; }
				m_draw[ b + 1 ] = m_draw[ b ];
				b--;
			}
			m_draw[ b + 1 ] = key;
		}
	}

	HF_Ecs::Entity		m_draw[ HF_UI_MAX_CONTROLS ];
	hfUiControlState	m_controls[ HF_UI_MAX_CONTROLS ];
	int					m_count;
	int					m_drawCount;
	hfUiRect			m_viewport;
};

} // namespace HF_Ui

#endif