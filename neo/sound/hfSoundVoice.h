#ifndef __SOUNDVOICE_H__
#define __SOUNDVOICE_H__

/*
================================================
hfSoundVoice_Base
================================================
*/

class hfSoundVoice_Base {
public:
				hfSoundVoice_Base();

	static void InitSurround( int outputChannels, int channelMask );

	void		CalculateSurround( int srcChannels, float pLevelMatrix[ MAX_CHANNELS_PER_VOICE * MAX_CHANNELS_PER_VOICE ], float scale );

	void		SetPosition( const hfVec3 & p ) { position = p; }
	void		SetGain( float g ) { gain = g; }
	void		SetCenterChannel( float c ) { centerChannel = c; }
	void		SetPitch( float p ) { pitch = p; }
	void		SetInnerRadius( float r ) { innerRadius = r; }
	void		SetChannelMask( uint32 mask ) { channelMask = mask; }

	const hfSoundSample * GetCurrentSample();

	// Controls the low pass filter, where 0.0f = no filtering, 1.0f = full filter
	void		SetOcclusion( float f ) { occlusion = f; }

	float		GetGain() { return gain; }
	float		GetPitch() { return pitch; }

protected:
	hfVec3		position;			// Position of the sound relative to listener
	float		gain;				// Volume (0-1)
	float		centerChannel;		// Value (0-1) which indicates how much of this voice goes to the center channel
	float		pitch;				// Pitch multiplier
	float		innerRadius;		// Anything closer than this is omni
	float		occlusion;			// How much of this sound is occluded (0-1)
	uint32		channelMask;		// Set to override the default channel mask

	// These are some setting used to do SSF_DISTANCE_BASED_STERO blending
	float		innerSampleRangeSqr;
	float		outerSampleRangeSqr;

	hfList< hfSoundSample *, TAG_AUDIO> samples;

	// These are constants which are initialized with InitSurround
	//-------------------------------------------------------------

	static hfVec2 speakerPositions[hfWaveFile::CHANNEL_INDEX_MAX];

	// This is to figure out which speakers are "next to" this one
	static int speakerLeft[hfWaveFile::CHANNEL_INDEX_MAX];
	static int speakerRight[hfWaveFile::CHANNEL_INDEX_MAX];

	// Number of channels in the output hardware
	static int dstChannels;

	// Mask indicating which speakers exist in the hardware configuration
	static int dstMask;

	// dstMap maps a destination channel to a speaker
	// invMap maps a speaker to a destination channel
	static int dstCenter;
	static int dstLFE;
	static int dstMap[MAX_CHANNELS_PER_VOICE];
	static int invMap[hfWaveFile::CHANNEL_INDEX_MAX];

	// specifies what volume to specify for each channel when a speaker is omni
	static float omniLevel;
};

#endif

