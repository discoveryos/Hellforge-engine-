#ifndef __SWF_H__
#define __SWF_H__

#include "hfSWF_Enums.h"
#include "hfSWF_Types.h"
#include "hfSWF_Bitstream.h"
#include "hfSWF_ScriptVar.h"
#include "hfSWF_Sprites.h"
#include "hfSWF_ScriptObject.h"
#include "hfSWF_ParmList.h"
#include "hfSWF_ScriptFunction.h"
#include "hfSWF_SpriteInstance.h"
#include "hfSWF_ShapeParser.h"
#include "hfSWF_TextInstance.h"

class hfSWFDictionaryEntry {
public:
	hfSWFDictionaryEntry();
	~hfSWFDictionaryEntry();
	hfSWFDictionaryEntry & operator=( hfSWFDictionaryEntry & other );

	swfDictType_t		type;
	const hfMaterial *	material;
	hfSWFShape *		shape;
	hfSWFSprite *		sprite;
	hfSWFFont *			font;
	hfSWFText *			text;
	hfSWFEditText *		edittext;

	hfVec2i				imageSize;
	hfVec2i				imageAtlasOffset;
	// the compressed images are normalize to reduce compression artifacts,
	// color must be scaled down by this
	hfVec4				channelScale;
};

struct purgableSwfImage_t {
	purgableSwfImage_t() { image = NULL; swfFrameNum = 0; }
	hfImage * image;
	unsigned swfFrameNum;
};

/*
================================================
This class handles loading and rendering SWF files
================================================
*/
class hfSWF {
public:
	hfSWF( const char * filename, hfSoundWorld * soundWorld = NULL );
	~hfSWF();

	bool	IsLoaded() { return ( frameRate > 0 ); }
	bool	IsActive() { return isActive; }
	void	Activate( bool b );
	
	const char * GetName() { return filename; }

	void Pause() { mainspriteInstance->Stop(); paused = true; }
	void Resume() { mainspriteInstance->Play(); paused = false; }
	bool IsPaused() { return paused; }
	void SetPausedRender( bool valid ) { pausedRender = valid; }
	bool GetPausedRender() { return pausedRender; } 

	void Render( hfRenderSystem * gui, int time = 0, bool isSplitscreen = false );
	bool HandleEvent( const sysEvent_t * event );
	bool InhibitControl();
	void ForceInhibitControl( bool val ) { inhibitControl = val; }

	void SetGlobal( const char * name, const hfSWFScriptVar & value ) { globals->Set( name, value ); }
	void SetGlobalNative( const char * name, hfSWFScriptNativeVariable * native ) { globals->SetNative( name, native ); }
	hfSWFScriptVar GetGlobal( const char * name ) { return globals->Get( name ); }
	hfSWFScriptObject & GetRootObject() { assert( mainspriteInstance->GetScriptObject() != NULL ); return *( mainspriteInstance->GetScriptObject() ); }

	void Invoke( const char *  functionName, const hfSWFParmList & parms );
	void Invoke( const char *  functionName, const hfSWFParmList & parms, hfSWFScriptVar & scriptVar );
	void Invoke( const char *  functionName, const hfSWFParmList & parms, bool & functionExists );

	int PlaySound( const char * sound, int channel = SCHANNEL_ANY, bool blocking = false );
	void StopSound( int channel = SCHANNEL_ANY );

	float GetFrameWidth() const { return frameWidth; }
	float GetFrameHeight() const { return frameHeight; }

	int GetMouseX() { return mouseX; }
	int GetMouseY() { return mouseY; }

	bool UseCircleForAccept();

	void SetSWFScale( float scale ) { swfScale = scale; }

	void SetForceNonPCGetPlatform() { forceNonPCPlatform = true; }

	hfRandom2 & GetRandom() { return random; }

	int	GetPlatform();

	//----------------------------------
	// hfSWF_Dictionary.cpp
	//----------------------------------
	hfSWFDictionaryEntry *	AddDictionaryEntry( int characterID, swfDictType_t type );
	hfSWFDictionaryEntry *	FindDictionaryEntry( int characterID, swfDictType_t type );
	hfSWFDictionaryEntry *	FindDictionaryEntry( int characterID );

	hfSWFDictionaryEntry *	GetDictionaryEntry( int index ) { return &dictionary[ index ];  }
	int	GetNumDictionaryEntry() { return dictionary.Num(); }

	hfSWFScriptObject * HitTest( hfSWFSpriteInstance * spriteInstance, const swfRenderState_t & renderState, int x, int y, hfSWFScriptObject * parentObject );

private:
	hfStr			filename;
	HF_TIME_T		timestamp;

	float			frameWidth;
	float			frameHeight;
	uint16			frameRate;
	float			renderBorder;
	float			swfScale;

	hfVec2			scaleToVirtual;

	int				lastRenderTime;

	bool			isActive;
	bool			inhibitControl;
	bool			useInhibtControl;

	// certain screens need to be rendered when the pause menu is up so if this flag is
	// set on the gui we will allow it to render at a paused state;
	bool			pausedRender;	

	bool			mouseEnabled;
	bool			useMouse;

	bool			blackbars;
	bool			crop;
	bool			paused;
	bool			hasHitObject;

	bool			forceNonPCPlatform;

	hfRandom2		random;

	static int		mouseX;		// mouse x coord for all flash files
	static int		mouseY;		// mouse y coord for all flash files
	static bool		isMouseInClientArea;

	hfSWFScriptObject *	mouseObject;
	hfSWFScriptObject * hoverObject;

	hfSWFSprite *			mainsprite;
	hfSWFSpriteInstance *	mainspriteInstance;

	hfSWFScriptObject *		globals;
	hfSWFScriptObject *		shortcutKeys;

	hfSoundWorld *			soundWorld;

	const hfMaterial *		atlasMaterial;

	hfBlockAlloc< hfSWFSpriteInstance, 16 >	spriteInstanceAllocator;
	hfBlockAlloc< hfSWFTextInstance, 16 >	textInstanceAllocator;

#define SWF_NATIVE_FUNCTION_SWF_DECLARE( x ) \
	class hfSWFScriptFunction_##x : public hfSWFScriptFunction_Nested< hfSWF > { \
	public: \
		hfSWFScriptVar Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ); \
	} scriptFunction_##x;

	SWF_NATIVE_FUNCTION_SWF_DECLARE( shortcutKeys_clear );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( deactivate );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( inhibitControl );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( useInhibit );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( precacheSound );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( playSound );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( stopSounds );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( getPlatform );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( getTruePlatform );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( getLocalString );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( swapPS3Buttons );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( getCVarInteger );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( setCVarInteger );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( strReplace );

	SWF_NATIVE_FUNCTION_SWF_DECLARE( acos );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( cos );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( sin );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( round );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( pow );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( sqrt );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( abs );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( rand );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( floor );
	SWF_NATIVE_FUNCTION_SWF_DECLARE( ceil );

	SWF_NATIVE_FUNCTION_SWF_DECLARE( toUpper );

	SWF_NATIVE_VAR_DECLARE_NESTED_READONLY( platform, hfSWFScriptFunction_getPlatform, Call( object, hfSWFParmList() ) );
	SWF_NATIVE_VAR_DECLARE_NESTED( blackbars, hfSWF );
	SWF_NATIVE_VAR_DECLARE_NESTED( crop, hfSWF );

	class hfSWFScriptFunction_Object : public hfSWFScriptFunction {
	public:
		hfSWFScriptVar	Call( hfSWFScriptObject * thisObject, const hfSWFParmList & parms ) { return hfSWFScriptVar(); }
		void			AddRef() { }
		void			Release() { }
		hfSWFScriptObject * GetPrototype() { return &object; }
		void			SetPrototype( hfSWFScriptObject * _object ) { assert( false ); }
		hfSWFScriptObject object;
	} scriptFunction_Object;

	hfList< hfSWFDictionaryEntry, TAG_SWF >	dictionary;

	struct keyButtonImages_t {

		keyButtonImages_t() {
			key = "";
			xbImage = "";
			psImage = "";
			width = 0;
			height = 0; 
			baseline = 0;
		}

		keyButtonImages_t( const char* _key, const char* _xbImage, const char* _psImage, int w, int h, int _baseline ) {
			key = _key;
			xbImage = _xbImage;
			psImage = _psImage;
			width = w;
			height = h; 
			baseline = _baseline;
		}

		const char * key;
		const char * xbImage;
		const char * psImage;
		int width;
		int height;
		int baseline;
	};
	hfList< keyButtonImages_t, TAG_SWF > tooltipButtonImage;

	struct tooltipIcon_t {
		tooltipIcon_t() {
			startIndex = -1;
			endIndex = -1;
			material = NULL;
			imageWidth = 0;
			imageHeight = 0;
			baseline = 0;
		};

		int					startIndex;
		int					endIndex;
		const hfMaterial *	material;
		short				imageWidth;
		short				imageHeight;
		int					baseline;
	};
	hfList< tooltipIcon_t, TAG_SWF > tooltipIconList;

	const hfMaterial * guiSolid;
	const hfMaterial * guiCursor_arrow;
	const hfMaterial * guiCursor_hand;
	const hfMaterial * white;

private:
	friend class hfSWFSprite;
	friend class hfSWFSpriteInstance;

	bool			LoadSWF( const char * fullpath );
	bool			LoadBinary( const char * bfilename, HF_TIME_T sourceTime );
	void			WriteBinary( const char * bfilename );

	//----------------------------------
	// hfSWF_Shapes.cpp
	//----------------------------------
	void			DefineShape( hfSWFBitStream & bitstream );
	void			DefineShape2( hfSWFBitStream & bitstream );
	void			DefineShape3( hfSWFBitStream & bitstream );
	void			DefineShape4( hfSWFBitStream & bitstream );
	void			DefineMorphShape( hfSWFBitStream & bitstream );

	//----------------------------------
	// hfSWF_Sprites.cpp
	//----------------------------------
	void			DefineSprite( hfSWFBitStream & bitstream );

	//----------------------------------
	// hfSWF_Sounds.cpp
	//----------------------------------
	void			DefineSound( hfSWFBitStream & bitstream );

	//----------------------------------
	// hfSWF_Render.cpp
	//----------------------------------
	void			DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const hfMaterial *material );
	void			DrawStretchPic( const hfVec4 & topLeft, const hfVec4 & topRight, const hfVec4 & bottomRight, const hfVec4 & bottomLeft, const hfMaterial * material );
	void			RenderSprite( hfRenderSystem * gui, hfSWFSpriteInstance * sprite, const swfRenderState_t & renderState, int time, bool isSplitscreen = false );
	void			RenderMask( hfRenderSystem * gui, const swfDisplayEntry_t * mask, const swfRenderState_t & renderState, const int stencilMode );
	void			RenderShape( hfRenderSystem * gui, const hfSWFShape * shape, const swfRenderState_t & renderState );
	void			RenderMorphShape( hfRenderSystem * gui, const hfSWFShape* shape, const swfRenderState_t & renderState );
	void			DrawEditCursor( hfRenderSystem * gui, float x, float y, float w, float h, const swfMatrix_t & matrix );
	void			DrawLine( hfRenderSystem * gui, const hfVec2 & p1, const hfVec2 & p2, float width, const swfMatrix_t & matrix );
	void			RenderEditText( hfRenderSystem * gui, hfSWFTextInstance * textInstance, const swfRenderState_t & renderState, int time, bool isSplitscreen = false );
	uint64			GLStateForRenderState( const swfRenderState_t & renderState );
	void			FindTooltipIcons( hfStr * text );

	//----------------------------------
	// hfSWF_Image.cpp
	//----------------------------------

	class hfDecompressJPEG {
	public:
		hfDecompressJPEG();
		~hfDecompressJPEG();

		byte * Load( const byte * input, int inputSize, int & width, int & height );

	private:
		void * vinfo;
	};

	hfDecompressJPEG	jpeg;

	void			LoadImage( int characterID, const byte * imageData, int width, int height );

	void			JPEGTables( hfSWFBitStream & bitstream );
	void			DefineBits( hfSWFBitStream & bitstream );
	void			DefineBitsJPEG2( hfSWFBitStream & bitstream );
	void			DefineBitsJPEG3( hfSWFBitStream & bitstream );
	void			DefineBitsLossless( hfSWFBitStream & bitstream );
	void			DefineBitsLossless2( hfSWFBitStream & bitstream );


	// per-swf image atlas
	struct imageToPack_t {
		int	characterID;
		hfVec2i	trueSize;	// in texels
		byte * imageData;	// trueSize.x * trueSize.y * 4
		hfVec2i	allocSize;	// in DXT tiles, includes a border texel and rounding up to DXT blocks
	};

 	class hfSortBlocks : public hfSort_Quick< imageToPack_t, hfSortBlocks > {
 	public:
 		int Compare( const imageToPack_t & a, const imageToPack_t & b ) const {
 			return ( b.allocSize.x * b.allocSize.y ) - ( a.allocSize.x * a.allocSize.y );
 		}
 	};

	hfList<imageToPack_t, TAG_SWF>	packImages;	// only used during creation
	void			WriteSwfImageAtlas( const char *filename );

	//----------------------------------
	// hfSWF_Text.cpp
	//----------------------------------
	void			DefineFont2( hfSWFBitStream & bitstream );
	void			DefineFont3( hfSWFBitStream & bitstream );
	void			DefineTextX( hfSWFBitStream & bitstream, bool rgba );
	void			DefineText( hfSWFBitStream & bitstream );
	void			DefineText2( hfSWFBitStream & bitstream );
	void			DefineEditText( hfSWFBitStream & bitstream );

	//----------------------------------
	// hfSWF_Zlib.cpp
	//----------------------------------
	bool			Inflate( const byte * input, int inputSize, byte * output, int outputSize );

public:
	//----------------------------------
	// hfSWF_Names.cpp
	//----------------------------------
	static const char * GetTagName( swfTag_t tag );
	static const char * GetActionName( swfAction_t action );

};

#endif // !__SWF_H__

