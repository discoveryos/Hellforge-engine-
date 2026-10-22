#ifndef __XA2_SOUNDSAMPLE_H__
#define __XA2_SOUNDSAMPLE_H__

/*
================================================
hfSoundSample_XAudio2
================================================
*/
class hfSampleInfo;
class hfSoundSample_XAudio2 {
public:
					hfSoundSample_XAudio2();

	// Loads and initializes the resource based on the name.
	virtual void	 LoadResource();

	void			SetName( const char * n ) { name = n; }
	const char *	GetName() const { return name; }
	HF_TIME_T		GetTimestamp() const { return timestamp; }

	// turns it into a beep
	void			MakeDefault();

	// frees all data
	void			FreeData();

	int				LengthInMsec() const { return SamplesToMsec( NumSamples(), SampleRate() ); }
	int				SampleRate() const { return format.basic.samplesPerSec; }
	int				NumSamples() const { return playLength; }
	int				NumChannels() const { return format.basic.numChannels; }
	int				BufferSize() const { return totalBufferSize; }

	bool			IsCompressed() const { return ( format.basic.formatTag != hfWaveFile::FORMAT_PCM ); }

	bool			IsDefault() const { return timestamp == FILE_NOT_FOUND_TIMESTAMP; }
	bool			IsLoaded() const { return loaded; }

	void			SetNeverPurge() { neverPurge = true; }
	bool			GetNeverPurge() const { return neverPurge; }

	void			SetLevelLoadReferenced() { levelLoadReferenced = true; }
	void			ResetLevelLoadReferenced() { levelLoadReferenced = false; }
	bool			GetLevelLoadReferenced() const { return levelLoadReferenced; }

	int				GetLastPlayedTime() const { return lastPlayedTime; }
	void			SetLastPlayedTime( int t ) { lastPlayedTime = t; }

	float			GetAmplitude( int timeMS ) const;

protected:
	friend class hfSoundHardware_XAudio2;
	friend class hfSoundVoice_XAudio2;

					~hfSoundSample_XAudio2();

	bool			LoadWav( const hfStr & name );
	bool			LoadAmplitude( const hfStr & name );
	void			WriteAllSamples( const hfStr &sampleName );
	bool			LoadGeneratedSample( const hfStr &name );
	void			WriteGeneratedSample( hfFile *fileOut );

	struct sampleBuffer_t {
		void * buffer;
		int bufferSize;
		int numSamples;
	};

	hfStr			name;

	HF_TIME_T		timestamp;
	bool			loaded;

	bool			neverPurge;
	bool			levelLoadReferenced;
	bool			usesMapHeap;

	uint32			lastPlayedTime;

	int				totalBufferSize;	// total size of all the buffers
	hfList<sampleBuffer_t, TAG_AUDIO> buffers;

	int				playBegin;
	int				playLength;

	hfWaveFile::waveFmt_t	format;

	hfList<byte, TAG_AMPLITUDE> amplitude;
};

/*
================================================
hfSoundSample

This reverse-inheritance purportedly makes working on
multiple platforms easier.
================================================
*/
class hfSoundSample : public hfSoundSample_XAudio2 {
public:
};

#endif

