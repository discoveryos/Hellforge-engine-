#ifndef __HF_UI_EVENTS_H__
#define __HF_UI_EVENTS_H__

/*
===============================================================================

	hfUiEvents — Phase 14 UI events

	UI input is mapped to high level events dispatched through HF_Ecs::EventBus.
	Events are named strings for flexibility; payloads are typed structs for
	zero-overhead extraction.

	Events:
	  "ui.hover"      { entity, x, y }
	  "ui.unhover"    { entity, x, y }
	  "ui.press"      { entity, button, x, y }
	  "ui.release"    { entity, button, x, y }
	  "ui.click"      { entity, button, x, y }
	  "ui.focus"      { entity }
	  "ui.blur"       { entity }
	  "ui.change"     { entity, valueF / valueI / text... }
	  "ui.submit"     { entity, text[128] }

	All headless-testable: tests can pump the input system directly and check
	the event bus.

===============================================================================
*/

#include "../hfprecompiled.h"
#include "../hfEcs.h"
#include "hfCanvasComponent.h"

namespace HF_Ui {

enum hfUiMouseButton {
	HF_UI_MOUSE_LEFT = 0,
	HF_UI_MOUSE_RIGHT,
	HF_UI_MOUSE_MIDDLE,
	HF_UI_MOUSE_COUNT
};

struct hfUiEventHover {
	HF_Ecs::Entity	entity;
	float			x, y;
};

struct hfUiEventPress {
	HF_Ecs::Entity	entity;
	int				button;
	float			x, y;
};

struct hfUiEventRelease {
	HF_Ecs::Entity	entity;
	int				button;
	float			x, y;
};

struct hfUiEventClick {
	HF_Ecs::Entity	entity;
	int				button;
	float			x, y;
};

struct hfUiEventFocus {
	HF_Ecs::Entity	entity;
};

struct hfUiEventBlur {
	HF_Ecs::Entity	entity;
};

struct hfUiEventChange {
	HF_Ecs::Entity	entity;
	float			valueF;
	int				valueI;
	char			text[ 128 ];
};

struct hfUiEventSubmit {
	HF_Ecs::Entity	entity;
	char			text[ 128 ];
};

// Published when a frontend changes screen.  Lets a host (or a test) observe
// transitions without polling GetState().
struct hfUiEventState {
	int				state;
	int				previous;
};

} // namespace HF_Ui

#endif
