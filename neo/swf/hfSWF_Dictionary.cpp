#pragma hdrstop
#include "../hflib/hfprecompiled.h"

/*
========================
hfSWF::hfSWFDictionaryEntry::hfSWFDictionaryEntry
========================
*/
hfSWFDictionaryEntry::hfSWFDictionaryEntry() :
	type( SWF_DICT_NULL ),
	material( NULL ),
	shape( NULL ),
	sprite( NULL ),
	font( NULL ),
	text( NULL ),
	edittext( NULL ),
	imageSize( 0, 0 ),
	imageAtlasOffset( 0, 0 ),
	channelScale( 1.0f, 1.0f, 1.0f, 1.0f ) {
}

/*
========================
hfSWF::hfSWFDictionaryEntry::hfSWFDictionaryEntry
========================
*/
hfSWFDictionaryEntry::~hfSWFDictionaryEntry() {
	delete shape;
	delete sprite;
	delete font;
	delete text;
	delete edittext;
}

/*
========================
hfSWF::hfSWFDictionaryEntry::operator=
This exists mostly so hfList works right
========================
*/
hfSWFDictionaryEntry & hfSWFDictionaryEntry::operator=( hfSWFDictionaryEntry & other ) {
	type = other.type;
	material = other.material;
	shape = other.shape;
	sprite = other.sprite;
	font = other.font;
	text = other.text;
	edittext = other.edittext;
	imageSize = other.imageSize;
	imageAtlasOffset = other.imageAtlasOffset;
	other.type = SWF_DICT_NULL;
	other.material = NULL;
	other.shape = NULL;
	other.sprite = NULL;
	other.font = NULL;
	other.text = NULL;
	other.edittext = NULL;
	return *this;
}

/*
========================
hfSWF::AddDictionaryEntry
========================
*/
hfSWFDictionaryEntry * hfSWF::AddDictionaryEntry( int characterID, swfDictType_t type ) {

	if ( dictionary.Num() < characterID + 1 ) {
		dictionary.SetNum( characterID + 1 );
	}

	if ( dictionary[ characterID ].type != SWF_DICT_NULL ) {
		hfLib::Warning( "%s: Duplicate character %d", filename.c_str(), characterID );
		return NULL;
	}

	dictionary[ characterID ].type = type;

	if ( ( type == SWF_DICT_SHAPE ) || ( type == SWF_DICT_MORPH ) ) {
		dictionary[ characterID ].shape = new (TAG_SWF) hfSWFShape;
	} else if ( type == SWF_DICT_SPRITE ) {
		dictionary[ characterID ].sprite = new (TAG_SWF) hfSWFSprite( this );
	} else if ( type == SWF_DICT_FONT ) {
		dictionary[ characterID ].font = new (TAG_SWF) hfSWFFont;
	} else if ( type == SWF_DICT_TEXT ) {
		dictionary[ characterID ].text = new (TAG_SWF) hfSWFText;
	} else if ( type == SWF_DICT_EDITTEXT ) {
		dictionary[ characterID ].edittext = new (TAG_SWF) hfSWFEditText;
	}

	return &dictionary[ characterID ];
}

/*
========================
FindDictionaryEntry
========================
*/
hfSWFDictionaryEntry * hfSWF::FindDictionaryEntry( int characterID, swfDictType_t type ) {

	if ( dictionary.Num() < characterID + 1 ) {
		hfLib::Warning( "%s: Could not find character %d", filename.c_str(), characterID );
		return NULL;
	}

	if ( dictionary[ characterID ].type != type ) {
		hfLib::Warning( "%s: Character %d is the wrong type", filename.c_str(), characterID );
		return NULL;
	}

	return &dictionary[ characterID ];
}


/*
========================
FindDictionaryEntry
========================
*/
hfSWFDictionaryEntry * hfSWF::FindDictionaryEntry( int characterID ) {

	if ( dictionary.Num() < characterID + 1 ) {
		hfLib::Warning( "%s: Could not find character %d", filename.c_str(), characterID );
		return NULL;
	}

	return &dictionary[ characterID ];
}

