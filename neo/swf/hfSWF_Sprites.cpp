#pragma hdrstop
#include "../hflib/hfprecompiled.h"

/*
========================
hfSWFSprite::hfSWFSprite
========================
*/
hfSWFSprite::hfSWFSprite( hfSWF * _swf ) :
frameCount( 0 ),
swf( _swf ),
commandBuffer( NULL )
{
}

/*
========================
hfSWFSprite::~hfSWFSprite
========================
*/
hfSWFSprite::~hfSWFSprite() {
	Mem_Free( commandBuffer );
}

/*
========================
hfSWF::DefineSprite
========================
*/
void hfSWF::DefineSprite( hfSWFBitStream & bitstream ) {
	uint16 characterID = bitstream.ReadU16();
	hfSWFDictionaryEntry * entry = AddDictionaryEntry( characterID, SWF_DICT_SPRITE );
	if ( entry == NULL ) {
		return;
	}
	entry->sprite->Load( bitstream, false );
}

/*
========================
hfSWFSprite::Load
========================
*/
void hfSWFSprite::Load( hfSWFBitStream & bitstream, bool parseDictionary ) {

	frameCount = bitstream.ReadU16();

	// run through the file once, building the dictionary and accumulating control tags
	frameOffsets.SetNum( frameCount + 1 );
	frameOffsets[0] = 0;

	unsigned int currentFrame = 1;

	while ( true ) {
		uint16 codeAndLength = bitstream.ReadU16();
		uint32 recordLength = ( codeAndLength & 0x3F );
		if ( recordLength == 0x3F ) {
			recordLength = bitstream.ReadU32();
		}

		hfSWFBitStream tagStream( bitstream.ReadData( recordLength ), recordLength, false );

		swfTag_t tag = (swfTag_t)( codeAndLength >> 6 );

		// ----------------------
		// Definition tags
		// definition tags are only allowed in the main sprite
		// ----------------------
		if ( parseDictionary ) {
			bool handled = true;
			switch ( tag ) {
#define HANDLE_SWF_TAG( x ) case Tag_##x: swf->x( tagStream ); break;
			HANDLE_SWF_TAG( JPEGTables );
			HANDLE_SWF_TAG( DefineBits );
			HANDLE_SWF_TAG( DefineBitsJPEG2 );
			HANDLE_SWF_TAG( DefineBitsJPEG3 );
			HANDLE_SWF_TAG( DefineBitsLossless );
			HANDLE_SWF_TAG( DefineBitsLossless2 );
			HANDLE_SWF_TAG( DefineShape );
			HANDLE_SWF_TAG( DefineShape2 );
			HANDLE_SWF_TAG( DefineShape3 );
			HANDLE_SWF_TAG( DefineShape4 );
			HANDLE_SWF_TAG( DefineSprite );
			HANDLE_SWF_TAG( DefineSound );
			//HANDLE_SWF_TAG( DefineMorphShape ); // these don't work right
			HANDLE_SWF_TAG( DefineFont2 );
			HANDLE_SWF_TAG( DefineFont3 );
			HANDLE_SWF_TAG( DefineText );
			HANDLE_SWF_TAG( DefineText2 );
			HANDLE_SWF_TAG( DefineEditText );
#undef HANDLE_SWF_TAG
			default: handled = false;
			}
			if ( handled ) {
				continue;
			}
		}
		// ----------------------
		// Control tags
		// control tags are stored off in the commands list and processed at run time
		// except for a couple really special control tags like "End" and "FrameLabel"
		// ----------------------
		switch ( tag ) {
		case Tag_End:
			return;

		case Tag_ShowFrame:
			frameOffsets[ currentFrame ] = commands.Num();
			currentFrame++;
			break;

		case Tag_FrameLabel: {
				swfFrameLabel_t & label = frameLabels.Alloc();
				label.frameNum = currentFrame;
				label.frameLabel = tagStream.ReadString();
			}
			break;

		case Tag_DoInitAction: {
			tagStream.ReadU16();

			hfSWFBitStream &initaction = doInitActions.Alloc();
			initaction.Load( tagStream.ReadData( recordLength - 2 ), recordLength - 2, true );
 		    }
			break;

		case Tag_DoAction:
		case Tag_PlaceObject2:
		case Tag_PlaceObject3:
		case Tag_RemoveObject2: {
				swfSpriteCommand_t & command = commands.Alloc();
				command.tag = tag;
				command.stream.Load( tagStream.ReadData( recordLength ), recordLength, true );
			}
			break;

		default:
			// We don't care, about sprite tags we don't support ... RobA
			//hfLib::Printf( "Load Sprite: Unhandled tag %s\n", hfSWF::GetTagName( tag ) );
			break;
		}
	}
}

/*
========================
hfSWFSprite::Read
========================
*/
void hfSWFSprite::Read( hfFile * f ) {
	int num = 0;
	f->ReadBig( frameCount );
	f->ReadBig( num ); frameOffsets.SetNum( num );
	f->ReadBigArray( frameOffsets.Ptr(), frameOffsets.Num() );
	f->ReadBig( num ); frameLabels.SetNum( num );
	for ( int i = 0; i < frameLabels.Num(); i++ ) {
		f->ReadBig( frameLabels[i].frameNum );
		f->ReadString( frameLabels[i].frameLabel );
	}

	uint32 bufferSize;
	f->ReadBig( bufferSize );

	commandBuffer = (byte *)Mem_Alloc( bufferSize, TAG_SWF );
	f->Read( commandBuffer, bufferSize );

	byte * currentBuffer = commandBuffer;

	f->ReadBig( num ); commands.SetNum( num );
	for ( int i = 0; i < commands.Num(); i++ ) {
		uint32 streamLength = 0;

		f->ReadBig( commands[i].tag );
		f->ReadBig( streamLength );

		commands[i].stream.Load( currentBuffer, streamLength, false );
		currentBuffer += streamLength;
	}

	uint32 doInitActionLength = 0;
	f->ReadBig( num );
	doInitActions.SetNum( num );
	for ( int i = 0; i < num; i++ ) {
		f->ReadBig( doInitActionLength );
		hfSWFBitStream &initaction = doInitActions[i];
		initaction.Load( currentBuffer, doInitActionLength, true );
		currentBuffer += doInitActionLength;
	}
}

/*
========================
hfSWFSprite::Write
========================
*/
void hfSWFSprite::Write( hfFile * f ) {
	f->WriteBig( frameCount );
	f->WriteBig( frameOffsets.Num() );
	f->WriteBigArray( frameOffsets.Ptr(), frameOffsets.Num() );
	f->WriteBig( frameLabels.Num() );
	for ( int i = 0; i < frameLabels.Num(); i++ ) {
		f->WriteBig( frameLabels[i].frameNum );
		f->WriteString( frameLabels[i].frameLabel );
	}
	uint32 totalLength = 0;
	for ( int i = 0; i < commands.Num(); i++ ) {
		totalLength += commands[i].stream.Length();
	}
	for (int i = 0; i < doInitActions.Num(); i++ ) {
		totalLength += doInitActions[i].Length();
	}
	f->WriteBig( totalLength );
	for ( int i = 0; i < commands.Num(); i++ ) {
		f->Write( commands[i].stream.Ptr(), commands[i].stream.Length() );
	}
	for ( int i = 0; i < doInitActions.Num(); i++ ){
		f->Write( doInitActions[i].Ptr(), doInitActions[i].Length() );
	}

	f->WriteBig( commands.Num() ); 
	for ( int i = 0; i < commands.Num(); i++ ) {
		f->WriteBig( commands[i].tag );
		f->WriteBig( commands[i].stream.Length() );
	}

	f->WriteBig( doInitActions.Num() );
	for ( int i = 0; i < doInitActions.Num(); i++ ){
		f->WriteBig( doInitActions[i].Length() ); 
	}
}

