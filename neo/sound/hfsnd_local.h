#ifndef __SND_LOCAL_H__
#define __SND_LOCAL_H__

#include "hfWaveFile.h"

// Maximum number of voices we can have allocated
#define MAX_HARDWARE_VOICES 48

// A single voice can play multiple channels (up to 5.1, but most commonly stereo)
// This is the maximum number of channels which can play simultaneously
// This is limited primarily by seeking on the optical drive, secondarily by memory consumption, and tertiarily by CPU time spent mixing
#define MAX_HARDWARE_CHANNELS 64

// We may need up to 3 buffers for each hardware voice if they are all long sounds
#define MAX_SOUND_BUFFERS ( MAX_HARDWARE_VOICES * 3 )

// Maximum number of channels in a sound sample
#define MAX_CHANNELS_PER_VOICE	8

/*
========================
MsecToSamples
SamplesToMsec
========================
*/
HF_INLINE_EXTERN uint32 MsecToSamples( uint32 msec, uint32 sampleRate ) { return ( msec * ( sampleRate / 100 ) ) / 10; }
HF_INLINE_EXTERN uint32 SamplesToMsec( uint32 samples, uint32 sampleRate ) { return sampleRate < 100 ? 0 : ( samples * 10 ) / ( sampleRate / 100 ); }

/*
========================
DBtoLinear
LinearToDB
========================
*/
HF_INLINE_EXTERN float DBtoLinear( float db ) { return hfMath::Pow( 2.0f, db * ( 1.0f / 6.0f ) ); }
HF_INLINE_EXTERN float LinearToDB( float linear ) { return ( linear > 0.0f ) ? ( hfMath::Log( linear ) * ( 6.0f / 0.693147181f ) ) : -999.0f; }

// demo sound commands
typedef enum {
	SCMD_STATE,				// followed by a load game state
	SCMD_PLACE_LISTENER,
	SCMD_ALLOC_EMITTER,

	SCMD_FREE,
	SCMD_UPDATE,
	SCMD_START,
	SCMD_MODIFY,
	SCMD_STOP,
	SCMD_FADE
} soundDemoCommand_t;

#include "hfSoundVoice.h"


#define OPERATION_SET 1

#if defined(_WIN32)

#define HF_USE_XAUDIO2 1

#else

#define HF_USE_NULL_AUDIO 1

#endif

#ifdef HF_USE_XAUDIO2
#include <dxsdkver.h>
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <X3DAudio.h>
#include <xma2defs.h>
#include "XAudio2/hfXA2_SoundSample.h"
#include "XAudio2/hfXA2_SoundVoice.h"
#include "XAudio2/hfXA2_SoundHardware.h"
#endif

#ifdef HF_USE_NULL_AUDIO
#include "Null/hfNull_SoundSample.h"
#include "Null/hfNull_SoundVoice.h"
#include "Null/hfNull_SoundHardware.h"
#endif



//------------------------
// Listener data
//------------------------
struct listener_t {
	hfMat3	axis;		// orientation of the listener
	hfVec3	pos;		// position in meters
	int		id;			// the entity number, used to detect when a sound is local
	int		area;		// area number the listener is in
};

class hfSoundFade {
public:
	int		fadeStartTime;
	int		fadeEndTime;
	float	fadeStartVolume;
	float	fadeEndVolume;


public:
	hfSoundFade() { Clear(); }

	void	Clear();
	void	SetVolume( float to );
	void	Fade( float to, int length, int soundTime );

	float	GetVolume( int soundTime ) const;
};

/*
================================================
hfSoundChannel
================================================
*/
class hfSoundChannel {
public:
	bool	CanMute() const;

	void	Mute();
	bool	CheckForCompletion( int currentTime );

	void	UpdateVolume( int currentTime );
	void	UpdateHardware( float volumeAdd, int currentTime );

	// returns true if this channel is marked as looping
	bool	IsLooping() const;

	class hfSoundEmitterLocal *	emitter;

	int						startTime;
	int						endTime;
	int						logicalChannel;
	bool					allowSlow;

	soundShaderParms_t		parms;				// combines shader parms and per-channel overrides
	const hfSoundShader *	soundShader;
	hfSoundSample *			leadinSample;
	hfSoundSample *			loopingSample;
	hfSoundFade				volumeFade;

	float					volumeDB;			// last volume at which this channel will play (calculated in UpdateVolume)
	float					currentAmplitude;	// current amplitude on the hardware voice

	// hardwareVoice will be freed and NULL'd when a sound is out of range, 
	// and reallocated when it comes back in range
	hfSoundVoice *			hardwareVoice;

	// only allocated by the soundWorld block allocator
	hfSoundChannel();
	~hfSoundChannel();
};

// Maximum number of SoundChannels for a single SoundEmitter.
// This is probably excessive...
const int MAX_CHANNELS_PER_EMITTER = 16;

/*
===================================================================================

hfSoundWorldLocal

===================================================================================
*/

class hfSoundWorldLocal : public hfSoundWorld {
public:
							hfSoundWorldLocal();
	virtual					~hfSoundWorldLocal();

	//------------------------
	// Functions from hfSoundWorld, implemented in SoundWorld.cpp
	//------------------------

	// Called at map start
	virtual void			ClearAllSoundEmitters();

	// stop all playing sounds
	virtual void			StopAllSounds();

	// get a new emitter that can play sounds in this world
	virtual hfSoundEmitter *AllocSoundEmitter();

	// for load games
	virtual hfSoundEmitter *EmitterForIndex( int index );

	// query data from all emitters in the world
	virtual float			CurrentShakeAmplitude();

	// where is the camera
	virtual void			PlaceListener( const hfVec3 &origin, const hfMat3 &axis, const int listenerId );

	// fade all sounds in the world with a given shader soundClass
	// to is in Db, over is in seconds
	virtual void			FadeSoundClasses( const int soundClass, const float to, const float over );

	// dumps the current state and begins archiving commands
	virtual void			StartWritingDemo( hfDemoFile *demo );
	virtual void			StopWritingDemo();

	// read a sound command from a demo file
	virtual void			ProcessDemoCommand( hfDemoFile *readDemo );

	// menu sounds
	virtual int				PlayShaderDirectly( const char *name, int channel = -1 );

	virtual void			Skip( int time );

	virtual void			Pause();
	virtual void			UnPause();
	virtual bool			IsPaused() { return isPaused; }

	virtual int				GetSoundTime();

	// avidump
	virtual void			AVIOpen( const char *path, const char *name );
	virtual void			AVIClose();

	// SaveGame Support
	virtual void			WriteToSaveGame( hfFile *savefile );
	virtual void			ReadFromSaveGame( hfFile *savefile );

	virtual void			SetSlowmoSpeed( float speed );
	virtual void			SetEnviroSuit( bool active );

	//=======================================

	//------------------------
	// Random stuff that's not exposed outside the sound system
	//------------------------
	void			Update();
	void			OnReloadSound( const hfDecl *decl );

	hfSoundChannel *	AllocSoundChannel();
	void				FreeSoundChannel( hfSoundChannel * );

public:
	// even though all these variables are public, nobody outside the sound system includes SoundWorld_local.h
	// so this is equivalent to making it private and friending all the other classes in the sound system

	hfSoundFade			volumeFade;						// master volume knob for the entire world
	hfSoundFade			soundClassFade[SOUND_MAX_CLASSES];

	hfRenderWorld *		renderWorld;	// for debug visualization and light amplitude sampling
	hfDemoFile *		writeDemo;		// if not NULL, archive commands here

	float				currentCushionDB;	// channels at or below this level will be faded to 0
	float				shakeAmp;			// last calculated shake amplitude

	listener_t			listener;
	hfList<hfSoundEmitterLocal *, TAG_AUDIO>	emitters;

	hfSoundEmitter *	localSound;			// for PlayShaderDirectly()

	hfBlockAlloc<hfSoundEmitterLocal, 16>	emitterAllocator;
	hfBlockAlloc<hfSoundChannel, 16>		channelAllocator;

	hfSoundFade				pauseFade;
	int						pausedTime;
	int						accumulatedPauseTime;
	bool					isPaused;

	float					slowmoSpeed;
	bool					enviroSuitActive;

public: 
	struct soundPortalTrace_t {
		int		portalArea;
		const soundPortalTrace_t * prevStack;
	};

	void			ResolveOrigin( const int stackDepth, const soundPortalTrace_t * prevStack, const int soundArea, const float dist, const hfVec3 & soundOrigin, hfSoundEmitterLocal * def );
};


/*
================================================
hfSoundEmitterLocal 
================================================
*/
class hfSoundEmitterLocal : public hfSoundEmitter {
public:
	virtual void	Free( bool immediate );

	virtual void	Reset();

	virtual void	UpdateEmitter( const hfVec3 &origin, int listenerId, const soundShaderParms_t *parms );

	virtual int		StartSound( const hfSoundShader *shader, const s_channelType channel, float diversity = 0, int shaderFlags = 0, bool allowSlow = true );

	virtual void	ModifySound( const s_channelType channel, const soundShaderParms_t *parms );
	virtual void	StopSound( const s_channelType channel );

	virtual void	FadeSound( const s_channelType channel, float to, float over );

	virtual bool	CurrentlyPlaying( const s_channelType channel = SCHANNEL_ANY ) const;

	virtual	float	CurrentAmplitude();

	virtual	int		Index() const;

	//----------------------------------------------

	void			Init( int i, hfSoundWorldLocal * sw );

	// Returns true if the emitter should be freed.
	bool			CheckForCompletion( int currentTime );

	void			OverrideParms( const soundShaderParms_t * base, const soundShaderParms_t * over, soundShaderParms_t * out );

	void			Update( int currentTime );
	void			OnReloadSound( const hfDecl *decl );

	//----------------------------------------------

	hfSoundWorldLocal *		soundWorld;						// the world that holds this emitter

	int			index;							// in world emitter list
	bool		canFree;						// if true, this emitter can be canFree (once channels.Num() == 0)

	// a single soundEmitter can have many channels playing from the same point
	hfStaticList<hfSoundChannel *, MAX_CHANNELS_PER_EMITTER> channels;

	//----- set by UpdateEmitter -----
	hfVec3				origin;
	soundShaderParms_t	parms;
	int					emitterId;						// sounds will be full volume when emitterId == listenerId

	//----- set by Update -----
	int			lastValidPortalArea;
	float		directDistance;
	float		spatializedDistance;
	hfVec3		spatializedOrigin;

	// sound emitters are only allocated by the soundWorld block allocator
					hfSoundEmitterLocal();
	virtual			~hfSoundEmitterLocal();
};


/*
===================================================================================

hfSoundSystemLocal

===================================================================================
*/
class hfSoundSystemLocal : public hfSoundSystem {
public:
	// all non-hardware initialization
	virtual void			Init();

	// shutdown routine
	virtual	void			Shutdown();

	virtual hfSoundWorld *	AllocSoundWorld( hfRenderWorld *rw );
	virtual void			FreeSoundWorld( hfSoundWorld *sw );

	// specifying NULL will cause silence to be played
	virtual void			SetPlayingSoundWorld( hfSoundWorld *soundWorld );

	// some tools, like the sound dialog, may be used in both the game and the editor
	// This can return NULL, so check!
	virtual hfSoundWorld *	GetPlayingSoundWorld();

	// sends the current playing sound world information to the sound hardware
	virtual void			Render();

	// Mutes the SSG_MUSIC group
	virtual void			MuteBackgroundMusic( bool mute ) { musicMuted = mute; }

	// sets the final output volume to 0
	// This should only be used when the app is deactivated
	// Since otherwise there will be problems with different subsystems muting and unmuting at different times
	virtual void			SetMute( bool mute ) { muted = mute; }
	virtual bool			IsMuted() { return muted; }

	virtual void			OnReloadSound( const hfDecl * sound );

	virtual void			StopAllSounds();

	virtual void			InitStreamBuffers();
	virtual void			FreeStreamBuffers();

	// video playback needs to get this
	virtual void *			GetIXAudio2() const;

	// for the sound level meter window
	virtual cinData_t		ImageForTime( const int milliseconds, const bool waveform );

	// Free all sounds loaded during the last map load
	virtual	void			BeginLevelLoad();

	// We might want to defer the loading of new sounds to this point
	virtual	void			EndLevelLoad();

	// prints memory info
	virtual void			PrintMemInfo( MemInfo_t *mi );

	//-------------------------

	// Before a sound is reloaded, any active voices using it must
	// be stopped.  Returns true if any were playing, and should be
	// restarted after the sound is reloaded.
	void					StopVoicesWithSample( const hfSoundSample * const sample );

	void					Restart();
	void					SetNeedsRestart() { needsRestart = true; }

	int						SoundTime() const;

	// may return NULL if there are no more voices left
	hfSoundVoice *			AllocateVoice( const hfSoundSample * leadinSample, const hfSoundSample * loopingSample );
	void					FreeVoice( hfSoundVoice * );

	hfSoundSample *			LoadSample( const char * name );

	virtual void			Preload( hfPreloadManifest & preload );

	struct bufferContext_t {
		bufferContext_t() :
			voice( NULL ),
			sample( NULL ),
			bufferNumber( 0 )
		{ }
		hfSoundVoice *	voice;
		hfSoundSample * sample;
		int bufferNumber;
	};

	// Get a stream buffer from the free pool, returns NULL if none are available
	bufferContext_t *			ObtainStreamBufferContext();
	void						ReleaseStreamBufferContext( bufferContext_t * p );

	hfSysMutex					streamBufferMutex;
	hfStaticList< bufferContext_t *, MAX_SOUND_BUFFERS > freeStreamBufferContexts;
	hfStaticList< bufferContext_t *, MAX_SOUND_BUFFERS > activeStreamBufferContexts;
	hfStaticList< bufferContext_t, MAX_SOUND_BUFFERS > bufferContexts;

	hfSoundWorldLocal *			currentSoundWorld;
	hfStaticList<hfSoundWorldLocal *, 32>	soundWorlds;

	hfList<hfSoundSample *, TAG_AUDIO>		samples;
	hfHashIndex					sampleHash;

	hfSoundHardware				hardware;

	hfRandom2					random;
	
	int							soundTime;
	bool						muted;
	bool						musicMuted;
	bool						needsRestart;

	bool						insideLevelLoad;

	//-------------------------

	hfSoundSystemLocal() :
		soundTime( 0 ),
		currentSoundWorld( NULL ),
		muted( false ),
		musicMuted( false ),
		needsRestart( false )
		{}
};

extern	hfSoundSystemLocal	soundSystemLocal;

#endif /* !__SND_LOCAL_H__ */

