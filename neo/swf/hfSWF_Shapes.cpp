#pragma hdrstop
#include "../hflib/hfprecompiled.h"

/*
========================
hfSWF::DefineShape
========================
*/
void hfSWF::DefineShape( hfSWFBitStream & bitstream ) {
	uint16 characterID = bitstream.ReadU16();
	hfSWFDictionaryEntry * entry = AddDictionaryEntry( characterID, SWF_DICT_SHAPE );
	if ( entry == NULL ) {
		return;
	}

	hfSWFShapeParser swfShapeParser;
	swfShapeParser.Parse( bitstream, *entry->shape, 1 );
}

/*
========================
hfSWF::DefineShape2
========================
*/
void hfSWF::DefineShape2( hfSWFBitStream & bitstream ) {
	uint16 characterID = bitstream.ReadU16();
	hfSWFDictionaryEntry * entry = AddDictionaryEntry( characterID, SWF_DICT_SHAPE );
	if ( entry == NULL ) {
		return;
	}

	hfSWFShapeParser swfShapeParser;
	swfShapeParser.Parse( bitstream, *entry->shape, 2 );
}

/*
========================
hfSWF::DefineShape3
========================
*/
void hfSWF::DefineShape3( hfSWFBitStream & bitstream ) {
	uint16 characterID = bitstream.ReadU16();
	hfSWFDictionaryEntry * entry = AddDictionaryEntry( characterID, SWF_DICT_SHAPE );
	if ( entry == NULL ) {
		return;
	}

	hfSWFShapeParser swfShapeParser;
	swfShapeParser.Parse( bitstream, *entry->shape, 3 );
}

/*
========================
hfSWF::DefineShape4
========================
*/
void hfSWF::DefineShape4( hfSWFBitStream & bitstream ) {
	uint16 characterID = bitstream.ReadU16();
	hfSWFDictionaryEntry * entry = AddDictionaryEntry( characterID, SWF_DICT_SHAPE );
	if ( entry == NULL ) {
		return;
	}

	hfSWFShapeParser swfShapeParser;
	swfShapeParser.Parse( bitstream, *entry->shape, 4 );
}


/*
========================
hfSWF::DefineMorphShape
========================
*/
void hfSWF::DefineMorphShape( hfSWFBitStream & bitstream ) {
	uint16 characterID = bitstream.ReadU16();
	hfSWFDictionaryEntry * entry = AddDictionaryEntry( characterID, SWF_DICT_MORPH );
	if ( entry == NULL ) {
		return;
	}

	hfSWFShapeParser swfShapeParser;
	swfShapeParser.ParseMorph( bitstream, *entry->shape );
}

