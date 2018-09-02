﻿#include "p_XAudio2_7.h"
#include "xa2_interface.h"

#include <cstring>
#include <cstdlib>
#include <cassert>
#include <cmath>




template<class Interface>
inline void SafeRelease(Interface *&pInterfaceToRelease) {
    if (pInterfaceToRelease != nullptr) {
        pInterfaceToRelease->Release();
        pInterfaceToRelease = nullptr;
    }
}

#define SFC_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    const GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

// CLSID_XAudio2
SFC_DEFINE_GUID(CLSID_XAudio2, 0x5a508685, 0xa254, 0x4fba, 0x9b, 0x82, 0x9a, 0x24, 0xb0, 0x03, 0x06, 0xaf);
// CLSID_XAudio2Debug
SFC_DEFINE_GUID(CLSID_XAudio2Debug, 0xdb05ea35, 0x0329, 0x4d4b, 0xa5, 0x3a, 0x6d, 0xea, 0xd0, 0x3d, 0x38, 0x52);
// IID_IXAudio2
SFC_DEFINE_GUID(IID_IXAudio2, 0x8bcf1f58, 0x9fe7, 0x4583, 0x8a, 0xc6, 0xe2, 0xad, 0xc4, 0x65, 0xc8, 0xbb);


struct sfc_ez_wave {
    XAudio2::Ver2_7::IXAudio2SourceVoice*   source;
    //size_t                                  callback[sizeof(SFCCallback) / sizeof(size_t)];
    uint8_t                                 wave[0];
};

struct alignas(8) sfc_square_channel_state {
    // 方波 频率 
    float       frequency;
    // 方波 音量
    uint16_t    volume;
    // 方波 占空比
    uint16_t    duty;
};

struct {

    HMODULE                                     dll_handle;

    XAudio2::Ver2_7::IXAudio2*                  xaudio2_7;
    XAudio2::Ver2_7::IXAudio2MasteringVoice*    xa_master;


    sfc_ez_wave*                                square1;
    sfc_square_channel_state                    square1_state;

    sfc_ez_wave*                                square2;
    sfc_square_channel_state                    square2_state;

} g_xa2_data;

// wave format
enum FormatWave : uint8_t {
    // unknown
    Wave_Unknown = 0,
    // pcm 
    Wave_PCM,
    // MS-ADPCM 
    Wave_MSADPCM,
    // IEEE FLOAT
    Wave_IEEEFloat,
};


//struct SFCCallback : XAudio2::Ver2_7::IXAudio2VoiceCallback {
//    uint32_t a = 0;
//    uint32_t b = 0;
//    uint32_t c = 0;
//    uint32_t d = 0;
//    uint32_t e = 0;
//    uint32_t f = 0;
//
//    STDMETHOD_(void, OnVoiceProcessingPassStart) (UINT32 BytesRequired) {
//        ++a;
//    }
//    STDMETHOD_(void, OnVoiceProcessingPassEnd) () {
//        ++b;
//    }
//    STDMETHOD_(void, OnStreamEnd) () {
//        ++c;
//    }
//    STDMETHOD_(void, OnBufferStart) (void* pBufferContext) {
//        ++d;
//    }
//    STDMETHOD_(void, OnBufferEnd) (void* pBufferContext) {
//        ++e;
//    }
//    STDMETHOD_(void, OnLoopEnd) (void* pBufferContext) {
//        ++f;
//    }
//    STDMETHOD_(void, OnVoiceError) (void* pBufferContext, HRESULT Error) {
//
//    }
//};


//SFCCallback* g_callback = nullptr;


static inline float frac(float value) {
    return value - (float)((long)value);
}

void make_square_wave(
    float buf[],
    uint32_t len,
    uint32_t samples_per_sec,
    float duty,
    float frequency,
    float value) {

    const float sps = (float)samples_per_sec;
    for (uint32_t i = 0; i != len; ++i) {
        const float index = (float)i;
        const float time = index * frequency / sps;
        buf[i] = frac(time) >= duty ? value : -value;
    }
}


void make_triangle_wave(
    float buf[],
    uint32_t len,
    uint32_t samples_per_sec,
    float frequency,
    float value) {

    const float sps = (float)samples_per_sec;
    for (uint32_t i = 0; i != len; ++i) {
        const float index = (float)i;
        const float time = index * frequency / sps;

        const float now = frac(time) * 2.f;
        buf[i] = value * (now >= 1.f ? (now - 0.5f) : (1.5f - now));
    }
}

enum {
    SAMPLE_PER_SEC = 44100,
    BASE_FREQUENCY = 880,
};

extern "C" void play_240() noexcept;


HRESULT xa2_create_clip(sfc_ez_wave** output) noexcept {
    XAudio2::WAVEFORMATEX fmt = {};
    // CD
    fmt.nSamplesPerSec = SAMPLE_PER_SEC;
    // a block = a float var
    fmt.nBlockAlign = sizeof(float);
    // one channel
    fmt.nChannels = 1;
    // single float
    fmt.wFormatTag = Wave_IEEEFloat;

    // get length of buffer
    const uint32_t countlen = fmt.nSamplesPerSec;
    const uint32_t bytelen = countlen * sizeof(float) + sizeof(sfc_ez_wave);


    XAudio2::Ver2_7::IXAudio2SourceVoice* source = nullptr;
    sfc_ez_wave* ez_wave = nullptr;
    // 生成波


    constexpr float pi = float(3.1415926536);
    constexpr float frequency = BASE_FREQUENCY;

    HRESULT hr = S_OK;

    // 生成波
    if (SUCCEEDED(hr)) {
        if (const auto buffer = std::malloc(bytelen)) {
            ez_wave = reinterpret_cast<sfc_ez_wave*>(buffer);
            const auto ptr = reinterpret_cast<float*>(ez_wave->wave);
            const auto at = 0.123f;

            constexpr uint32_t length_unit = SAMPLE_PER_SEC / 4;
            make_square_wave(ptr + length_unit * 0, length_unit, fmt.nSamplesPerSec, 0.125f, frequency, at);
            make_square_wave(ptr + length_unit * 1, length_unit, fmt.nSamplesPerSec, 0.25f, frequency, at);
            make_square_wave(ptr + length_unit * 2, length_unit, fmt.nSamplesPerSec, 0.5f, frequency, at);
            make_square_wave(ptr + length_unit * 3, length_unit, fmt.nSamplesPerSec, 0.75f, frequency, at);

        }
        else hr = E_OUTOFMEMORY;
    }
    // 创建Source
    if (SUCCEEDED(hr)) {
        hr = g_xa2_data.xaudio2_7->CreateSourceVoice(
            &source,
            &fmt
            //, 0, 2.f, g_callback
        );
    }
    // 提交片段
    //if (SUCCEEDED(hr)) {
    //    XAudio2::XAUDIO2_BUFFER xbuffer = {};
    //    xbuffer.pAudioData = ez_wave->wave;
    //    xbuffer.AudioBytes = countlen * sizeof(float);
    //    xbuffer.LoopCount = XAudio2::XAUDIO2_LOOP_INFINITE;
    //    hr = source->SubmitSourceBuffer(&xbuffer);
    //}
    //// 播放
    //if (SUCCEEDED(hr)) {
    //    hr = source->Start(0);
    //}
    // 返回对象?
    if (SUCCEEDED(hr)) {
        ez_wave->source = source;
        *output = ez_wave;
    }
    else {
        if (source) source->DestroyVoice();
        std::free(ez_wave);
    }
    return hr;
}

#include <cstdio>


extern "C" void xa2_play_square1(float frequency, uint16_t duty, uint16_t volume) noexcept {
    sfc_square_channel_state    state = {};
    state.frequency = frequency;
    state.duty = duty;
    state.volume = volume;

    auto& now64 = reinterpret_cast<uint64_t&>(state);
    auto& old64 = reinterpret_cast<uint64_t&>(g_xa2_data.square1_state);

    if (now64 == old64) return;
    old64 = now64;
    std::printf("%f\n", frequency);


    const auto ez_wave = g_xa2_data.square1;
    const auto square = ez_wave->source;
    if (!volume) { square->Stop(0);  return; }

    constexpr uint32_t length_unit = SAMPLE_PER_SEC * sizeof(float) / 4;

    XAudio2::XAUDIO2_BUFFER xbuffer = {};
    xbuffer.pAudioData = ez_wave->wave + length_unit * duty;
    xbuffer.AudioBytes = length_unit;
    xbuffer.LoopCount = XAudio2::XAUDIO2_LOOP_INFINITE;
    square->FlushSourceBuffers();
    square->SubmitSourceBuffer(&xbuffer);
    square->SetVolume((float)volume / 15.f);
    square->SetFrequencyRatio(frequency / (float)BASE_FREQUENCY);
    square->Start();
}


extern "C" void xa2_play_square2(float frequency, uint16_t duty, uint16_t volume) noexcept {
    sfc_square_channel_state    state = {};
    state.frequency = frequency;
    state.duty = duty;
    state.volume = volume;

    auto& now64 = reinterpret_cast<uint64_t&>(state);
    auto& old64 = reinterpret_cast<uint64_t&>(g_xa2_data.square2_state);

    if (now64 == old64) return;
    old64 = now64;

    const auto ez_wave = g_xa2_data.square2;
    const auto square = ez_wave->source;
    if (!volume) { square->Stop(0);  return; }

    constexpr uint32_t length_unit = SAMPLE_PER_SEC * sizeof(float) / 4;

    XAudio2::XAUDIO2_BUFFER xbuffer = {};
    xbuffer.pAudioData = ez_wave->wave + length_unit * duty;
    xbuffer.AudioBytes = length_unit;
    xbuffer.LoopCount = XAudio2::XAUDIO2_LOOP_INFINITE;
    square->FlushSourceBuffers();
    square->SubmitSourceBuffer(&xbuffer);
    square->SetVolume((float)volume / 15.f);
    square->SetFrequencyRatio(frequency / (float)BASE_FREQUENCY);
    square->Start();
}


extern "C" void xa2_delete_clip(void* p) noexcept {
    const auto wave = reinterpret_cast<sfc_ez_wave*>(p);
    if (wave) {
        assert(wave->source);
        wave->source->DestroyVoice();
        std::free(wave);
    }
}




extern "C" int xa2_init() noexcept {
    auto hr = ::CoInitialize(nullptr);
    std::memset(&g_xa2_data, 0, sizeof(g_xa2_data));
    // 载入 XAudio 2.7 的dll文件
    if (SUCCEEDED(hr)) {
        g_xa2_data.dll_handle = ::LoadLibraryW(L"XAudio2_7.dll");
        assert(g_xa2_data.dll_handle && "XAudio2_7.dll not found");
        if (!g_xa2_data.dll_handle)
            hr = E_FAIL;
    }
    // 创建XAudio2 对象
    if (SUCCEEDED(hr)) {
        using namespace XAudio2;
        using namespace XAudio2::Ver2_7;
        const auto create = [](IXAudio2** ppXAudio2, UINT32 flags, XAUDIO2_PROCESSOR XAudio2Processor) noexcept {
            IXAudio2* pXAudio2 = nullptr;
            // 创建实例
            HRESULT hr = ::CoCreateInstance(
                CLSID_XAudio2,
                nullptr,
                CLSCTX_INPROC_SERVER,
                IID_IXAudio2,
                reinterpret_cast<void**>(&pXAudio2)
            );
            // 初始化
            if (SUCCEEDED(hr)) {
                hr = pXAudio2->Initialize(flags, XAudio2Processor);
                // OK!
                if (SUCCEEDED(hr)) {
                    *ppXAudio2 = pXAudio2;
                }
                else {
                    pXAudio2->Release();
                }
            }
            return hr;
        };

        hr = create(&g_xa2_data.xaudio2_7, 0, XAUDIO2_DEFAULT_PROCESSOR);
    }
    // 创建Mastering
    if (SUCCEEDED(hr)) {
        using namespace XAudio2;
        using namespace XAudio2::Ver2_7;
        hr = g_xa2_data.xaudio2_7->CreateMasteringVoice(
            &g_xa2_data.xa_master
        );
    }

    ::CreateThread(0, 0, [](void*) ->DWORD {
        while (true) {
            play_240();
            Sleep(4);
        }
        return 0;
    }, 0, 0, 0);
    
    // 创建方波#1
    if (SUCCEEDED(hr)) {
       hr = xa2_create_clip(&g_xa2_data.square1);
    }
    // 创建方波#2
    if (SUCCEEDED(hr)) {
        hr = xa2_create_clip(&g_xa2_data.square2);
    }


    return !!SUCCEEDED(hr);
}


extern "C" void xa2_clean() noexcept {
    if (g_xa2_data.square2) {
        g_xa2_data.square2->source->DestroyVoice();
        std::free(g_xa2_data.square2);
        g_xa2_data.square2 = nullptr;
    }
    if (g_xa2_data.square1) {
        g_xa2_data.square1->source->DestroyVoice();
        std::free(g_xa2_data.square1);
        g_xa2_data.square1 = nullptr;
    }
    if (g_xa2_data.xa_master) {
        g_xa2_data.xa_master->DestroyVoice();
        g_xa2_data.xa_master = nullptr;
    }
    ::SafeRelease(g_xa2_data.xaudio2_7);
    ::FreeLibrary(g_xa2_data.dll_handle);
    ::CoUninitialize();
}



extern "C" void xa2_play() noexcept {

}