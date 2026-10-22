#pragma hdrstop
#include "../hflib/hfprecompiled.h"

hfSWFScriptObject_SpriteInstancePrototype spriteInstanceScriptObjectPrototype;

/*
========================
hfSWFSpriteInstance::hfSWFSpriteInstance
========================
*/
hfSWFSpriteInstance::hfSWFSpriteInstance() :
sprite( NULL ),
parent( NULL ),
depth( 0 ),
isPlaying( true ),
isVisible( true ),
childrenRunning( true ),
currentFrame( 0 ),
itemIndex( 0 ),
materialOverride( NULL ),
materialWidth( 0 ),
materialHeight( 0 ),
xOffset( 0.0f ),
yOffset( 0.0f ),
moveToXScale( 1.0f ),
moveToYScale( 1.0f ), 
moveToSpeed( 1.0f ),
firstRun( false ),
stereoDepth( 0 )
{
}

/*
========================
hfSWFSpriteInstance::Init
========================
*/
void hfSWFSpriteInstance::Init( hfSWFSprite * _sprite, hfSWFSpriteInstance * _parent, int _depth )  {
	sprite = _sprite;
	parent = _parent;
	depth = _depth;

	frameCount = sprite->frameCount;

	scriptObject = hfSWFScriptObject::Alloc();
	scriptObject->SetPrototype( &spriteInstanceScriptObjectPrototype );
	scriptObject->SetSprite( this );

	firstRun = true;

	actionScript = hfSWFScriptFunction_Script::Alloc();

	hfList<hfSWFScriptObject *, TAG_SWF> scope;
	scope.Append( sprite->swf->globals );
	scope.Append( scriptObject );
	actionScript->SetScope( scope );
	actionScript->SetDefaultSprite( this );

	for	(int i = 0; i < sprite->doInitActions.Num(); i++) {
		actionScript->SetData( sprite->doInitActions[i].Ptr(), sprite->doInitActions[i].Length() );
		actionScript->Call( scriptObject, hfSWFParmList() );
	}

	Play();
}

/*
========================
hfSWFSpriteInstance::~hfSWFSpriteInstance
========================
*/
hfSWFSpriteInstance::~hfSWFSpriteInstance() {
	if ( parent != NULL ) {
		parent->scriptObject->Set( name, hfSWFScriptVar() );
	}
	FreeDisplayList();
	displayList.Clear();
	scriptObject->SetSprite( NULL );
	scriptObject->Clear();
	scriptObject->Release();
	actionScript->Release();
}

/*
========================
hfSWFSpriteInstance::FreeDisplayList
========================
*/
void hfSWFSpriteInstance::FreeDisplayList() {
	for ( int i = 0; i < displayList.Num(); i++ ) {
		sprite->swf->spriteInstanceAllocator.Free( displayList[i].spriteInstance );
		sprite->swf->textInstanceAllocator.Free( displayList[i].textInstance );
	}
	displayList.SetNum( 0 );	// not calling Clear() so we don't continuously re-allocate memory
	currentFrame = 0;
}

/*
========================
hfSWFSpriteInstance::FindDisplayEntry
========================
*/
swfDisplayEntry_t * hfSWFSpriteInstance::FindDisplayEntry( int depth ) {
	int len = displayList.Num();
	int mid = len;
	int offset = 0;
	while ( mid > 0 ) {
		mid = len >> 1;
		if ( displayList[offset+mid].depth <= depth ) {
			offset += mid;
		}
		len -= mid;
	}
	if ( displayList[offset].depth == depth ) {
		return &displayList[offset];
	}
	return NULL;
}

/*
========================
hfSWFSpriteInstance::AddDisplayEntry
========================
*/
swfDisplayEntry_t * hfSWFSpriteInstance::AddDisplayEntry( int depth, int characterID ) {
	int i = 0;
	for ( ; i < displayList.Num(); i++ ) {
		if ( displayList[i].depth == depth ) {
			return NULL;
		}
		if ( displayList[i].depth > depth ) {
			break;
		}
	}

	swfDisplayEntry_t & display = displayList[ displayList.Insert( swfDisplayEntry_t(), i ) ];
	display.depth = depth;
	display.characterID = characterID;

	hfSWFDictionaryEntry * dictEntry = sprite->swf->FindDictionaryEntry( characterID );
	if ( dictEntry != NULL ) {
		if ( dictEntry->type == SWF_DICT_SPRITE ) {
			display.spriteInstance = sprite->swf->spriteInstanceAllocator.Alloc();
			display.spriteInstance->Init( dictEntry->sprite, this, depth );
			display.spriteInstance->RunTo( 1 );
		} else if ( dictEntry->type == SWF_DICT_EDITTEXT ) {
			display.textInstance = sprite->swf->textInstanceAllocator.Alloc();
			display.textInstance->Init( dictEntry->edittext, sprite->GetSWF() );
		}
	}
	return &display;
}

/*
========================
hfSWFSpriteInstance::RemoveDisplayEntry
========================
*/
void hfSWFSpriteInstance::RemoveDisplayEntry( int depth ) {
	swfDisplayEntry_t * entry = FindDisplayEntry( depth );
	if ( entry != NULL ) {
		sprite->swf->spriteInstanceAllocator.Free( entry->spriteInstance );
		sprite->swf->textInstanceAllocator.Free( entry->textInstance );
		displayList.RemoveIndex( displayList.IndexOf( entry ) );
	}
}

/*
================================================
hfSort_SpriteDepth 
================================================
*/
class hfSort_SpriteDepth : public hfSort_Quick< swfDisplayEntry_t, hfSort_SpriteDepth > {
public:
	int Compare( const swfDisplayEntry_t & a, const swfDisplayEntry_t & b ) const { return a.depth - b.depth; }
};

/*
========================
hfSWFSpriteInstance::SwapDepths
========================
*/
void hfSWFSpriteInstance::SwapDepths( int depth1, int depth2 ) {
	for ( int i = 0; i < displayList.Num(); i++ ) {
		if ( displayList[i].depth == depth1 ) {
			displayList[i].depth = depth2;

		} else if ( displayList[i].depth == depth2 ) {
			displayList[i].depth = depth1;
		}
		if ( displayList[i].spriteInstance != NULL ) {
			displayList[i].spriteInstance->depth = displayList[i].depth;
		}
	}

	displayList.SortWithTemplate( hfSort_SpriteDepth() );
}

/*
===================
hfSWFSpriteInstance::Run
===================
*/
bool hfSWFSpriteInstance::Run() {
	if ( !isVisible ) {
		return false;
	}

	if ( childrenRunning ) {
		childrenRunning = false;
		for ( int i = 0; i < displayList.Num(); i++ ) {
			if ( displayList[i].spriteInstance != NULL ) {
				Prefetch( displayList[i].spriteInstance, 0 );
			}
		}
		for ( int i = 0; i < displayList.Num(); i++ ) {
			if ( displayList[i].spriteInstance != NULL ) {
				childrenRunning |= displayList[i].spriteInstance->Run();
			}
		}
	}
	if ( isPlaying ) {
		if ( currentFrame == frameCount ) {
			if ( frameCount > 1 ) {
				FreeDisplayList();
				RunTo( 1 );
			}
		} else {
			RunTo( currentFrame + 1 );
		}
	}
	return childrenRunning || isPlaying;
}

/*
===================
hfSWFSpriteInstance::RunActions
===================
*/
bool hfSWFSpriteInstance::RunActions() {
	if ( !isVisible ) {
		actions.SetNum( 0 );
		return false;
	}

	if ( firstRun && scriptObject->HasProperty( "onLoad" ) ) {
		firstRun = false;
		hfSWFScriptVar onLoad = scriptObject->Get( "onLoad" );
		onLoad.GetFunction()->Call( scriptObject, hfSWFParmList() );
	}

	if ( onEnterFrame.IsFunction() ) {
		onEnterFrame.GetFunction()->Call( scriptObject, hfSWFParmList() );
	}

	for ( int i = 0; i < actions.Num(); i++ ) {
		actionScript->SetData( actions[i].data, actions[i].dataLength );
		actionScript->Call( scriptObject, hfSWFParmList() );
	}
	actions.SetNum( 0 );

	for ( int i = 0; i < displayList.Num(); i++ ) {
		if ( displayList[i].spriteInstance != NULL ) {
			Prefetch( displayList[i].spriteInstance, 0 );
		}
	}
	for ( int i = 0; i < displayList.Num(); i++ ) {
		if ( displayList[i].spriteInstance != NULL ) {
			displayList[i].spriteInstance->RunActions();
		}
	}

	return true;
}

/*
===================
hfSWFSpriteInstance::NextFrame
===================
*/
void hfSWFSpriteInstance::NextFrame() {
	if ( currentFrame < frameCount ) {
		RunTo( currentFrame + 1 );
	}
}

/*
===================
hfSWFSpriteInstance::PrevFrame
===================
*/
void hfSWFSpriteInstance::PrevFrame() {
	if ( currentFrame > 1 ) {
		RunTo( currentFrame - 1 );
	}
}

/*
===================
hfSWFSpriteInstance::Play
===================
*/
void hfSWFSpriteInstance::Play() {
	for ( hfSWFSpriteInstance * p = parent; p != NULL; p = p->parent ) {
		p->childrenRunning = true;
	}
	isPlaying = true;
}

/*
===================
hfSWFSpriteInstance::Stop
===================
*/
void hfSWFSpriteInstance::Stop() {
	isPlaying = false;
}

/*
===================
hfSWFSpriteInstance::RunTo
===================
*/
void hfSWFSpriteInstance::RunTo( int targetFrame ) {
	if ( targetFrame == currentFrame ) {
		return; // otherwise we'll re-run the current frame
	}
	if ( targetFrame < currentFrame ) {
		FreeDisplayList();
	}
	if ( targetFrame < 1 ) {
		return;
	}

	if ( targetFrame > sprite->frameOffsets.Num() - 1 ) {
		targetFrame = sprite->frameOffsets.Num() - 1;
	}

	//actions.Clear();

	uint32 firstActionCommand = sprite->frameOffsets[ targetFrame - 1 ];

	for ( uint32 c = sprite->frameOffsets[ currentFrame ]; c < sprite->frameOffsets[ targetFrame ]; c++ ) {
		hfSWFSprite::swfSpriteCommand_t & command = sprite->commands[ c ];
		if ( command.tag == Tag_DoAction && c < firstActionCommand ) {
			// Skip DoAction up to the firstActionCommand
			// This is to properly support skipping to a specific frame
			// for example if we're on frame 3 and skipping to frame 10, we want
			// to run all the commands PlaceObject commands for frames 4-10 but
			// only the DoAction commands for frame 10
			continue;
		}
		command.stream.Rewind();
		switch ( command.tag ) {
#define HANDLE_SWF_TAG( x ) case Tag_##x: x( command.stream ); break;
		HANDLE_SWF_TAG( PlaceObject2 );
		HANDLE_SWF_TAG( PlaceObject3 );
		HANDLE_SWF_TAG( RemoveObject2 );
		HANDLE_SWF_TAG( StartSound );
		HANDLE_SWF_TAG( DoAction );
#undef HANDLE_SWF_TAG
		default:
			hfLib::Printf( "Run Sprite: Unhandled tag %s\n", hfSWF::GetTagName( command.tag ) );
		}
	}

	currentFrame = targetFrame;
}

/*
========================
hfSWFSpriteInstance::DoAction
========================
*/
void hfSWFSpriteInstance::DoAction( hfSWFBitStream & bitstream ) {
	swfAction_t & action = actions.Alloc();
	action.data = bitstream.ReadData( bitstream.Length() );
	action.dataLength = bitstream.Length();
}

/*
========================
hfSWFSpriteInstance::FindChildSprite
========================
*/
hfSWFSpriteInstance * hfSWFSpriteInstance::FindChildSprite( const char * targetName ) {
	for ( int i = 0; i < displayList.Num(); i++ ) {
		if ( displayList[i].spriteInstance != NULL ) {
			if ( displayList[i].spriteInstance->name.Icmp( targetName ) == 0 ) {
				return displayList[i].spriteInstance;
			}
		}
	}
	return NULL;
}

/*
========================
hfSWFSpriteInstance::ResolveTarget
Gets the sprite instance for names like:
../foo/bar
/foo/bar
foo/bar
========================
*/
hfSWFSpriteInstance * hfSWFSpriteInstance::ResolveTarget( const char * targetName ) {
	if ( targetName[0] == 0 ) {
		return this;
	}
	hfSWFSpriteInstance * target = this;
	const char * c = targetName;
	if ( c[0] == '/' ) {
		while ( target->parent != NULL ) {
			target = target->parent;
		}
		c++;
	}
	hfStrList spriteNames;
	spriteNames.Append( c );
	for ( int index = 0, ofs = spriteNames[index].Find( '/' ); ofs != -1; index++, ofs = spriteNames[index].Find( '/' ) ) {
		spriteNames.Append( spriteNames[index].c_str() + ofs + 1 );
		spriteNames[index].CapLength( ofs );
	}
	for ( int i = 0; i < spriteNames.Num(); i++ ) {
		if ( spriteNames[i] == ".." ) {
			target = target->parent;
		} else {
			target = target->FindChildSprite( spriteNames[i] );
		}
		if ( target == NULL ) {
			// Everything is likely to fail after this point
			hfLib::Warning( "SWF: Could not resolve %s, %s not found", targetName, spriteNames[i].c_str() );
			return this;
		}
	}
	return target;
}

/*
========================
hfSWFSpriteInstance::FindFrame
========================
*/
uint32 hfSWFSpriteInstance::FindFrame( const char * labelName ) const {
	int frameNum = atoi( labelName );
	if ( frameNum > 0 ) {
		return frameNum;
	}
	for ( int i = 0; i < sprite->frameLabels.Num(); i++ ) {
		if ( sprite->frameLabels[i].frameLabel.Icmp( labelName ) == 0 ) {
			return sprite->frameLabels[i].frameNum;
		}
	}
	hfLib::Warning( "Could not find frame '%s' in sprite '%s'", labelName, GetName() );
	return currentFrame;
}

/*
========================
hfSWFSpriteInstance::FrameExists
========================
*/
bool hfSWFSpriteInstance::FrameExists( const char * labelName ) const {
	int frameNum = atoi( labelName );
	if ( frameNum > 0 ) {
		return frameNum <= sprite->frameCount;
	}

	for ( int i = 0; i < sprite->frameLabels.Num(); i++ ) {
		if ( sprite->frameLabels[i].frameLabel.Icmp( labelName ) == 0 ) {
			return true;
		}
	}

	return false;
}

/*
========================
hfSWFSpriteInstance::IsBetweenFrames
Checks if the current frame is between the given inclusive range.
========================
*/
bool hfSWFSpriteInstance::IsBetweenFrames( const char * frameLabel1, const char * frameLabel2 ) const {
	return currentFrame >= FindFrame( frameLabel1 ) && currentFrame <= FindFrame( frameLabel2 );
}

/*
========================
hfSWFSpriteInstance::SetMaterial
========================
*/
void hfSWFSpriteInstance::SetMaterial( const hfMaterial * material, int width, int height ) {
	materialOverride = material;

	if ( materialOverride != NULL ) {
		// Converting this to a short should be safe since we don't support images larger than 8k anyway
		if ( materialOverride->GetStage(0) != NULL && materialOverride->GetStage(0)->texture.cinematic != NULL ) {
			materialWidth = 256;
			materialHeight = 256;
		} else {
			assert( materialOverride->GetImageWidth() > 0 && materialOverride->GetImageHeight() > 0 );
			assert( materialOverride->GetImageWidth() <= 8192 && materialOverride->GetImageHeight() <= 8192 );
			materialWidth = (uint16)materialOverride->GetImageWidth();
			materialHeight = (uint16)materialOverride->GetImageHeight();
		}
	} else {
		materialWidth = 0;
		materialHeight = 0;
	}

	if ( width >= 0 ) {
		materialWidth = (uint16)width;
	}

	if ( height >= 0 ) {
		materialHeight = (uint16)height;
	}
}

/*
========================
hfSWFSpriteInstance::SetVisible
========================
*/
void hfSWFSpriteInstance::SetVisible( bool visible ) {
	isVisible = visible;
	if ( isVisible ) {
		for ( hfSWFSpriteInstance * p = parent; p != NULL; p = p->parent ) {
			p->childrenRunning = true;
		}
	}
}

/*
========================
hfSWFSpriteInstance::PlayFrame
========================
*/
void hfSWFSpriteInstance::PlayFrame( const hfSWFParmList & parms ) {
	if ( parms.Num() > 0 ) {
		actions.Clear();
		RunTo( FindFrame( parms[0].ToString() ) );
		Play();
	} else {
		hfLib::Warning( "gotoAndPlay: expected 1 paramater" );
	}
}

/*
========================
hfSWFSpriteInstance::StopFrame
========================
*/
void hfSWFSpriteInstance::StopFrame( const hfSWFParmList & parms ) {
	if ( parms.Num() > 0 ) {
		if ( parms[0].IsNumeric() && parms[0].ToInteger() < 1 ) {
			RunTo( FindFrame( "1" ) );
		} else {
			RunTo( FindFrame( parms[0].ToString() ) );
		}
		Stop();
	} else {
		hfLib::Warning( "gotoAndStop: expected 1 paramater" );
	}
}

/*
========================
hfSWFSpriteInstance::GetXPos
========================
*/
float hfSWFSpriteInstance::GetXPos() const {
	if ( parent == NULL ) {
		return 0.0f;
	}

	swfDisplayEntry_t * thisDisplayEntry = parent->FindDisplayEntry( depth );
	if ( thisDisplayEntry == NULL || thisDisplayEntry->spriteInstance != this ) {
		hfLib::Warning( "GetXPos: Couldn't find our display entry in our parent's display list for depth %d", depth );
		return 0.0f;
	}

	return thisDisplayEntry->matrix.tx;
}

/*
========================
hfSWFSpriteInstance::GetYPos
========================
*/
float hfSWFSpriteInstance::GetYPos( bool overallPos ) const {
	if ( parent == NULL ) {
		return 0.0f;
	}

	swfDisplayEntry_t * thisDisplayEntry = parent->FindDisplayEntry( depth );
	if ( thisDisplayEntry == NULL || thisDisplayEntry->spriteInstance != this ) {
		hfLib::Warning( "GetYPos: Couldn't find our display entry in our parents display list for depth %d", depth );
		return 0.0f;
	}

	return thisDisplayEntry->matrix.ty;
}

/*
========================
hfSWFSpriteInstance::SetXPos
========================
*/
void hfSWFSpriteInstance::SetXPos( float xPos ) {
	if ( parent == NULL ) {
		return;
	}

	swfDisplayEntry_t * thisDisplayEntry = parent->FindDisplayEntry( depth );
	if ( thisDisplayEntry == NULL || thisDisplayEntry->spriteInstance != this ) {
		hfLib::Warning( "_y: Couldn't find our display entry in our parents display list" );
		return;
	}

	thisDisplayEntry->matrix.tx = xPos;
}

/*
========================
hfSWFSpriteInstance::SetYPos
========================
*/
void hfSWFSpriteInstance::SetYPos( float yPos ) {
	if ( parent == NULL ) {
		return;
	}

	swfDisplayEntry_t * thisDisplayEntry = parent->FindDisplayEntry( depth );
	if ( thisDisplayEntry == NULL || thisDisplayEntry->spriteInstance != this ) {
		hfLib::Warning( "_y: Couldn't find our display entry in our parents display list" );
		return;
	}

	thisDisplayEntry->matrix.ty = yPos;
}

/*
========================
hfSWFSpriteInstance::SetPos
========================
*/
void hfSWFSpriteInstance::SetPos( float xPos, float yPos ) {
	if ( parent == NULL ) {
		return;
	}

	swfDisplayEntry_t * thisDisplayEntry = parent->FindDisplayEntry( depth );
	if ( thisDisplayEntry == NULL || thisDisplayEntry->spriteInstance != this ) {
		hfLib::Warning( "_y: Couldn't find our display entry in our parents display list" );
		return;
	}

	thisDisplayEntry->matrix.tx = xPos;
	thisDisplayEntry->matrix.ty = yPos;
}

/*
========================
hfSWFSpriteInstance::SetRotation
========================
*/
void hfSWFSpriteInstance::SetRotation( float rot ) {
	if ( parent == NULL ) {
		return;
	}
	swfDisplayEntry_t * thisDisplayEntry = parent->FindDisplayEntry( depth );
	if ( thisDisplayEntry == NULL || thisDisplayEntry->spriteInstance != this ) {
		hfLib::Warning( "_rotation: Couldn't find our display entry in our parents display list" );
		return;
	}

	swfMatrix_t & matrix = thisDisplayEntry->matrix;
	float xscale = matrix.Scale( hfVec2( 1.0f, 0.0f ) ).Length(); 
	float yscale = matrix.Scale( hfVec2( 0.0f, 1.0f ) ).Length(); 

	float s, c;
	hfMath::SinCos( DEG2RAD( rot ), s, c ); 
	matrix.xx = c * xscale;
	matrix.yx = s * xscale;
	matrix.xy = -s * yscale;
	matrix.yy = c * yscale;
}

/*
========================
hfSWFSpriteInstance::SetScale
========================
*/
void hfSWFSpriteInstance::SetScale( float x, float y ) {
	if ( parent == NULL ) {
		return;
	}
	swfDisplayEntry_t * thisDisplayEntry = parent->FindDisplayEntry( depth );
	if ( thisDisplayEntry == NULL || thisDisplayEntry->spriteInstance != this ) {
		hfLib::Warning( "scale: Couldn't find our display entry in our parents display list" );
		return;
	}

	float newScale = x / 100.0f;
	// this is done funky to maintain the current rotation
	hfVec2 currentScale = thisDisplayEntry->matrix.Scale( hfVec2( 1.0f, 0.0f ) );
	if ( currentScale.Normalize() == 0.0f ) {
		thisDisplayEntry->matrix.xx = newScale;
		thisDisplayEntry->matrix.yx = 0.0f;
	} else {
		thisDisplayEntry->matrix.xx = currentScale.x * newScale;
		thisDisplayEntry->matrix.yx = currentScale.y * newScale;
	}
	
	newScale = y / 100.0f;
	// this is done funky to maintain the current rotation
	currentScale = thisDisplayEntry->matrix.Scale( hfVec2( 0.0f, 1.0f ) );
	if ( currentScale.Normalize() == 0.0f ) {
		thisDisplayEntry->matrix.yy = newScale;
		thisDisplayEntry->matrix.xy = 0.0f;
	} else {
		thisDisplayEntry->matrix.yy = currentScale.y * newScale;
		thisDisplayEntry->matrix.xy = currentScale.x * newScale;
	}
}

/*
========================
hfSWFSpriteInstance::SetMoveToScale
========================
*/
void hfSWFSpriteInstance::SetMoveToScale( float x, float y ) {
	moveToXScale = x;
	moveToYScale = y;
}

/*
========================
hfSWFSpriteInstance::SetMoveToScale
========================
*/
bool hfSWFSpriteInstance::UpdateMoveToScale( float speed ) {

	if ( parent == NULL ) {
		return false;
	}
	swfDisplayEntry_t * thisDisplayEntry = parent->FindDisplayEntry( depth );
	if ( thisDisplayEntry == NULL || thisDisplayEntry->spriteInstance != this ) {
		hfLib::Warning( "SetMoveToScale: Couldn't find our display entry in our parents display list" );
		return false;
	}

	swfMatrix_t & matrix = thisDisplayEntry->matrix;
	float xscale = matrix.Scale( hfVec2( 1.0f, 0.0f ) ).Length() * 100.0f; 
	float yscale = matrix.Scale( hfVec2( 0.0f, 1.0f ) ).Length() * 100.0f;
	
	float toX = xscale;
	if ( moveToXScale >= 0.0f ) {
		toX = moveToXScale * 100.0f;
	}

	float toY = yscale;
	if ( moveToYScale >= 0.0f ) {
		toY = moveToYScale * 100.0f;
	}

	int rXTo = hfMath::Ftoi( toX + 0.5f );
	int rYTo = hfMath::Ftoi( toY + 0.5f );
	int rXScale = hfMath::Ftoi( xscale + 0.5f );
	int rYScale = hfMath::Ftoi( yscale + 0.5f );

	if ( rXTo == rXScale && rYTo == rYScale ) {
		return false;
	}

	float newXScale = xscale;
	float newYScale = yscale;

	if ( rXTo != rXScale && toX >= 0.0f ) {
		if ( toX < xscale ) {
			newXScale -= speed;
			newXScale = hfMath::ClampFloat( toX, 100.0f, newXScale );
		} else if ( toX > xscale ) {
			newXScale += speed;
			newXScale = hfMath::ClampFloat( 0.0f, toX, newXScale );
		}
	}

	if ( rYTo != rYScale && toY >= 0.0f ) {
		if ( toY < yscale ) {
			newYScale -= speed;
			newYScale = hfMath::ClampFloat( toY, 100.0f, newYScale );
		} else if ( toY > yscale ) {
			newYScale += speed;
			newYScale = hfMath::ClampFloat( 0.0f, toY, newYScale );
		}
	}

	SetScale( newXScale, newYScale );
	return true;
}

/*
========================
hfSWFSpriteInstance::SetAlpha
========================
*/
void hfSWFSpriteInstance::SetAlpha( float val ) {
	if ( parent == NULL ) {
		return;
	}

	swfDisplayEntry_t * thisDisplayEntry = parent->FindDisplayEntry( depth );
	if ( thisDisplayEntry == NULL || thisDisplayEntry->spriteInstance != this ) {
		hfLib::Warning( "_alpha: Couldn't find our display entry in our parents display list" );
		return;
	}
	thisDisplayEntry->cxf.mul.w = val;
}

/*
========================
hfSWFScriptObject_SpriteInstancePrototype
========================
*/
#define SWF_SPRITE_FUNCTION_DEFINE( x ) hfSWFScriptVar hfSWFScriptObject_SpriteInstancePrototype::hfSWFScriptFunction_##x::Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms )
#define SWF_SPRITE_NATIVE_VAR_DEFINE_GET( x ) hfSWFScriptVar hfSWFScriptObject_SpriteInstancePrototype::hfSWFScriptNativeVar_##x::Get( class hfSWFScriptObject * object )
#define SWF_SPRITE_NATIVE_VAR_DEFINE_SET( x ) void  hfSWFScriptObject_SpriteInstancePrototype::hfSWFScriptNativeVar_##x::Set( class hfSWFScriptObject * object, const hfSWFScriptVar & value )

#define SWF_SPRITE_PTHIS_FUNC( x ) hfSWFSpriteInstance * pThis = thisObject ? thisObject->GetSprite() : NULL; if ( !verify( pThis != NULL ) ) { hfLib::Warning( "SWF: tried to call " x " on NULL sprite" ); return hfSWFScriptVar(); }
#define SWF_SPRITE_PTHIS_GET( x ) hfSWFSpriteInstance * pThis = object ? object->GetSprite() : NULL; if ( pThis == NULL ) { return hfSWFScriptVar(); }
#define SWF_SPRITE_PTHIS_SET( x ) hfSWFSpriteInstance * pThis = object ? object->GetSprite() : NULL; if ( pThis == NULL ) { return; }

#define SWF_SPRITE_FUNCTION_SET( x ) scriptFunction_##x.AddRef(); Set( #x, &scriptFunction_##x );
#define SWF_SPRITE_NATIVE_VAR_SET( x ) SetNative( #x, &swfScriptVar_##x );

hfSWFScriptObject_SpriteInstancePrototype::hfSWFScriptObject_SpriteInstancePrototype() {
	SWF_SPRITE_FUNCTION_SET( duplicateMovieClip );
	SWF_SPRITE_FUNCTION_SET( gotoAndPlay );
	SWF_SPRITE_FUNCTION_SET( gotoAndStop );
	SWF_SPRITE_FUNCTION_SET( swapDepths );
	SWF_SPRITE_FUNCTION_SET( nextFrame );
	SWF_SPRITE_FUNCTION_SET( prevFrame );
	SWF_SPRITE_FUNCTION_SET( play );
	SWF_SPRITE_FUNCTION_SET( stop );

	SWF_SPRITE_NATIVE_VAR_SET( _x );
	SWF_SPRITE_NATIVE_VAR_SET( _y );
	SWF_SPRITE_NATIVE_VAR_SET( _xscale );
	SWF_SPRITE_NATIVE_VAR_SET( _yscale );
	SWF_SPRITE_NATIVE_VAR_SET( _alpha );
	SWF_SPRITE_NATIVE_VAR_SET( _brightness );
	SWF_SPRITE_NATIVE_VAR_SET( _visible );
	SWF_SPRITE_NATIVE_VAR_SET( _width );
	SWF_SPRITE_NATIVE_VAR_SET( _height );
	SWF_SPRITE_NATIVE_VAR_SET( _rotation );
	SWF_SPRITE_NATIVE_VAR_SET( _name );
	SWF_SPRITE_NATIVE_VAR_SET( _currentframe );
	SWF_SPRITE_NATIVE_VAR_SET( _totalframes );
	SWF_SPRITE_NATIVE_VAR_SET( _target );
	SWF_SPRITE_NATIVE_VAR_SET( _framesloaded );
	SWF_SPRITE_NATIVE_VAR_SET( _droptarget );
	SWF_SPRITE_NATIVE_VAR_SET( _url );
	SWF_SPRITE_NATIVE_VAR_SET( _highquality );
	SWF_SPRITE_NATIVE_VAR_SET( _focusrect );
	SWF_SPRITE_NATIVE_VAR_SET( _soundbuftime );
	SWF_SPRITE_NATIVE_VAR_SET( _quality );
	SWF_SPRITE_NATIVE_VAR_SET( _mousex );
	SWF_SPRITE_NATIVE_VAR_SET( _mousey );

	SWF_SPRITE_NATIVE_VAR_SET( _stereoDepth );
	SWF_SPRITE_NATIVE_VAR_SET( _itemindex );
	SWF_SPRITE_NATIVE_VAR_SET( material );
	SWF_SPRITE_NATIVE_VAR_SET( materialWidth );
	SWF_SPRITE_NATIVE_VAR_SET( materialHeight );
	SWF_SPRITE_NATIVE_VAR_SET( xOffset );

	SWF_SPRITE_NATIVE_VAR_SET( onEnterFrame );
	//SWF_SPRITE_NATIVE_VAR_SET( onLoad );
}

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _target ) { return ""; }
SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _droptarget ) { return ""; }
SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _url ) { return ""; }
SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _highquality ) { return 2; }
SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _focusrect ) { return true; }
SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _soundbuftime ) { return 0; }
SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _quality ) { return "BEST"; }

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _width ) { return 0.0f; }
SWF_SPRITE_NATIVE_VAR_DEFINE_SET( _width ) { }

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _height ) { return 0.0f; }
SWF_SPRITE_NATIVE_VAR_DEFINE_SET( _height ) { }

SWF_SPRITE_FUNCTION_DEFINE( duplicateMovieClip ) {
	SWF_SPRITE_PTHIS_FUNC( "duplicateMovieClip" );

	if ( pThis->parent == NULL ) {
		hfLib::Warning( "Tried to duplicate root movie clip" );
		return hfSWFScriptVar();
	}
	if ( parms.Num() < 2 ) {
		hfLib::Warning( "duplicateMovieClip: expected 2 parameters" );
		return hfSWFScriptVar();
	}	
	swfDisplayEntry_t * thisDisplayEntry = pThis->parent->FindDisplayEntry( pThis->depth );
	if ( thisDisplayEntry == NULL || thisDisplayEntry->spriteInstance != pThis ) {
		hfLib::Warning( "duplicateMovieClip: Couldn't find our display entry in our parents display list" );
		return hfSWFScriptVar();
	}

	swfMatrix_t matrix = thisDisplayEntry->matrix;
	swfColorXform_t cxf = thisDisplayEntry->cxf;

	swfDisplayEntry_t * display = pThis->parent->AddDisplayEntry( 16384 + parms[1].ToInteger(), thisDisplayEntry->characterID );
	if ( display == NULL ) {
		return hfSWFScriptVar();
	}
	display->matrix = matrix;
	display->cxf = cxf;

	hfStr name = parms[0].ToString();
	pThis->parent->scriptObject->Set( name, display->spriteInstance->scriptObject );
	display->spriteInstance->name = name;
	display->spriteInstance->RunTo( 1 );

	return display->spriteInstance->scriptObject;
}

SWF_SPRITE_FUNCTION_DEFINE( gotoAndPlay ) {
	SWF_SPRITE_PTHIS_FUNC( "gotoAndPlay" );

	if ( parms.Num() > 0 ) {
		pThis->actions.Clear();
		pThis->RunTo( pThis->FindFrame( parms[0].ToString() ) );
		pThis->Play();
	} else {
		hfLib::Warning( "gotoAndPlay: expected 1 paramater" );
	}
	return hfSWFScriptVar();
}

SWF_SPRITE_FUNCTION_DEFINE( gotoAndStop ) {
	SWF_SPRITE_PTHIS_FUNC( "gotoAndStop" );

	if ( parms.Num() > 0 ) {
		// Flash forces frames values less than 1 to 1.
		if ( parms[0].IsNumeric() && parms[0].ToInteger() < 1 ) {
			pThis->RunTo( pThis->FindFrame( "1" ) );
		} else {
			pThis->RunTo( pThis->FindFrame( parms[0].ToString() ) );
		}
		pThis->Stop();
	} else {
		hfLib::Warning( "gotoAndStop: expected 1 paramater" );
	}
	return hfSWFScriptVar();
}

SWF_SPRITE_FUNCTION_DEFINE( swapDepths ) {
	SWF_SPRITE_PTHIS_FUNC( "swapDepths" );

	if ( pThis->parent == NULL ) {
		hfLib::Warning( "Tried to swap depths on root movie clip" );
		return hfSWFScriptVar();
	}
	if ( parms.Num() < 1 ) {
		hfLib::Warning( "swapDepths: expected 1 parameters" );
		return hfSWFScriptVar();
	}
	pThis->parent->SwapDepths( pThis->depth, parms[0].ToInteger() );
	return hfSWFScriptVar();
}

SWF_SPRITE_FUNCTION_DEFINE( nextFrame ) {
	SWF_SPRITE_PTHIS_FUNC( "nextFrame" );
	pThis->NextFrame();
	return hfSWFScriptVar();
}

SWF_SPRITE_FUNCTION_DEFINE( prevFrame ) {
	SWF_SPRITE_PTHIS_FUNC( "prevFrame" );
	pThis->PrevFrame();
	return hfSWFScriptVar();
}
SWF_SPRITE_FUNCTION_DEFINE( play ) {
	SWF_SPRITE_PTHIS_FUNC( "play" );
	pThis->Play();
	return hfSWFScriptVar();
}
SWF_SPRITE_FUNCTION_DEFINE( stop ) {
	SWF_SPRITE_PTHIS_FUNC( "stop" );
	pThis->Stop();
	return hfSWFScriptVar();
}

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _x ) {
	SWF_SPRITE_PTHIS_GET( "_x" );
	return pThis->GetXPos();
}

SWF_SPRITE_NATIVE_VAR_DEFINE_SET( _x ) {
	SWF_SPRITE_PTHIS_SET( "_x" );
	pThis->SetXPos( value.ToFloat() );
}

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _y ) {
	SWF_SPRITE_PTHIS_GET( "_y" );
	return pThis->GetYPos();
}

SWF_SPRITE_NATIVE_VAR_DEFINE_SET( _y ) {
	SWF_SPRITE_PTHIS_SET( "_y" );
	pThis->SetYPos( value.ToFloat() );
}

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _xscale ) {
	SWF_SPRITE_PTHIS_GET( "_xscale" );
	if ( pThis->parent == NULL ) {
		return 1.0f;
	}
	swfDisplayEntry_t * thisDisplayEntry = pThis->parent->FindDisplayEntry( pThis->depth );
	if ( thisDisplayEntry == NULL || thisDisplayEntry->spriteInstance != pThis ) {
		hfLib::Warning( "_xscale: Couldn't find our display entry in our parents display list" );
		return 1.0f;
	}
	return thisDisplayEntry->matrix.Scale( hfVec2( 1.0f, 0.0f ) ).Length() * 100.0f;
}

SWF_SPRITE_NATIVE_VAR_DEFINE_SET( _xscale ) {
	SWF_SPRITE_PTHIS_SET( "_xscale" );
	if ( pThis->parent == NULL ) {
		return;
	}
	swfDisplayEntry_t * thisDisplayEntry = pThis->parent->FindDisplayEntry( pThis->depth );
	if ( thisDisplayEntry == NULL || thisDisplayEntry->spriteInstance != pThis ) {
		hfLib::Warning( "_xscale: Couldn't find our display entry in our parents display list" );
		return;
	}
	float newScale = value.ToFloat() / 100.0f;
	// this is done funky to maintain the current rotation
	hfVec2 currentScale = thisDisplayEntry->matrix.Scale( hfVec2( 1.0f, 0.0f ) );
	if ( currentScale.Normalize() == 0.0f ) {
		thisDisplayEntry->matrix.xx = newScale;
		thisDisplayEntry->matrix.yx = 0.0f;
	} else {
		thisDisplayEntry->matrix.xx = currentScale.x * newScale;
		thisDisplayEntry->matrix.yx = currentScale.y * newScale;
	}
}

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _yscale ) {
	SWF_SPRITE_PTHIS_GET( "_yscale" );
	if ( pThis->parent == NULL ) {
		return 1.0f;
	}
	swfDisplayEntry_t * thisDisplayEntry = pThis->parent->FindDisplayEntry( pThis->depth );
	if ( thisDisplayEntry == NULL || thisDisplayEntry->spriteInstance != pThis ) {
		hfLib::Warning( "_yscale: Couldn't find our display entry in our parents display list" );
		return 1.0f;
	}
	return thisDisplayEntry->matrix.Scale( hfVec2( 0.0f, 1.0f ) ).Length() * 100.0f;
}

SWF_SPRITE_NATIVE_VAR_DEFINE_SET( _yscale ) {
	SWF_SPRITE_PTHIS_SET( "_yscale" );
	if ( pThis->parent == NULL ) {
		return;
	}
	swfDisplayEntry_t * thisDisplayEntry = pThis->parent->FindDisplayEntry( pThis->depth );
	if ( thisDisplayEntry == NULL || thisDisplayEntry->spriteInstance != pThis ) {
		hfLib::Warning( "_yscale: Couldn't find our display entry in our parents display list" );
		return;
	}
	float newScale = value.ToFloat() / 100.0f;
	// this is done funky to maintain the current rotation
	hfVec2 currentScale = thisDisplayEntry->matrix.Scale( hfVec2( 0.0f, 1.0f ) );
	if ( currentScale.Normalize() == 0.0f ) {
		thisDisplayEntry->matrix.yy = newScale;
		thisDisplayEntry->matrix.xy = 0.0f;
	} else {
		thisDisplayEntry->matrix.yy = currentScale.y * newScale;
		thisDisplayEntry->matrix.xy = currentScale.x * newScale;
	}
}

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _alpha ) {
	SWF_SPRITE_PTHIS_GET( "_alpha" );
	if ( pThis->parent == NULL ) {
		return 1.0f;
	}
	swfDisplayEntry_t * thisDisplayEntry = pThis->parent->FindDisplayEntry( pThis->depth );
	if ( thisDisplayEntry == NULL || thisDisplayEntry->spriteInstance != pThis ) {
		hfLib::Warning( "_alpha: Couldn't find our display entry in our parents display list" );
		return 1.0f;
	}
	return thisDisplayEntry->cxf.mul.w;
}

SWF_SPRITE_NATIVE_VAR_DEFINE_SET( _alpha ) {
	SWF_SPRITE_PTHIS_SET( "_alpha" );

	pThis->SetAlpha( value.ToFloat() );
}

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _brightness ) {
	SWF_SPRITE_PTHIS_GET( "_brightness" );
	if ( pThis->parent == NULL ) {
		return 1.0f;
	}
	swfDisplayEntry_t * thisDisplayEntry = pThis->parent->FindDisplayEntry( pThis->depth );
	if ( thisDisplayEntry == NULL || thisDisplayEntry->spriteInstance != pThis ) {
		hfLib::Warning( "_brightness: Couldn't find our display entry in our parents display list" );
		return 1.0f;
	}
	// This works as long as the user only used the "brightess" control in the editor
	// If they used anything else (tint/advanced) then this will return fairly random values
	const hfVec4 & mul = thisDisplayEntry->cxf.mul;
	const hfVec4 & add = thisDisplayEntry->cxf.add;
	float avgMul = ( mul.x + mul.y + mul.z ) / 3.0f;
	float avgAdd = ( add.x + add.y + add.z ) / 3.0f;
	if ( avgAdd > 1.0f ) {
		return avgAdd;
	} else {
		return avgMul - 1.0f;
	}
}

SWF_SPRITE_NATIVE_VAR_DEFINE_SET( _brightness ) {
	SWF_SPRITE_PTHIS_SET( "_brightness" );
	if ( pThis->parent == NULL ) {
		return;
	}
	swfDisplayEntry_t * thisDisplayEntry = pThis->parent->FindDisplayEntry( pThis->depth );
	if ( thisDisplayEntry == NULL || thisDisplayEntry->spriteInstance != pThis ) {
		hfLib::Warning( "_brightness: Couldn't find our display entry in our parents display list" );
		return;
	}
	// This emulates adjusting the "brightness" slider in the editor
	// Although the editor forces alpha to 100%
	float b = value.ToFloat();
	float c = 1.0f - b;
	if ( b < 0.0f ) {
		c = 1.0f + b;
		b = 0.0f;
	}
	thisDisplayEntry->cxf.add.Set( b, b, b, thisDisplayEntry->cxf.add.w );
	thisDisplayEntry->cxf.mul.Set( c, c, c, thisDisplayEntry->cxf.mul.w );
}

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _visible ) {
	SWF_SPRITE_PTHIS_GET( "_visible" );
	return pThis->isVisible;
}

SWF_SPRITE_NATIVE_VAR_DEFINE_SET( _visible ) {
	SWF_SPRITE_PTHIS_SET( "_visible" );
	pThis->isVisible = value.ToBool();
	if ( pThis->isVisible ) {
		for ( hfSWFSpriteInstance * p = pThis->parent; p != NULL; p = p->parent ) {
			p->childrenRunning = true;
		}
	}
}

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _rotation ) {
	SWF_SPRITE_PTHIS_GET( "_rotation" );
	if ( pThis->parent == NULL ) {
		return 0.0f;
	}
	swfDisplayEntry_t * thisDisplayEntry = pThis->parent->FindDisplayEntry( pThis->depth );
	if ( thisDisplayEntry == NULL || thisDisplayEntry->spriteInstance != pThis ) {
		hfLib::Warning( "_rotation: Couldn't find our display entry in our parents display list" );
		return 0.0f;
	}
	hfVec2 scale = thisDisplayEntry->matrix.Scale( hfVec2( 0.0f, 1.0f ) );
	scale.Normalize();
	float rotation = RAD2DEG( hfMath::ACos( scale.y ) );
	if ( scale.x < 0.0f ) {
		rotation = -rotation;
	}
	return rotation;
}

SWF_SPRITE_NATIVE_VAR_DEFINE_SET( _rotation ) {
	SWF_SPRITE_PTHIS_SET( "_rotation" );
	if ( pThis->parent == NULL ) {
		return;
	}
	swfDisplayEntry_t * thisDisplayEntry = pThis->parent->FindDisplayEntry( pThis->depth );
	if ( thisDisplayEntry == NULL || thisDisplayEntry->spriteInstance != pThis ) {
		hfLib::Warning( "_rotation: Couldn't find our display entry in our parents display list" );
		return;
	}
	swfMatrix_t & matrix = thisDisplayEntry->matrix;
	float xscale = matrix.Scale( hfVec2( 1.0f, 0.0f ) ).Length(); 
	float yscale = matrix.Scale( hfVec2( 0.0f, 1.0f ) ).Length(); 

	float s, c;
	hfMath::SinCos( DEG2RAD( value.ToFloat() ), s, c ); 
	matrix.xx = c * xscale;
	matrix.yx = s * xscale;
	matrix.xy = -s * yscale;
	matrix.yy = c * yscale;
}

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _name ) {
	SWF_SPRITE_PTHIS_GET( "_name" );
	return pThis->name.c_str();
}

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _currentframe ) {
	SWF_SPRITE_PTHIS_GET( "_currentframe" );
	return pThis->currentFrame;
}

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _totalframes ) {
	SWF_SPRITE_PTHIS_GET( "_totalframes" );
	return pThis->frameCount;
}

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _framesloaded ) {
	SWF_SPRITE_PTHIS_GET( "_framesloaded" );
	return pThis->frameCount;
}

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _mousex ) {
	SWF_SPRITE_PTHIS_GET( "_mousex" );
	if ( pThis->parent == NULL ) {
		return pThis->sprite->GetSWF()->GetMouseX();
	}
	swfDisplayEntry_t * thisDisplayEntry = pThis->parent->FindDisplayEntry( pThis->depth );
	if ( thisDisplayEntry == NULL || thisDisplayEntry->spriteInstance != pThis ) {
		hfLib::Warning( "_mousex: Couldn't find our display entry in our parents display list" );
		return pThis->sprite->GetSWF()->GetMouseX();
	}
	return pThis->sprite->GetSWF()->GetMouseX() - thisDisplayEntry->matrix.ty;
}
SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _mousey ) {
	SWF_SPRITE_PTHIS_GET( "_mousey" );
	if ( pThis->parent == NULL ) {
		return pThis->sprite->GetSWF()->GetMouseY();
	}
	swfDisplayEntry_t * thisDisplayEntry = pThis->parent->FindDisplayEntry( pThis->depth );
	if ( thisDisplayEntry == NULL || thisDisplayEntry->spriteInstance != pThis ) {
		hfLib::Warning( "_mousey: Couldn't find our display entry in our parents display list" );
		return pThis->sprite->GetSWF()->GetMouseY();
	}
	return pThis->sprite->GetSWF()->GetMouseY() - thisDisplayEntry->matrix.ty;
}

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _itemindex ) {
	SWF_SPRITE_PTHIS_GET( "_itemindex" );
	return pThis->itemIndex;
}

SWF_SPRITE_NATIVE_VAR_DEFINE_SET( _itemindex ) {
	SWF_SPRITE_PTHIS_SET( "_itemindex" );
	pThis->itemIndex = value.ToInteger();
}

SWF_SPRITE_NATIVE_VAR_DEFINE_SET( _stereoDepth ) {
	SWF_SPRITE_PTHIS_SET( "_stereoDepth" );
	pThis->stereoDepth = value.ToInteger();
}

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( _stereoDepth ) {
	SWF_SPRITE_PTHIS_GET( "_stereoDepth" );
	return pThis->stereoDepth;
}

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( material ) {
	SWF_SPRITE_PTHIS_GET( "material" );
	if ( pThis->materialOverride == NULL ) {
		return hfSWFScriptVar();
	} else {
		return pThis->materialOverride->GetName();
	}
}

SWF_SPRITE_NATIVE_VAR_DEFINE_SET( material ) {
	SWF_SPRITE_PTHIS_SET( "material" );
	if ( !value.IsString() ) {
		pThis->materialOverride = NULL;
	} else {
		// God I hope this material was referenced during map load
		pThis->SetMaterial( declManager->FindMaterial( value.ToString(), false ) );
	}
}

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( materialWidth ) {
	SWF_SPRITE_PTHIS_GET( "materialWidth" );
	return pThis->materialWidth;
}

SWF_SPRITE_NATIVE_VAR_DEFINE_SET( materialWidth ) {
	SWF_SPRITE_PTHIS_SET( "materialWidth" );
	assert( value.ToInteger() > 0 );
	assert( value.ToInteger() <= 8192 );
	pThis->materialWidth = (uint16)value.ToInteger();
}

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( materialHeight ) {
	SWF_SPRITE_PTHIS_GET( "materialHeight" );
	return pThis->materialHeight;
}

SWF_SPRITE_NATIVE_VAR_DEFINE_SET( materialHeight ) {
	SWF_SPRITE_PTHIS_SET( "materialHeight" );
	assert( value.ToInteger() > 0 );
	assert( value.ToInteger() <= 8192 );
	pThis->materialHeight = (uint16)value.ToInteger();
}

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( xOffset ) {
	SWF_SPRITE_PTHIS_GET( "xOffset" );
	return pThis->xOffset;
}

SWF_SPRITE_NATIVE_VAR_DEFINE_SET( xOffset ) {
	SWF_SPRITE_PTHIS_SET( "xOffset" );
	pThis->xOffset = value.ToFloat();
}

SWF_SPRITE_NATIVE_VAR_DEFINE_GET( onEnterFrame ) {
	SWF_SPRITE_PTHIS_GET( "onEnterFrame" );
	return pThis->onEnterFrame;
}

SWF_SPRITE_NATIVE_VAR_DEFINE_SET( onEnterFrame ) {
	SWF_SPRITE_PTHIS_SET( "onEnterFrame" );
	pThis->onEnterFrame = value;
}
