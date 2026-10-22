/*
===========================================================================

        Hellforge Engine - Cross-platform game engine
        Powered by Hellforge Power 1
        Copyright (C) 1993-2026

        Null sound hardware - portable cross-platform placeholder.
        Implements the hfSoundHardware interface with stubbed audio
        output. No driver dependencies (no DirectX/ALSA/PulseAudio).

===========================================================================
*/

#ifndef __NULL_SOUNDHARDWARE_H__
#define __NULL_SOUNDHARDWARE_H__

class hfSoundSample_Null;
class hfSoundVoice_Null;

/*
================================================
hfSoundHardware_Null
================================================
*/

class hfSoundHardware_Null {
public:
					hfSoundHardware_Null();

	void			Init();
	void			Shutdown();

	void			Update();

	hfSoundVoice *	AllocateVoice( const hfSoundSample * leadinSample, const hfSoundSample * loopingSample );
	void			FreeVoice( hfSoundVoice * voice );

	// video playback placeholder - returns NULL
	void *			GetAudioDevice() const { return NULL; }

	int				GetNumZombieVoices() const { return zombieVoices.Num(); }
	int				GetNumFreeVoices() const { return freeVoices.Num(); }

protected:
					~hfSoundHardware_Null();

private:
	hfList<hfSoundVoice_Null *, TAG_AUDIO>	zombieVoices;
	hfList<hfSoundVoice_Null *, TAG_AUDIO>	freeVoices;
};

/*
================================================
hfSoundHardware
================================================
*/
class hfSoundHardware : public hfSoundHardware_Null {
public:
};

#endif