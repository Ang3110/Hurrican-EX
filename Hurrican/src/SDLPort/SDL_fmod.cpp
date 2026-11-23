/**
 *
 *  Copyright (C) 2011-2012 Scott R. Smith
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 *
 */

#include "SDL_fmod.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#if defined(USE_OPENMPT)
#  include <atomic>
#  include <libopenmpt/libopenmpt.h>
#endif

constexpr int MusicVolumeMax = 255;

struct MUSIC_MODULE {
#if defined(USE_OPENMPT)
    openmpt_module *decoder;
    std::atomic<int> volume{MusicVolumeMax};
    std::atomic<bool> paused{false};
    std::atomic<bool> finished{false};
#else
    Mix_Music *decoder;
    int volume = MusicVolumeMax;
#endif
    bool looped = true;
};

static int g_allocated = 0;
#if defined(USE_OPENMPT)
static int g_mixrate = 0;
static void hookmusic(void *ptr, uint8_t *buffer, int size);
#endif

static MUSIC_MODULE *g_music_current = nullptr;

#if defined(USE_OPENMPT)
    static void log_func( const char * message, void * userdata ) {}
    static int error_func( int error, void * userdata ) {
          (void)userdata;
          (void)error;
          return OPENMPT_ERROR_FUNC_RESULT_DEFAULT & ~OPENMPT_ERROR_FUNC_RESULT_LOG;
        }
#endif

signed char SOUND_Init(int mixrate, int maxsoftwarechannels, unsigned int flags) {
    g_allocated = 0;

    int chunksize = 2048;
    if (Mix_OpenAudio(mixrate, MIX_DEFAULT_FORMAT, 2, chunksize) == -1) {
        std::cerr << "->ERROR: Mix_OpenAudio: " << Mix_GetError() << std::endl;
        return 0;
    } else {
        // DKS - Check what we actually received:
        int cur_freq = 0, cur_channels = 0;
        uint16_t cur_format = 0;
        Mix_QuerySpec(&cur_freq, &cur_format, &cur_channels);
        std::cout << "SDL_mixer initialized. Buffer size:" << chunksize << ", Freq:" << cur_freq
                  << ", Channels:" << cur_channels << std::endl;
        switch (cur_format) {
            case AUDIO_U8:
                std::cout << "Unsigned 8-bit samples " << std::endl;
                break;
            case AUDIO_S8:
                std::cout << "Signed 8-bit samples " << std::endl;
                break;
            case AUDIO_U16LSB:
                std::cout << "Unsigned 16-bit samples (little-endian) " << std::endl;
                break;
            case AUDIO_S16LSB:
                std::cout << "Signed 16-bit samples (little-endian) " << std::endl;
                break;
            case AUDIO_U16MSB:
                std::cout << "Unsigned 16-bit samples (big-endian) " << std::endl;
                break;
            case AUDIO_S16MSB:
                std::cout << "Signed 16-bit samples (big-endian) " << std::endl;
                break;
            default:
                std::cout << "Unrecognized SDL_mixer sample format. " << std::endl;
                break;
        }

#if defined(USE_OPENMPT)
        g_mixrate = cur_freq;
#endif
    }
    g_allocated = Mix_AllocateChannels(maxsoftwarechannels);

#if defined(USE_OPENMPT)
    std::cout << "Using libopenmpt for ImpulseTracker music file decoding. " << std::endl;
#else
    std::cout << "Using SDL_mixer's default decoder for ImpulseTracker music file decoding. " << std::endl;
#endif

    return 1;
}

void SOUND_Close() {
    Mix_CloseAudio();
}

int SOUND_GetMaxChannels() {
    return g_allocated;
}

signed char SOUND_SetPan(int channel, int pan) {
    // DKS - If pan is 128, disable panning effect on the channel:
    if (pan == 128) {
        Mix_SetPanning(channel, 255, 255);
    } else {
        Mix_SetPanning(channel, 255 - pan, pan);
    }

    return 1;
}

MUSIC_MODULE *MUSIC_LoadSong(const char *filename) {
#if defined(USE_OPENMPT)
    std::vector<char> buffer = LoadFileToMemory(filename);

    if (buffer.empty()) {
        std::cerr << "ERROR Error file operation: File: " << filename << std::endl;
        return nullptr;
    }

    auto *decoder = openmpt_module_create_from_memory2(buffer.data(), buffer.size() - 1, &log_func, nullptr,
                                                       &error_func, nullptr, nullptr, nullptr, nullptr);
#else
    auto *decoder = Mix_LoadMUS(filename);
#endif

    return decoder ? new MUSIC_MODULE{decoder} : nullptr;
}

MUSIC_MODULE *MUSIC_LoadSongEx(const char *filename,
                               int offset,
                               int length,
                               unsigned int mode,
                               const int *samplelist,
                               int samplelistnum) {
    return NULL;
}

// DKS - Added default parameter to specify if a song should loop ('gameover.it' and 'stageclear.it' songs shouldn't):
// signed char MUSIC_PlaySong( MUSIC_MODULE* music )
signed char MUSIC_PlaySong(MUSIC_MODULE *music, bool loop) {
    MUSIC_StopAllSongs();
    music->looped = loop;
    g_music_current = music;
#if defined(USE_OPENMPT)
    openmpt_module_set_position_seconds(music->decoder, 0.0);
    music->paused = false;
    music->finished = false;
    Mix_HookMusic(hookmusic, music);
#else
    Mix_VolumeMusic(music->volume * MIX_MAX_VOLUME / MusicVolumeMax);
    if (Mix_PlayMusic(music->decoder, loop ? -1 : 0) < 0)
        g_music_current = nullptr;
#endif
    return 0;
}

signed char MUSIC_IsPlaying(MUSIC_MODULE *music) {
    if (!music || g_music_current != music)
        return 0;
#if defined(USE_OPENMPT)
    return !music->finished;
#else
    return Mix_PlayingMusic() != 0;
#endif
}

signed char MUSIC_FreeSong(MUSIC_MODULE *music) {
    if (!music)
        return 0;
    MUSIC_StopSong(music);
#if defined(USE_OPENMPT)
    openmpt_module_destroy(music->decoder);
#else
    Mix_FreeMusic(music->decoder);
#endif
    delete music;
    return 0;
}

signed char MUSIC_StopSong(MUSIC_MODULE *music) {
    // printf( "%8X MUSIC_StopSong\n", music );
    if (music && g_music_current == music) {
#if defined(USE_OPENMPT)
        Mix_HookMusic(nullptr, nullptr);
#else
        Mix_HaltMusic();
#endif
        g_music_current = nullptr;
    }
    return 0;
}

signed char MUSIC_SetPaused(MUSIC_MODULE *music, signed char pause) {
    if (!music)
        return 0;
#if defined(USE_OPENMPT)
    if (g_music_current == music) {
        music->paused = pause != 0;
    } else if (!pause) {
        music->paused = false;
        g_music_current = music;
        Mix_HookMusic(hookmusic, music);
    }
#else
    if (g_music_current == music) {
        if (pause)
            Mix_PauseMusic();
        else
            Mix_ResumeMusic();
    } else if (!pause) {
        // DKS - Whereas with SDL_mixer, it can only handle one song at a time so if another
        //      gets played after a song was paused, we have to start the paused song over again:
        MUSIC_PlaySong(music, music->looped);
    }
#endif
    return 0;
}

signed char MUSIC_GetPaused(MUSIC_MODULE *music) {
#if defined(USE_OPENMPT)
    return music && music->paused;
#else
    return music && g_music_current == music && Mix_PausedMusic();
#endif
}

signed char MUSIC_SetMasterVolume(MUSIC_MODULE *music, int volume) {
    if (!music)
        return 0;
    const int gain = std::clamp(volume, 0, MusicVolumeMax);
    music->volume = gain;
#if !defined(USE_OPENMPT)
    if (g_music_current == music)
        Mix_VolumeMusic(gain * MIX_MAX_VOLUME / MusicVolumeMax);
#endif
    return 0;
}

void MUSIC_StopAllSongs() {
    // printf( "MUSIC_StopAllSongs\n" );
    MUSIC_StopSong(g_music_current);
}

signed char MUSIC_IsFinished(MUSIC_MODULE *music) {
    return !MUSIC_IsPlaying(music);
}

#if defined(USE_OPENMPT)
static void hookmusic(void *ptr, uint8_t *buffer, int size) {
    auto *music = static_cast<MUSIC_MODULE *>(ptr);
    if (music->paused || music->finished) {
        std::memset(buffer, 0, size);
        return;
    }

    constexpr int channels = 2;
    auto *samples = reinterpret_cast<int16_t *>(buffer);
    const size_t frames = size / (channels * sizeof(*samples));
    size_t written = openmpt_module_read_interleaved_stereo(music->decoder, g_mixrate, frames, samples);
    if (written < frames && music->looped) {
        openmpt_module_set_position_seconds(music->decoder, 0.0);
        written += openmpt_module_read_interleaved_stereo(music->decoder, g_mixrate, frames - written,
                                                         samples + written * channels);
    } else if (written < frames) {
        music->finished = true;
    }

    // The custom music callback bypasses SDL_mixer's music volume. Scale only the decoded track.
    const int gain = music->volume.load(std::memory_order_relaxed);
    if (gain == 0) {
        written = 0;
    } else if (gain < MusicVolumeMax) {
        for (size_t i = 0; i < written * channels; ++i)
            samples[i] = static_cast<int16_t>(samples[i] * gain / MusicVolumeMax);
    }
    const size_t bytes = written * channels * sizeof(*samples);
    std::memset(buffer + bytes, 0, size - bytes);
}
#endif

Mix_Chunk *SOUND_Sample_Load(int index, const char *filename, unsigned int inputmode, int offset, int length) {
    Mix_Chunk *chunk = nullptr;

    // Load from memory
    if ((inputmode & FSOUND_LOADMEMORY) == FSOUND_LOADMEMORY) {
#if 1
        SDL_RWops *sdl_rw = SDL_RWFromConstMem(reinterpret_cast<const void *>(filename), length);
        chunk = Mix_LoadWAV_RW(sdl_rw, 1);
#else
        chunk = Mix_QuickLoad_WAV((Uint8 *)filename);
#endif
    } else  // Load from file
    {
        chunk = Mix_LoadWAV(filename);
    }

    return chunk;
}

// DKS - Added looped boolean parameter, so we no longer have to keep track of which
//      sounds are looped (DX8Sound.cpp already does that)
int SOUND_PlaySound(int channel, Mix_Chunk *chunk, bool looped) {
    return Mix_PlayChannel(channel, chunk, looped ? -1 : 0);
}

void SOUND_Sample_Free(Mix_Chunk *chunk) {
    Mix_FreeChunk(chunk);
}

signed char SOUND_IsPlaying(int channel) {
    if (channel < 0)
        return 0;

    return Mix_Playing(channel);
}

int SOUND_GetVolume(int channel) {
    if (channel < 0)
        return 0;

    float vol = static_cast<float>(Mix_Volume(channel, -1)) / 128.0f;
    // DKS - SDL volumes are 0-128, while fmod's are 0-255:
    return static_cast<int>(vol * 255.0f);
}

signed char SOUND_SetVolume(int channel, int volume) {
    if (channel < 0)
        return 0;

    return Mix_Volume(channel, volume / 2);  // FMOD range is 0-255 SDL_mixer is 0-128
}

signed char SOUND_StopSound(int channel) {
    return Mix_HaltChannel(channel);
}

// DKS - Added:
signed char SOUND_SetPaused(int channel, signed char paused) {
    if (paused)
        Mix_Pause(channel);
    else
        Mix_Resume(channel);

    return 1;
}

// DKS - Added:
signed char SOUND_GetPaused(int channel) {
    return Mix_Paused(channel);
}
