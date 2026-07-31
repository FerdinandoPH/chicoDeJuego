#include "audio.h"
#include <SDL3/SDL.h>

// SDL3 audio backend. The core accumulates the samples and hands them over in
// blocks; here they are just pushed to the stream and the queue is queried.
class Sdl_audio : public IAudio {
    private:
        SDL_AudioStream* stream = nullptr;

    public:
        bool init(int freq, int channels) override {
            if (!SDL_Init(SDL_INIT_AUDIO)) return false;
            SDL_AudioSpec spec;
            spec.freq     = freq;
            spec.format   = SDL_AUDIO_F32;
            spec.channels = channels;
            this->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                                     &spec, nullptr, nullptr);
            if (!this->stream) return false;
            SDL_ResumeAudioStreamDevice(this->stream);
            return true;
        }

        void shutdown() override {
            if (this->stream) SDL_DestroyAudioStream(this->stream);
            this->stream = nullptr;
        }

        void submit(const float* interleaved, size_t n_floats) override {
            if (!this->stream) return;
            SDL_PutAudioStreamData(this->stream, interleaved, (int)(n_floats * sizeof(float)));
        }

        int queued_bytes() override {
            return this->stream ? SDL_GetAudioStreamAvailable(this->stream) : 0;
        }

        void clear() override {
            if (this->stream) SDL_ClearAudioStream(this->stream);
        }
};

static Sdl_audio g_sdl_audio;

IAudio* create_audio(){
    return &g_sdl_audio;
}
