/*
===========================================================================

        Hellforge Engine - Cross-platform game engine
        Powered by Hellforge Power 1
        Copyright (C) 1993-2026

        Null sound voice - portable placeholder.
        No audio output; the mix/update logic is retained for future
        backends and amplitude tracking.

===========================================================================
*/

#ifndef __NULL_SOUNDVOICE_H__
#define __NULL_SOUNDVOICE_H__

class hfSoundVoice_Null : public hfSoundVoice_Base {
public:
				hfSoundVoice_Null();

	void		Start( const hfSoundSample * leadinSample, const hfSoundSample * loopingSample );
	void		Start( int offsetMS, int ssFlags );
	void		Stop();
	void		Pause();
	void		UnPause();
	bool		IsPlaying() const;

	// updates the voice for a frame
	void		Update();

	// returns the current amplitude used for vu meter / game feedback
	float		GetAmplitude() const;

	// returns the number of samples that have been played
	int			GetPlayedSampleCount() const;

	// updates the current amplitude for the vu meter / game feedback
	float		GetCurrentAmplitude() const;

protected:
	friend class hfSoundHardware_Null;
	virtual		~hfSoundVoice_Null();

private:
	bool		playing;
	float		currentAmplitude;
	bool		leadinSampleDone;
};

/*
================================================
hfSoundVoice
================================================
*/
class hfSoundVoice : public hfSoundVoice_Null {
public:
};

#endif