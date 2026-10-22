#pragma hdrstop
#include "../../hflib/hfprecompiled.h"
#include "../hfsnd_local.h"

extern hfCVar s_useCompression;
extern hfCVar s_noSound;

#define GPU_CONVERT_CPU_TO_CPU_CACHED_READONLY_ADDRESS( x ) x

const uint32 SOUND_MAGIC_IDMSA = 0x6D7A7274;

extern hfCVar sys_lang;

/*
========================
AllocBuffer
========================
*/
static void * AllocBuffer( int size, const char * name ) {
	return Mem_Alloc( size, TAG_AUDIO );
}

/*
========================
FreeBuffer
========================
*/
static void FreeBuffer( void * p ) {
	return Mem_Free( p );
}

/*
========================
hfSoundSample_XAudio2::hfSoundSample_XAudio2
========================
*/
hfSoundSample_XAudio2::hfSoundSample_XAudio2() {
	timestamp = FILE_NOT_FOUND_TIMESTAMP;
	loaded = false;
	neverPurge = false;
	levelLoadReferenced = false;

	memset( &format, 0, sizeof( format ) );

	totalBufferSize = 0;

	playBegin = 0;
	playLength = 0;

	lastPlayedTime = 0;
}

/*
========================
hfSoundSample_XAudio2::~hfSoundSample_XAudio2
========================
*/
hfSoundSample_XAudio2::~hfSoundSample_XAudio2() {
	FreeData();
}

/*
========================
hfSoundSample_XAudio2::WriteGeneratedSample
========================
*/
void hfSoundSample_XAudio2::WriteGeneratedSample( hfFile *fileOut ) {
	fileOut->WriteBig( SOUND_MAGIC_IDMSA );
	fileOut->WriteBig( timestamp );
	fileOut->WriteBig( loaded );
	fileOut->WriteBig( playBegin );
	fileOut->WriteBig( playLength );
	hfWaveFile::WriteWaveFormatDirect( format, fileOut );
	fileOut->WriteBig( ( int )amplitude.Num() );
	fileOut->Write( amplitude.Ptr(), amplitude.Num() );
	fileOut->WriteBig( totalBufferSize );
	fileOut->WriteBig( ( int )buffers.Num() );
	for ( int i = 0; i < buffers.Num(); i++ ) {
		fileOut->WriteBig( buffers[ i ].numSamples );
		fileOut->WriteBig( buffers[ i ].bufferSize );
		fileOut->Write( buffers[ i ].buffer, buffers[ i ].bufferSize );
	};
}
/*
========================
hfSoundSample_XAudio2::WriteAllSamples
========================
*/
void hfSoundSample_XAudio2::WriteAllSamples( const hfStr &sampleName ) {
	hfSoundSample_XAudio2 * samplePC = new hfSoundSample_XAudio2();
	{
		hfStrStatic< MAX_OSPATH > inName = sampleName;
		inName.Append( ".msadpcm" );
		hfStrStatic< MAX_OSPATH > inName2 = sampleName;
		inName2.Append( ".wav" );

		hfStrStatic< MAX_OSPATH > outName = "generated/";
		outName.Append( sampleName );
		outName.Append( ".idwav" );

		if ( samplePC->LoadWav( inName ) || samplePC->LoadWav( inName2 ) ) {
			hfFile *fileOut = fileSystem->OpenFileWrite( outName, "fs_basepath" );
			samplePC->WriteGeneratedSample( fileOut );
			delete fileOut;
		} 
	}
	delete samplePC;
}

/*
========================
hfSoundSample_XAudio2::LoadGeneratedSound
========================
*/
bool hfSoundSample_XAudio2::LoadGeneratedSample( const hfStr &filename ) {
	hfFileLocal fileIn( fileSystem->OpenFileReadMemory( filename ) );
	if ( fileIn != NULL ) {
		uint32 magic;
		fileIn->ReadBig( magic );
		fileIn->ReadBig( timestamp );
		fileIn->ReadBig( loaded );
		fileIn->ReadBig( playBegin );
		fileIn->ReadBig( playLength );
		hfWaveFile::ReadWaveFormatDirect( format, fileIn );
		int num;
		fileIn->ReadBig( num );
		amplitude.Clear();
		amplitude.SetNum( num );
		fileIn->Read( amplitude.Ptr(), amplitude.Num() );
		fileIn->ReadBig( totalBufferSize );
		fileIn->ReadBig( num );
		buffers.SetNum( num );
		for ( int i = 0; i < num; i++ ) {
			fileIn->ReadBig( buffers[ i ].numSamples );
			fileIn->ReadBig( buffers[ i ].bufferSize );
			buffers[ i ].buffer = AllocBuffer( buffers[ i ].bufferSize, GetName() );
			fileIn->Read( buffers[ i ].buffer, buffers[ i ].bufferSize );
			buffers[ i ].buffer = GPU_CONVERT_CPU_TO_CPU_CACHED_READONLY_ADDRESS( buffers[ i ].buffer );
		}
		return true;
	}
	return false;
}
/*
========================
hfSoundSample_XAudio2::Load
========================
*/
void hfSoundSample_XAudio2::LoadResource() {
	FreeData();

	if ( hfStr::Icmpn( GetName(), "_default", 8 ) == 0 ) {
		MakeDefault();
		return;
	}

	if ( s_noSound.GetBool() ) {
		MakeDefault();
		return;
	}

	loaded = false;

	for ( int i = 0; i < 2; i++ ) {
		hfStrStatic< MAX_OSPATH > sampleName = GetName();
		if ( ( i == 0 ) && !sampleName.Replace( "/vo/", va( "/vo/%s/", sys_lang.GetString() ) ) ) {
			i++;
		}
		hfStrStatic< MAX_OSPATH > generatedName = "generated/";
		generatedName.Append( sampleName );

		{
			if ( s_useCompression.GetBool() ) {
				sampleName.Append( ".msadpcm" );
			} else {
				sampleName.Append( ".wav" );
			}
			generatedName.Append( ".idwav" );
		}
		loaded = LoadGeneratedSample( generatedName ) || LoadWav( sampleName );

		if ( !loaded && s_useCompression.GetBool() ) {
			sampleName.SetFileExtension( "wav" );
			loaded = LoadWav( sampleName );
		}

		if ( loaded ) {
			if ( cvarSystem->GetCVarBool( "fs_buildresources" ) ) {
				fileSystem->AddSamplePreload( GetName() );
				WriteAllSamples( GetName() );

				if ( sampleName.Find( "/vo/" ) >= 0 ) {
					for ( int i = 0; i < Sys_NumLangs(); i++ ) {
						const char * lang = Sys_Lang( i );
						if ( hfStr::Icmp( lang, HF_LANG_ENGLISH ) == 0 ) {
							continue;
						}
						hfStrStatic< MAX_OSPATH > locName = GetName();
						locName.Replace( "/vo/", va( "/vo/%s/", Sys_Lang( i ) ) );
						WriteAllSamples( locName );
					}
				}
			}
			return;
		}
	}

	if ( !loaded ) {
		// make it default if everything else fails
		MakeDefault();
	}
	return;
}

/*
========================
hfSoundSample_XAudio2::LoadWav
========================
*/
bool hfSoundSample_XAudio2::LoadWav( const hfStr & filename ) {

	// load the wave
	hfWaveFile wave;
	if ( !wave.Open( filename ) ) {
		return false;
	}

	hfStrStatic< MAX_OSPATH > sampleName = filename;
	sampleName.SetFileExtension( "amp" );
	LoadAmplitude( sampleName );

	const char * formatError = wave.ReadWaveFormat( format );
	if ( formatError != NULL ) {
		hfLib::Warning( "LoadWav( %s ) : %s", filename.c_str(), formatError );
		MakeDefault();
		return false;
	}
	timestamp = wave.Timestamp();

	totalBufferSize = wave.SeekToChunk( 'data' );

	if ( format.basic.formatTag == hfWaveFile::FORMAT_PCM || format.basic.formatTag == hfWaveFile::FORMAT_EXTENSIBLE ) {

		if ( format.basic.bitsPerSample != 16 ) {
			hfLib::Warning( "LoadWav( %s ) : %s", filename.c_str(), "Not a 16 bit PCM wav file" );
			MakeDefault();
			return false;
		}

		playBegin = 0;
		playLength = ( totalBufferSize ) / format.basic.blockSize;

		buffers.SetNum( 1 );
		buffers[0].bufferSize = totalBufferSize;
		buffers[0].numSamples = playLength;
		buffers[0].buffer = AllocBuffer( totalBufferSize, GetName() );
		

		wave.Read( buffers[0].buffer, totalBufferSize );

		if ( format.basic.bitsPerSample == 16 ) {
			hfSwap::LittleArray( (short *)buffers[0].buffer, totalBufferSize / sizeof( short ) );
		}

		buffers[0].buffer = GPU_CONVERT_CPU_TO_CPU_CACHED_READONLY_ADDRESS( buffers[0].buffer );

	} else if ( format.basic.formatTag == hfWaveFile::FORMAT_ADPCM ) {

		playBegin = 0;
		playLength = ( ( totalBufferSize / format.basic.blockSize ) * format.extra.adpcm.samplesPerBlock );

		buffers.SetNum( 1 );
		buffers[0].bufferSize = totalBufferSize;
		buffers[0].numSamples = playLength;
		buffers[0].buffer  = AllocBuffer( totalBufferSize, GetName() );
		
		wave.Read( buffers[0].buffer, totalBufferSize );

		buffers[0].buffer = GPU_CONVERT_CPU_TO_CPU_CACHED_READONLY_ADDRESS( buffers[0].buffer );

	} else if ( format.basic.formatTag == hfWaveFile::FORMAT_XMA2 ) {

		if ( format.extra.xma2.blockCount == 0 ) {
			hfLib::Warning( "LoadWav( %s ) : %s", filename.c_str(), "No data blocks in file" );
			MakeDefault();
			return false;
		}

		int bytesPerBlock = format.extra.xma2.bytesPerBlock;
		assert( format.extra.xma2.blockCount == ALIGN( totalBufferSize, bytesPerBlock ) / bytesPerBlock );
		assert( format.extra.xma2.blockCount * bytesPerBlock >= totalBufferSize );
		assert( format.extra.xma2.blockCount * bytesPerBlock < totalBufferSize + bytesPerBlock );

		buffers.SetNum( format.extra.xma2.blockCount );
		for ( int i = 0; i < buffers.Num(); i++ ) {
			if ( i == buffers.Num() - 1 ) {
				buffers[i].bufferSize = totalBufferSize - ( i * bytesPerBlock );
			} else {
				buffers[i].bufferSize = bytesPerBlock;
			}

			buffers[i].buffer = AllocBuffer( buffers[i].bufferSize, GetName() );
			wave.Read( buffers[i].buffer, buffers[i].bufferSize );
			buffers[i].buffer = GPU_CONVERT_CPU_TO_CPU_CACHED_READONLY_ADDRESS( buffers[i].buffer );
		}

		int seekTableSize = wave.SeekToChunk( 'seek' );
		if ( seekTableSize != 4 * buffers.Num() ) {
			hfLib::Warning( "LoadWav( %s ) : %s", filename.c_str(), "Wrong number of entries in seek table" );
			MakeDefault();
			return false;
		}

		for ( int i = 0; i < buffers.Num(); i++ ) {
			wave.Read( &buffers[i].numSamples, sizeof( buffers[i].numSamples ) );
			hfSwap::Big( buffers[i].numSamples );
		}

		playBegin = format.extra.xma2.loopBegin;
		playLength = format.extra.xma2.loopLength;

		if ( buffers[buffers.Num()-1].numSamples < playBegin + playLength ) {
			// This shouldn't happen, but it's not fatal if it does
			playLength = buffers[buffers.Num()-1].numSamples - playBegin;
		} else {
			// Discard samples beyond playLength
			for ( int i = 0; i < buffers.Num(); i++ ) {
				if ( buffers[i].numSamples > playBegin + playLength ) {
					buffers[i].numSamples = playBegin + playLength;
					// Ideally, the following loop should always have 0 iterations because playBegin + playLength ends in the last block already
					// But there is no guarantee for that, so to be safe, discard all buffers beyond this one
					for ( int j = i + 1; j < buffers.Num(); j++ ) {
						FreeBuffer( buffers[j].buffer );
					}
					buffers.SetNum( i + 1 );
					break;
				}
			}
		}

	} else {
		hfLib::Warning( "LoadWav( %s ) : Unsupported wave format %d", filename.c_str(), format.basic.formatTag );
		MakeDefault();
		return false;
	}

	wave.Close();

	if ( format.basic.formatTag == hfWaveFile::FORMAT_EXTENSIBLE ) {
		// HACK: XAudio2 doesn't really support FORMAT_EXTENSIBLE so we convert it to a basic format after extracting the channel mask
		format.basic.formatTag = format.extra.extensible.subFormat.data1;
	}

	// sanity check...
	assert( buffers[buffers.Num()-1].numSamples == playBegin + playLength );

	return true;
}


/*
========================
hfSoundSample_XAudio2::MakeDefault
========================
*/
void hfSoundSample_XAudio2::MakeDefault() {
	FreeData();

	static const int DEFAULT_NUM_SAMPLES = 256;

	timestamp = FILE_NOT_FOUND_TIMESTAMP;
	loaded = true;

	memset( &format, 0, sizeof( format ) );
	format.basic.formatTag = hfWaveFile::FORMAT_PCM;
	format.basic.numChannels = 1;
	format.basic.bitsPerSample = 16;
	format.basic.samplesPerSec = XAUDIO2_MIN_SAMPLE_RATE;
	format.basic.blockSize = format.basic.numChannels * format.basic.bitsPerSample / 8;
	format.basic.avgBytesPerSec = format.basic.samplesPerSec * format.basic.blockSize;

	assert( format.basic.blockSize == 2 );

	totalBufferSize = DEFAULT_NUM_SAMPLES * 2;

	short * defaultBuffer = (short *)AllocBuffer( totalBufferSize, GetName() );
	for ( int i = 0; i < DEFAULT_NUM_SAMPLES; i += 2 ) {
		defaultBuffer[i + 0] = SHRT_MIN;
		defaultBuffer[i + 1] = SHRT_MAX;
	}

	buffers.SetNum( 1 );
	buffers[0].buffer = defaultBuffer;
	buffers[0].bufferSize = totalBufferSize;
	buffers[0].numSamples = DEFAULT_NUM_SAMPLES;
	buffers[0].buffer = GPU_CONVERT_CPU_TO_CPU_CACHED_READONLY_ADDRESS( buffers[0].buffer );

	playBegin = 0;
	playLength = DEFAULT_NUM_SAMPLES;
}

/*
========================
hfSoundSample_XAudio2::FreeData

Called before deleting the object and at the start of LoadResource()
========================
*/
void hfSoundSample_XAudio2::FreeData() {
	if ( buffers.Num() > 0 ) {
		soundSystemLocal.StopVoicesWithSample( (hfSoundSample *)this );
		for ( int i = 0; i < buffers.Num(); i++ ) {
			FreeBuffer( buffers[i].buffer );
		}
		buffers.Clear();
	}
	amplitude.Clear();

	timestamp = FILE_NOT_FOUND_TIMESTAMP;
	memset( &format, 0, sizeof( format ) );
	loaded = false;
	totalBufferSize = 0;
	playBegin = 0;
	playLength = 0;
}

/*
========================
hfSoundSample_XAudio2::LoadAmplitude
========================
*/
bool hfSoundSample_XAudio2::LoadAmplitude( const hfStr & name ) {
	amplitude.Clear();
	hfFileLocal f( fileSystem->OpenFileRead( name ) );
	if ( f == NULL ) {
		return false;
	}
	amplitude.SetNum( f->Length() );
	f->Read( amplitude.Ptr(), amplitude.Num() );
	return true;
}

/*
========================
hfSoundSample_XAudio2::GetAmplitude
========================
*/
float hfSoundSample_XAudio2::GetAmplitude( int timeMS ) const {
	if ( timeMS < 0 || timeMS > LengthInMsec() ) {
		return 0.0f;
	}
	if ( IsDefault() ) {
		return 1.0f;
	}
	int index = timeMS * 60 / 1000;
	if ( index < 0 || index >= amplitude.Num() ) {
		return 0.0f;
	}
	return (float)amplitude[index] / 255.0f;
}

