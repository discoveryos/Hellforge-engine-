#pragma hdrstop
#include "../hflib/hfprecompiled.h"
#include "../hflib/audio/miniaudio.h"	// ma_* API (declarations only here)

#include "hfMiniAudio.h"
#include "../hflib/audio/hfAudioData.h"

namespace HF_Audio {

hfMiniAudioEngine::hfMiniAudioEngine() : m_engine( NULL ), m_sampleRate( 48000 ), m_previewRate( 48000.0f ) {
}

hfMiniAudioEngine::~hfMiniAudioEngine() {
	Shutdown();
}

void hfMiniAudioEngine::Init( unsigned sampleRate ) {
	if ( m_engine ) { return; }
	m_sampleRate = sampleRate ? sampleRate : 48000;

	ma_engine_config config = ma_engine_config_init();
	config.sampleRate = m_sampleRate;

	ma_engine *engine = new ma_engine();
	if ( ma_engine_init( &config, engine ) != MA_SUCCESS ) {
		delete engine;
		return;
	}
	m_engine = engine;
	m_previewRate = ( float )m_sampleRate;
}

void hfMiniAudioEngine::Shutdown() {
	if ( !m_engine ) { return; }
	StopAllSounds();
	if ( m_preview.isInitialized ) {
		PausePreview();
		ma_sound_uninit( reinterpret_cast< ma_sound * >( m_preview.sound ) );
		ma_audio_buffer_uninit( reinterpret_cast< ma_audio_buffer * >( m_preview.buffer ) );
		m_preview = Active();
	}
	ma_engine_uninit( reinterpret_cast< ma_engine * >( m_engine ) );
	delete reinterpret_cast< ma_engine * >( m_engine );
	m_engine = NULL;
}

void hfMiniAudioEngine::SetListenerTransform( const float pos[3], const float fwd[3], const float up[3] ) {
	if ( !m_engine ) { return; }
	ma_engine *engine = reinterpret_cast< ma_engine * >( m_engine );
	ma_engine_listener_set_position( engine, 0, pos[0], pos[1], pos[2] );
	ma_engine_listener_set_direction( engine, 0, fwd[0], fwd[1], fwd[2] );
	ma_engine_listener_set_world_up( engine, 0, up[0], up[1], up[2] );
}

void hfMiniAudioEngine::UpdateSound( unsigned key, const hfAudioSampleInfo &info, const float *samples,
									 const hfPlayParams &params, const float pos[3] ) {
	if ( !m_engine || !key ) { return; }
	if ( !samples || !info.totalFrames ) {
		StopSound( key );
		return;
	}
	ma_engine *engine = reinterpret_cast< ma_engine * >( m_engine );

	Active &active = m_active[key];
	if ( !active.isInitialized ) {
		active.buffer = new ma_audio_buffer();
		active.sound = new ma_sound();

		ma_audio_buffer_config bufCfg = ma_audio_buffer_config_init(
			ma_format_f32, ( ma_uint32 )info.channels, ( ma_uint64 )info.totalFrames, samples, NULL );
		bufCfg.sampleRate = info.sampleRate ? info.sampleRate : m_sampleRate;
		if ( ma_audio_buffer_init( &bufCfg, reinterpret_cast< ma_audio_buffer * >( active.buffer ) ) != MA_SUCCESS ) {
			StopSound( key );
			return;
		}

		ma_sound_config sndCfg = ma_sound_config_init();
		sndCfg.pDataSource = reinterpret_cast< ma_audio_buffer * >( active.buffer );
		if ( !params.isSpatialized ) {
			sndCfg.flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;
		}
		if ( ma_sound_init_ex( engine, &sndCfg, reinterpret_cast< ma_sound * >( active.sound ) ) != MA_SUCCESS ) {
			ma_audio_buffer_uninit( reinterpret_cast< ma_audio_buffer * >( active.buffer ) );
			delete reinterpret_cast< ma_audio_buffer * >( active.buffer );
			active = Active();
			return;
		}
		active.isInitialized = true;
		ma_sound_set_attenuation_model( reinterpret_cast< ma_sound * >( active.sound ), ma_attenuation_model_linear );
		ma_sound_set_looping( reinterpret_cast< ma_sound * >( active.sound ), params.loop ? MA_TRUE : MA_FALSE );
		ma_sound_start( reinterpret_cast< ma_sound * >( active.sound ) );
	}

	ma_sound *sound = reinterpret_cast< ma_sound * >( active.sound );

	if ( params.play ) {
		if ( !ma_sound_is_playing( sound ) ) {
			if ( ma_sound_at_end( sound ) ) {
				ma_sound_seek_to_pcm_frame( sound, 0 );
			}
			ma_sound_start( sound );
		}
	} else {
		if ( ma_sound_is_playing( sound ) ) {
			ma_sound_stop( sound );
		}
	}

	ma_sound_set_volume( sound, params.volume );
	ma_sound_set_pitch( sound, params.pitch );
	ma_sound_set_looping( sound, params.loop ? MA_TRUE : MA_FALSE );

	bool spatialized = ( ma_sound_is_spatialization_enabled( sound ) == MA_TRUE );
	if ( spatialized != params.isSpatialized ) {
		ma_sound_set_spatialization_enabled( sound, params.isSpatialized ? MA_TRUE : MA_FALSE );
	}
	if ( params.isSpatialized && pos ) {
		ma_sound_set_position( sound, pos[0], pos[1], pos[2] );
		ma_sound_set_min_distance( sound, params.minDistance );
		ma_sound_set_max_distance( sound, params.maxDistance );
		ma_sound_set_attenuation_model( sound, ma_attenuation_model_linear );
	}
}

void hfMiniAudioEngine::StopSound( unsigned key ) {
	if ( !key ) { return; }
	std::map<unsigned, Active>::iterator it = m_active.find( key );
	if ( it == m_active.end() ) { return; }
	Active &active = it->second;
	if ( active.isInitialized ) {
		ma_sound *sound = reinterpret_cast< ma_sound * >( active.sound );
		ma_sound_stop( sound );
		ma_sound_uninit( sound );
		ma_audio_buffer *buf = reinterpret_cast< ma_audio_buffer * >( active.buffer );
		ma_audio_buffer_uninit( buf );
		delete sound;
		delete buf;
	}
	m_active.erase( it );
}

void hfMiniAudioEngine::StopAllSounds() {
	for ( std::map<unsigned, Active>::iterator it = m_active.begin(); it != m_active.end(); ++it ) {
		StopSound( it->first );
	}
	m_active.clear();
}

void hfMiniAudioEngine::PlayPreview( const hfAudioSampleInfo &info, const float *samples ) {
	if ( !m_engine || !info.totalFrames || !samples ) { return; }

	if ( m_preview.isInitialized ) {
		PausePreview();
		ma_sound_uninit( reinterpret_cast< ma_sound * >( m_preview.sound ) );
		ma_audio_buffer_uninit( reinterpret_cast< ma_audio_buffer * >( m_preview.buffer ) );
		delete reinterpret_cast< ma_sound * >( m_preview.sound );
		delete reinterpret_cast< ma_audio_buffer * >( m_preview.buffer );
		m_preview = Active();
	}

	m_preview.buffer = new ma_audio_buffer();
	m_preview.sound = new ma_sound();

	ma_audio_buffer_config bufCfg = ma_audio_buffer_config_init(
		ma_format_f32, ( ma_uint32 )info.channels, ( ma_uint64 )info.totalFrames, samples, NULL );
	bufCfg.sampleRate = info.sampleRate ? info.sampleRate : m_sampleRate;
	if ( ma_audio_buffer_init( &bufCfg, reinterpret_cast< ma_audio_buffer * >( m_preview.buffer ) ) != MA_SUCCESS ) {
		delete reinterpret_cast< ma_audio_buffer * >( m_preview.buffer );
		m_preview.buffer = NULL;
		return;
	}

	ma_sound_config sndCfg = ma_sound_config_init();
	sndCfg.pDataSource = reinterpret_cast< ma_audio_buffer * >( m_preview.buffer );
	sndCfg.flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;

	ma_engine *engine = reinterpret_cast< ma_engine * >( m_engine );
	if ( ma_sound_init_ex( engine, &sndCfg, reinterpret_cast< ma_sound * >( m_preview.sound ) ) != MA_SUCCESS ) {
		ma_audio_buffer_uninit( reinterpret_cast< ma_audio_buffer * >( m_preview.buffer ) );
		delete reinterpret_cast< ma_audio_buffer * >( m_preview.buffer );
		delete reinterpret_cast< ma_sound * >( m_preview.sound );
		m_preview = Active();
		return;
	}
	m_preview.isInitialized = true;
	m_previewRate = ( float )( info.sampleRate ? info.sampleRate : m_sampleRate );
	ma_sound_start( reinterpret_cast< ma_sound * >( m_preview.sound ) );
}

void hfMiniAudioEngine::PausePreview() {
	if ( m_preview.isInitialized ) {
		ma_sound_stop( reinterpret_cast< ma_sound * >( m_preview.sound ) );
	}
}

void hfMiniAudioEngine::StopPreview() {
	if ( m_preview.isInitialized ) {
		ma_sound *sound = reinterpret_cast< ma_sound * >( m_preview.sound );
		ma_sound_stop( sound );
		ma_sound_seek_to_pcm_frame( sound, 0 );
	}
}

void hfMiniAudioEngine::SetPreviewTime( float seconds ) {
	if ( m_preview.isInitialized && m_previewRate > 0.0f ) {
		ma_sound_seek_to_pcm_frame( reinterpret_cast< ma_sound * >( m_preview.sound ),
			( ma_uint64 )( seconds * m_previewRate ) );
	}
}

bool hfMiniAudioEngine::IsPreviewPlaying() const {
	if ( !m_preview.isInitialized ) { return false; }
	return ma_sound_is_playing( reinterpret_cast< ma_sound * >( m_preview.sound ) ) == MA_TRUE;
}

float hfMiniAudioEngine::GetPreviewTime() const {
	if ( !m_preview.isInitialized || m_previewRate <= 0.0f ) { return 0.0f; }
	ma_uint64 cursor = 0;
	ma_sound_get_cursor_in_pcm_frames( reinterpret_cast< ma_sound * >( m_preview.sound ), &cursor );
	return ( float )cursor / m_previewRate;
}

} // namespace HF_Audio