#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#pragma warning( disable: 4189 ) // local variable is initialized but not referenced

/*
========================
hfSWF::DefineFont2
========================
*/
void hfSWF::DefineFont2( hfSWFBitStream & bitstream ) {
	uint16 characterID = bitstream.ReadU16();
	hfSWFDictionaryEntry * entry = AddDictionaryEntry( characterID, SWF_DICT_FONT );
	if ( entry == NULL ) {
		return;
	}
	uint8 flags = bitstream.ReadU8();
	uint8 language = bitstream.ReadU8();

	char fontName[257];
	uint8 fontNameLength = bitstream.ReadU8();
	memcpy( fontName, bitstream.ReadData( fontNameLength ), fontNameLength );
	fontName[ fontNameLength ] = 0;

	entry->font->fontID = renderSystem->RegisterFont( fontName );

	uint16 numGlyphs = bitstream.ReadU16();
	entry->font->glyphs.SetNum( numGlyphs );

	if ( flags & BIT( 3 ) ) {
		// 32 bit offsets
		uint32 offsetTableSize = ( numGlyphs + 1 ) * 4;
		hfSWFBitStream offsetStream( bitstream.ReadData( offsetTableSize ), offsetTableSize, false );
		if ( offsetStream.ReadU32() != offsetTableSize ) {
			hfLib::Warning( "hfSWF::DefineFont2: first glyph offset != offsetTableSize" );
			return;
		}
		uint32 previousOffset = offsetTableSize;
		for ( int i = 0; i < numGlyphs; i++ ) {
			uint32 nextOffset = offsetStream.ReadU32();
			uint32 shapeSize = nextOffset - previousOffset;
			previousOffset = nextOffset;
			hfSWFBitStream shapeStream( bitstream.ReadData( shapeSize ), shapeSize, false );
			hfSWFShapeParser swfShapeParser;
			swfShapeParser.ParseFont( shapeStream, entry->font->glyphs[i] );
		}
	} else {
		// 16 bit offsets
		uint16 offsetTableSize = ( numGlyphs + 1 ) * 2;
		hfSWFBitStream offsetStream( bitstream.ReadData( offsetTableSize ), offsetTableSize, false );
		if ( offsetStream.ReadU16() != offsetTableSize ) {
			hfLib::Warning( "hfSWF::DefineFont2: first glyph offset != offsetTableSize" );
			return;
		}
		uint16 previousOffset = offsetTableSize;
		for ( int i = 0; i < numGlyphs; i++ ) {
			uint16 nextOffset = offsetStream.ReadU16();
			uint16 shapeSize = nextOffset - previousOffset;
			previousOffset = nextOffset;
			hfSWFBitStream shapeStream( bitstream.ReadData( shapeSize ), shapeSize, false );
			hfSWFShapeParser swfShapeParser;
			swfShapeParser.ParseFont( shapeStream, entry->font->glyphs[i] );
		}
	}
	if ( flags & BIT( 2 ) ) {
		// 16 bit codes
		for ( int i = 0; i < numGlyphs; i++ ) {
			entry->font->glyphs[i].code = bitstream.ReadU16();
		}
	} else {
		// 8 bit codes
		for ( int i = 0; i < numGlyphs; i++ ) {
			entry->font->glyphs[i].code = bitstream.ReadU8();
		}
	}
	if ( flags & BIT( 7 ) ) {
		entry->font->ascent = bitstream.ReadS16();
		entry->font->descent = bitstream.ReadS16();
		entry->font->leading = bitstream.ReadS16();
		for ( int i = 0; i < numGlyphs; i++ ) {
			entry->font->glyphs[i].advance = bitstream.ReadS16();
		}
		for ( int i = 0; i < numGlyphs; i++ ) {
			swfRect_t ignored;
			bitstream.ReadRect( ignored );
		}
		uint16 kearningCount = bitstream.ReadU16();
		if ( flags & BIT( 2 ) ) {
			for ( int i = 0; i < kearningCount; i++ ) {
				uint16 code1 = bitstream.ReadU16();
				uint16 code2 = bitstream.ReadU16();
				int16 adjustment = bitstream.ReadS16();
			}
		} else {
			for ( int i = 0; i < kearningCount; i++ ) {
				uint16 code1 = bitstream.ReadU8();
				uint16 code2 = bitstream.ReadU8();
				int16 adjustment = bitstream.ReadS16();
			}
		}
	}
}

/*
========================
hfSWF::DefineFont3
========================
*/
void hfSWF::DefineFont3( hfSWFBitStream & bitstream ) {
	DefineFont2( bitstream );
}

/*
========================
hfSWF::DefineTextX
========================
*/
void hfSWF::DefineTextX( hfSWFBitStream & bitstream, bool rgba ) {
	uint16 characterID = bitstream.ReadU16();
	hfSWFDictionaryEntry * entry = AddDictionaryEntry( characterID, SWF_DICT_TEXT );
	if ( entry == NULL ) {
		return;
	}
	hfSWFText * text = entry->text;

	bitstream.ReadRect( text->bounds );
	bitstream.ReadMatrix( text->matrix );

	uint8 glyphBits = bitstream.ReadU8();
	uint8 advanceBits = bitstream.ReadU8();

	while ( true ) {
		uint8 flags = bitstream.ReadU8();
		if ( flags == 0 ) {
			break;
		}
		hfSWFTextRecord & textRecord = text->textRecords.Alloc();

		if ( flags & BIT( 3 ) ) {
			textRecord.fontID = bitstream.ReadU16();
		}
		if ( flags & BIT( 2 ) ) {
			if ( rgba ) {
				bitstream.ReadColorRGBA( textRecord.color );
			} else {
				bitstream.ReadColorRGB( textRecord.color );
			}
		}
		if ( flags & BIT( 0 ) ) {
			textRecord.xOffset = bitstream.ReadS16();
		}
		if ( flags & BIT( 1 ) ) {
			textRecord.yOffset = bitstream.ReadS16();
		}
		if ( flags & BIT( 3 ) ) {
			textRecord.textHeight = bitstream.ReadU16();
		}
		textRecord.firstGlyph = text->glyphs.Num();
		textRecord.numGlyphs = bitstream.ReadU8();
		for ( int i = 0; i < textRecord.numGlyphs; i++ ) {
			swfGlyphEntry_t & glyph = text->glyphs.Alloc();
			glyph.index = bitstream.ReadU( glyphBits );
			glyph.advance = bitstream.ReadS( advanceBits );
		}
	};
}

/*
========================
hfSWF::DefineText
========================
*/
void hfSWF::DefineText( hfSWFBitStream & bitstream ) {
	DefineTextX( bitstream, false );
}

/*
========================
hfSWF::DefineText2
========================
*/
void hfSWF::DefineText2( hfSWFBitStream & bitstream ) {
	DefineTextX( bitstream, true );
}

/*
========================
hfSWF::DefineEditText
========================
*/
void hfSWF::DefineEditText( hfSWFBitStream & bitstream ) {
	uint16 characterID = bitstream.ReadU16();
	hfSWFDictionaryEntry * entry = AddDictionaryEntry( characterID, SWF_DICT_EDITTEXT );
	if ( entry == NULL ) {
		return;
	}
	hfSWFEditText * edittext = entry->edittext;
	bitstream.ReadRect( edittext->bounds );
	bitstream.ResetBits();
	bool hasText = bitstream.ReadBool();
	bool wordWrap = bitstream.ReadBool();
	bool multiline = bitstream.ReadBool();
	bool password = bitstream.ReadBool();
	bool readonly = bitstream.ReadBool();
	bool hasTextColor = bitstream.ReadBool();
	bool hasMaxLength = bitstream.ReadBool();
	bool hasFont = bitstream.ReadBool();
	bool hasFontClass = bitstream.ReadBool();
	bool autoSize = bitstream.ReadBool();
	bool hasLayout = bitstream.ReadBool();
	bool noSelect = bitstream.ReadBool();
	bool border = bitstream.ReadBool();
	bool wasStatic = bitstream.ReadBool();
	bool html = bitstream.ReadBool();
	bool useOutlines = bitstream.ReadBool();
	if ( hasFont ) {
		edittext->fontID = bitstream.ReadU16();
		edittext->fontHeight = bitstream.ReadU16();
	}
	if ( hasFontClass ) {
		hfStr fontClass = bitstream.ReadString();
	}
	if ( hasTextColor ) {
		bitstream.ReadColorRGBA( edittext->color );
	}
	if ( hasMaxLength ) {
		edittext->maxLength = bitstream.ReadU16();
	}
	if ( hasLayout ) {
		edittext->align = (swfEditTextAlign_t)bitstream.ReadU8();
		edittext->leftMargin = bitstream.ReadU16();
		edittext->rightMargin = bitstream.ReadU16();
		edittext->indent = bitstream.ReadU16();
		edittext->leading = bitstream.ReadS16();
	}
	edittext->variable = bitstream.ReadString();
	if ( hasText ) {
		const char * text = bitstream.ReadString();
		hfStr initialText;

		// convert html tags if necessary
		for ( int i = 0; text[i] != 0; i++ ) {
			if ( text[i] == '<' ) {
				if ( i != 0 && text[i+1] == 'p' ) {
					initialText.Append( '\n' );
				}
				for ( ; text[i] != 0 && text[i] != '>'; i++ ) {
				}
				continue;
			}
			byte tc = (byte)text[i];
			if ( tc == '&' ) {
				hfStr special;
				for ( i++; text[i] != 0 && text[i] != ';'; i++ ) {
					special.Append( text[i] );
				}
				if ( special.Icmp( "amp" ) == 0 ) {
					tc = '&';
				} else if ( special.Icmp( "apos" ) == 0 ) {
					tc = '\'';
				} else if ( special.Icmp( "lt" ) == 0 ) {
					tc = '<';
				} else if ( special.Icmp( "gt" ) == 0 ) {
					tc = '>';
				} else if ( special.Icmp( "quot" ) == 0 ) {
					tc = '\"';
				}
			}
			initialText.Append( tc );
		}
		edittext->initialText = initialText;
	}
	edittext->flags |= wordWrap ? SWF_ET_WORDWRAP : 0;
	edittext->flags |= multiline ? SWF_ET_MULTILINE : 0;
	edittext->flags |= password ? SWF_ET_PASSWORD : 0;
	edittext->flags |= readonly ? SWF_ET_READONLY : 0;
	edittext->flags |= autoSize ? SWF_ET_AUTOSIZE : 0;
	edittext->flags |= border ? SWF_ET_BORDER : 0;
}

