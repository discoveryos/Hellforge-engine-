#ifndef __HF_UI_DRAW_PASS_H__
#define __HF_UI_DRAW_PASS_H__

#include "../hflib/hfprecompiled.h"
#include "../hflib/ui/hfUiSystem.h"
#include "../hflib/ui/hfUiWidget.h"
#include "../hflib/ui/hfBitmapFont.h"

/*
===============================================================================

	hfUiDrawPass — draws a hfUiSystem tree as a 2D overlay.

	Draw() is the explicit entry point (used by tests and by hosts that own
	their own frame ordering).  A host that wants the frontend drawn by the
	renderer installs a source once:

		hfUiDrawPass::SetSource( &registry, &uiSystem );

	and the backend calls DrawInstalled() once per frame after the 3D views.
	The source is a pair of pointers, so a frontend must keep both alive (and
	keep them at a stable address) for as long as it is installed.

===============================================================================
*/
class hfUiDrawPass {
public:
	// Draw a tree immediately.  textCoverage, when supplied, receives the
	// rasterized glyphs instead of (in addition to) the GPU quads.
	static void	Draw( HF_Ecs::Registry &registry, HF_Ui::hfUiSystem &uiSystem, HF_Ui::hfUiCoverage *textCoverage = NULL );

	// Install / uninstall the tree the backend should draw every frame.
	static void	SetSource( HF_Ecs::Registry *registry, HF_Ui::hfUiSystem *uiSystem );
	static void	ClearSource();
	static bool	HasSource();

	// Called by the backend after the frame's views.  Safe to call when
	// nothing is installed, in headless mode, or before the renderer exists.
	static void	DrawInstalled();

private:
	static HF_Ecs::Registry *		s_registry;
	static HF_Ui::hfUiSystem *		s_system;
};

#endif
