#ifndef __XA_SOUNDHARDWARE_H__
#define __XA_SOUNDHARDWARE_H__

class hfSoundSample_XAudio2;
class hfSoundVoice_XAudio2;

/*
================================================
hfSoundEngineCallback
================================================
*/
class hfSoundEngineCallback : public IXAudio2EngineCallback {
public:
	hfSoundHardware_XAudio2 * hardware;

private:
	    // Called by XAudio2 just before an audio processing pass begins.
    STDMETHOD_( void, OnProcessingPassStart ) ( THIS ) {}

    // Called just after an audio processing pass ends.
    STDMETHOD_( void, OnProcessingPassEnd ) ( THIS ) {}

    // Called in the event of a critical system error which requires XAudio2
    // to be closed down and restarted.  The error code is given in Error.
    STDMETHOD_( void, OnCriticalError ) ( THIS_ HRESULT Error );
};

/*
================================================
hfSoundHardware_XAudio2
================================================
*/

class hfSoundHardware_XAudio2 {
public:
					hfSoundHardware_XAudio2();

	void			Init();
	void			Shutdown();

	void 			Update();

	hfSoundVoice *	AllocateVoice( const hfSoundSample * leadinSample, const hfSoundSample * loopingSample );
	void			FreeVoice( hfSoundVoice * voice );

	// video playback needs this
	IXAudio2 *		GetIXAudio2() const { return pXAudio2; };

	int				GetNumZombieVoices() const { return zombieVoices.Num(); }
	int				GetNumFreeVoices() const { return freeVoices.Num(); }

protected:
	friend class hfSoundSample_XAudio2;
	friend class hfSoundVoice_XAudio2;

private:
	IXAudio2 * pXAudio2;
	IXAudio2MasteringVoice * pMasterVoice;
	IXAudio2SubmixVoice * pSubmixVoice;

	hfSoundEngineCallback	soundEngineCallback;
	int					lastResetTime;

	int					outputChannels;
	int					channelMask;

	hfDebugGraph *		vuMeterRMS;
	hfDebugGraph *		vuMeterPeak;
	int					vuMeterPeakTimes[ 8 ];

	// Can't stop and start a voice on the same frame, so we have to double this to handle the worst case scenario of stopping all voices and starting a full new set
	hfStaticList<hfSoundVoice_XAudio2, MAX_HARDWARE_VOICES * 2 > voices;
	hfStaticList<hfSoundVoice_XAudio2 *, MAX_HARDWARE_VOICES * 2 > zombieVoices;
	hfStaticList<hfSoundVoice_XAudio2 *, MAX_HARDWARE_VOICES * 2 > freeVoices;
};

/*
================================================
hfSoundHardware
================================================
*/
class hfSoundHardware : public hfSoundHardware_XAudio2 {
};

#endif

