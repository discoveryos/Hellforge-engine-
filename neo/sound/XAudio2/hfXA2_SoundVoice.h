#ifndef __XA2_SOUNDVOICE_H__
#define __XA2_SOUNDVOICE_H__

static const int MAX_QUEUED_BUFFERS = 3;

/*
================================================
hfSoundVoice_XAudio2
================================================
*/
class hfSoundVoice_XAudio2 : public hfSoundVoice_Base {
public:
							hfSoundVoice_XAudio2();
							~hfSoundVoice_XAudio2();

	void					Create( const hfSoundSample * leadinSample, const hfSoundSample * loopingSample );

	// Start playing at a particular point in the buffer.  Does an Update() too
	void					Start( int offsetMS, int ssFlags );

	// Stop playing.
	void					Stop();

	// Stop consuming buffers
	void					Pause();
	// Start consuming buffers again
	void					UnPause();

	// Sends new position/volume/pitch information to the hardware
	bool					Update();

	// returns the RMS levels of the most recently processed block of audio, SSF_FLICKER must have been passed to Start
	float					GetAmplitude();

	// returns true if we can re-use this voice
	bool					CompatibleFormat( hfSoundSample_XAudio2 * s );

	uint32					GetSampleRate() const { return sampleRate; }

	// callback function
	void					OnBufferStart( hfSoundSample_XAudio2 * sample, int bufferNumber );

private:
	friend class hfSoundHardware_XAudio2;

	// Returns true when all the buffers are finished processing
	bool					IsPlaying();

	// Called after the voice has been stopped
	void					FlushSourceBuffers();

	// Destroy the internal hardware resource
	void					DestroyInternal();

	// Helper function used by the initial start as well as for looping a streamed buffer
	int						RestartAt( int offsetSamples );

	// Helper function to submit a buffer
	int						SubmitBuffer( hfSoundSample_XAudio2 * sample, int bufferNumber, int offset );

	// Adjust the voice frequency based on the new sample rate for the buffer
	void					SetSampleRate( uint32 newSampleRate, uint32 operationSet );

	IXAudio2SourceVoice *	pSourceVoice;
	hfSoundSample_XAudio2 * leadinSample;
	hfSoundSample_XAudio2 * loopingSample;

	// These are the fields from the sample format that matter to us for voice reuse
	uint16					formatTag;
	uint16					numChannels;

	uint32					sourceVoiceRate;
	uint32					sampleRate;

	bool					hasVUMeter;
	bool					paused;
};

/*
================================================
hfSoundVoice
================================================
*/
class hfSoundVoice : public hfSoundVoice_XAudio2 {
};

#endif

