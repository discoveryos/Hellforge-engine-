#pragma hdrstop
#include "../hflib/hfprecompiled.h"
#include "../renderer/hftr_local.h"

hfCVar swf_timescale( "swf_timescale", "1", CVAR_FLOAT, "timescale for swf files" );
hfCVar swf_stopat( "swf_stopat", "0", CVAR_FLOAT, "stop at a specific frame" );

hfCVar swf_titleSafe( "swf_titleSafe", "0.005", CVAR_FLOAT, "space between UI elements and screen edge", 0.0f, 0.075f );

hfCVar swf_forceAlpha( "swf_forceAlpha", "0", CVAR_FLOAT, "force an alpha value on all elements, useful to show invisible animating elements", 0.0f, 1.0f );

extern hfCVar swf_textStrokeSize;
extern hfCVar swf_textStrokeSizeGlyphSpacer;
extern hfCVar in_useJoystick;

#define ALPHA_EPSILON	0.001f

#define STENCIL_DECR -1
#define STENCIL_INCR -2

/*
========================
hfSWF::DrawStretchPic
========================
*/
void hfSWF::DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const hfMaterial *material ) {
	renderSystem->DrawStretchPic( x * scaleToVirtual.x, y * scaleToVirtual.y, w * scaleToVirtual.x, h * scaleToVirtual.y, s1, t1, s2, t2, material );
}

/*
========================
hfSWF::DrawStretchPic
========================
*/
void hfSWF::DrawStretchPic( const hfVec4 & topLeft, const hfVec4 & topRight, const hfVec4 & bottomRight, const hfVec4 & bottomLeft, const hfMaterial * material ) {
	renderSystem->DrawStretchPic(
		hfVec4( topLeft.x * scaleToVirtual.x, topLeft.y * scaleToVirtual.y, topLeft.z, topLeft.w ),
		hfVec4( topRight.x * scaleToVirtual.x, topRight.y * scaleToVirtual.y, topRight.z, topRight.w ),
		hfVec4( bottomRight.x * scaleToVirtual.x, bottomRight.y * scaleToVirtual.y, bottomRight.z, bottomRight.w ),
		hfVec4( bottomLeft.x * scaleToVirtual.x, bottomLeft.y * scaleToVirtual.y, bottomLeft.z, bottomLeft.w ),
		material );
}

/*
========================
hfSWF::Render
========================
*/
void hfSWF::Render( hfRenderSystem * gui, int time, bool isSplitscreen ) {
	if ( !IsLoaded() ) {
		return;
	}
	if ( !IsActive() ) {
		return;
	}
	if ( swf_stopat.GetInteger() > 0 ) {
		if ( mainspriteInstance->currentFrame == swf_stopat.GetInteger() ) {
			swf_timescale.SetFloat( 0.0f );
		}
	}

	int currentTime = Sys_Milliseconds();
	int framesToRun = 0;

	if ( paused ) {
		lastRenderTime = currentTime;
	}

	if ( swf_timescale.GetFloat() > 0.0f ) {
		if ( lastRenderTime == 0 ) {
			lastRenderTime = currentTime;
			framesToRun = 1;
		} else {
			float deltaTime = ( currentTime - lastRenderTime );
			float fr = ( (float)frameRate / 256.0f ) * swf_timescale.GetFloat();
			framesToRun = hfMath::Ftoi( ( fr * deltaTime ) / 1000.0f );
			lastRenderTime += ( framesToRun * ( 1000.0f / fr ) );
			if ( framesToRun > 10 ) {
				framesToRun = 10;
			}
		}
		for ( int i = 0; i < framesToRun; i++ ) {
			mainspriteInstance->Run();
			mainspriteInstance->RunActions();
		}
	}

	const float pixelAspect = renderSystem->GetPixelAspect();
	const float sysWidth = renderSystem->GetWidth() * ( pixelAspect > 1.0f ? pixelAspect : 1.0f );
	const float sysHeight = renderSystem->GetHeight() / ( pixelAspect < 1.0f ? pixelAspect : 1.0f );
	float scale = swfScale * sysHeight / (float)frameHeight;

	swfRenderState_t renderState;
	renderState.stereoDepth = (stereoDepthType_t)mainspriteInstance->GetStereoDepth();
	renderState.matrix.xx = scale;
	renderState.matrix.yy = scale;
	renderState.matrix.tx = 0.5f * ( sysWidth - ( frameWidth * scale ) );
	renderState.matrix.ty = 0.5f * ( sysHeight - ( frameHeight * scale ) );

	renderBorder = renderState.matrix.tx / scale;

	scaleToVirtual.Set( (float)SCREEN_WIDTH / sysWidth, (float)SCREEN_HEIGHT / sysHeight );

	RenderSprite( gui, mainspriteInstance, renderState, time, isSplitscreen );

	if ( blackbars ) {
		float barWidth = renderState.matrix.tx + 0.5f;
		float barHeight = renderState.matrix.ty + 0.5f;
		if ( barWidth > 0.0f ) {
			gui->SetColor( hfVec4( 0.0f, 0.0f, 0.0f, 1.0f ) );
			DrawStretchPic( 0.0f, 0.0f, barWidth, sysHeight, 0, 0, 1, 1, white );
			DrawStretchPic( sysWidth - barWidth, 0.0f, barWidth, sysHeight, 0, 0, 1, 1, white );
		}
		if ( barHeight > 0.0f ) {
			gui->SetColor( hfVec4( 0.0f, 0.0f, 0.0f, 1.0f ) );
			DrawStretchPic( 0.0f, 0.0f, sysWidth, barHeight, 0, 0, 1, 1, white );
			DrawStretchPic( 0.0f, sysHeight - barHeight, sysWidth, barHeight, 0, 0, 1, 1, white );
		}
	}

	if ( isMouseInClientArea && ( mouseEnabled && useMouse ) && ( InhibitControl() || ( !InhibitControl() && !useInhibtControl ) ) ) {
		gui->SetGLState( GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
		gui->SetColor( hfVec4( 1.0f, 1.0f, 1.0f, 1.0f ) );
		hfVec2 mouse = renderState.matrix.Transform( hfVec2( mouseX - 1, mouseY - 2 ) );
		//hfSWFScriptObject * hitObject = HitTest( mainspriteInstance, swfRenderState_t(), mouseX, mouseY, NULL );
		if ( !hasHitObject ) { //hitObject == NULL ) {
			DrawStretchPic( mouse.x, mouse.y, 32.0f, 32.0f, 0, 0, 1, 1, guiCursor_arrow );
		} else {
			DrawStretchPic( mouse.x, mouse.y, 32.0f, 32.0f, 0, 0, 1, 1, guiCursor_hand );
		}
	}

	// restore the GL State
	gui->SetGLState( 0 );
}

/*
========================
hfSWF::RenderMask
========================
*/
void hfSWF::RenderMask( hfRenderSystem * gui, const swfDisplayEntry_t * mask, const swfRenderState_t & renderState, const int stencilMode ) {
	swfRenderState_t renderState2;
	renderState2.stereoDepth = renderState.stereoDepth;
	renderState2.matrix = mask->matrix.Multiply( renderState.matrix );
	renderState2.cxf = mask->cxf.Multiply( renderState.cxf );
	renderState2.ratio = mask->ratio;
	renderState2.material = guiSolid;
	renderState2.activeMasks = stencilMode;

	hfSWFDictionaryEntry & entry = dictionary[ mask->characterID ];
	if ( entry.type == SWF_DICT_SHAPE ) {
		RenderShape( gui, entry.shape, renderState2 );
	} else if ( entry.type == SWF_DICT_MORPH ) {
		RenderMorphShape( gui, entry.shape, renderState2 );
	}
}

/*
========================
hfSWF::RenderSprite
========================
*/
void hfSWF::RenderSprite( hfRenderSystem * gui, hfSWFSpriteInstance * spriteInstance, const swfRenderState_t & renderState, int time, bool isSplitscreen ) {

	if ( spriteInstance == NULL ) {
		hfLib::Warning( "%s: RenderSprite: spriteInstance == NULL", filename.c_str() );
		return;
	}
	if ( !spriteInstance->isVisible ) {
		return;
	}
	if ( ( ( renderState.cxf.mul.w + renderState.cxf.add.w ) <= ALPHA_EPSILON ) && ( swf_forceAlpha.GetFloat() <= 0.0f ) ) {
		return;
	}

	hfStaticList<const swfDisplayEntry_t *, 256> activeMasks;

	for ( int i = 0; i < spriteInstance->displayList.Num(); i++ ) {
		const swfDisplayEntry_t & display = spriteInstance->displayList[i];

		for ( int j = 0; j < activeMasks.Num(); j++ ) {
			const swfDisplayEntry_t * mask = activeMasks[ j ];
			if ( display.depth > mask->clipDepth ) {
				RenderMask( gui, mask, renderState, STENCIL_DECR );
				activeMasks.RemoveIndexFast( j );
			}
		}
		if ( display.clipDepth > 0 ) {
			activeMasks.Append( &display );
			RenderMask( gui, &display, renderState, STENCIL_INCR );
			continue;
		}
		hfSWFDictionaryEntry * entry = FindDictionaryEntry( display.characterID );
		if ( entry == NULL ) {
			continue;
		}

		swfRenderState_t renderState2;

		if ( spriteInstance->stereoDepth != STEREO_DEPTH_TYPE_NONE ) {
			renderState2.stereoDepth = ( stereoDepthType_t )spriteInstance->stereoDepth; 
		} else if ( renderState.stereoDepth != STEREO_DEPTH_TYPE_NONE ) {
			renderState2.stereoDepth = renderState.stereoDepth;
		}

		renderState2.matrix = display.matrix.Multiply( renderState.matrix );
		renderState2.cxf = display.cxf.Multiply( renderState.cxf );
		renderState2.ratio = display.ratio;
		if ( display.blendMode != 0 ) {
			renderState2.blendMode = display.blendMode;
		} else {
			renderState2.blendMode = renderState.blendMode;
		}
		renderState2.activeMasks = renderState.activeMasks + activeMasks.Num();

		if ( spriteInstance->materialOverride != NULL ) {
			renderState2.material = spriteInstance->materialOverride;
			renderState2.materialWidth = spriteInstance->materialWidth;
			renderState2.materialHeight = spriteInstance->materialHeight;
		} else {
			renderState2.material = renderState.material;
			renderState2.materialWidth = renderState.materialWidth;
			renderState2.materialHeight = renderState.materialHeight;
		}

		float xOffset = 0.0f;
		float yOffset = 0.0f;

		if ( entry->type == SWF_DICT_SPRITE ) {
			display.spriteInstance->SetAlignment( spriteInstance->xOffset, spriteInstance->yOffset );

			if ( display.spriteInstance->name[0] == '_' ) {
				//if ( display.spriteInstance->name.Icmp( "_leftAlign" ) == 0 ) {
				//	float adj = (float)frameWidth  * 0.10;
				//	renderState2.matrix.tx = ( display.matrix.tx - adj ) * renderState.matrix.xx;
				//}
				//if ( display.spriteInstance->name.Icmp( "_rightAlign" ) == 0 ) {
				//	renderState2.matrix.tx = ( (float)renderSystem->GetWidth() - ( ( (float)frameWidth - display.matrix.tx - adj ) * renderState.matrix.xx ) );
				//}
		
				float widthAdj = swf_titleSafe.GetFloat() * frameWidth;
				float heightAdj = swf_titleSafe.GetFloat() * frameHeight;

				const float pixelAspect = renderSystem->GetPixelAspect();
				const float sysWidth = renderSystem->GetWidth() * ( pixelAspect > 1.0f ? pixelAspect : 1.0f );
				const float sysHeight = renderSystem->GetHeight() / ( pixelAspect < 1.0f ? pixelAspect : 1.0f );

				if ( display.spriteInstance->name.Icmp( "_fullScreen" ) == 0 ) {
					renderState2.matrix.tx = display.matrix.tx * renderState.matrix.xx;
					renderState2.matrix.ty = display.matrix.ty * renderState.matrix.yy;

					float xScale = sysWidth / (float)frameWidth;
					float yScale = sysHeight / (float)frameHeight;

					renderState2.matrix.xx = xScale;
					renderState2.matrix.yy = yScale;
				}

				if ( display.spriteInstance->name.Icmp( "_absTop" ) == 0 ) {
					renderState2.matrix.ty = display.matrix.ty * renderState.matrix.yy;
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				} else if ( display.spriteInstance->name.Icmp( "_top" ) == 0 ) {
					renderState2.matrix.ty = ( display.matrix.ty + heightAdj ) * renderState.matrix.yy;
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				} else if ( display.spriteInstance->name.Icmp( "_topLeft" ) == 0 ) {
					renderState2.matrix.tx = ( display.matrix.tx + widthAdj ) * renderState.matrix.xx;
					renderState2.matrix.ty = ( display.matrix.ty + heightAdj ) * renderState.matrix.yy;
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				} else if ( display.spriteInstance->name.Icmp( "_left" ) == 0 ) {
					float prevX = renderState2.matrix.tx;
					renderState2.matrix.tx = ( display.matrix.tx + widthAdj ) * renderState.matrix.xx;
					xOffset = (( renderState2.matrix.tx - prevX ) / renderState.matrix.xx );
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				} else if ( hfStr::FindText( display.spriteInstance->name, "_absLeft", false ) >= 0 ) {
					float prevX = renderState2.matrix.tx;
					renderState2.matrix.tx = display.matrix.tx * renderState.matrix.xx;
					xOffset = (( renderState2.matrix.tx - prevX ) / renderState.matrix.xx );
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				} else if ( display.spriteInstance->name.Icmp( "_bottomLeft" ) == 0 ) {
					float prevX = renderState2.matrix.tx;
					renderState2.matrix.tx = ( display.matrix.tx + widthAdj ) * renderState.matrix.xx;
					xOffset = (( renderState2.matrix.tx - prevX ) / renderState.matrix.xx );
					

					float prevY = renderState2.matrix.ty;
					renderState2.matrix.ty = ( (float)sysHeight - ( ( (float)frameHeight - display.matrix.ty + heightAdj ) * renderState.matrix.yy ) );
					yOffset = (( renderState2.matrix.ty - prevY ) / renderState.matrix.yy );
					
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				} else if ( display.spriteInstance->name.Icmp( "_absBottom" ) == 0 ) {
					renderState2.matrix.ty = ( (float)sysHeight - ( ( (float)frameHeight - display.matrix.ty ) * renderState.matrix.yy ) );
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				} else if ( display.spriteInstance->name.Icmp( "_bottom" ) == 0 ) {
					renderState2.matrix.ty = ( (float)sysHeight - ( ( (float)frameHeight - display.matrix.ty + heightAdj ) * renderState.matrix.yy ) );
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				} else if ( display.spriteInstance->name.Icmp( "_topRight" ) == 0 ) {
					renderState2.matrix.tx = ( (float)sysWidth - ( ( (float)frameWidth - display.matrix.tx + widthAdj ) * renderState.matrix.xx ) );
					renderState2.matrix.ty = ( display.matrix.ty + heightAdj ) * renderState.matrix.yy;
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				} else if ( display.spriteInstance->name.Icmp( "_right" ) == 0 ) {
					float prevX = renderState2.matrix.tx;
					renderState2.matrix.tx = ( (float)sysWidth - ( ( (float)frameWidth - display.matrix.tx + widthAdj ) * renderState.matrix.xx ) );
					xOffset = (( renderState2.matrix.tx - prevX ) / renderState.matrix.xx );
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				} else if ( hfStr::FindText( display.spriteInstance->name, "_absRight", true ) >= 0 ) {
					float prevX = renderState2.matrix.tx;
					renderState2.matrix.tx = ( (float)sysWidth - ( ( (float)frameWidth - display.matrix.tx ) * renderState.matrix.xx ) );
					xOffset = (( renderState2.matrix.tx - prevX ) / renderState.matrix.xx );
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );					
				} else if ( display.spriteInstance->name.Icmp( "_bottomRight" ) == 0 ) {
					renderState2.matrix.tx = ( (float)sysWidth - ( ( (float)frameWidth - display.matrix.tx + widthAdj ) * renderState.matrix.xx ) );
					renderState2.matrix.ty = ( (float)sysHeight - ( ( (float)frameHeight - display.matrix.ty + heightAdj ) * renderState.matrix.yy ) );
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				} else if ( display.spriteInstance->name.Icmp( "_absTopLeft" ) == 0 ) {	// ABSOLUTE CORNERS OF SCREEN
					renderState2.matrix.tx = display.matrix.tx * renderState.matrix.xx;
					renderState2.matrix.ty = display.matrix.ty * renderState.matrix.yy;
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				} else if ( display.spriteInstance->name.Icmp( "_absTopRight" ) == 0 ) {
					renderState2.matrix.tx = ( (float)sysWidth - ( ( (float)frameWidth - display.matrix.tx ) * renderState.matrix.xx ) );
					renderState2.matrix.ty = display.matrix.ty * renderState.matrix.yy;
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				} else if ( display.spriteInstance->name.Icmp( "_absBottomLeft" ) == 0 ) {
					renderState2.matrix.tx = display.matrix.tx * renderState.matrix.xx;
					renderState2.matrix.ty = ( (float)sysHeight - ( ( (float)frameHeight - display.matrix.ty ) * renderState.matrix.yy ) );
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				} else if ( display.spriteInstance->name.Icmp( "_absBottomRight" ) == 0 ) {
					renderState2.matrix.tx = ( (float)sysWidth - ( ( (float)frameWidth - display.matrix.tx ) * renderState.matrix.xx ) );
					renderState2.matrix.ty = ( (float)sysHeight - ( ( (float)frameHeight - display.matrix.ty ) * renderState.matrix.yy ) );
					display.spriteInstance->SetAlignment( spriteInstance->xOffset + xOffset, spriteInstance->yOffset + yOffset );
				}
			}
			
			RenderSprite( gui, display.spriteInstance, renderState2, time, isSplitscreen );
		} else if ( entry->type == SWF_DICT_SHAPE ) {
			RenderShape( gui, entry->shape, renderState2 );
		} else if ( entry->type == SWF_DICT_MORPH ) {
			RenderMorphShape( gui, entry->shape, renderState2 );
		} else if ( entry->type == SWF_DICT_EDITTEXT ) {
			RenderEditText( gui, display.textInstance, renderState2, time, isSplitscreen );
		} else {
			//hfLib::Warning( "%s: Tried to render an unrenderable character %d", filename.c_str(), entry->type );
		}
	}
	for ( int j = 0; j < activeMasks.Num(); j++ ) {
		const swfDisplayEntry_t * mask = activeMasks[ j ];
		RenderMask( gui, mask, renderState, STENCIL_DECR );
	}
}

/*
========================
hfSWF::GLStateForBlendMode
========================
*/
uint64 hfSWF::GLStateForRenderState( const swfRenderState_t & renderState ) {
	uint64 extraGLState = GLS_OVERRIDE | GLS_DEPTHFUNC_LESS | GLS_DEPTHMASK; // SWF GL State always overrides what's set in the material

	if ( renderState.activeMasks > 0 ) {
		extraGLState |= GLS_STENCIL_FUNC_EQUAL | GLS_STENCIL_MAKE_REF( 128 + renderState.activeMasks ) | GLS_STENCIL_MAKE_MASK( 255 );
	} else if ( renderState.activeMasks == STENCIL_INCR ) {
		return GLS_COLORMASK | GLS_ALPHAMASK | GLS_STENCIL_OP_FAIL_KEEP | GLS_STENCIL_OP_ZFAIL_KEEP | GLS_STENCIL_OP_PASS_INCR;
	} else if ( renderState.activeMasks == STENCIL_DECR ) {
		return GLS_COLORMASK | GLS_ALPHAMASK | GLS_STENCIL_OP_FAIL_KEEP | GLS_STENCIL_OP_ZFAIL_KEEP | GLS_STENCIL_OP_PASS_DECR;
	}

	switch ( renderState.blendMode ) {
		case 7: // difference : dst = abs( dst - src )
		case 9: // subtract : dst = dst - src
			return extraGLState | ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_BLENDOP_SUB );
		case 8: // add : dst = dst + src
			return extraGLState | ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
		case 6: // darken : dst = min( dst, src )
			return extraGLState | ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_BLENDOP_MIN );
		case 5: // lighten : dst = max( dst, src )
			return extraGLState | ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_BLENDOP_MAX );
		case 4: // screen : dst = dst + src - dst*src ( we only do dst - dst * src, we could do the extra + src with another pass if we need to)
			return extraGLState | ( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_BLENDOP_SUB );
		case 14: // hardlight : src < 0.5 ? multiply : screen
		case 13: // overlay : dst < 0.5 ? multiply  : screen
		case 3: // multiply : dst = ( dst * src ) + ( dst * (1-src.a) )
			return extraGLState | ( GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
		case 12: // erase
		case 11: // alpha
		case 10: // invert
		case 2: // layer
		case 1: // normal
		case 0: // normaler
		default:
			return extraGLState | ( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );
	}
}

/*
========================
hfSWF::RenderMorphShape
========================
*/
void hfSWF::RenderMorphShape( hfRenderSystem * gui, const hfSWFShape * shape, const swfRenderState_t & renderState ) {
	if ( shape == NULL ) {
		hfLib::Warning( "%s: RenderMorphShape: shape == NULL", filename.c_str() );
		return;
	}

	for ( int i = 0; i < shape->fillDraws.Num(); i++ ) {
		const hfSWFShapeDrawFill & fill = shape->fillDraws[i];
		const hfMaterial * material = NULL;
		swfColorXform_t color;

		if ( renderState.material != NULL ) {
			material = renderState.material;
		} else if ( fill.style.type == 0 ) {
			material = guiSolid;
			hfVec4 startColor = fill.style.startColor.ToVec4();
			hfVec4 endColor = fill.style.endColor.ToVec4();
			color.mul = Lerp( startColor, endColor, renderState.ratio );
		} else if ( fill.style.type == 4 && fill.style.bitmapID != 65535 ) {
			material = dictionary[ fill.style.bitmapID ].material;
		} else {
			material = guiSolid;
		}
		color = color.Multiply( renderState.cxf );
		if ( swf_forceAlpha.GetFloat() > 0.0f ) {
			color.mul.w = swf_forceAlpha.GetFloat();
			color.add.w = 0.0f;
		}
		if ( ( color.mul.w + color.add.w ) <= ALPHA_EPSILON ) {
			continue;
		}
		uint32 packedColorM = LittleLong( PackColor( color.mul ) );
		uint32 packedColorA = LittleLong( PackColor( ( color.add * 0.5f ) + hfVec4( 0.5f ) ) ); // Compress from -1..1 to 0..1

		swfRect_t bounds;
		bounds.tl = Lerp( shape->startBounds.tl, shape->endBounds.tl, renderState.ratio );
		bounds.br = Lerp( shape->startBounds.br, shape->endBounds.br, renderState.ratio );
		hfVec2 size( material->GetImageWidth(), material->GetImageHeight() );
		if ( renderState.materialWidth > 0 ) {
			size.x = renderState.materialWidth;
		}
		if ( renderState.materialHeight > 0 ) {
			size.y = renderState.materialHeight;
		}
		hfVec2 oneOverSize( 1.0f / size.x, 1.0f / size.y );

		swfMatrix_t styleMatrix;
		styleMatrix.xx = Lerp( fill.style.startMatrix.xx, fill.style.endMatrix.xx, renderState.ratio );
		styleMatrix.yy = Lerp( fill.style.startMatrix.yy, fill.style.endMatrix.yy, renderState.ratio );
		styleMatrix.xy = Lerp( fill.style.startMatrix.xy, fill.style.endMatrix.xy, renderState.ratio );
		styleMatrix.yx = Lerp( fill.style.startMatrix.yx, fill.style.endMatrix.yx, renderState.ratio );
		styleMatrix.tx = Lerp( fill.style.startMatrix.tx, fill.style.endMatrix.tx, renderState.ratio );
		styleMatrix.ty = Lerp( fill.style.startMatrix.ty, fill.style.endMatrix.ty, renderState.ratio );

		swfMatrix_t invMatrix = styleMatrix.Inverse();

		gui->SetGLState( GLStateForRenderState( renderState ) );

		hfDrawVert * verts = gui->AllocTris( fill.startVerts.Num(), fill.indices.Ptr(), fill.indices.Num(), material, renderState.stereoDepth );	
		if ( verts == NULL ) {
			continue;
		}

		for ( int j = 0; j < fill.startVerts.Num(); j++ ) {
			hfVec2 xy = Lerp( fill.startVerts[j], fill.endVerts[j], renderState.ratio );

			hfVec2 st;
			st.x = ( ( xy.x - bounds.tl.x ) * oneOverSize.x ) * 20.0f;
			st.y = ( ( xy.y - bounds.tl.y ) * oneOverSize.y ) * 20.0f;
			hfVec2 adjust( 0.5f * oneOverSize.x, 0.5f * oneOverSize.y );

			ALIGNTYPE16 hfDrawVert tempVert;

			tempVert.Clear();
			tempVert.xyz.ToVec2() = renderState.matrix.Transform( xy ).Scale( scaleToVirtual );
			tempVert.xyz.z = 0.0f;
			tempVert.SetTexCoord( invMatrix.Transform( st ) + adjust );
			tempVert.SetNativeOrderColor( packedColorM );
			tempVert.SetNativeOrderColor2( packedColorA );

			WriteDrawVerts16( & verts[j], & tempVert, 1 );
		}
	}
}

/*
========================
hfSWF::RenderShape
========================
*/
void hfSWF::RenderShape( hfRenderSystem * gui, const hfSWFShape * shape, const swfRenderState_t & renderState ) {
	if ( shape == NULL ) {
		hfLib::Warning( "%s: RenderShape: shape == NULL", filename.c_str() );
		return;
	}

	for ( int i = 0; i < shape->fillDraws.Num(); i++ ) {
		const hfSWFShapeDrawFill & fill = shape->fillDraws[i];
		const hfMaterial * material = NULL;
		swfColorXform_t color;

		swfMatrix_t invMatrix;
		hfVec2	atlasScale( 0.0f, 0.0f );
		hfVec2	atlasBias( 0.0f, 0.0f );
		bool	useAtlas = false;

		hfVec2 size( 1.0f, 1.0f );

		if ( renderState.material != NULL ) {
			material = renderState.material;
			invMatrix.xx = invMatrix.yy = ( 1.0f / 20.0f );
		} else if ( fill.style.type == 0 ) {
			material = guiSolid;
			color.mul = fill.style.startColor.ToVec4();
		} else if ( fill.style.type == 4 && fill.style.bitmapID != 65535 ) {
			// everything in a single image atlas
			hfSWFDictionaryEntry * entry = &dictionary[ fill.style.bitmapID ];
			material = atlasMaterial;
			hfVec2i	atlasSize( material->GetImageWidth(), material->GetImageHeight() );
			for ( int i = 0 ; i < 2 ; i++ ) {
				size[i] = entry->imageSize[i];
				atlasScale[i] = (float)size[i] / atlasSize[i];
				atlasBias[i] = (float)entry->imageAtlasOffset[i] / atlasSize[i];
			}
			// de-normalize color channels after DXT decompression
			color.mul = entry->channelScale;
			useAtlas = true;

			const swfMatrix_t & styleMatrix = fill.style.startMatrix;
			invMatrix = styleMatrix.Inverse();
		} else {
			material = guiSolid;
		}
		color = color.Multiply( renderState.cxf );
		if ( swf_forceAlpha.GetFloat() > 0.0f ) {
			color.mul.w = swf_forceAlpha.GetFloat();
			color.add.w = 0.0f;
		}
		if ( ( color.mul.w + color.add.w ) <= ALPHA_EPSILON ) {
			continue;
		}

		uint32 packedColorM = LittleLong( PackColor( color.mul ) );
		uint32 packedColorA = LittleLong( PackColor( ( color.add * 0.5f ) + hfVec4( 0.5f ) ) ); // Compress from -1..1 to 0..1

		const swfRect_t & bounds = shape->startBounds;
		if ( renderState.materialWidth > 0 ) {
			size.x = renderState.materialWidth;
		}
		if ( renderState.materialHeight > 0 ) {
			size.y = renderState.materialHeight;
		}
		hfVec2 oneOverSize( 1.0f / size.x, 1.0f / size.y );

		gui->SetGLState( GLStateForRenderState( renderState ) );

		hfDrawVert * verts = gui->AllocTris( fill.startVerts.Num(), fill.indices.Ptr(), fill.indices.Num(), material, renderState.stereoDepth );	
		if ( verts == NULL ) {
			continue;
		}

		ALIGNTYPE16 hfDrawVert tempVerts[4];
		for ( int j = 0; j < fill.startVerts.Num(); j++ ) {
			const hfVec2 & xy = fill.startVerts[j];

			hfDrawVert & vert = tempVerts[j & 3];

			vert.Clear();
			vert.xyz.ToVec2() = renderState.matrix.Transform( xy ).Scale( scaleToVirtual );
			vert.xyz.z = 0.0f;
			vert.SetNativeOrderColor( packedColorM );
			vert.SetNativeOrderColor2( packedColorA );

			// For some reason I don't understand, having texcoords
			// in the range of 2000 or so causes what should be solid
			// fill areas to have horizontal bands on nvidia, but not 360.
			// Forcing the texcoords to zero fixes it.
			if ( fill.style.type != 0 ) {
				hfVec2 st;
				// all the swf vertexes have an implicit scale of 1/20 for some reason...
				st.x = ( ( xy.x - bounds.tl.x ) * oneOverSize.x ) * 20.0f;
				st.y = ( ( xy.y - bounds.tl.y ) * oneOverSize.y ) * 20.0f;
				st = invMatrix.Transform( st );
				if ( useAtlas ) {
					st = st.Scale( atlasScale ) + atlasBias;
				}

				// inset the tc - the gui may use a vmtr and the tc might end up
				// crossing page boundaries if using [0.0,1.0]
				st.x = hfMath::ClampFloat( 0.001f, 0.999f, st.x );
				st.y = hfMath::ClampFloat( 0.001f, 0.999f, st.y );
				vert.SetTexCoord( st );
			}

			// write four verts at a time to video memory
			if ( ( j & 3 ) == 3 ) {
				WriteDrawVerts16( & verts[j & ~3], tempVerts, 4 );
			}
		}
		// write any remaining verts to video memory
		WriteDrawVerts16( & verts[fill.startVerts.Num() & ~3], tempVerts, fill.startVerts.Num() & 3 );
	}

	for ( int i = 0; i < shape->lineDraws.Num(); i++ ) {
		const hfSWFShapeDrawLine & line = shape->lineDraws[i];
		swfColorXform_t color;
		color.mul = line.style.startColor.ToVec4();
		color = color.Multiply( renderState.cxf );
		if ( swf_forceAlpha.GetFloat() > 0.0f ) {
			color.mul.w = swf_forceAlpha.GetFloat();
			color.add.w = 0.0f;
		}
		if ( ( color.mul.w + color.add.w ) <= ALPHA_EPSILON ) {
			continue;
		}
		uint32 packedColorM = LittleLong( PackColor( color.mul ) );
		uint32 packedColorA = LittleLong( PackColor( ( color.add * 0.5f ) + hfVec4( 0.5f ) ) ); // Compress from -1..1 to 0..1

		gui->SetGLState( GLStateForRenderState( renderState ) | GLS_POLYMODE_LINE );

		hfDrawVert * verts = gui->AllocTris( line.startVerts.Num(), line.indices.Ptr(), line.indices.Num(), white, renderState.stereoDepth );	
		if ( verts == NULL ) {
			continue;
		}

		for ( int j = 0; j < line.startVerts.Num(); j++ ) {
			const hfVec2 & xy = line.startVerts[j];

			ALIGNTYPE16 hfDrawVert tempVert;

			tempVert.Clear();
			tempVert.xyz.ToVec2() = renderState.matrix.Transform( xy ).Scale( scaleToVirtual );
			tempVert.xyz.z = 0.0f;
			tempVert.SetTexCoord( 0.0f, 0.0f );
			tempVert.SetNativeOrderColor( packedColorM );
			tempVert.SetNativeOrderColor2( packedColorA );

			WriteDrawVerts16( & verts[j], & tempVert, 1 );
		}
	}
}

/*
========================
hfSWF::DrawEditCursor
========================
*/
void hfSWF::DrawEditCursor( hfRenderSystem * gui, float x, float y, float w, float h, const swfMatrix_t & matrix ) {
	hfVec2 topl = matrix.Transform( hfVec2( x, y ) );
	hfVec2 topr = matrix.Transform( hfVec2( x + w, y ) );
	hfVec2 br = matrix.Transform( hfVec2( x + w, y + h ) );
	hfVec2 bl = matrix.Transform( hfVec2( x, y + h ) );
	DrawStretchPic( hfVec4( topl.x, topl.y, 0.0f, 0.0f ), hfVec4( topr.x, topr.y, 1.0f, 0.0f ), hfVec4( br.x, br.y, 1.0f, 1.0f ), hfVec4( bl.x, bl.y, 0.0f, 1.0f ), white );
}

/*
========================
hfSWF::RenderEditText
========================
*/
void hfSWF::RenderEditText( hfRenderSystem * gui, hfSWFTextInstance * textInstance, const swfRenderState_t & renderState, int time, bool isSplitscreen ) {
	if ( textInstance == NULL ) {
		hfLib::Warning( "%s: RenderEditText: textInstance == NULL", filename.c_str() );
		return;
	}

	if ( !textInstance->visible ) {
		return;
	}

	const hfSWFEditText * shape = textInstance->editText;

	hfStr text;

	if ( textInstance->variable.IsEmpty() ) {
		if ( textInstance->renderMode == SWF_TEXT_RENDER_PARAGRAPH ) {
			if ( textInstance->NeedsGenerateRandomText() ) {
				textInstance->StartParagraphText( Sys_Milliseconds() );
			}
			text = textInstance->GetParagraphText( Sys_Milliseconds() );
		} else if ( textInstance->renderMode == SWF_TEXT_RENDER_RANDOM_APPEAR || textInstance->renderMode == SWF_TEXT_RENDER_RANDOM_APPEAR_CAPS ) {
			if ( textInstance->NeedsGenerateRandomText() ) {
				textInstance->StartRandomText( Sys_Milliseconds() );
			}
			text = textInstance->GetRandomText( Sys_Milliseconds() );
		} else {
			text = hfLocalization::GetString( textInstance->text );
		}
	} else {
		hfSWFScriptVar var = globals->Get( textInstance->variable );
		if ( var.IsUndefined() ) {
			text = hfLocalization::GetString( textInstance->text );
		} else {
			text = hfLocalization::GetString( var.ToString() );
		}
	}

	if ( text.Length() == 0 ) {
		textInstance->selectionEnd = -1;
		textInstance->selectionStart = -1;
	}

	if ( textInstance->NeedsSoundPlayed() ) {
		PlaySound( textInstance->GetSoundClip() );
		textInstance->ClearPlaySound();
	}	

	if ( textInstance->tooltip ) {
		FindTooltipIcons( &text );
	} else {
		tooltipIconList.Clear();
	}

	int selStart = textInstance->selectionStart;
	int selEnd = textInstance->selectionEnd;

	int cursorPos = selEnd;

	bool inputField = false;

	hfSWFScriptVar focusWindow = globals->Get( "focusWindow" );
	if ( focusWindow.IsObject() && focusWindow.GetObject() == &textInstance->scriptObject ) {
		inputField = true;
	}

	bool drawCursor = false;

	if ( inputField && ( ( hfLib::frameNumber >> 4 ) & 1 ) == 0 ) {
		cursorPos = selEnd;
		drawCursor = true;
	}
	if ( selStart > selEnd ) {
		SwapValues( selStart, selEnd );
	}

	hfVec2 xScaleVec = renderState.matrix.Scale( hfVec2( 1.0f, 0.0f ) );
	hfVec2 yScaleVec = renderState.matrix.Scale( hfVec2( 0.0f, 1.0f ) );

	float xScale = xScaleVec.Length();
	float yScale = yScaleVec.Length();

	if ( isSplitscreen ) {
		yScale *= 0.5f;
	}

	float invXScale = 1.0f / xScale;
	float invYScale = 1.0f / yScale;

	swfMatrix_t matrix = renderState.matrix;
	matrix.xx *= invXScale;
	matrix.xy *= invXScale;
	matrix.yy *= invYScale;
	matrix.yx *= invYScale;

	hfSWFDictionaryEntry * fontEntry = FindDictionaryEntry( shape->fontID, SWF_DICT_FONT );
	if ( fontEntry == NULL ) {
		hfLib::Warning( "hfSWF::RenderEditText: NULL Font" );
		return;
	}
	hfSWFFont * swfFont = fontEntry->font;

	float postTransformHeight = SWFTWIP( shape->fontHeight ) * yScale;

	const hfFont * fontInfo = swfFont->fontID;

	float glyphScale = postTransformHeight / 48.0f;
	float imageScale = postTransformHeight / 24.0f;
	textInstance->glyphScale = glyphScale;

	hfVec4 defaultColor = textInstance->color.ToVec4();
	defaultColor = defaultColor.Multiply( renderState.cxf.mul ) + renderState.cxf.add;
	if ( swf_forceAlpha.GetFloat() > 0.0f ) {
		defaultColor.w = swf_forceAlpha.GetFloat();
	}
	if ( defaultColor.w <= ALPHA_EPSILON ) {
		return;
	}

	hfVec4 selColor( defaultColor );
	selColor.w *= 0.5f;

	gui->SetColor( defaultColor );
	gui->SetGLState( GLStateForRenderState( renderState ) );

	swfRect_t bounds;
	bounds.tl.x = xScale * ( shape->bounds.tl.x + SWFTWIP( shape->leftMargin ) );
	bounds.br.x = xScale * ( shape->bounds.br.x - SWFTWIP( shape->rightMargin ) );

	float linespacing = fontInfo->GetAscender( 1.15f * glyphScale );
	if ( shape->leading != 0 ) {
		linespacing += SWFTWIP( shape->leading );
	}

	bounds.tl.y = yScale * ( shape->bounds.tl.y + ( 1.15f * glyphScale ) );
	bounds.br.y = yScale * ( shape->bounds.br.y );

	textInstance->linespacing = linespacing;
	textInstance->bounds = bounds;
	
	if ( shape->flags & SWF_ET_AUTOSIZE ) {
		bounds.br.x = frameWidth;
		bounds.br.y = frameHeight;
	}
	
	if ( drawCursor && cursorPos <= 0 ) {
		float yPos = 0.0f;
		scaledGlyphInfo_t glyph;
		fontInfo->GetScaledGlyph( glyphScale, ' ', glyph );
		yPos = glyph.height / 2.0f;
		DrawEditCursor( gui, bounds.tl.x, yPos, 1.0f, linespacing, matrix );
	}

	if ( textInstance->IsSubtitle() ) {
		if ( text.IsEmpty() && textInstance->subtitleText.IsEmpty() ) {
			return;
		}
	} else if ( text.IsEmpty() ) {
		return;
	}

	float x = bounds.tl.x;
	float y = bounds.tl.y;

	int maxLines = hfMath::Ftoi( ( bounds.br.y - bounds.tl.y ) / linespacing );
	if ( maxLines == 0 ) {
		maxLines = 1;
	}

	textInstance->maxLines = maxLines;

	hfList< hfStr > textLines;
	hfStr * currentLine = &textLines.Alloc();

	// tracks the last breakable character we found
	int lastbreak = 0;
	float lastbreakX = 0;

	bool insertingImage = false;
	int iconIndex = 0;

	int charIndex = 0;

	if ( textInstance->IsSubtitle() ) {
		charIndex = textInstance->GetSubStartIndex();
	}

	while ( charIndex < text.Length() ) {
		if ( text[ charIndex ] == '\n' ) {
			if ( shape->flags & SWF_ET_MULTILINE ) {
				currentLine->Append( '\n' );
				x = bounds.tl.x;
				y += linespacing;
				currentLine = &textLines.Alloc();			
				lastbreak = 0;
				charIndex++;
				continue;
			} else {
				break;
			}
		}
		int glyphStart = charIndex;
		uint32 tc = text.UTF8Char( charIndex );
		scaledGlyphInfo_t glyph;
		fontInfo->GetScaledGlyph( glyphScale, tc, glyph );
		float glyphSkip = glyph.xSkip;
		if ( textInstance->HasStroke() ) {
			glyphSkip += ( swf_textStrokeSizeGlyphSpacer.GetFloat() * textInstance->GetStrokeWeight() * glyphScale );
		}

		tooltipIcon_t iconCheck;
		
		if ( iconIndex < tooltipIconList.Num() ) {
			iconCheck = tooltipIconList[iconIndex];
		}

		float imageSkip = 0.0f;

		if ( charIndex - 1 == iconCheck.startIndex ) {
			insertingImage = true;
			imageSkip = iconCheck.imageWidth * imageScale;
		} else if ( charIndex - 1 == iconCheck.endIndex ) {
			insertingImage = false;
			iconIndex++;
			glyphSkip = 0.0f;
		}

		if ( insertingImage ) {
			glyphSkip = 0.0f;
		}

		if ( !inputField ) { // only break lines of text when we are not inputting data
			if ( x + glyphSkip > bounds.br.x || x + imageSkip > bounds.br.x ) {
				if ( shape->flags & ( SWF_ET_MULTILINE | SWF_ET_WORDWRAP ) ) {
					if ( lastbreak > 0 ) {
						int curLineIndex = currentLine - &textLines[0];
						hfStr * newline = &textLines.Alloc();
						currentLine = &textLines[ curLineIndex ];
						if ( maxLines == 1 ) {
							currentLine->CapLength( currentLine->Length() - 3 );
							currentLine->Append( "..." );
							break;
						} else {
							*newline = currentLine->c_str() + lastbreak;
							currentLine->CapLength( lastbreak );
							currentLine = newline;
							x -= lastbreakX;
						}
					} else {
						currentLine = &textLines.Alloc();
						x = bounds.tl.x;
					}
					lastbreak = 0;
				} else {
					break;
				}
			}
		}
		while ( glyphStart < charIndex && glyphStart < text.Length() ) {
			currentLine->Append( text[ glyphStart++ ] );
		}
		x += glyphSkip + imageSkip;
		if ( tc == ' ' || tc == '-' ) {
			lastbreak = currentLine->Length();
			lastbreakX = x;
		}
	}

	// Subtitle functionality
	if ( textInstance->IsSubtitle() && textInstance->IsUpdatingSubtitle() ) {
		if ( textLines.Num() > 0 && textInstance->SubNeedsSwitch() ) {

			int lastWordIndex = textInstance->GetApporoximateSubtitleBreak( time );	
			int newEndChar = textInstance->GetSubStartIndex() + textLines[0].Length();

			int wordCount = 0;
			bool earlyOut = false;
			for ( int index = 0; index < textLines[0].Length(); ++index ) {
				if ( textLines[0][index] == ' ' || textLines[0][index] == '-' ) {
					if ( index != 0 ) {
						if ( wordCount == lastWordIndex ) {
							newEndChar = textInstance->GetSubStartIndex() + index;
							earlyOut = true;
							break;
						}

						// cover the double space at the beginning of sentences
						if ( index > 0 && textLines[0][index - 1 ] != ' ' ) {
							wordCount++;
						}
					}
				} else if ( index == textLines[0].Length() ) {
					if ( wordCount == lastWordIndex ) {
						newEndChar = textInstance->GetSubStartIndex() + index;
						earlyOut = true;
						break;
					}
					wordCount++;
				}
			}

			if ( wordCount <= 0 && textLines[0].Length() > 0 ) {
				wordCount = 1;
			}
			
			if ( !earlyOut ) {
				textInstance->LastWordChanged( wordCount, time );
			}

			textInstance->SetSubEndIndex( newEndChar, time );
				
			hfStr subText = textLines[0].Left( newEndChar - textInstance->GetSubStartIndex() );
			hfSWFParmList parms;
			parms.Append( subText );
			parms.Append( textInstance->GetSpeaker().c_str() );
			parms.Append( textInstance->GetSubAlignment() );
			Invoke( "subtitleChanged", parms );
			parms.Clear();

			textInstance->SetSubNextStartIndex( textInstance->GetSubEndIndex() );
			textInstance->SwitchSubtitleText( time );
		}

		if ( !textInstance->UpdateSubtitle( time ) ) {			
			textInstance->SubtitleComplete();
			hfSWFParmList parms;
			parms.Append( textInstance->GetSubAlignment() );
			Invoke( "subtitleComplete", parms );
			parms.Clear();
			textInstance->SubtitleCleanup();
		}
	}

	//*************************************************
	// CALCULATE THE NUMBER OF SCROLLS LINES LEFT
	//*************************************************

	textInstance->CalcMaxScroll( textLines.Num() - maxLines );

	int c = 1;
	int textLine = textInstance->scroll;	

	if ( textLine + maxLines > textLines.Num() && maxLines < textLines.Num() ) {
		textLine = textLines.Num() - maxLines;
		textInstance->scroll = textLine;
	} else if ( textLine < 0 || textLines.Num() <= maxLines ) {
		textLine = 0;
		textInstance->scroll = textLine;
	} else if ( textInstance->renderMode == SWF_TEXT_RENDER_AUTOSCROLL ) {
		textLine = textLines.Num() - maxLines;
		textInstance->scroll = textInstance->maxscroll;
	}

	// END SCROLL CALCULATION
	//*************************************************

	int index = 0;

	int startCharacter = 0;
	int endCharacter = 0;
	int inputEndChar = 0;
	iconIndex = 0;
	int overallIndex = 0;
	int curIcon = 0;
	float yPrevBottomOffset = 0.0f;
	float yOffset = 0;

	int strokeXOffsets[] = { -1, 1, -1, 1 };
	int strokeYOffsets[] = { -1, -1, 1, 1 };

	hfStr inputText;
	if ( inputField ) {
		if ( textLines.Num() > 0 ) {
			hfStr & text = textLines[0];
			float left = bounds.tl.x;

			int startCheckIndex = textInstance->GetInputStartChar();

			if ( startCheckIndex >= text.Length() ) {
				startCheckIndex = 0;
			}

			if ( cursorPos < startCheckIndex && cursorPos >= 0 ) {
				startCheckIndex = cursorPos;
			}

			bool endFound = false;
			int c = startCheckIndex;
			while ( c < text.Length() ) {
				uint32 tc = text.UTF8Char( c );
				scaledGlyphInfo_t glyph;
				fontInfo->GetScaledGlyph( glyphScale, tc, glyph );
				float glyphSkip = glyph.xSkip;
				if ( textInstance->HasStroke() ) {
					glyphSkip += ( swf_textStrokeSizeGlyphSpacer.GetFloat() * textInstance->GetStrokeWeight() * glyphScale );
				}

				if ( left + glyphSkip > bounds.br.x ) {
					if ( cursorPos > c && cursorPos != endCharacter ) {

						float removeSize = 0.0f;

						while ( removeSize < glyphSkip ) {
							if ( endCharacter == c ) {
								break;
							}
							scaledGlyphInfo_t removeGlyph;
							fontInfo->GetScaledGlyph( glyphScale, inputText[ endCharacter++ ], removeGlyph );
							removeSize += removeGlyph.xSkip;
						}

						left -= removeSize;
					} else {
						inputEndChar = c;
						endFound = true;
						break;
					}
				} 
				inputText.AppendUTF8Char( tc );
				left += glyphSkip;
			}

			if ( !endFound ) {
				inputEndChar = text.Length();
			}

			startCheckIndex += endCharacter;
			textInstance->SetInputStartCharacter( startCheckIndex );
			endCharacter = startCheckIndex;
		}
	}
	
	for ( int t = 0; t < textLines.Num(); t++ ) {

		if ( textInstance->IsSubtitle() && t > 0 ) {
			break;
		}
		
		if ( t < textLine ) {
			hfStr & text = textLines[t];
			c += text.Length();
			startCharacter = endCharacter;
			endCharacter = startCharacter + text.Length();
			overallIndex += text.Length();

			// find the right icon index if we scrolled passed the previous ones
			for ( int iconChar = curIcon; iconChar < tooltipIconList.Num(); ++iconChar ) {
				if ( endCharacter > tooltipIconList[iconChar].startIndex ) {
					curIcon++;
				} else {
					break;
				}
			}

			continue;
		}

		if ( index == maxLines ) {
			break;
		}

		startCharacter = endCharacter;

		hfStr & text = textLines[textLine];
		int lastChar = text.Length();
		if ( textInstance->IsSubtitle() ) {
			lastChar = textInstance->GetSubEndIndex();
		}

		textLine++;

		if ( inputField ) {
			if ( inputEndChar == 0 ) {
				inputEndChar += 1;
			}
			selStart -= startCharacter;
			selEnd -= startCharacter;
			cursorPos -= startCharacter;
			endCharacter = inputEndChar;
			lastChar = endCharacter;
			text = text.Mid( startCharacter, endCharacter - startCharacter );
		} else {

			if ( lastChar == 0 ) {
				// blank line so add space char
				endCharacter = startCharacter + 1;
			} else {
				endCharacter = startCharacter + lastChar;
			}
		}

		float width = 0.0f;
		insertingImage = false;
		int i = 0;
		while ( i < lastChar ) {
			if ( curIcon < tooltipIconList.Num() && tooltipIconList[curIcon].startIndex == startCharacter + i ) {
				width += tooltipIconList[curIcon].imageWidth * imageScale;
				i += tooltipIconList[curIcon].endIndex - tooltipIconList[curIcon].startIndex - 1;
				curIcon++;
			} else {
				if ( i < text.Length() ) {
					scaledGlyphInfo_t glyph;
					fontInfo->GetScaledGlyph( glyphScale, text.UTF8Char( i ), glyph );
					width += glyph.xSkip;
					if ( textInstance->HasStroke() ) {
						width += ( swf_textStrokeSizeGlyphSpacer.GetFloat() * textInstance->GetStrokeWeight() * glyphScale );
					}
				} else {
					i++;
				}
			}
		}

		y = bounds.tl.y + ( index * linespacing );

		float biggestGlyphHeight = 0.0f;		
		/*for ( int image = 0; image < tooltipIconList.Num(); ++image ) {
			if ( tooltipIconList[image].startIndex >= startCharacter && tooltipIconList[image].endIndex < endCharacter ) {
				biggestGlyphHeight = tooltipIconList[image].imageHeight > biggestGlyphHeight ? tooltipIconList[image].imageHeight : biggestGlyphHeight;
			}
		}*/

		float yBottomOffset = 0.0f;
		float yTopOffset = 0.0f;

		if ( biggestGlyphHeight > 0.0f ) {
		
			float topSpace = 0.0f;
			float bottomSpace = 0.0f;

			int idx = 0;
			scaledGlyphInfo_t glyph;
			fontInfo->GetScaledGlyph( glyphScale, text.UTF8Char( idx ), glyph );

			topSpace = ( ( biggestGlyphHeight * imageScale ) - glyph.height ) / 2.0f;

			bottomSpace = topSpace;

			if ( topSpace > 0.0f && t != 0 ) {
				yTopOffset += topSpace;
			}

			if ( bottomSpace > 0.0f ) {
				yBottomOffset += bottomSpace;
			}
		} else {
			yBottomOffset = 0.0f;
		}

		if ( t != 0 ) {
			if ( yPrevBottomOffset > 0 || yTopOffset > 0 ) {
				yOffset += yTopOffset > yPrevBottomOffset ? yTopOffset : yPrevBottomOffset;
			}
		}

		y += yOffset;		
		yPrevBottomOffset = yBottomOffset;		

		float extraSpace = 0.0f;
		switch ( shape->align ) {
		case SWF_ET_ALIGN_LEFT:
			x = bounds.tl.x;
			break;
		case SWF_ET_ALIGN_RIGHT:
			x = bounds.br.x - width;
			break;
		case SWF_ET_ALIGN_CENTER:
			x = ( bounds.tl.x + bounds.br.x - width ) * 0.5f;
			break;
		case SWF_ET_ALIGN_JUSTIFY:
			x = bounds.tl.x;
			if ( width > ( bounds.br.x - bounds.tl.x ) * 0.5f && index < textLines.Num() - 1 ) {
				extraSpace = ( ( bounds.br.x - bounds.tl.x ) - width ) / ( (float) lastChar - 1.0f );
			}
			break;
		}

		tooltipIcon_t icon;
		insertingImage = false;

		// find the right icon index if we scrolled passed the previous ones
		for ( int iconChar = iconIndex; iconChar < tooltipIconList.Num(); ++iconChar ) {
			if ( overallIndex > tooltipIconList[iconChar].startIndex ) {
				iconIndex++;
			} else {
				break;
			}
		}

		float baseLine = y + ( fontInfo->GetAscender( glyphScale ) );

		i = 0;
		int overallLineIndex = 0;
		hfVec4 textColor = defaultColor;
		while ( i < lastChar ) {		

			if ( i >= text.Length() ) {
				break;
			}

			// Support colors
			if ( !textInstance->ignoreColor ) {
				if ( text[ i ] == C_COLOR_ESCAPE ) {
					if ( hfStr::IsColor( text.c_str() + i++ ) ) {
						if ( text[ i ] == C_COLOR_DEFAULT ) {
							i++;
							textColor = defaultColor;
						} else {
							textColor = hfStr::ColorForIndex( text[ i++ ] );
							textColor.w = defaultColor.w;
						}
						continue;
					}
				}
			}

			uint32 character = text.UTF8Char( i );

			if ( character == '\n' ) {
				c++;
				overallIndex += i - overallLineIndex;
				overallLineIndex = i;;
				continue;
			}

			// Skip a single leading space
			if ( character == ' ' && i == 1 ) {
				c++;
				overallIndex += i - overallLineIndex;
				overallLineIndex = i;
				continue;
			}

			if ( iconIndex <  tooltipIconList.Num() ) {
				icon = tooltipIconList[iconIndex];
			}
			
			if ( overallIndex == icon.startIndex ) {
				insertingImage = true;

				scaledGlyphInfo_t glyph;
				fontInfo->GetScaledGlyph( glyphScale, character, glyph );

				float imageHeight = icon.imageHeight * imageScale;
				float glyphHeight = glyph.height;

				float imageY = 0.0f;
				if ( icon.baseline == 0 ) {
					imageY = baseLine - glyph.top;
					imageY += ( glyphHeight - imageHeight ) * 0.5f;
					imageY += 2.0f;
				} else {
					imageY = ( y + glyphHeight ) - ( ( icon.imageHeight * imageScale ) - ( glyphHeight ) );	
				} 
				
				float imageX = x + glyph.left;
				float imageW = icon.imageWidth * imageScale;
				float imageH = icon.imageHeight * imageScale;

				hfVec2 topl = matrix.Transform( hfVec2( imageX, imageY ) );
				hfVec2 topr = matrix.Transform( hfVec2( imageX + imageW, imageY ) );
				hfVec2 br = matrix.Transform( hfVec2( imageX + imageW, imageY + imageH ) );
				hfVec2 bl = matrix.Transform( hfVec2( imageX, imageY + imageH ) );

				float s1 = 0.0f;
				float t1 = 0.0f;
				float s2 = 1.0f;
				float t2 = 1.0f;

				//uint32 color = gui->GetColor();
				hfVec4 imgColor = colorWhite;
				imgColor.w = defaultColor.w;
				gui->SetColor( imgColor );
				DrawStretchPic( hfVec4( topl.x, topl.y, s1, t1 ), hfVec4( topr.x, topr.y, s2, t1 ), hfVec4( br.x, br.y, s2, t2 ), hfVec4( bl.x, bl.y, s1, t2 ), icon.material );
				gui->SetColor( defaultColor );

				x += icon.imageWidth * imageScale;
				x += extraSpace;

			} else if ( overallIndex == icon.endIndex ) {
				insertingImage = false;
				iconIndex++;	
			}

			if ( insertingImage ) {
				overallIndex += i - overallLineIndex;
				overallLineIndex = i;
				continue;
			}

			// the glyphs texcoords assume nearest filtering, to get proper
			// bilinear support we need to go an extra half texel on each side
			scaledGlyphInfo_t glyph;
			fontInfo->GetScaledGlyph( glyphScale, character, glyph );

			float glyphSkip = glyph.xSkip;
			if ( textInstance->HasStroke() ) {
				glyphSkip += ( swf_textStrokeSizeGlyphSpacer.GetFloat() * textInstance->GetStrokeWeight() * glyphScale );
			}

			float glyphW = glyph.width + 1.0f;	// +1 for bilinear half texel on each side
			float glyphH = glyph.height + 1.0f;

			float glyphY = baseLine - glyph.top;
			float glyphX = x + glyph.left;

			hfVec2 topl = matrix.Transform( hfVec2( glyphX, glyphY ) );
			hfVec2 topr = matrix.Transform( hfVec2( glyphX + glyphW, glyphY ) );
			hfVec2 br = matrix.Transform( hfVec2( glyphX + glyphW, glyphY + glyphH ) );
			hfVec2 bl = matrix.Transform( hfVec2( glyphX, glyphY + glyphH ) );

			float s1 = glyph.s1;
			float t1 = glyph.t1;
			float s2 = glyph.s2;
			float t2 = glyph.t2;
			if ( c > selStart && c <= selEnd ) {
				hfVec2 topl = matrix.Transform( hfVec2( x, y ) );
				hfVec2 topr = matrix.Transform( hfVec2( x + glyphSkip, y ) );
				hfVec2 br = matrix.Transform( hfVec2( x + glyphSkip, y + linespacing ) );
				hfVec2 bl = matrix.Transform( hfVec2( x, y + linespacing ) );
				gui->SetColor( selColor );
				DrawStretchPic( hfVec4( topl.x, topl.y, 0, 0 ), hfVec4( topr.x, topr.y, 1, 0 ), hfVec4( br.x, br.y, 1, 1 ), hfVec4( bl.x, bl.y, 0, 1 ), white );
				gui->SetColor( textColor );
			}

			if ( textInstance->GetHasDropShadow() ) {
			
				float dsY = glyphY + glyphScale * 2.0f;
				float dsX = glyphX + glyphScale * 2.0f;

				hfVec2 dstopl = matrix.Transform( hfVec2( dsX, dsY ) );
				hfVec2 dstopr = matrix.Transform( hfVec2( dsX + glyphW, dsY ) );
				hfVec2 dsbr = matrix.Transform( hfVec2( dsX + glyphW, dsY + glyphH ) );
				hfVec2 dsbl = matrix.Transform( hfVec2( dsX, dsY + glyphH ) );

				hfVec4 dsColor = colorBlack;
				dsColor.w = defaultColor.w;
				gui->SetColor( dsColor );
				DrawStretchPic( hfVec4( dstopl.x, dstopl.y, s1, t1 ), hfVec4( dstopr.x, dstopr.y, s2, t1 ), hfVec4( dsbr.x, dsbr.y, s2, t2 ), hfVec4( dsbl.x, dsbl.y, s1, t2 ), glyph.material );
				gui->SetColor( textColor );
			} else if ( textInstance->HasStroke() ) {
				
				hfVec4 strokeColor = colorBlack;
				strokeColor.w = textInstance->GetStrokeStrength() * defaultColor.w;
				gui->SetColor( strokeColor );
				for ( int index = 0; index < 4; ++index ) {
					float xPos = glyphX + ( ( strokeXOffsets[ index ] * textInstance->GetStrokeWeight() ) * glyphScale );
					float yPos = glyphY + ( ( strokeYOffsets[ index ] * textInstance->GetStrokeWeight() ) * glyphScale );
					hfVec2 topLeft = matrix.Transform( hfVec2( xPos, yPos ) );
					hfVec2 topRight = matrix.Transform( hfVec2( xPos + glyphW, yPos ) );
					hfVec2 botRight = matrix.Transform( hfVec2( xPos + glyphW, yPos + glyphH ) );
					hfVec2 botLeft = matrix.Transform( hfVec2( xPos, yPos + glyphH ) );					
					DrawStretchPic( hfVec4( topLeft.x, topLeft.y, s1, t1 ), hfVec4( topRight.x, topRight.y, s2, t1 ), hfVec4( botRight.x, botRight.y, s2, t2 ), hfVec4( botLeft.x, botLeft.y, s1, t2 ), glyph.material );					
				}
				gui->SetColor( textColor );
			}

			DrawStretchPic( hfVec4( topl.x, topl.y, s1, t1 ), hfVec4( topr.x, topr.y, s2, t1 ), hfVec4( br.x, br.y, s2, t2 ), hfVec4( bl.x, bl.y, s1, t2 ), glyph.material );
			x += glyphSkip;
			x += extraSpace;
			if ( cursorPos == c ) {
				DrawEditCursor( gui, x - 1.0f, y, 1.0f, linespacing, matrix );
			}
			c++;
			overallIndex += i - overallLineIndex;
			overallLineIndex = i;
		}

		index++;
	}
}

/*
========================
hfSWF::FindTooltipIcons

This replaces text like "_use" with platform specific text like "<JOY1>"
========================
*/
void hfSWF::FindTooltipIcons( hfStr * text ) {

	tooltipIconList.Clear();

	for ( int i = UB_MAX_BUTTONS - 1; i >= 0; i-- ) {
	//for ( userCmdString_t * ucs = userCmdStrings ; ucs->string ; ucs++ ) {
		userCmdString_t ucs = userCmdStrings[i];
		if ( ucs.string && hfStr::FindText( text->c_str(), ucs.string, false ) != hfStr::INVALID_POSITION ) {
			hfStr replacement;

			keyBindings_t bind = hfKeyInput::KeyBindingsFromBinding( ucs.string, true );
			hfStr gamepad = "<";
			gamepad.Append( bind.gamepad );
			gamepad.Append( ">" );

			if ( !in_useJoystick.GetBool() ) {
				
				if ( !bind.mouse.IsEmpty() ) {
					replacement.Format( "<%s>", bind.mouse.c_str() );
				} else if ( !bind.keyboard.IsEmpty() ) {
					replacement = bind.keyboard;
				}
				if ( replacement.IsEmpty() ) {
					text->Replace( ucs.string, hfStrId( "#str_swf_unbound" ).GetLocalizedString() );
				}
			} else {
				replacement = gamepad;
			}
			if ( !replacement.IsEmpty() ) {
				replacement.ToUpper();
				text->Replace( ucs.string, replacement.c_str() );
			}
		}
	}

	for ( int count = 0; count < tooltipButtonImage.Num(); ++count ) {
		int index = -1;
		while ( ( index = hfStr::FindText( text->c_str(), tooltipButtonImage[count].key, false, index + 1 ) ) != hfStr::INVALID_POSITION ) {
			tooltipIcon_t icon;
			icon.startIndex = index;
			icon.endIndex = index + hfStr::Length(tooltipButtonImage[count].key);

			icon.material = declManager->FindMaterial( tooltipButtonImage[count].xbImage );

			if ( icon.material ) {
				icon.imageWidth = tooltipButtonImage[count].width;
				icon.imageHeight = tooltipButtonImage[count].height;
				icon.baseline = tooltipButtonImage[count].baseline;
			} else {
				icon.imageWidth = 0;
				icon.imageHeight = 0;
				icon.baseline = 0;
			}

			bool inserted = false;
			if ( tooltipIconList.Num() > 0 ) {
				for ( int i = 0; i < tooltipIconList.Num(); ++i ) {
					if ( tooltipIconList[i].startIndex > icon.startIndex ) {
						tooltipIconList.Insert( icon, i );
						inserted = true;
						break;
					}
				}				
			} 
	
			if ( !inserted ) {
				tooltipIconList.Append( icon );
			}			
		}
	}
}



