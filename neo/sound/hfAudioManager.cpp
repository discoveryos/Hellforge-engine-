#pragma hdrstop
#include "../hflib/hfprecompiled.h"
#include "../hflib/audio/miniaudio.h"	// decoder API (declarations only)

#include "hfAudioManager.h"
#include "../hflib/audio/hfAudioData.h"

namespace HF_Audio {

hfAudioManager::hfAudioManager( hfPreviewAllocateCallback previewAllocate,
								hfPreviewMarkDirtyCallback previewMarkDirty )
	: m_previewAllocate( previewAllocate ), m_previewMarkDirty( previewMarkDirty ), m_nextId( 1 ) {
}

hfAudioManager::~hfAudioManager() {
	for ( int i = 0; i < m_entries.Num(); i++ ) {
		if ( m_entries[i].samples ) {
			delete[] m_entries[i].samples;
			m_entries[i].samples = NULL;
		}
	}
	m_entries.Clear();
}

unsigned int hfAudioManager::LoadAsync( const char *filePath ) {
	return LoadSync( filePath );
}

unsigned int hfAudioManager::LoadSync( const char *filePath ) {
	if ( !filePath || !filePath[0] ) { return 0; }
	unsigned int existing = GetIdByPath( filePath );
	if ( existing ) { return existing; }

	ma_decoder decoder;
	ma_decoder_config cfg = ma_decoder_config_init( ma_format_f32, 2, 0 );
	if ( ma_decoder_init_file( filePath, &cfg, &decoder ) != MA_SUCCESS ) {
		Entry e;
		e.id = m_nextId++;
		e.name = filePath;
		e.path = filePath;
		e.state = HF_SOUND_ERROR;
		e.samples = NULL;
		m_entries.Append( e );
		return e.id;
	}

	ma_uint64 length = 0;
	if ( ma_decoder_get_length_in_pcm_frames( &decoder, &length ) != MA_SUCCESS ) { length = 0; }

	hfAudioSampleInfo info;
	info.channels = ( unsigned int )decoder.outputChannels;
	info.sampleRate = ( unsigned int )decoder.outputSampleRate;
	info.totalFrames = ( unsigned int )length;
	info.totalBytes = info.totalFrames * info.channels * sizeof( float );

	float *samples = NULL;
	if ( info.totalFrames > 0 ) {
		samples = new float[ ( size_t )info.totalFrames * info.channels ];
		ma_uint64 read = 0;
		ma_decoder_read_pcm_frames( &decoder, samples, ( ma_uint64 )info.totalFrames, &read );
		info.totalFrames = ( unsigned int )read;
		info.totalBytes = info.totalFrames * info.channels * sizeof( float );
	}
	ma_decoder_uninit( &decoder );

	Entry e;
	e.id = m_nextId++;
	e.name = filePath;
	e.path = filePath;
	e.state = HF_SOUND_READY;
	e.info = info;
	e.samples = samples;
	m_entries.Append( e );
	return e.id;
}

unsigned int hfAudioManager::LoadFromSourceAsync( const char *name, float **samplesOut ) {
	(void)samplesOut;
	return InternalLoadSync( name ? name : "animate.source", "", NULL, NULL, false );
}

const hfAudioManager::Entry *hfAudioManager::GetEntry( unsigned int id ) const {
	for ( int i = 0; i < m_entries.Num(); i++ ) {
		if ( m_entries[i].id == id ) { return &m_entries[i]; }
	}
	return NULL;
}

unsigned int hfAudioManager::GetIdByPath( const char *filePath ) const {
	if ( !filePath ) { return 0; }
	for ( int i = 0; i < m_entries.Num(); i++ ) {
		if ( m_entries[i].path.IcmpPath( filePath ) == 0 ) { return m_entries[i].id; }
	}
	return 0;
}

int hfAudioManager::GetIdByName( const char *name ) const {
	if ( !name ) { return -1; }
	for ( int i = 0; i < m_entries.Num(); i++ ) {
		if ( m_entries[i].name.Icmp( name ) == 0 ) { return ( int )m_entries[i].id; }
	}
	return -1;
}

void hfAudioManager::FinalizeResource( const Entry &entry ) {
	for ( int i = 0; i < m_entries.Num(); i++ ) {
		if ( m_entries[i].id == entry.id ) {
			m_entries[i].state = HF_SOUND_READY;
			if ( m_previewAllocate ) { m_previewAllocate( entry.id ); }
			if ( m_previewMarkDirty ) { m_previewMarkDirty( entry.id ); }
			return;
		}
	}
}

unsigned int hfAudioManager::InternalLoadSync( const char *name, const char *path, const float *extSamples,
											   const hfAudioSampleInfo *extInfo, bool externalData ) {
	unsigned int existing = GetIdByName( name );
	if ( existing ) { return existing; }

	Entry e;
	e.id = m_nextId++;
	e.name = name;
	e.path = path;
	if ( externalData && extSamples && extInfo ) {
		e.info = *extInfo;
		e.samples = new float[ ( size_t )extInfo->totalFrames * extInfo->channels ];
		memcpy( e.samples, extSamples, ( size_t )extInfo->totalBytes );
		e.state = HF_SOUND_READY;
	} else {
		e.state = HF_SOUND_ERROR;
		e.samples = NULL;
	}
	m_entries.Append( e );
	return e.id;
}

} // namespace HF_Audio