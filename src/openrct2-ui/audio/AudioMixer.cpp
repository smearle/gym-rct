/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "AudioContext.h"
#include "AudioFormat.h"

#include <SDL.h>
#include <algorithm>
#include <iterator>
#include <list>
#include <openrct2/Context.h>
#include <openrct2/OpenRCT2.h>
#include <openrct2/audio/AudioChannel.h>
#include <openrct2/audio/AudioMixer.h>
#include <openrct2/audio/AudioSource.h>
#include <openrct2/audio/audio.h>
#include <openrct2/common.h>
#include <openrct2/config/Config.h>
#include <openrct2/core/Guard.hpp>
#include <openrct2/localisation/Localisation.h>
#include <openrct2/platform/platform.h>
#include <speex/speex_resampler.h>
#include <vector>

namespace OpenRCT2::Audio
{
    class AudioMixerImpl final : public IAudioMixer
    {
    private:
        IAudioSource* _nullSource = nullptr;

        SDL_AudioDeviceID _deviceId = 0;
        AudioFormat _format = {};
        std::list<ISDLAudioChannel*> _channels;
        float _volume = 1.0f;
        float _adjustSoundVolume = 0.0f;
        float _adjustMusicVolume = 0.0f;
        uint8_t _settingSoundVolume = 0xFF;
        uint8_t _settingMusicVolume = 0xFF;

        IAudioSource* _css1Sources[RCT2SoundCount] = { nullptr };
        IAudioSource* _musicSources[PATH_ID_END] = { nullptr };

        std::vector<uint8_t> _channelBuffer;
        std::vector<uint8_t> _convertBuffer;
        std::vector<uint8_t> _effectBuffer;

    public:
        AudioMixerImpl()
        {
            _nullSource = AudioSource::CreateNull();
        }

        ~AudioMixerImpl()
        {
            Close();
            delete _nullSource;
        }

        void Init(const char* device) override
        {
            Close();

            SDL_AudioSpec want = {};
            want.freq = 22050;
            want.format = AUDIO_S16SYS;
            want.channels = 2;
            want.samples = 2048;
            want.callback = [](void* arg, uint8_t* dst, int32_t length) -> void {
                auto mixer = static_cast<AudioMixerImpl*>(arg);
                mixer->GetNextAudioChunk(dst, static_cast<size_t>(length));
            };
            want.userdata = this;

            SDL_AudioSpec have;
            _deviceId = SDL_OpenAudioDevice(device, 0, &want, &have, 0);
            _format.format = have.format;
            _format.channels = have.channels;
            _format.freq = have.freq;

            LoadAllSounds();

            SDL_PauseAudioDevice(_deviceId, 0);
        }

        void Close() override
        {
            // Free channels
            Lock();
            for (IAudioChannel* channel : _channels)
            {
                delete channel;
            }
            _channels.clear();
            Unlock();

            SDL_CloseAudioDevice(_deviceId);

            // Free sources
            for (size_t i = 0; i < std::size(_css1Sources); i++)
            {
                if (_css1Sources[i] != _nullSource)
                {
                    SafeDelete(_css1Sources[i]);
                }
            }
            for (size_t i = 0; i < std::size(_musicSources); i++)
            {
                if (_musicSources[i] != _nullSource)
                {
                    SafeDelete(_musicSources[i]);
                }
            }

            // Free buffers
            _channelBuffer.clear();
            _channelBuffer.shrink_to_fit();
            _convertBuffer.clear();
            _convertBuffer.shrink_to_fit();
            _effectBuffer.clear();
            _effectBuffer.shrink_to_fit();
        }

        void Lock() override
        {
            SDL_LockAudioDevice(_deviceId);
        }

        void Unlock() override
        {
            SDL_UnlockAudioDevice(_deviceId);
        }

        IAudioChannel* Play(IAudioSource* source, int32_t loop, bool deleteondone, bool deletesourceondone) override
        {
            Lock();
            ISDLAudioChannel* channel = AudioChannel::Create();
            if (channel != nullptr)
            {
                channel->Play(source, loop);
                channel->SetDeleteOnDone(deleteondone);
                channel->SetDeleteSourceOnDone(deletesourceondone);
                _channels.push_back(channel);
            }
            Unlock();
            return channel;
        }

        void Stop(IAudioChannel* channel) override
        {
            Lock();
            channel->SetStopping(true);
            Unlock();
        }

        bool LoadMusic(size_t pathId) override
        {
            bool result = false;
            if (pathId < std::size(_musicSources))
            {
                IAudioSource* source = _musicSources[pathId];
                if (source == nullptr)
                {
                    const utf8* path = context_get_path_legacy(static_cast<int32_t>(pathId));
                    source = AudioSource::CreateMemoryFromWAV(path, &_format);
                    if (source == nullptr)
                    {
                        source = _nullSource;
                    }
                    _musicSources[pathId] = source;
                }
                result = source != _nullSource;
            }
            return result;
        }

        void SetVolume(float volume) override
        {
            _volume = volume;
        }

        IAudioSource* GetSoundSource(SoundId id) override
        {
            return _css1Sources[static_cast<uint32_t>(id)];
        }

        IAudioSource* GetMusicSource(int32_t id) override
        {
            return _musicSources[id];
        }

    private:
        void LoadAllSounds()
        {
            const utf8* css1Path = context_get_path_legacy(PATH_ID_CSS1);
            for (size_t i = 0; i < std::size(_css1Sources); i++)
            {
                auto source = AudioSource::CreateMemoryFromCSS1(css1Path, i, &_format);
                if (source == nullptr)
                {
                    source = _nullSource;
                }
                _css1Sources[i] = source;
            }
        }

        void GetNextAudioChunk(uint8_t* dst, size_t length)
        {
            UpdateAdjustedSound();

            // Zero the output buffer
            std::fill_n(dst, length, 0);

            // Mix channels onto output buffer
            auto it = _channels.begin();
            while (it != _channels.end())
            {
                auto channel = *it;
                int32_t group = channel->GetGroup();
                if ((group != MIXER_GROUP_SOUND || gConfigSound.sound_enabled) && gConfigSound.master_sound_enabled
                    && gConfigSound.master_volume != 0)
                {
                    MixChannel(channel, dst, length);
                }
                if ((channel->IsDone() && channel->DeleteOnDone()) || channel->IsStopping())
                {
                    delete channel;
                    it = _channels.erase(it);
                }
                else
                {
                    it++;
                }
            }
        }

        void UpdateAdjustedSound()
        {
            // Did the volume level get changed? Recalculate level in this case.
            if (_settingSoundVolume != gConfigSound.sound_volume)
            {
                _settingSoundVolume = gConfigSound.sound_volume;
                _adjustSoundVolume = powf(_settingSoundVolume / 100.f, 10.f / 6.f);
            }
            if (_settingMusicVolume != gConfigSound.ride_music_volume)
            {
                _settingMusicVolume = gConfigSound.ride_music_volume;
                _adjustMusicVolume = powf(_settingMusicVolume / 100.f, 10.f / 6.f);
            }
        }

        void MixChannel(ISDLAudioChannel* channel, uint8_t* data, size_t length)
        {
            int32_t byteRate = _format.GetByteRate();
            int32_t numSamples = static_cast<int32_t>(length / byteRate);
            double rate = 1;
            if (_format.format == AUDIO_S16SYS)
            {
                rate = channel->GetRate();
            }

            bool mustConvert = false;
            SDL_AudioCVT cvt;
            cvt.len_ratio = 1;
            AudioFormat streamformat = channel->GetFormat();
            if (streamformat != _format)
            {
                if (SDL_BuildAudioCVT(
                        &cvt, streamformat.format, streamformat.channels, streamformat.freq, _format.format, _format.channels,
                        _format.freq)
                    == -1)
                {
                    // Unable to convert channel data
                    return;
                }
                mustConvert = true;
            }

            // Read raw PCM from channel
            int32_t readSamples = static_cast<int32_t>(numSamples * rate);
            size_t readLength = static_cast<size_t>(readSamples / cvt.len_ratio) * byteRate;
            _channelBuffer.resize(readLength);
            size_t bytesRead = channel->Read(_channelBuffer.data(), readLength);

            // Convert data to required format if necessary
            void* buffer = nullptr;
            size_t bufferLen = 0;
            if (mustConvert)
            {
                if (Convert(&cvt, _channelBuffer.data(), bytesRead))
                {
                    buffer = cvt.buf;
                    bufferLen = cvt.len_cvt;
                }
                else
                {
                    return;
                }
            }
            else
            {
                buffer = _channelBuffer.data();
                bufferLen = bytesRead;
            }

            // Apply effects
            if (rate != 1)
            {
                int32_t inRate = static_cast<int32_t>(bufferLen / byteRate);
                int32_t outRate = numSamples;
                if (bytesRead != readLength)
                {
                    inRate = _format.freq;
                    outRate = _format.freq * (1 / rate);
                }
                _effectBuffer.resize(length);
                bufferLen = ApplyResample(
                    channel, buffer, static_cast<int32_t>(bufferLen / byteRate), numSamples, inRate, outRate);
                buffer = _effectBuffer.data();
            }

            // Apply panning and volume
            ApplyPan(channel, buffer, bufferLen, byteRate);
            int32_t mixVolume = ApplyVolume(channel, buffer, bufferLen);

            // Finally mix on to destination buffer
            size_t dstLength = std::min(length, bufferLen);
            SDL_MixAudioFormat(
                data, static_cast<const uint8_t*>(buffer), _format.format, static_cast<uint32_t>(dstLength), mixVolume);

            channel->UpdateOldVolume();
        }

        /**
         * Resample the given buffer into _effectBuffer.
         * Assumes that srcBuffer is the same format as _format.
         */
        size_t ApplyResample(
            ISDLAudioChannel* channel, const void* srcBuffer, int32_t srcSamples, int32_t dstSamples, int32_t inRate,
            int32_t outRate)
        {
            int32_t byteRate = _format.GetByteRate();

            // Create resampler
            SpeexResamplerState* resampler = channel->GetResampler();
            if (resampler == nullptr)
            {
                resampler = speex_resampler_init(_format.channels, _format.freq, _format.freq, 0, nullptr);
                channel->SetResampler(resampler);
            }
            speex_resampler_set_rate(resampler, inRate, outRate);

            uint32_t inLen = srcSamples;
            uint32_t outLen = dstSamples;
            speex_resampler_process_interleaved_int(
                resampler, static_cast<const spx_int16_t*>(srcBuffer), &inLen,
                reinterpret_cast<spx_int16_t*>(_effectBuffer.data()), &outLen);

            return outLen * byteRate;
        }

        void ApplyPan(const IAudioChannel* channel, void* buffer, size_t len, size_t sampleSize)
        {
            if (channel->GetPan() != 0.5f && _format.channels == 2)
            {
                switch (_format.format)
                {
                    case AUDIO_S16SYS:
                        EffectPanS16(channel, static_cast<int16_t*>(buffer), static_cast<int32_t>(len / sampleSize));
                        break;
                    case AUDIO_U8:
                        EffectPanU8(channel, static_cast<uint8_t*>(buffer), static_cast<int32_t>(len / sampleSize));
                        break;
                }
            }
        }

        int32_t ApplyVolume(const IAudioChannel* channel, void* buffer, size_t len)
        {
            float volumeAdjust = _volume;
            volumeAdjust *= gConfigSound.master_sound_enabled ? (gConfigSound.master_volume / 100.0f) : 0;

            switch (channel->GetGroup())
            {
                case MIXER_GROUP_SOUND:
                    volumeAdjust *= _adjustSoundVolume;

                    // Cap sound volume on title screen so music is more audible
                    if (gScreenFlags & SCREEN_FLAGS_TITLE_DEMO)
                    {
                        volumeAdjust = std::min(volumeAdjust, 0.75f);
                    }
                    break;
                case MIXER_GROUP_RIDE_MUSIC:
                    volumeAdjust *= _adjustMusicVolume;
                    break;
            }

            int32_t startVolume = static_cast<int32_t>(channel->GetOldVolume() * volumeAdjust);
            int32_t endVolume = static_cast<int32_t>(channel->GetVolume() * volumeAdjust);
            if (channel->IsStopping())
            {
                endVolume = 0;
            }

            int32_t mixVolume = static_cast<int32_t>(channel->GetVolume() * volumeAdjust);
            if (startVolume != endVolume)
            {
                // Set to max since we are adjusting the volume ourselves
                mixVolume = MIXER_VOLUME_MAX;

                // Fade between volume levels to smooth out sound and minimize clicks from sudden volume changes
                int32_t fadeLength = static_cast<int32_t>(len) / _format.BytesPerSample();
                switch (_format.format)
                {
                    case AUDIO_S16SYS:
                        EffectFadeS16(static_cast<int16_t*>(buffer), fadeLength, startVolume, endVolume);
                        break;
                    case AUDIO_U8:
                        EffectFadeU8(static_cast<uint8_t*>(buffer), fadeLength, startVolume, endVolume);
                        break;
                }
            }
            return mixVolume;
        }

        static void EffectPanS16(const IAudioChannel* channel, int16_t* data, int32_t length)
        {
            const float dt = 1.0f / (length * 2);
            float volumeL = channel->GetOldVolumeL();
            float volumeR = channel->GetOldVolumeR();
            const float d_left = dt * (channel->GetVolumeL() - channel->GetOldVolumeL());
            const float d_right = dt * (channel->GetVolumeR() - channel->GetOldVolumeR());

            for (int32_t i = 0; i < length * 2; i += 2)
            {
                data[i] = static_cast<int16_t>(data[i] * volumeL);
                data[i + 1] = static_cast<int16_t>(data[i + 1] * volumeR);
                volumeL += d_left;
                volumeR += d_right;
            }
        }

        static void EffectPanU8(const IAudioChannel* channel, uint8_t* data, int32_t length)
        {
            float volumeL = channel->GetVolumeL();
            float volumeR = channel->GetVolumeR();
            float oldVolumeL = channel->GetOldVolumeL();
            float oldVolumeR = channel->GetOldVolumeR();

            for (int32_t i = 0; i < length * 2; i += 2)
            {
                float t = static_cast<float>(i) / (length * 2);
                data[i] = static_cast<uint8_t>(data[i] * ((1.0 - t) * oldVolumeL + t * volumeL));
                data[i + 1] = static_cast<uint8_t>(data[i + 1] * ((1.0 - t) * oldVolumeR + t * volumeR));
            }
        }

        static void EffectFadeS16(int16_t* data, int32_t length, int32_t startvolume, int32_t endvolume)
        {
            static_assert(SDL_MIX_MAXVOLUME == MIXER_VOLUME_MAX, "Max volume differs between OpenRCT2 and SDL2");

            float startvolume_f = static_cast<float>(startvolume) / SDL_MIX_MAXVOLUME;
            float endvolume_f = static_cast<float>(endvolume) / SDL_MIX_MAXVOLUME;
            for (int32_t i = 0; i < length; i++)
            {
                float t = static_cast<float>(i) / length;
                data[i] = static_cast<int16_t>(data[i] * ((1 - t) * startvolume_f + t * endvolume_f));
            }
        }

        static void EffectFadeU8(uint8_t* data, int32_t length, int32_t startvolume, int32_t endvolume)
        {
            static_assert(SDL_MIX_MAXVOLUME == MIXER_VOLUME_MAX, "Max volume differs between OpenRCT2 and SDL2");

            float startvolume_f = static_cast<float>(startvolume) / SDL_MIX_MAXVOLUME;
            float endvolume_f = static_cast<float>(endvolume) / SDL_MIX_MAXVOLUME;
            for (int32_t i = 0; i < length; i++)
            {
                float t = static_cast<float>(i) / length;
                data[i] = static_cast<uint8_t>(data[i] * ((1 - t) * startvolume_f + t * endvolume_f));
            }
        }

        bool Convert(SDL_AudioCVT* cvt, const void* src, size_t len)
        {
            // tofix: there seems to be an issue with converting audio using SDL_ConvertAudio in the callback vs preconverted,
            // can cause pops and static depending on sample rate and channels
            bool result = false;
            if (len != 0 && cvt->len_mult != 0)
            {
                size_t reqConvertBufferCapacity = len * cvt->len_mult;
                _convertBuffer.resize(reqConvertBufferCapacity);
                std::copy_n(static_cast<const uint8_t*>(src), len, _convertBuffer.data());

                cvt->len = static_cast<int32_t>(len);
                cvt->buf = static_cast<uint8_t*>(_convertBuffer.data());
                if (SDL_ConvertAudio(cvt) >= 0)
                {
                    result = true;
                }
            }
            return result;
        }
    };

    IAudioMixer* AudioMixer::Create()
    {
        return new AudioMixerImpl();
    }
} // namespace OpenRCT2::Audio
