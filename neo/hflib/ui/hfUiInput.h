#ifndef __HF_UI_INPUT_H__
#define __HF_UI_INPUT_H__

/*
===============================================================================

	hfUiInput — Phase 14 input mapping for UI

	Converts mouse, keyboard and gamepad state into UI events via:
	- hfUiSystem::HitTest() for mouse over/press/release/click
	- focus tracking (click to focus, tab to cycle - later)
	- button state transitions for press/release/click detection

	Usage:
	  hfUiInput input; input.SetViewport(...);
	  input.Update( registry, uiSystem, eventBus, mouseX, mouseY, buttons, pressed );
	  or call individual methods.

	Headless friendly - takes abstract input state, no windowing dependency.

===============================================================================
*/

#include "../hfprecompiled.h"
#include "../hfEcs.h"
#include "hfUiEvents.h"
#include "hfUiWidget.h"

namespace HF_Ui {

class hfUiSystem;

class hfUiInput {
public:
	hfUiInput() {
		m_hovered = HF_Ecs::ENTITY_NULL;
		m_pressed = HF_Ecs::ENTITY_NULL;
		m_focused = HF_Ecs::ENTITY_NULL;
		m_pressedButton = -1;
		m_mouseX = 0.0f;
		m_mouseY = 0.0f;
		for ( int i = 0; i < HF_UI_MOUSE_COUNT; i++ ) {
			m_buttonDown[i] = false;
			m_buttonPressed[i] = false;
			m_buttonReleased[i] = false;
		}
	}

	HF_INLINE void SetMouse( float x, float y ) { m_mouseX = x; m_mouseY = y; }
	HF_INLINE void SetButton( int button, bool down ) {
		if ( button < 0 || button >= HF_UI_MOUSE_COUNT ) return;
		m_buttonPressed[button] = down && !m_buttonDown[button];
		m_buttonReleased[button] = !down && m_buttonDown[button];
		m_buttonDown[button] = down;
	}

	HF_INLINE HF_Ecs::Entity GetHovered() const { return m_hovered; }
	HF_INLINE HF_Ecs::Entity GetPressed() const { return m_pressed; }
	HF_INLINE HF_Ecs::Entity GetFocused() const { return m_focused; }
	HF_INLINE void SetFocused( HF_Ecs::Entity e ) { m_focused = e; }

	// Move focus to the nearest interactive control in direction (dx,dy).
	// Returns true when focus moved.
	bool MoveFocus( HF_Ecs::Registry &registry, class hfUiSystem &ui, HF_Ecs::EventBus &bus, int dx, int dy ) {
		if ( dx == 0 && dy == 0 ) { return false; }
		const hfUiRect cur = ( m_focused != HF_Ecs::ENTITY_NULL ) ? ui.GetRect( m_focused ) : hfUiRect();
		const float cx = cur.x + cur.w * 0.5f;
		const float cy = cur.y + cur.h * 0.5f;

		HF_Ecs::Entity best = HF_Ecs::ENTITY_NULL;
		float bestScore = 1e30f;

		registry.Each< hfUiCanvasComponent >( [&]( HF_Ecs::Entity ent, hfUiCanvasComponent &c ) {
			if ( !c.interactive || ent == m_focused ) { return; }
			// Only focus actual controls.
			const bool isControl = registry.Has< hfUiButtonComponent >( ent )
				|| registry.Has< hfUiSliderComponent >( ent )
				|| registry.Has< hfUiToggleComponent >( ent )
				|| registry.Has< hfUiTextInputComponent >( ent );
			if ( !isControl ) { return; }

			const hfUiRect r = ui.GetRect( ent );
			const float ex = r.x + r.w * 0.5f;
			const float ey = r.y + r.h * 0.5f;
			const float ddx = ex - cx;
			const float ddy = ey - cy;

			// Direction filter: must be in the requested half-plane.
			const float dot = ddx * ( float )dx + ddy * ( float )dy;
			if ( dot <= 0.0f ) { return; }

			// Prefer aligned controls in the direction; penalize off-axis distance.
			const float primary = ( dx != 0 ) ? fabsf( ddx ) : fabsf( ddy );
			const float off = ( dx != 0 ) ? fabsf( ddy ) : fabsf( ddx );
			const float score = primary + off * 4.0f;
			if ( score < bestScore ) {
				bestScore = score;
				best = ent;
			}
		} );

		if ( best == HF_Ecs::ENTITY_NULL ) { return false; }
		if ( m_focused != HF_Ecs::ENTITY_NULL ) {
			hfUiEventBlur bev = { m_focused };
			bus.Publish( "ui.blur", &bev, sizeof(bev) );
		}
		m_focused = best;
		hfUiEventFocus fev = { m_focused };
		bus.Publish( "ui.focus", &fev, sizeof(fev) );
		return true;
	}

	void Update( HF_Ecs::Registry &registry, class hfUiSystem &ui, HF_Ecs::EventBus &bus ) {
		HF_Ecs::Entity hit = ui.HitTest( m_mouseX, m_mouseY );
		
		// hover changes
		if ( hit != m_hovered ) {
			if ( m_hovered != HF_Ecs::ENTITY_NULL ) {
				hfUiEventHover ev = { m_hovered, m_mouseX, m_mouseY };
				bus.Publish( "ui.unhover", &ev, sizeof(ev) );
			}
			m_hovered = hit;
			if ( m_hovered != HF_Ecs::ENTITY_NULL ) {
				hfUiEventHover ev = { m_hovered, m_mouseX, m_mouseY };
				bus.Publish( "ui.hover", &ev, sizeof(ev) );
			}
		}
		
		// update button widget hover/pressed states
		registry.Each< hfUiButtonComponent >( [&]( HF_Ecs::Entity ent, hfUiButtonComponent &btn ) {
			if ( btn.state == HF_UI_BUTTON_DISABLED ) return;
			if ( m_pressed == ent ) {
				btn.state = HF_UI_BUTTON_PRESSED;
			} else if ( m_hovered == ent ) {
				btn.state = HF_UI_BUTTON_HOVER;
			} else {
				btn.state = HF_UI_BUTTON_NORMAL;
			}
		} );
		
		// button presses
		for ( int b = 0; b < HF_UI_MOUSE_COUNT; b++ ) {
			if ( m_buttonPressed[b] && hit != HF_Ecs::ENTITY_NULL ) {
				hfUiCanvasComponent *c = registry.GetComponent< hfUiCanvasComponent >( hit );
				if ( c && c->interactive ) {
					m_pressed = hit;
					m_pressedButton = b;
					hfUiEventPress ev = { hit, b, m_mouseX, m_mouseY };
					bus.Publish( "ui.press", &ev, sizeof(ev) );
					
					// click to focus
					if ( m_focused != hit ) {
						if ( m_focused != HF_Ecs::ENTITY_NULL ) {
							hfUiEventBlur bev = { m_focused };
							bus.Publish( "ui.blur", &bev, sizeof(bev) );
						}
						m_focused = hit;
						hfUiEventFocus fev = { m_focused };
						bus.Publish( "ui.focus", &fev, sizeof(fev) );
					}

					// slider: capture and update immediately on press
					UpdateSliderFromMouse( registry, ui, bus, m_pressed );
				}
			}
			
			if ( m_buttonReleased[b] ) {
				if ( m_pressed != HF_Ecs::ENTITY_NULL && m_pressed == hit ) {
					hfUiEventRelease rev = { m_pressed, b, m_mouseX, m_mouseY };
					bus.Publish( "ui.release", &rev, sizeof(rev) );
					
					hfUiEventClick cev = { m_pressed, b, m_mouseX, m_mouseY };
					bus.Publish( "ui.click", &cev, sizeof(cev) );
					
					// toggle button
					hfUiButtonComponent *btn = registry.GetComponent< hfUiButtonComponent >( m_pressed );
					if ( btn && btn->toggle ) {
						btn->active = !btn->active;
						hfUiEventChange ch = { m_pressed, btn->active ? 1.0f : 0.0f, btn->active ? 1 : 0 };
						hfStr::Copynz( ch.text, "", sizeof(ch.text) );
						bus.Publish( "ui.change", &ch, sizeof(ch) );
					}

					// checkbox / toggle widget
					hfUiToggleComponent *tog = registry.GetComponent< hfUiToggleComponent >( m_pressed );
					if ( tog && !tog->disabled ) {
						tog->checked = !tog->checked;
						hfUiEventChange ch = { m_pressed, tog->checked ? 1.0f : 0.0f, tog->checked ? 1 : 0 };
						hfStr::Copynz( ch.text, "", sizeof(ch.text) );
						bus.Publish( "ui.change", &ch, sizeof(ch) );
					}
				} else if ( m_pressed != HF_Ecs::ENTITY_NULL ) {
					// released outside - still update captured slider
					UpdateSliderFromMouse( registry, ui, bus, m_pressed );
					hfUiEventRelease rev = { m_pressed, b, m_mouseX, m_mouseY };
					bus.Publish( "ui.release", &rev, sizeof(rev) );
				}
				if ( m_pressedButton == b ) {
					m_pressed = HF_Ecs::ENTITY_NULL;
					m_pressedButton = -1;
				}
			}
		}

		// Pointer capture: keep dragging sliders even outside their rect.
		if ( m_pressed != HF_Ecs::ENTITY_NULL && m_pressedButton >= 0 && m_buttonDown[m_pressedButton] ) {
			UpdateSliderFromMouse( registry, ui, bus, m_pressed );
		}
	}

	void EndFrame() {
		for ( int i = 0; i < HF_UI_MOUSE_COUNT; i++ ) {
			m_buttonPressed[i] = false;
			m_buttonReleased[i] = false;
		}
	}

	// Drop all interaction state.  A frontend rebuilds (and therefore
	// invalidates) its whole entity graph on a state change, so any held
	// hover/press/focus target no longer exists.
	void Reset() {
		m_hovered = HF_Ecs::ENTITY_NULL;
		m_pressed = HF_Ecs::ENTITY_NULL;
		m_focused = HF_Ecs::ENTITY_NULL;
		m_pressedButton = -1;
		for ( int i = 0; i < HF_UI_MOUSE_COUNT; i++ ) {
			m_buttonDown[i] = false;
			m_buttonPressed[i] = false;
			m_buttonReleased[i] = false;
		}
	}

private:
	void UpdateSliderFromMouse( HF_Ecs::Registry &registry, class hfUiSystem &ui, HF_Ecs::EventBus &bus, HF_Ecs::Entity entity ) {
		hfUiSliderComponent *s = registry.GetComponent< hfUiSliderComponent >( entity );
		if ( s == NULL ) { return; }
		const hfUiRect r = ui.GetRect( entity );
		if ( r.w <= 0.0f || r.h <= 0.0f ) { return; }

		float t = 0.0f;
		if ( s->vertical ) {
			t = 1.0f - ( ( m_mouseY - r.y ) / r.h );
		} else {
			t = ( m_mouseX - r.x ) / r.w;
		}
		if ( t < 0.0f ) { t = 0.0f; }
		if ( t > 1.0f ) { t = 1.0f; }

		float value = s->minValue + t * ( s->maxValue - s->minValue );
		if ( s->step > 0.0f ) {
			value = s->minValue + floorf( ( value - s->minValue ) / s->step + 0.5f ) * s->step;
		}
		if ( s->integer ) {
			value = floorf( value + 0.5f );
		}
		if ( value < s->minValue ) { value = s->minValue; }
		if ( value > s->maxValue ) { value = s->maxValue; }

		if ( value != s->value ) {
			s->value = value;
			hfUiEventChange ch = { entity, value, ( int )value };
			hfStr::Copynz( ch.text, "", sizeof(ch.text) );
			bus.Publish( "ui.change", &ch, sizeof(ch) );
		}
	}

	float				m_mouseX, m_mouseY;
	bool				m_buttonDown[ HF_UI_MOUSE_COUNT ];
	bool				m_buttonPressed[ HF_UI_MOUSE_COUNT ];
	bool				m_buttonReleased[ HF_UI_MOUSE_COUNT ];
	HF_Ecs::Entity		m_hovered;
	HF_Ecs::Entity		m_pressed;
	HF_Ecs::Entity		m_focused;
	int					m_pressedButton;
};

} // namespace HF_Ui

#endif
