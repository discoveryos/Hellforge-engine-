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
				} else if ( m_pressed != HF_Ecs::ENTITY_NULL ) {
					// released outside
					hfUiEventRelease rev = { m_pressed, b, m_mouseX, m_mouseY };
					bus.Publish( "ui.release", &rev, sizeof(rev) );
				}
				if ( m_pressedButton == b ) {
					m_pressed = HF_Ecs::ENTITY_NULL;
					m_pressedButton = -1;
				}
			}
		}
	}

	void EndFrame() {
		for ( int i = 0; i < HF_UI_MOUSE_COUNT; i++ ) {
			m_buttonPressed[i] = false;
			m_buttonReleased[i] = false;
		}
	}

private:
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
