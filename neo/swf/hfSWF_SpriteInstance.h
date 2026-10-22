#ifndef __SWF_SPRITEINSTANCE_H__
#define __SWF_SPRITEINSTANCE_H__

/*
================================================
There can be multiple instances of a single sprite running
================================================
*/
class hfSWFSpriteInstance {
public:
	hfSWFSpriteInstance();
	~hfSWFSpriteInstance();

	void	Init( hfSWFSprite * sprite, hfSWFSpriteInstance * parent, int depth );

	bool	Run();
	bool	RunActions();

	const char * GetName() const { return name.c_str(); }

	hfSWFScriptObject * GetScriptObject() { return scriptObject; }
	void SetAlignment( float x, float y ) { xOffset = x; yOffset = y; }

	void SetMaterial( const hfMaterial * material, int width = -1, int height = -1 );
	void SetVisible( bool visible );
	bool IsVisible() { return isVisible; }
	void PlayFrame( const hfSWFParmList & parms );
	void PlayFrame( const char * frameName ) {
		hfSWFParmList parms;
		parms.Append( frameName );
		PlayFrame( parms );
	}
	void PlayFrame( const int frameNum ) {
		hfSWFParmList parms;
		parms.Append( frameNum );
		PlayFrame( parms );
	}
	void StopFrame( const hfSWFParmList & parms );
	void StopFrame( const char * frameName ) {
		hfSWFParmList parms;
		parms.Append( frameName );
		StopFrame( parms );
	}
	void StopFrame( const int frameNum ) {
		hfSWFParmList parms;
		parms.Append( frameNum );
		StopFrame( parms );
	}
	// FIXME: Why do all the Set functions have defaults of -1.0f?  This seems arbitrar.
	// Probably better to not have a default at all, so any non-parametized calls throw a
	// compilation error.
	float GetXPos() const;
	float GetYPos( bool overallPos = false ) const;
	void SetXPos( float xPos = -1.0f );
	void SetYPos( float yPos = -1.0f );
	void SetPos( float xPos = -1.0f, float yPos = -1.0f );
	void SetAlpha( float val );
	void SetScale( float x = -1.0f, float y = -1.0f );
	void SetMoveToScale( float x = -1.0f, float y = -1.0f );
	bool UpdateMoveToScale( float speed );	// returns true if the update was successful
	void SetRotation( float rot );
	uint16 GetCurrentFrame() { return currentFrame; }
	bool IsPlaying() const { return isPlaying; }
	int GetStereoDepth() { return stereoDepth; }

	// Removing the private access control statement due to cl 214702
	// Apparently MS's C++ compiler supports the newer C++ standard, and GCC supports C++03
	// In the new C++ standard, nested members of a friend class have access to private/protected members of the class granting friendship
	// In C++03, nested members defined in a friend class do NOT have access to private/protected members of the class granting friendship	friend class hfSWF;

	bool isPlaying;
	bool isVisible;
	bool childrenRunning;
	bool firstRun;

	// currentFrame is the frame number currently in the displayList
	// we use 1 based frame numbers because currentFrame = 0 means nothing is in the display list
	// it's also convenient because Flash also uses 1 based frame numbers
	uint16	currentFrame;
	uint16	frameCount;
	
	// the sprite this is an instance of
	hfSWFSprite * sprite;

	// sprite instances can be nested
	hfSWFSpriteInstance * parent;

	// depth of this sprite instance in the parent's display list
	int depth;

	// if this is set, apply this material when rendering any child shapes
	int itemIndex;

	const hfMaterial * materialOverride;
	uint16 materialWidth;
	uint16 materialHeight;

	float xOffset;
	float yOffset;

	float moveToXScale;
	float moveToYScale;
	float moveToSpeed;

	int stereoDepth;
	
	hfSWFScriptObject * scriptObject;

	// children display entries
	hfList< swfDisplayEntry_t, TAG_SWF > displayList;
	swfDisplayEntry_t * FindDisplayEntry( int depth );

	// name of this sprite instance
	hfStr name;

	struct swfAction_t {
		const byte * data;
		uint32 dataLength;
	};
	hfList< swfAction_t, TAG_SWF > actions;

	hfSWFScriptFunction_Script * actionScript;

	hfSWFScriptVar onEnterFrame;
	//hfSWFScriptVar onLoad;

	// Removing the private access control statement due to cl 214702
	// Apparently MS's C++ compiler supports the newer C++ standard, and GCC supports C++03
	// In the new C++ standard, nested members of a friend class have access to private/protected members of the class granting friendship
	// In C++03, nested members defined in a friend class do NOT have access to private/protected members of the class granting friendship

	//----------------------------------
	// hfSWF_PlaceObject.cpp
	//----------------------------------
	void			PlaceObject2( hfSWFBitStream & bitstream );
	void			PlaceObject3( hfSWFBitStream & bitstream );
	void			RemoveObject2( hfSWFBitStream & bitstream );

	//----------------------------------
	// hfSWF_Sounds.cpp
	//----------------------------------
	void			StartSound( hfSWFBitStream & bitstream );

	//----------------------------------
	// hfSWF_SpriteInstance.cpp
	//----------------------------------
	void			NextFrame();
	void			PrevFrame();
	void			RunTo( int frameNum );

	void			Play();
	void			Stop();

	void					FreeDisplayList();
	swfDisplayEntry_t *		AddDisplayEntry( int depth, int characterID );
	void					RemoveDisplayEntry( int depth );
	void					SwapDepths( int depth1, int depth2 );

	void					DoAction( hfSWFBitStream & bitstream );

	hfSWFSpriteInstance *	FindChildSprite( const char * childName );
	hfSWFSpriteInstance *	ResolveTarget( const char * targetName );
	uint32					FindFrame( const char * frameLabel ) const;
	bool					FrameExists( const char * frameLabel ) const;
	bool					IsBetweenFrames( const char * frameLabel1, const char * frameLabel2 ) const;
};

/*
================================================
This is the prototype object that all the sprite instance script objects reference
================================================
*/
class hfSWFScriptObject_SpriteInstancePrototype : public hfSWFScriptObject {
public:
	hfSWFScriptObject_SpriteInstancePrototype();

#define SWF_SPRITE_FUNCTION_DECLARE( x ) \
	class hfSWFScriptFunction_##x : public hfSWFScriptFunction { \
	public: \
		void			AddRef() {} \
		void			Release() {} \
		hfSWFScriptVar Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ); \
	} scriptFunction_##x

	SWF_SPRITE_FUNCTION_DECLARE( duplicateMovieClip );
	SWF_SPRITE_FUNCTION_DECLARE( gotoAndPlay );
	SWF_SPRITE_FUNCTION_DECLARE( gotoAndStop );
	SWF_SPRITE_FUNCTION_DECLARE( swapDepths );
	SWF_SPRITE_FUNCTION_DECLARE( nextFrame );
	SWF_SPRITE_FUNCTION_DECLARE( prevFrame );
	SWF_SPRITE_FUNCTION_DECLARE( play );
	SWF_SPRITE_FUNCTION_DECLARE( stop );

	SWF_NATIVE_VAR_DECLARE( _x );
	SWF_NATIVE_VAR_DECLARE( _y );
	SWF_NATIVE_VAR_DECLARE( _xscale );
	SWF_NATIVE_VAR_DECLARE( _yscale );
	SWF_NATIVE_VAR_DECLARE( _alpha );
	SWF_NATIVE_VAR_DECLARE( _brightness );
	SWF_NATIVE_VAR_DECLARE( _visible );
	SWF_NATIVE_VAR_DECLARE( _width );
	SWF_NATIVE_VAR_DECLARE( _height );
	SWF_NATIVE_VAR_DECLARE( _rotation );

	SWF_NATIVE_VAR_DECLARE_READONLY( _name );
	SWF_NATIVE_VAR_DECLARE_READONLY( _currentframe );
	SWF_NATIVE_VAR_DECLARE_READONLY( _totalframes );
	SWF_NATIVE_VAR_DECLARE_READONLY( _target );
	SWF_NATIVE_VAR_DECLARE_READONLY( _framesloaded );
	SWF_NATIVE_VAR_DECLARE_READONLY( _droptarget );
	SWF_NATIVE_VAR_DECLARE_READONLY( _url );
	SWF_NATIVE_VAR_DECLARE_READONLY( _highquality );
	SWF_NATIVE_VAR_DECLARE_READONLY( _focusrect );
	SWF_NATIVE_VAR_DECLARE_READONLY( _soundbuftime );
	SWF_NATIVE_VAR_DECLARE_READONLY( _quality );
	SWF_NATIVE_VAR_DECLARE_READONLY( _mousex );
	SWF_NATIVE_VAR_DECLARE_READONLY( _mousey );

	SWF_NATIVE_VAR_DECLARE( _stereoDepth );
	SWF_NATIVE_VAR_DECLARE( _itemindex );

	SWF_NATIVE_VAR_DECLARE( material );
	SWF_NATIVE_VAR_DECLARE( materialWidth );
	SWF_NATIVE_VAR_DECLARE( materialHeight );

	SWF_NATIVE_VAR_DECLARE( xOffset );
	SWF_NATIVE_VAR_DECLARE( onEnterFrame );
	//SWF_NATIVE_VAR_DECLARE( onLoad );
};

#endif

