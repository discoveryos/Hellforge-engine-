#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfUiDrawPass.h"
#include "hftr_local.h"

void hfUiDrawPass::Draw( HF_Ecs::Registry &registry, HF_Ui::hfUiSystem &uiSystem, HF_Ui::hfUiCoverage *textCoverage ) {
	int drawCount = uiSystem.GetDrawCount();
	
	// Save GL state
	qglPushAttrib( GL_ALL_ATTRIB_BITS );
	qglPushMatrix();
	
	qglMatrixMode( GL_PROJECTION );
	qglPushMatrix();
	qglLoadIdentity();
	qglOrtho( 0, renderSystem->GetWidth(), renderSystem->GetHeight(), 0, -1, 1 );
	
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity();
	
	GL_State( GLS_DEPTHFUNC_ALWAYS | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ); qglDisable( GL_DEPTH_TEST );
	GL_Cull( CT_TWO_SIDED );
	qglDisable( GL_TEXTURE_2D );
	
	for ( int d = 0; d < drawCount; d++ ) {
		HF_Ecs::Entity ent = uiSystem.GetDrawEntity( d );
		if ( ent == HF_Ecs::ENTITY_NULL ) continue;
		
		const HF_Ui::hfUiCanvasComponent *canvas = registry.GetComponent< HF_Ui::hfUiCanvasComponent >( ent );
		if ( !canvas || !canvas->visible ) continue;
		
		HF_Ui::hfUiRect rect = uiSystem.GetRect( ent );
		HF_Ui::hfUiRect visible = uiSystem.GetVisible( ent );
		
		if ( visible.Empty() ) continue;
		
		// Panel background
		const HF_Ui::hfUiPanelComponent *panel = registry.GetComponent< HF_Ui::hfUiPanelComponent >( ent );
		if ( panel && panel->a > 0.0f ) {
			GL_Color( panel->r, panel->g, panel->b, panel->a );
			qglBegin( GL_QUADS );
			qglVertex2f( visible.x, visible.y );
			qglVertex2f( visible.x + visible.w, visible.y );
			qglVertex2f( visible.x + visible.w, visible.y + visible.h );
			qglVertex2f( visible.x, visible.y + visible.h );
			qglEnd();
		}
		
		// Button
		const HF_Ui::hfUiButtonComponent *btn = registry.GetComponent< HF_Ui::hfUiButtonComponent >( ent );
		if ( btn && panel == NULL ) { // draw button bg if no panel
			float r,g,b,a;
			if ( btn->state == HF_Ui::HF_UI_BUTTON_DISABLED ) { r=btn->disabledR; g=btn->disabledG; b=btn->disabledB; a=btn->disabledA; }
			else if ( btn->state == HF_Ui::HF_UI_BUTTON_PRESSED ) { r=btn->pressedR; g=btn->pressedG; b=btn->pressedB; a=btn->pressedA; }
			else if ( btn->state == HF_Ui::HF_UI_BUTTON_HOVER ) { r=btn->hoverR; g=btn->hoverG; b=btn->hoverB; a=btn->hoverA; }
			else { r=btn->normalR; g=btn->normalG; b=btn->normalB; a=btn->normalA; }
			GL_Color( r,g,b,a );
			qglBegin( GL_QUADS );
			qglVertex2f( visible.x, visible.y );
			qglVertex2f( visible.x + visible.w, visible.y );
			qglVertex2f( visible.x + visible.w, visible.y + visible.h );
			qglVertex2f( visible.x, visible.y + visible.h );
			qglEnd();
		}
		
		// Toggle/checkbox
		const HF_Ui::hfUiToggleComponent *toggle = registry.GetComponent< HF_Ui::hfUiToggleComponent >( ent );
		if ( toggle ) {
			float bx = visible.x + 4.0f;
			float by = visible.y + (visible.h - toggle->boxSize) * 0.5f;
			GL_Color( toggle->boxR, toggle->boxG, toggle->boxB, toggle->boxA );
			qglBegin( GL_QUADS );
			qglVertex2f( bx, by );
			qglVertex2f( bx + toggle->boxSize, by );
			qglVertex2f( bx + toggle->boxSize, by + toggle->boxSize );
			qglVertex2f( bx, by + toggle->boxSize );
			qglEnd();
			if ( toggle->checked ) {
				float pad = 3.0f;
				GL_Color( toggle->checkR, toggle->checkG, toggle->checkB, toggle->checkA );
				qglBegin( GL_QUADS );
				qglVertex2f( bx + pad, by + pad );
				qglVertex2f( bx + toggle->boxSize - pad, by + pad );
				qglVertex2f( bx + toggle->boxSize - pad, by + toggle->boxSize - pad );
				qglVertex2f( bx + pad, by + toggle->boxSize - pad );
				qglEnd();
			}
		}
		
		// Slider
		const HF_Ui::hfUiSliderComponent *slider = registry.GetComponent< HF_Ui::hfUiSliderComponent >( ent );
		if ( slider ) {
			float t = slider->maxValue - slider->minValue;
			if ( t < 0.0001f ) t = 1.0f;
			float pct = ( slider->value - slider->minValue ) / t;
			if ( pct < 0.0f ) pct = 0.0f;
			if ( pct > 1.0f ) pct = 1.0f;
			
			if ( !slider->vertical ) {
				float barY = visible.y + (visible.h - 4.0f) * 0.5f;
				GL_Color( slider->barR, slider->barG, slider->barB, slider->barA );
				qglBegin( GL_QUADS );
				qglVertex2f( visible.x, barY );
				qglVertex2f( visible.x + visible.w, barY );
				qglVertex2f( visible.x + visible.w, barY + 4.0f );
				qglVertex2f( visible.x, barY + 4.0f );
				qglEnd();
				
				float hx = visible.x + pct * ( visible.w - slider->handleW );
				float hy = visible.y + (visible.h - slider->handleH) * 0.5f;
				GL_Color( slider->handleR, slider->handleG, slider->handleB, slider->handleA );
				qglBegin( GL_QUADS );
				qglVertex2f( hx, hy );
				qglVertex2f( hx + slider->handleW, hy );
				qglVertex2f( hx + slider->handleW, hy + slider->handleH );
				qglVertex2f( hx, hy + slider->handleH );
				qglEnd();
			}
		}
		
		// Text label
		const HF_Ui::hfUiLabelComponent *label = registry.GetComponent< HF_Ui::hfUiLabelComponent >( ent );
		if ( label && label->visible && label->text[0] ) {
			// For headless/text coverage, rasterize into coverage buffer if provided
			if ( textCoverage ) {
				float sx = visible.x + 4.0f;
				float sy = visible.y + 4.0f;
				HF_Ui::hfBitmapFont::Rasterize( label->text, -1, (int)sx, (int)sy, *textCoverage, 255 );
			}
			// GPU path would use textured quads; for now bitmap font rasterizes to coverage
			// and can be uploaded as a texture. The harness tests coverage/metrics.
		}
	}
	
	qglMatrixMode( GL_PROJECTION );
	qglPopMatrix();
	qglMatrixMode( GL_MODELVIEW );
	qglPopMatrix();
	qglPopAttrib();
}
