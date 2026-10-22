#ifndef __HF_UI_DRAW_PASS_H__
#define __HF_UI_DRAW_PASS_H__

#include "../hflib/hfprecompiled.h"
#include "../hflib/ui/hfUiSystem.h"
#include "../hflib/ui/hfUiWidget.h"
#include "../hflib/ui/hfBitmapFont.h"

class hfUiDrawPass {
public:
	static void Draw( HF_Ecs::Registry &registry, HF_Ui::hfUiSystem &uiSystem, HF_Ui::hfUiCoverage *textCoverage = NULL );
};

#endif
