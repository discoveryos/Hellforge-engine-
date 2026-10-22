#pragma hdrstop
#include "../hflib/hfprecompiled.h"

#include "hfsnd_local.h"

hfCVar s_subFraction( "s_subFraction", "0.5", CVAR_ARCHIVE | CVAR_FLOAT, "Amount of each sound to send to the LFE channel" );

hfVec2 hfSoundVoice_Base::speakerPositions[hfWaveFile::CHANNEL_INDEX_MAX];
int hfSoundVoice_Base::speakerLeft[hfWaveFile::CHANNEL_INDEX_MAX] = {0 };
int hfSoundVoice_Base::speakerRight[hfWaveFile::CHANNEL_INDEX_MAX] = {0 };
int hfSoundVoice_Base::dstChannels = 0;
int hfSoundVoice_Base::dstMask = 0;
int hfSoundVoice_Base::dstCenter = -1;
int hfSoundVoice_Base::dstLFE = -1;
int hfSoundVoice_Base::dstMap[MAX_CHANNELS_PER_VOICE] = { 0 };
int hfSoundVoice_Base::invMap[hfWaveFile::CHANNEL_INDEX_MAX] = { 0 };
float hfSoundVoice_Base::omniLevel = 1.0f;

/*
========================
hfSoundVoice_Base::hfSoundVoice_Base
========================
*/
hfSoundVoice_Base::hfSoundVoice_Base() :
position( 0.0f ),
gain( 1.0f ),
centerChannel( 0.0f ),
pitch( 1.0f ),
innerRadius( 32.0f ),
occlusion( 0.0f ),
channelMask( 0 ),
innerSampleRangeSqr( 0.0f ),
outerSampleRangeSqr( 0.0f )
{
}

/*
========================
hfSoundVoice_Base::InitSurround
========================
*/
void hfSoundVoice_Base::InitSurround( int outputChannels, int channelMask ) {

	speakerPositions[hfWaveFile::CHANNEL_INDEX_FRONT_LEFT			].Set(  0.70710678118654752440084436210485f,  0.70710678118654752440084436210485f );	// 45 degrees
	speakerPositions[hfWaveFile::CHANNEL_INDEX_FRONT_RIGHT			].Set(  0.70710678118654752440084436210485f, -0.70710678118654752440084436210485f );	// 315 degrees
	speakerPositions[hfWaveFile::CHANNEL_INDEX_FRONT_CENTER			].Set(  0.0f,								  0.0f );									// 0 degrees
	speakerPositions[hfWaveFile::CHANNEL_INDEX_LOW_FREQUENCY		].Set(  0.0f,								  0.0f );									// -
	speakerPositions[hfWaveFile::CHANNEL_INDEX_BACK_LEFT			].Set( -0.70710678118654752440084436210485f,  0.70710678118654752440084436210485f );	// 135 degrees
	speakerPositions[hfWaveFile::CHANNEL_INDEX_BACK_RIGHT			].Set( -0.70710678118654752440084436210485f, -0.70710678118654752440084436210485f );	// 225 degrees
	speakerPositions[hfWaveFile::CHANNEL_INDEX_FRONT_LEFT_CENTER	].Set(  0.92387953251128675612818318939679f,  0.3826834323650897717284599840304f );		// 22.5 degrees
	speakerPositions[hfWaveFile::CHANNEL_INDEX_FRONT_RIGHT_CENTER	].Set(  0.92387953251128675612818318939679f, -0.3826834323650897717284599840304f );		// 337.5 degrees
	speakerPositions[hfWaveFile::CHANNEL_INDEX_BACK_CENTER			].Set( -1.0f,								  0.0f );									// 180 degrees
	speakerPositions[hfWaveFile::CHANNEL_INDEX_SIDE_LEFT			].Set(  0.0f,								  1.0f );									// 90 degrees
	speakerPositions[hfWaveFile::CHANNEL_INDEX_SIDE_RIGHT			].Set( 0.0f,								 -1.0f );									// 270 degrees

	speakerLeft[hfWaveFile::CHANNEL_INDEX_FRONT_LEFT_CENTER] = hfWaveFile::CHANNEL_INDEX_FRONT_LEFT;
	speakerLeft[hfWaveFile::CHANNEL_INDEX_FRONT_LEFT] = hfWaveFile::CHANNEL_INDEX_SIDE_LEFT;
	speakerLeft[hfWaveFile::CHANNEL_INDEX_SIDE_LEFT] = hfWaveFile::CHANNEL_INDEX_BACK_LEFT;
	speakerLeft[hfWaveFile::CHANNEL_INDEX_BACK_LEFT] = hfWaveFile::CHANNEL_INDEX_BACK_CENTER;
	speakerLeft[hfWaveFile::CHANNEL_INDEX_BACK_CENTER] = hfWaveFile::CHANNEL_INDEX_BACK_RIGHT;
	speakerLeft[hfWaveFile::CHANNEL_INDEX_BACK_RIGHT] = hfWaveFile::CHANNEL_INDEX_SIDE_RIGHT;
	speakerLeft[hfWaveFile::CHANNEL_INDEX_SIDE_RIGHT] = hfWaveFile::CHANNEL_INDEX_FRONT_RIGHT;
	speakerLeft[hfWaveFile::CHANNEL_INDEX_FRONT_RIGHT] = hfWaveFile::CHANNEL_INDEX_FRONT_RIGHT_CENTER;
	speakerLeft[hfWaveFile::CHANNEL_INDEX_FRONT_RIGHT_CENTER] = hfWaveFile::CHANNEL_INDEX_FRONT_LEFT_CENTER;

	speakerLeft[hfWaveFile::CHANNEL_INDEX_FRONT_CENTER] = hfWaveFile::CHANNEL_INDEX_FRONT_CENTER;
	speakerLeft[hfWaveFile::CHANNEL_INDEX_LOW_FREQUENCY] = hfWaveFile::CHANNEL_INDEX_LOW_FREQUENCY;

	speakerRight[hfWaveFile::CHANNEL_INDEX_FRONT_RIGHT_CENTER] = hfWaveFile::CHANNEL_INDEX_FRONT_RIGHT;
	speakerRight[hfWaveFile::CHANNEL_INDEX_FRONT_RIGHT] = hfWaveFile::CHANNEL_INDEX_SIDE_RIGHT;
	speakerRight[hfWaveFile::CHANNEL_INDEX_SIDE_RIGHT] = hfWaveFile::CHANNEL_INDEX_BACK_RIGHT;
	speakerRight[hfWaveFile::CHANNEL_INDEX_BACK_RIGHT] = hfWaveFile::CHANNEL_INDEX_BACK_CENTER;
	speakerRight[hfWaveFile::CHANNEL_INDEX_BACK_CENTER] = hfWaveFile::CHANNEL_INDEX_BACK_LEFT;
	speakerRight[hfWaveFile::CHANNEL_INDEX_BACK_LEFT] = hfWaveFile::CHANNEL_INDEX_SIDE_LEFT;
	speakerRight[hfWaveFile::CHANNEL_INDEX_SIDE_LEFT] = hfWaveFile::CHANNEL_INDEX_FRONT_LEFT;
	speakerRight[hfWaveFile::CHANNEL_INDEX_FRONT_LEFT] = hfWaveFile::CHANNEL_INDEX_FRONT_LEFT_CENTER;
	speakerRight[hfWaveFile::CHANNEL_INDEX_FRONT_LEFT_CENTER] = hfWaveFile::CHANNEL_INDEX_FRONT_RIGHT_CENTER;

	speakerRight[hfWaveFile::CHANNEL_INDEX_FRONT_CENTER] = hfWaveFile::CHANNEL_INDEX_FRONT_CENTER;
	speakerRight[hfWaveFile::CHANNEL_INDEX_LOW_FREQUENCY] = hfWaveFile::CHANNEL_INDEX_LOW_FREQUENCY;

	dstChannels = outputChannels;
	dstMask = channelMask;

	// dstMap maps a destination channel to a speaker
	// invMap maps a speaker to a destination channel
	dstLFE = -1;
	dstCenter = -1;
	memset( dstMap, 0, sizeof( dstMap ) );
	memset( invMap, 0, sizeof( invMap ) );
	for ( int i = 0, c = 0; i < hfWaveFile::CHANNEL_INDEX_MAX && c < MAX_CHANNELS_PER_VOICE; i++ ) {
		if ( dstMask & BIT(i) ) {
			if ( i == hfWaveFile::CHANNEL_INDEX_LOW_FREQUENCY ) {
				dstLFE = c;
			}
			if ( i == hfWaveFile::CHANNEL_INDEX_FRONT_CENTER ) {
				dstCenter = c;
			}
			dstMap[c] = i;
			invMap[i] = c++;
		} else {
			// Remove this speaker from the chain
			int right = speakerRight[i];
			int left = speakerLeft[i];
			speakerRight[left] = right;
			speakerLeft[right] = left;
		}
	}
	assert( ( dstLFE == -1 ) || ( ( dstMask & hfWaveFile::CHANNEL_MASK_LOW_FREQUENCY ) != 0 ) );
	assert( ( dstCenter == -1 ) || ( ( dstMask & hfWaveFile::CHANNEL_MASK_FRONT_CENTER ) != 0 ) );

	float omniChannels = (float)dstChannels;
	if ( dstMask & hfWaveFile::CHANNEL_MASK_LOW_FREQUENCY ) {
		omniChannels -= 1.0f;
	}
	if ( dstMask & hfWaveFile::CHANNEL_MASK_FRONT_CENTER ) {
		omniChannels -= 1.0f;
	}
	if ( omniChannels > 0.0f ) {
		omniLevel = 1.0f / omniChannels;
	} else {
		// This happens in mono mode
		omniLevel = 1.0f;
	}
}

/*
========================
hfSoundVoice_Base::CalculateSurround
========================
*/
void hfSoundVoice_Base::CalculateSurround( int srcChannels, float pLevelMatrix[ MAX_CHANNELS_PER_VOICE * MAX_CHANNELS_PER_VOICE ], float scale ) {
	// Hack for mono
	if ( dstChannels == 1 ) {
		if ( srcChannels == 1 ) {
			pLevelMatrix[ 0 ] = scale;
		} else if ( srcChannels == 2 ) {
			pLevelMatrix[ 0 ] = scale * 0.7071f;
			pLevelMatrix[ 1 ] = scale * 0.7071f;
		}
		return;
	}

#define MATINDEX( src, dst ) ( srcChannels * dst + src )

	float subFraction = s_subFraction.GetFloat();

	if ( srcChannels == 1 ) {
		hfVec2 p2 = position.ToVec2();

		float centerFraction = centerChannel;

		float sqrLength = p2.LengthSqr();
		if ( sqrLength <= 0.01f ) {
			// If we are on top of the listener, simply route all channels to each speaker equally
			for ( int i = 0; i < dstChannels; i++ ) {
				pLevelMatrix[MATINDEX( 0, i )] = omniLevel;
			}
		} else {
			float invLength = hfMath::InvSqrt( sqrLength );
			float distance = ( invLength * sqrLength );
			p2 *= invLength;

			float spatialize = 1.0f;
			if ( distance < innerRadius ) {
				spatialize = distance / innerRadius;
			}
			float omni = omniLevel * ( 1.0f - spatialize );

			if ( dstCenter != -1 ) {
				centerFraction *= Max( 0.0f, p2.x );
				spatialize *= ( 1.0f - centerFraction );
				omni *= ( 1.0f - centerFraction );
			}

			float channelDots[MAX_CHANNELS_PER_VOICE] = { 0 };
			for ( int i = 0; i < dstChannels; i++ ) {
				// Calculate the contribution to each destination channel
				channelDots[i] = speakerPositions[dstMap[i]] * p2;
			}
			// Find the speaker nearest to the sound
			int channelA = 0;
			for ( int i = 1; i < dstChannels; i++ ) {
				if ( channelDots[i] > channelDots[channelA] ) {
					channelA = i;
				}
			}
			int speakerA = dstMap[channelA];

			// Find the 2nd nearest speaker
			int speakerB;
			float speakerACross = ( speakerPositions[speakerA].x * p2.y ) - ( speakerPositions[speakerA].y * p2.x );
			if ( speakerACross > 0.0f ) {
				speakerB = speakerLeft[speakerA];
			} else {
				speakerB = speakerRight[speakerA];
			}
			int channelB = invMap[speakerB];

			// Divide the amplitude between the 2 closest speakers
			float distA = ( speakerPositions[speakerA] - p2 ).Length();
			float distB = ( speakerPositions[speakerB] - p2 ).Length();
			float distCinv = 1.0f / ( distA + distB );
			float volumes[MAX_CHANNELS_PER_VOICE] = { 0 };
			volumes[channelA] = ( distB * distCinv );
			volumes[channelB] = ( distA * distCinv );
			for ( int i = 0; i < dstChannels; i++ ) {
				pLevelMatrix[MATINDEX( 0, i )] = ( volumes[i] * spatialize ) + omni;
			}
		}
		if ( dstLFE != -1 ) {
			pLevelMatrix[MATINDEX( 0, dstLFE )] = subFraction;
		}
		if ( dstCenter != -1 ) {
			pLevelMatrix[MATINDEX( 0, dstCenter )] = centerFraction;
		}
	} else if ( srcChannels == 2 ) {
		pLevelMatrix[ MATINDEX( 0, 0 ) ] = 1.0f;
		pLevelMatrix[ MATINDEX( 1, 1 ) ] = 1.0f;
		if ( dstLFE != -1 ) {
			pLevelMatrix[ MATINDEX( 0, dstLFE ) ] = subFraction * 0.5f;
			pLevelMatrix[ MATINDEX( 1, dstLFE ) ] = subFraction * 0.5f;
		}
	} else {
		hfLib::Warning( "We don't support %d channel sound files", srcChannels );
	}
	for ( int i = 0; i < srcChannels * dstChannels; i++ ) {
		pLevelMatrix[ i ] *= scale;
	}
}

