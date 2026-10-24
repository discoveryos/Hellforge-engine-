#ifndef __HF_UI_BUILDER_H__
#define __HF_UI_BUILDER_H__

/*
===============================================================================

	hfUiBuilder — Phase 15 widget helpers

	Godot-style convenience constructors for building UI trees. Mirrors how
	other engines do it: CreatePanel/CreateButton/CreateLabel/etc. attach
	components and wire up hierarchy via hfUiCanvasComponent.parent.

	All headless-testable.

===============================================================================
*/

#include "../hfprecompiled.h"
#include "../hfEcs.h"
#include "hfCanvasComponent.h"
#include "hfUiWidget.h"
#include "hfUiSystem.h"

namespace HF_Ui {

class hfUiBuilder {
public:
	static HF_Ecs::Entity CreateRoot( HF_Ecs::Registry &reg, float w, float h ) {
		HF_Ecs::Entity e = reg.Create();
		hfUiCanvasComponent &c = reg.AddComponent< hfUiCanvasComponent >( e );
		c.parent = HF_Ecs::ENTITY_NULL;
		c.anchor.SetFixed( 0, 0, w, h );
		c.visible = true;
		c.interactive = false;
		return e;
	}
	
	static HF_Ecs::Entity CreatePanel( HF_Ecs::Registry &reg, HF_Ecs::Entity parent, float x, float y, float w, float h,
										float r=0.15f, float g=0.15f, float b=0.2f, float a=1.0f ) {
		HF_Ecs::Entity e = reg.Create();
		hfUiCanvasComponent &c = reg.AddComponent< hfUiCanvasComponent >( e );
		c.parent = parent;
		c.anchor.SetFixed( x, y, w, h );
		c.visible = true;
		c.interactive = false;
		hfUiPanelComponent &p = reg.AddComponent< hfUiPanelComponent >( e );
		p.r=r; p.g=g; p.b=b; p.a=a;
		return e;
	}
	
	static HF_Ecs::Entity CreateLabel( HF_Ecs::Registry &reg, HF_Ecs::Entity parent, float x, float y, float w, float h, const char *text,
										float scale=1.0f, float r=1.0f, float g=1.0f, float b=1.0f, float a=1.0f ) {
		HF_Ecs::Entity e = reg.Create();
		hfUiCanvasComponent &c = reg.AddComponent< hfUiCanvasComponent >( e );
		c.parent = parent;
		c.anchor.SetFixed( x, y, w, h );
		c.visible = true;
		c.interactive = false;
		hfUiLabelComponent &l = reg.AddComponent< hfUiLabelComponent >( e );
		l.SetText( text );
		l.scale = scale;
		l.r=r; l.g=g; l.b=b; l.a=a;
		l.visible = true;
		l.wrap = false;
		return e;
	}
	
	static HF_Ecs::Entity CreateButton( HF_Ecs::Registry &reg, HF_Ecs::Entity parent, float x, float y, float w, float h, const char *text ) {
		HF_Ecs::Entity e = reg.Create();
		hfUiCanvasComponent &c = reg.AddComponent< hfUiCanvasComponent >( e );
		c.parent = parent;
		c.anchor.SetFixed( x, y, w, h );
		c.minSize.Set( w, h );
		c.visible = true;
		c.interactive = true;
		reg.AddComponent< hfUiButtonComponent >( e );
		
		// Add label centered in button
		HF_Ecs::Entity lbl = reg.Create();
		hfUiCanvasComponent &lc = reg.AddComponent< hfUiCanvasComponent >( lbl );
		lc.parent = e;
		lc.anchor.SetFixed( 0, 0, w, h );
		lc.visible = true;
		lc.interactive = false;
		hfUiLabelComponent &l = reg.AddComponent< hfUiLabelComponent >( lbl );
		l.SetText( text );
		l.scale = 1.0f;
		l.r=l.g=l.b=l.a = 1.0f;
		l.align = HF_UI_TEXT_CENTER;
		
		return e;
	}
	
	static HF_Ecs::Entity CreateSlider( HF_Ecs::Registry &reg, HF_Ecs::Entity parent, float x, float y, float w, float h,
										float minv=0.0f, float maxv=1.0f, float val=0.5f ) {
		HF_Ecs::Entity e = reg.Create();
		hfUiCanvasComponent &c = reg.AddComponent< hfUiCanvasComponent >( e );
		c.parent = parent;
		c.anchor.SetFixed( x, y, w, h );
		c.minSize.Set( w, h );
		c.visible = true;
		c.interactive = true;
		hfUiSliderComponent &s = reg.AddComponent< hfUiSliderComponent >( e );
		s.minValue = minv; s.maxValue = maxv; s.value = val;
		return e;
	}
	
	static HF_Ecs::Entity CreateToggle( HF_Ecs::Registry &reg, HF_Ecs::Entity parent, float x, float y, float w, float h, const char *text, bool checked=false ) {
		HF_Ecs::Entity e = reg.Create();
		hfUiCanvasComponent &c = reg.AddComponent< hfUiCanvasComponent >( e );
		c.parent = parent;
		c.anchor.SetFixed( x, y, w, h );
		c.minSize.Set( w, h );
		c.visible = true;
		c.interactive = true;
		hfUiToggleComponent &t = reg.AddComponent< hfUiToggleComponent >( e );
		t.checked = checked;
		
		// Label to the right
		HF_Ecs::Entity lbl = reg.Create();
		hfUiCanvasComponent &lc = reg.AddComponent< hfUiCanvasComponent >( lbl );
		lc.parent = e;
		lc.anchor.SetFixed( 24, 0, w - 24, h );
		lc.visible = true;
		lc.interactive = false;
		hfUiLabelComponent &l = reg.AddComponent< hfUiLabelComponent >( lbl );
		l.SetText( text );
		l.scale = 1.0f;
		l.r=l.g=l.b=l.a = 1.0f;
		
		return e;
	}
	
	static HF_Ecs::Entity CreateTextInput( HF_Ecs::Registry &reg, HF_Ecs::Entity parent, float x, float y, float w, float h, const char *placeholder="" ) {
		HF_Ecs::Entity e = reg.Create();
		hfUiCanvasComponent &c = reg.AddComponent< hfUiCanvasComponent >( e );
		c.parent = parent;
		c.anchor.SetFixed( x, y, w, h );
		c.minSize.Set( w, h );
		c.visible = true;
		c.interactive = true;
		hfUiTextInputComponent &ti = reg.AddComponent< hfUiTextInputComponent >( e );
		if ( placeholder ) hfStr::Copynz( ti.placeholder, placeholder, sizeof(ti.placeholder) );
		return e;
	}
	
	static HF_Ecs::Entity CreateVerticalBox( HF_Ecs::Registry &reg, HF_Ecs::Entity parent, float x, float y, float w, float h, float spacing=4.0f ) {
		HF_Ecs::Entity e = reg.Create();
		hfUiCanvasComponent &c = reg.AddComponent< hfUiCanvasComponent >( e );
		c.parent = parent;
		c.anchor.SetFixed( x, y, w, h );
		c.layout = HF_UI_LAYOUT_COLUMN;
		c.spacing = spacing;
		c.padding = hfUiMargin::All( 4.0f );
		c.visible = true;
		c.interactive = false;
		return e;
	}
	
	static HF_Ecs::Entity CreateHorizontalBox( HF_Ecs::Registry &reg, HF_Ecs::Entity parent, float x, float y, float w, float h, float spacing=4.0f ) {
		HF_Ecs::Entity e = reg.Create();
		hfUiCanvasComponent &c = reg.AddComponent< hfUiCanvasComponent >( e );
		c.parent = parent;
		c.anchor.SetFixed( x, y, w, h );
		c.layout = HF_UI_LAYOUT_ROW;
		c.spacing = spacing;
		c.padding = hfUiMargin::All( 4.0f );
		c.visible = true;
		c.interactive = false;
		return e;
	}
};

} // namespace HF_Ui

#endif
