#pragma hdrstop
#include "../hflib/hfprecompiled.h"

/*
===================
hfSWF::HitTest
===================
*/
hfSWFScriptObject * hfSWF::HitTest( hfSWFSpriteInstance * spriteInstance, const swfRenderState_t & renderState, int x, int y, hfSWFScriptObject * parentObject ) {

	if ( spriteInstance->parent != NULL ) {
		swfDisplayEntry_t * thisDisplayEntry = spriteInstance->parent->FindDisplayEntry( spriteInstance->depth );
		if ( thisDisplayEntry->cxf.mul.w + thisDisplayEntry->cxf.add.w < 0.001f ) {
			return NULL;
		}
	}

	if ( !spriteInstance->isVisible ) {
		return NULL;
	}

	if ( spriteInstance->scriptObject->HasValidProperty( "onRelease" ) 
		|| spriteInstance->scriptObject->HasValidProperty( "onPress" ) 
		|| spriteInstance->scriptObject->HasValidProperty( "onRollOver" )
		|| spriteInstance->scriptObject->HasValidProperty( "onRollOut" )
		|| spriteInstance->scriptObject->HasValidProperty( "onDrag" ) 
		) {
		parentObject = spriteInstance->scriptObject;
	}

	// rather than returning the first object we find, we actually want to return the last object we find
	hfSWFScriptObject * returnObject = NULL;
	
	float xOffset = spriteInstance->xOffset;
	float yOffset = spriteInstance->yOffset;
	
	for ( int i = 0; i < spriteInstance->displayList.Num(); i++ ) {
		const swfDisplayEntry_t & display = spriteInstance->displayList[i];
		hfSWFDictionaryEntry * entry = FindDictionaryEntry( display.characterID );
		if ( entry == NULL ) {
			continue;
		}
		swfRenderState_t renderState2;
		renderState2.matrix = display.matrix.Multiply( renderState.matrix );
		renderState2.ratio = display.ratio;

		if ( entry->type == SWF_DICT_SPRITE ) {
			hfSWFScriptObject * object = HitTest( display.spriteInstance, renderState2, x, y, parentObject );
			if ( object != NULL && object->Get( "_visible" ).ToBool() ) {
				returnObject = object;
			}
		} else if ( entry->type == SWF_DICT_SHAPE && ( parentObject != NULL ) ) {
			hfSWFShape * shape = entry->shape;
			for ( int i = 0; i < shape->fillDraws.Num(); i++ ) {
				const hfSWFShapeDrawFill & fill = shape->fillDraws[i];
				for ( int j = 0; j < fill.indices.Num(); j+=3 ) {
					hfVec2 xy1 = renderState2.matrix.Transform( fill.startVerts[fill.indices[j+0]] );
					hfVec2 xy2 = renderState2.matrix.Transform( fill.startVerts[fill.indices[j+1]] );
					hfVec2 xy3 = renderState2.matrix.Transform( fill.startVerts[fill.indices[j+2]] );

					hfMat3 edgeEquations;
					edgeEquations[0].Set( xy1.x + xOffset, xy1.y + yOffset, 1.0f );
					edgeEquations[1].Set( xy2.x + xOffset, xy2.y + yOffset, 1.0f );
					edgeEquations[2].Set( xy3.x + xOffset, xy3.y + yOffset, 1.0f );
					edgeEquations.InverseSelf();

					hfVec3 p( x, y, 1.0f );
					hfVec3 signs = p * edgeEquations;

					bool bx = signs.x > 0;
					bool by = signs.y > 0;
					bool bz = signs.z > 0;
					if ( bx == by && bx == bz ) {
						// point inside
						returnObject = parentObject;
					}
				}
			}
		} else if ( entry->type == SWF_DICT_MORPH ) {
			// FIXME: this should be roughly the same as SWF_DICT_SHAPE
		} else if ( entry->type == SWF_DICT_TEXT ) {
			// FIXME: this should be roughly the same as SWF_DICT_SHAPE
		} else if ( entry->type == SWF_DICT_EDITTEXT ) {
			hfSWFScriptObject * editObject = NULL;

			if ( display.textInstance->scriptObject.HasProperty( "onRelease" ) || display.textInstance->scriptObject.HasProperty( "onPress" ) ) {
				// if the edit box itself can be clicked, then we want to return it when it's clicked on
				editObject = &display.textInstance->scriptObject;
			} else if ( parentObject != NULL ) {
				// otherwise, we want to return the parent object
				editObject = parentObject;
			}

			if ( editObject == NULL ) {
				continue;
			}

			if ( display.textInstance->text.IsEmpty() ) {
				continue;
			}

			const hfSWFEditText * shape = entry->edittext;
			const hfSWFEditText * text = display.textInstance->GetEditText();
			float textLength = display.textInstance->GetTextLength();

			float lengthDiff = fabs( shape->bounds.br.x - shape->bounds.tl.x ) - textLength;
			
			hfVec3 tl; 
			hfVec3 tr; 
			hfVec3 br; 
			hfVec3 bl;

			float xOffset = spriteInstance->xOffset;
			float yOffset = spriteInstance->yOffset;

			float topOffset = 0.0f;

			if ( text->align == SWF_ET_ALIGN_LEFT ) {
				tl.ToVec2() = renderState2.matrix.Transform( hfVec2( shape->bounds.tl.x  + xOffset, shape->bounds.tl.y + topOffset + yOffset ) ); 
				tr.ToVec2() = renderState2.matrix.Transform( hfVec2( shape->bounds.br.x - lengthDiff + xOffset, shape->bounds.tl.y + topOffset + yOffset ) ); 
				br.ToVec2() = renderState2.matrix.Transform( hfVec2( shape->bounds.br.x - lengthDiff + xOffset, shape->bounds.br.y + topOffset + yOffset ) ); 
				bl.ToVec2() = renderState2.matrix.Transform( hfVec2( shape->bounds.tl.x + xOffset, shape->bounds.br.y + topOffset + yOffset ) );									
			} else if ( text->align == SWF_ET_ALIGN_RIGHT ) {
				tl.ToVec2() = renderState2.matrix.Transform( hfVec2( shape->bounds.tl.x + lengthDiff + xOffset, shape->bounds.tl.y + topOffset + yOffset ) );
				tr.ToVec2() = renderState2.matrix.Transform( hfVec2( shape->bounds.br.x + xOffset, shape->bounds.tl.y + topOffset + yOffset ) );
				br.ToVec2() = renderState2.matrix.Transform( hfVec2( shape->bounds.br.x + xOffset, shape->bounds.br.y + topOffset + yOffset ) );
				bl.ToVec2() = renderState2.matrix.Transform( hfVec2( shape->bounds.tl.x + lengthDiff + xOffset, shape->bounds.br.y + topOffset + yOffset ) );				
			} else if ( text->align == SWF_ET_ALIGN_CENTER ) {
				float middle = ( ( shape->bounds.br.x + xOffset ) + ( shape->bounds.tl.x + xOffset ) ) / 2.0f;
				tl.ToVec2() = renderState2.matrix.Transform( hfVec2( middle - ( textLength / 2.0f ), shape->bounds.tl.y + topOffset + yOffset ) );
				tr.ToVec2() = renderState2.matrix.Transform( hfVec2( middle + ( textLength / 2.0f ), shape->bounds.tl.y + topOffset + yOffset ) );
				br.ToVec2() = renderState2.matrix.Transform( hfVec2( middle + ( textLength / 2.0f ), shape->bounds.br.y + topOffset + yOffset ) );
				bl.ToVec2() = renderState2.matrix.Transform( hfVec2( middle - ( textLength / 2.0f ), shape->bounds.br.y + topOffset + yOffset ) );				
			} else {
				tl.ToVec2() = renderState2.matrix.Transform( hfVec2( shape->bounds.tl.x + xOffset, shape->bounds.tl.y + topOffset + yOffset ) );
				tr.ToVec2() = renderState2.matrix.Transform( hfVec2( shape->bounds.br.x + xOffset, shape->bounds.tl.y + topOffset + yOffset ) );
				br.ToVec2() = renderState2.matrix.Transform( hfVec2( shape->bounds.br.x + xOffset, shape->bounds.br.y + topOffset + yOffset ) );
				bl.ToVec2() = renderState2.matrix.Transform( hfVec2( shape->bounds.tl.x + xOffset, shape->bounds.br.y + topOffset + yOffset ) );
			}

			tl.z = 1.0f;
			tr.z = 1.0f;
			br.z = 1.0f;
			bl.z = 1.0f;

			hfMat3 edgeEquations;
			edgeEquations[0] = tl;
			edgeEquations[1] = tr;
			edgeEquations[2] = br;
			edgeEquations.InverseSelf();

			hfVec3 p( x, y, 1.0f );
			hfVec3 signs = p * edgeEquations;

			bool bx = signs.x > 0;
			bool by = signs.y > 0;
			bool bz = signs.z > 0;
			if ( bx == by && bx == bz ) {
				// point inside top right triangle
				returnObject = editObject;
			}

			edgeEquations[0] = tl;
			edgeEquations[1] = br;
			edgeEquations[2] = bl;
			edgeEquations.InverseSelf();
			signs = p * edgeEquations;

			bx = signs.x > 0;
			by = signs.y > 0;
			bz = signs.z > 0;
			if ( bx == by && bx == bz ) {
				// point inside bottom left triangle
				returnObject = editObject;
			}
		}
	}
	return returnObject;
}

/*
===================
hfSWF::HandleEvent
===================
*/
bool hfSWF::HandleEvent( const sysEvent_t * event ) {
	if ( !IsLoaded() || !IsActive() || ( !inhibitControl && useInhibtControl ) ) {
		return false;
	}
	if ( event->evType == SE_KEY ) {
		if ( event->evValue == K_MOUSE1 ) {
			mouseEnabled = true;
			hfSWFScriptVar var;
			if ( event->evValue2 ) {

				hfSWFScriptVar waitInput = globals->Get( "waitInput" );
				if ( waitInput.IsFunction() ) { 
					useMouse = false;
					hfSWFParmList waitParms;
					waitParms.Append( event->evValue );
					waitInput.GetFunction()->Call( NULL, waitParms );
					waitParms.Clear();
				} else {
					useMouse = true;
				}

				hfSWFScriptObject * hitObject = HitTest( mainspriteInstance, swfRenderState_t(), mouseX, mouseY, NULL );
				if ( hitObject != NULL ) {
					mouseObject = hitObject;
					mouseObject->AddRef();
					
					var = hitObject->Get( "onPress" );
					if ( var.IsFunction() ) {
						hfSWFParmList parms;
						parms.Append( event->inputDevice );
						var.GetFunction()->Call( hitObject, parms );
						parms.Clear();
						return true;
					}

					hfSWFScriptVar var = hitObject->Get( "onDrag" );
					if ( var.IsFunction() ) {
						hfSWFParmList parms;
						parms.Append( mouseX );
						parms.Append( mouseY );
						parms.Append( true );
						var.GetFunction()->Call( hitObject, parms );
						parms.Clear();
						return true;
					}
				}

				hfSWFParmList parms;
				parms.Append( hitObject );
				Invoke( "setHitObject", parms );

			} else {
				if ( mouseObject ) {
					var = mouseObject->Get( "onRelease" );
					if ( var.IsFunction() ) {
						hfSWFParmList parms;
						parms.Append( mouseObject ); // FIXME: Remove this
						var.GetFunction()->Call( mouseObject, parms );
					}					
					mouseObject->Release();
					mouseObject = NULL;
				}
				if ( hoverObject ) {
					hoverObject->Release();
					hoverObject = NULL;
				}

				if ( var.IsFunction() ) {
					return true;
				}
			}
			
			return false;
		}
		const char * keyName = hfKeyInput::KeyNumToString( (keyNum_t)event->evValue );
		hfSWFScriptVar var = shortcutKeys->Get( keyName );
		// anything more than 32 levels of indirection we can be pretty sure is an infinite loop
		for ( int runaway = 0; runaway < 32; runaway++ ) {
			hfSWFParmList eventParms;
			eventParms.Clear();
			eventParms.Append( event->inputDevice );
			if ( var.IsString() ) {
				// alias to another key
				var = shortcutKeys->Get( var.ToString() );
				continue;
			} else if ( var.IsObject() ) {
				// if this object is a sprite, send fake mouse events to it
				hfSWFScriptObject * object = var.GetObject();
				// make sure we don't send an onRelease event unless we have already sent that object an onPress
				bool wasPressed = object->Get( "_pressed" ).ToBool();
				object->Set( "_pressed", event->evValue2 );
				if ( event->evValue2 ) {
					var = object->Get( "onPress" );
				} else if ( wasPressed ) {
					var = object->Get( "onRelease" );
				}
				if ( var.IsFunction() ) {
					var.GetFunction()->Call( object, eventParms );
					return true;
				}
			} else if ( var.IsFunction() ) {
				if ( event->evValue2 ) {
					// anonymous functions only respond to key down events
					var.GetFunction()->Call( NULL, eventParms );
					return true;
				}
				return false;
			}

			hfSWFScriptVar useFunction = globals->Get( "useFunction" );
			if ( useFunction.IsFunction() && event->evValue2 ) {
				const char * action = hfKeyInput::GetBinding( event->evValue );
				if ( hfStr::Cmp( "_use", action ) == 0 ) {
					useFunction.GetFunction()->Call( NULL, hfSWFParmList() );
				}				
			}

			hfSWFScriptVar waitInput = globals->Get( "waitInput" );
			if ( waitInput.IsFunction() ) { 
				useMouse = false;
				if ( event->evValue2 ) {
					hfSWFParmList waitParms;
					waitParms.Append( event->evValue );
					waitInput.GetFunction()->Call( NULL, waitParms );
				}
			} else {
				useMouse = true;
			}

			hfSWFScriptVar focusWindow = globals->Get( "focusWindow" );
			if ( focusWindow.IsObject() ) {
				hfSWFScriptVar onKey = focusWindow.GetObject()->Get( "onKey" );
				if ( onKey.IsFunction() ) {

					// make sure we don't send an onRelease event unless we have already sent that object an onPress
					hfSWFScriptObject * object = focusWindow.GetObject();
					bool wasPressed = object->Get( "_kpressed" ).ToBool();
					object->Set( "_kpressed", event->evValue2 );
					if ( event->evValue2 || wasPressed ) {
						hfSWFParmList parms;
						parms.Append( event->evValue );
						parms.Append( event->evValue2 );
						onKey.GetFunction()->Call( focusWindow.GetObject(), parms ).ToBool();
						return true;
					} else if ( event->evValue == K_LSHIFT || event->evValue == K_RSHIFT ) {
						hfSWFParmList parms;
						parms.Append( event->evValue );
						parms.Append( event->evValue2 );
						onKey.GetFunction()->Call( focusWindow.GetObject(), parms ).ToBool();
					}
				}
			}
			return false;
		}
		hfLib::Warning( "Circular reference in %s shortcutKeys.%s", filename.c_str(), keyName );
	} else if ( event->evType == SE_CHAR ) {
		hfSWFScriptVar focusWindow = globals->Get( "focusWindow" );
		if ( focusWindow.IsObject() ) {
			hfSWFScriptVar onChar = focusWindow.GetObject()->Get( "onChar" );
			if ( onChar.IsFunction() ) {
				hfSWFParmList parms;
				parms.Append( event->evValue );
				parms.Append( hfKeyInput::KeyNumToString( (keyNum_t)event->evValue ) );
				onChar.GetFunction()->Call( focusWindow.GetObject(), parms ).ToBool();
				return true;
			}
		}
	} else if ( event->evType == SE_MOUSE_ABSOLUTE || event->evType == SE_MOUSE ) {
		mouseEnabled = true;
		isMouseInClientArea = true;

		// Mouse position in screen space needs to be converted to SWF space
		if ( event->evType == SE_MOUSE_ABSOLUTE ) {
			const float pixelAspect = renderSystem->GetPixelAspect();
			const float sysWidth = renderSystem->GetWidth() * ( pixelAspect > 1.0f ? pixelAspect : 1.0f );
			const float sysHeight = renderSystem->GetHeight() / ( pixelAspect < 1.0f ? pixelAspect : 1.0f );
			float scale = swfScale * sysHeight / (float)frameHeight;
			float invScale = 1.0f / scale;
			float tx = 0.5f * ( sysWidth - ( frameWidth * scale ) );
			float ty = 0.5f * ( sysHeight - ( frameHeight * scale ) );

			mouseX = hfMath::Ftoi( ( static_cast<float>( event->evValue ) - tx ) * invScale );
			mouseY = hfMath::Ftoi( ( static_cast<float>( event->evValue2 ) - ty ) * invScale );
		} else {

			mouseX += event->evValue;
			mouseY += event->evValue2;

			mouseX = Max( Min( mouseX, hfMath::Ftoi( frameWidth + renderBorder ) ), hfMath::Ftoi( 0.0f - renderBorder ) );
			mouseY = Max( Min( mouseY, hfMath::Ftoi(frameHeight) ), 0 );
		}

		bool retVal = false;

		hfSWFScriptObject * hitObject = HitTest( mainspriteInstance, swfRenderState_t(), mouseX, mouseY, NULL );
		if ( hitObject != NULL ) {
			hasHitObject = true;
		} else {
			hasHitObject = false;
		}

		if ( hitObject != hoverObject ) {
			// First check to see if we should call onRollOut on our previous hoverObject
			if ( hoverObject != NULL ) {
				hfSWFScriptVar var = hoverObject->Get( "onRollOut" );
				if ( var.IsFunction() ) {
					var.GetFunction()->Call( hoverObject, hfSWFParmList() );
					retVal = true;
				}
				hoverObject->Release();
				hoverObject = NULL;
			}
			// Then call onRollOver on our hitObject
			if ( hitObject != NULL ) {
				hoverObject = hitObject;
				hoverObject->AddRef();
				hfSWFScriptVar var = hitObject->Get( "onRollOver" );
				if ( var.IsFunction() ) {
					var.GetFunction()->Call( hitObject, hfSWFParmList() );
					retVal = true;
				}
			}
		}
		if ( mouseObject != NULL ) {
			hfSWFScriptVar var = mouseObject->Get( "onDrag" );
			if ( var.IsFunction() ) {
				hfSWFParmList parms;
				parms.Append( mouseX );
				parms.Append( mouseY );
				parms.Append( false );
				var.GetFunction()->Call( mouseObject, parms );
				return true;
			}
		}
		return retVal;
	} else if ( event->evType == SE_MOUSE_LEAVE ) {
		isMouseInClientArea = false;
	} else if ( event->evType == SE_JOYSTICK ) {
		hfSWFParmList parms;
		parms.Append( event->evValue );
		parms.Append( event->evValue2 / 32.0f );
		Invoke( "onJoystick", parms );
	}
	return false;
}

