#ifndef __HF_AUDIO_MANAGER_H__
#define __HF_AUDIO_MANAGER_H__

#include "../hflib/audio/hfAudioData.h"

// Audio resource manager mirroring the reference integration's AudioManager:
// load sounds by path, keep a name->id registry with Ready/Error/Loading
// state, hand decoded float32 buffers to the playback engine, and notify a
// preview layer that an entry changed so it can rebuild its data source.
// Fresh implementation: decode uses miniaudio's decoder; everything else is
// engine-native containers.

namespace HF_Audio {

typedef void (*hfPreviewAllocateCallback)( unsigned int id );
typedef void (*hfPreviewMarkDirtyCallback)( unsigned int id );

enum hfSoundState {
	HF_SOUND_LOADING = 0,	// reserved for a future worker thread; see note
	HF_SOUND_READY,
	HF_SOUND_ERROR,
};

class hfAudioManager {
public:
	typedef struct {
		unsigned int		id;
		hfStr				name;
		hfStr				path;
		hfSoundState		state;
		hfAudioSampleInfo	info;
		float *				samples;		// interleaved f32, owned by manager
	} Entry;

	explicit hfAudioManager( hfPreviewAllocateCallback previewAllocate = NULL,
							 hfPreviewMarkDirtyCallback previewMarkDirty = NULL );
	~hfAudioManager();

	// Both entry points decode synchronously for now (the reference engine
	// schedules them on a worker thread; the API shape is preserved so that
	// path can be added without touching callers).
	unsigned int	LoadAsync( const char *filePath );
	unsigned int	LoadFromSourceAsync( const char *name, float **samplesOut );		// raw f32 source
	unsigned int	LoadSync( const char *filePath );

	const Entry *	GetEntry( unsigned int id ) const;
	unsigned int	GetIdByPath( const char *filePath ) const;
	int				GetIdByName( const char *name ) const;
	int				GetCount() const { return m_entries.Num(); }

	// marks the entry ready and fires the preview dirty hook
	void			FinalizeResource( const Entry &entry );

private:
	unsigned int	InternalLoadSync( const char *name, const char *path, const float *extSamples,
									  const hfAudioSampleInfo *extInfo, bool externalData );

	hfList<Entry>				m_entries;
	hfPreviewAllocateCallback	m_previewAllocate;
	hfPreviewMarkDirtyCallback	m_previewMarkDirty;
	unsigned int				m_nextId;
};

} // namespace HF_Audio

#endif