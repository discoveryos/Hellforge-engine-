#pragma hdrstop
#include "../hflib/hfprecompiled.h"

int c_PlaceObject2;
int c_PlaceObject3;

#define PlaceFlagHasClipActions		BIT( 7 )
#define PlaceFlagHasClipDepth		BIT( 6 )
#define PlaceFlagHasName			BIT( 5 )
#define PlaceFlagHasRatio			BIT( 4 )
#define PlaceFlagHasColorTransform	BIT( 3 )
#define PlaceFlagHasMatrix			BIT( 2 )
#define PlaceFlagHasCharacter		BIT( 1 )
#define PlaceFlagMove				BIT( 0 )

#define PlaceFlagPad0				BIT( 7 )
#define PlaceFlagPad1				BIT( 6 )
#define PlaceFlagPad2				BIT( 5 )
#define PlaceFlagHasImage			BIT( 4 )
#define PlaceFlagHasClassName		BIT( 3 )
#define PlaceFlagCacheAsBitmap		BIT( 2 )
#define PlaceFlagHasBlendMode		BIT( 1 )
#define PlaceFlagHasFilterList		BIT( 0 )

/*
========================
hfSWFSpriteInstance::PlaceObject2
========================
*/
void hfSWFSpriteInstance::PlaceObject2( hfSWFBitStream & bitstream ) {
	c_PlaceObject2++;

	uint64 flags = bitstream.ReadU8();
	int depth = bitstream.ReadU16();

	int characterID = -1;
	if ( ( flags & PlaceFlagHasCharacter ) != 0 ) {
		characterID = bitstream.ReadU16();
	}

	swfDisplayEntry_t * display = NULL;

	if ( ( flags & PlaceFlagMove ) != 0 ) {
		// modify an existing entry
		display = FindDisplayEntry( depth );
		if ( display == NULL ) {
			hfLib::Warning( "PlaceObject2: trying to modify entry %d, which doesn't exist", depth );
			return;
		}
		if ( characterID >= 0 ) {
			// We are very picky about what kind of objects can change characters
			// Shapes can become other shapes, but sprites can never change
			if ( display->spriteInstance || display->textInstance ) {
				hfLib::Warning( "PlaceObject2: Trying to change the character of a sprite after it's been created" );
				return;
			}
			hfSWFDictionaryEntry * dictEntry = sprite->swf->FindDictionaryEntry( characterID );
			if ( dictEntry != NULL ) {
				if ( dictEntry->type == SWF_DICT_SPRITE || dictEntry->type == SWF_DICT_EDITTEXT ) {
					hfLib::Warning( "PlaceObject2: Trying to change the character of a shape to a sprite" );
					return;
				}
			}
			display->characterID = characterID;
		}
	} else {
		if ( characterID < 0 ) {
			hfLib::Warning( "PlaceObject2: Trying to create a new object without a character" );
			return;
		}
		// create a new entry
		display = AddDisplayEntry( depth, characterID );
		if ( display == NULL ) {
			hfLib::Warning( "PlaceObject2: trying to create a new entry at %d, but an item already exists there", depth );
			return;
		}
	}
	if ( ( flags & PlaceFlagHasMatrix ) != 0 ) {
		bitstream.ReadMatrix( display->matrix );
	}
	if ( ( flags & PlaceFlagHasColorTransform ) != 0 ) {
		bitstream.ReadColorXFormRGBA( display->cxf );
	}
	if ( ( flags & PlaceFlagHasRatio ) != 0 ) {
		display->ratio = bitstream.ReadU16() * ( 1.0f / 65535.0f );
	}
	if ( ( flags & PlaceFlagHasName ) != 0 ) {
		hfStr name = bitstream.ReadString();
		if ( display->spriteInstance ) {
			display->spriteInstance->name = name;
			scriptObject->Set( name, display->spriteInstance->GetScriptObject() );
		} else if ( display->textInstance ) {
			scriptObject->Set( name, display->textInstance->GetScriptObject() );
		}
	}
	if ( ( flags & PlaceFlagHasClipDepth ) != 0 ) {
		display->clipDepth = bitstream.ReadU16();
	}
	if ( ( flags & PlaceFlagHasClipActions ) != 0 ) {
		// FIXME: clip actions
	}
}

/*
========================
hfSWFSpriteInstance::PlaceObject3
========================
*/
void hfSWFSpriteInstance::PlaceObject3( hfSWFBitStream & bitstream ) {
	c_PlaceObject3++;

	uint64 flags1 = bitstream.ReadU8();
	uint64 flags2 = bitstream.ReadU8();
	uint16 depth = bitstream.ReadU16();

	if ( ( flags2 & PlaceFlagHasClassName ) != 0 || ( ( ( flags2 & PlaceFlagHasImage ) != 0 ) && ( ( flags1 & PlaceFlagHasCharacter ) != 0 ) ) ) {
		bitstream.ReadString(); // ignored
	}

	int characterID = -1;
	if ( ( flags1 & PlaceFlagHasCharacter ) != 0 ) {
		characterID = bitstream.ReadU16();
	}

	swfDisplayEntry_t * display = NULL;

	if ( ( flags1 & PlaceFlagMove ) != 0 ) {
		// modify an existing entry
		display = FindDisplayEntry( depth );
		if ( display == NULL ) {
			hfLib::Warning( "PlaceObject3: trying to modify entry %d, which doesn't exist", depth );
			return;
		}
		if ( characterID >= 0 ) {
			// We are very picky about what kind of objects can change characters
			// Shapes can become other shapes, but sprites can never change
			if ( display->spriteInstance || display->textInstance ) {
				hfLib::Warning( "PlaceObject3: Trying to change the character of a sprite after it's been created" );
				return;
			}
			hfSWFDictionaryEntry * dictEntry = sprite->swf->FindDictionaryEntry( characterID );
			if ( dictEntry != NULL ) {
				if ( dictEntry->type == SWF_DICT_SPRITE || dictEntry->type == SWF_DICT_EDITTEXT ) {
					hfLib::Warning( "PlaceObject3: Trying to change the character of a shape to a sprite" );
					return;
				}
			}
			display->characterID = characterID;
		}
	} else {
		if ( characterID < 0 ) {
			hfLib::Warning( "PlaceObject3: Trying to create a new object without a character" );
			return;
		}
		// create a new entry
		display = AddDisplayEntry( depth, characterID );
		if ( display == NULL ) {
			hfLib::Warning( "PlaceObject3: trying to create a new entry at %d, but an item already exists there", depth );
			return;
		}
	}
	if ( ( flags1 & PlaceFlagHasMatrix ) != 0 ) {
		bitstream.ReadMatrix( display->matrix );
	}
	if ( ( flags1 & PlaceFlagHasColorTransform ) != 0 ) {
		bitstream.ReadColorXFormRGBA( display->cxf );
	}
	if ( ( flags1 & PlaceFlagHasRatio ) != 0 ) {
		display->ratio = bitstream.ReadU16() * ( 1.0f / 65535.0f );
	}
	if ( ( flags1 & PlaceFlagHasName ) != 0 ) {
		hfStr name = bitstream.ReadString();
		if ( display->spriteInstance ) {
			display->spriteInstance->name = name;
			scriptObject->Set( name, display->spriteInstance->GetScriptObject() );
		} else if ( display->textInstance ) {
			scriptObject->Set( name, display->textInstance->GetScriptObject() );
		}
	}
	if ( ( flags1 & PlaceFlagHasClipDepth ) != 0 ) {
		display->clipDepth = bitstream.ReadU16();
	}
	if ( ( flags2 & PlaceFlagHasFilterList ) != 0 ) {
		// we don't support filters and because the filter list is variable length we
		// can't support anything after the filter list either (blend modes and clip actions)
		hfLib::Warning( "PlaceObject3: has filters" );
		return;
	}
	if ( ( flags2 & PlaceFlagHasBlendMode ) != 0 ) {
		display->blendMode = bitstream.ReadU8();
	}
	if ( ( flags1 & PlaceFlagHasClipActions ) != 0 ) {
		// FIXME:
	}
}

/*
========================
hfSWFSpriteInstance::RemoveObject2
========================
*/
void hfSWFSpriteInstance::RemoveObject2( hfSWFBitStream & bitstream ) {
	RemoveDisplayEntry( bitstream.ReadU16() );
}

