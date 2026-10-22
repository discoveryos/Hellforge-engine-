#ifndef __HF_AUDIO_ENGINE_INIT_H__
#define __HF_AUDIO_ENGINE_INIT_H__

// Console module registration for the miniaudio-backed engine. Called once
// from engine startup (R_InitOpenGL). The backend itself is created lazily by
// the first audioInit/audioPlay/audioTone/audioPreview command, so a machine
// without an audio device degrades to a printed warning instead of a crash.
namespace HF_Audio {

void HF_AudioRegisterCommands();

} // namespace HF_Audio

#endif