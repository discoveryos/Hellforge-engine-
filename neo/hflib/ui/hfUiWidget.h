#ifndef __HF_UI_WIDGET_H__
#define __HF_UI_WIDGET_H__

/*
===============================================================================

	hfUiWidget — Phase 15 widget components

	Godot-style widgets as ECS components. The UI system resolves layout and
	hit-testing; the input system translates mouse/keyboard/gamepad to events;
	the renderer draws them.

	Widgets:
	  - Panel: visual container (bg color/texture, rounded corners later)
	  - Button: clickable, hover/pressed states, optional label
	  - Label: text display (uses hfUiLabelComponent)
	  - Slider: value in [0,1] or integer range
	  - Toggle/Checkbox: boolean state
	  - TextInput: editable text field

	All data is plain structs so serialization/save/load and headless tests
	work naturally.

===============================================================================
*/

#include "../hfprecompiled.h"
#include "../hfEcs.h"
#include "hfCanvasComponent.h"

namespace HF_Ui {

enum hfUiWidgetType {
	HF_UI_WIDGET_NONE = 0,
	HF_UI_WIDGET_PANEL,
	HF_UI_WIDGET_BUTTON,
	HF_UI_WIDGET_LABEL,
	HF_UI_WIDGET_SLIDER,
	HF_UI_WIDGET_TOGGLE,
	HF_UI_WIDGET_TEXTINPUT,
	HF_UI_WIDGET_LIST
};

// --- Panel -------------------------------------------------------------------
struct hfUiPanelComponent {
	float	r, g, b, a;
	float	borderR, borderG, borderB, borderA;
	float	borderWidth;
	float	cornerRadius;

	HF_INLINE hfUiPanelComponent() {
		r = g = b = a = 1.0f;
		borderR = borderG = borderB = 0.0f; borderA = 0.0f;
		borderWidth = 0.0f;
		cornerRadius = 0.0f;
	}
};

// --- Button ------------------------------------------------------------------
enum hfUiButtonState {
	HF_UI_BUTTON_NORMAL = 0,
	HF_UI_BUTTON_HOVER,
	HF_UI_BUTTON_PRESSED,
	HF_UI_BUTTON_DISABLED
};

struct hfUiButtonComponent {
	hfUiButtonState	state;
	bool			toggle;		// if true, click toggles pressed/active
	bool			active;		// toggled state
	
	// colors per state
	float			normalR,normalG,normalB,normalA;
	float			hoverR,hoverG,hoverB,hoverA;
	float			pressedR,pressedG,pressedB,pressedA;
	float			disabledR,disabledG,disabledB,disabledA;
	
	// text label entity (optional) - set to button entity to auto-use its label
	HF_Ecs::Entity	labelEntity;
	char			soundClick[ 64 ];
	
	HF_INLINE hfUiButtonComponent() {
		state = HF_UI_BUTTON_NORMAL;
		toggle = false;
		active = false;
		labelEntity = HF_Ecs::ENTITY_NULL;
		soundClick[ 0 ] = '\0';
		
		normalR=0.2f; normalG=0.2f; normalB=0.25f; normalA=1.0f;
		hoverR=0.3f;  hoverG=0.3f;  hoverB=0.35f;  hoverA=1.0f;
		pressedR=0.15f; pressedG=0.15f; pressedB=0.2f; pressedA=1.0f;
		disabledR=0.15f; disabledG=0.15f; disabledB=0.15f; disabledA=0.5f;
	}
};

// --- Slider ------------------------------------------------------------------
struct hfUiSliderComponent {
	float	minValue;
	float	maxValue;
	float	value;
	float	step;			// 0 = continuous
	bool	vertical;
	bool	integer;		// snap to integers
	
	float	handleW, handleH;
	
	// colors
	float	barR,barG,barB,barA;
	float	handleR,handleG,handleB,handleA;
	
	HF_INLINE hfUiSliderComponent() {
		minValue = 0.0f; maxValue = 1.0f; value = 0.0f; step = 0.0f;
		vertical = false;
		integer = false;
		handleW = 16.0f; handleH = 24.0f;
		barR=0.15f; barG=0.15f; barB=0.2f; barA=1.0f;
		handleR=0.8f; handleG=0.8f; handleB=0.8f; handleA=1.0f;
	}
};

// --- Toggle/Checkbox ---------------------------------------------------------
struct hfUiToggleComponent {
	bool	checked;
	bool	disabled;
	
	float	boxSize;
	float	checkR,checkG,checkB,checkA;
	float	boxR,boxG,boxB,boxA;
	
	HF_INLINE hfUiToggleComponent() {
		checked = false;
		disabled = false;
		boxSize = 16.0f;
		checkR=0.2f; checkG=0.8f; checkB=0.4f; checkA=1.0f;
		boxR=0.25f; boxG=0.25f; boxB=0.3f; boxA=1.0f;
	}
};

// --- TextInput ---------------------------------------------------------------
struct hfUiTextInputComponent {
	char	text[ 256 ];
	char	placeholder[ 64 ];
	int		cursorPos;
	int		selectionStart;
	int		maxLength;
	bool	password;		// mask with asterisks
	bool	readonly;
	bool	focused;
	
	float	caretBlinkTime;
	float	r,g,b,a;
	
	HF_INLINE hfUiTextInputComponent() {
		text[0] = '\0';
		placeholder[0] = '\0';
		cursorPos = 0;
		selectionStart = -1;
		maxLength = 255;
		password = false;
		readonly = false;
		focused = false;
		caretBlinkTime = 0.0f;
		r=g=b=a=1.0f;
	}
};

} // namespace HF_Ui

#endif
