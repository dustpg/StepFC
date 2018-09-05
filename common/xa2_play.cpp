#include "p_XAudio2_7.h"
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
    //XAudio2::Ver2_7::IXAudio2SubmixVoice*       xa_square;
    //XAudio2::Ver2_7::IXAudio2SubmixVoice*       xa_tnd;
    


    sfc_ez_wave*                                square1;
    sfc_square_channel_state                    square1_state;

    sfc_ez_wave*                                square2;
    sfc_square_channel_state                    square2_state;

    sfc_ez_wave*                                triangle;
    float                                       triangle_frequency;

    bool                                        square1_stop;
    bool                                        square2_stop;
    bool                                        triangle_stop;


    sfc_ez_wave*                                noise_l;
    sfc_ez_wave*                                noise_s;
    uint32_t                                    noise_data;
    bool                                        noise_l_stop;
    bool                                        noise_s_stop;

    sfc_ez_wave*                                dmc;

    //sfc_ez_wave*                                test;
} g_xa2_data;

static const float NOSIE_FLIST[] = {
#if 0
    178977.30f,
    99431.83f,
    52640.38f,
    27117.77f,
    13767.48f,
    9225.63f,
    6937.10f,
    5558.30f,
    4408.31f,
    3509.36f,
    2348.78f,
    1758.13f,
    1172.85f,
    879.93f,
    439.75f,
    219.93f,
#else
    223721.625f,
    111860.812f,
    55930.406f,
    27965.203f,
    13982.602f,
    9321.734f,
    6991.301f,
    5593.041f,
    4430.131f,
    3523.175f,
    2354.964f,
    1761.588f,
    1174.392f,
    880.794f,
    439.964f,
    219.982f,
#endif
};

const static int timer_period[] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

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

//struct SFCTriangleCallback : XAudio2::Ver2_7::IXAudio2VoiceCallback {
//    STDMETHOD_(void, OnVoiceProcessingPassStart) (UINT32 BytesRequired) {
//    }
//    STDMETHOD_(void, OnVoiceProcessingPassEnd) () {
//    }
//    STDMETHOD_(void, OnStreamEnd) () {
//    }
//    STDMETHOD_(void, OnBufferStart) (void* pBufferContext) {
//    }
//    STDMETHOD_(void, OnBufferEnd) (void* pBufferContext) {
//    }
//    STDMETHOD_(void, OnLoopEnd) (void* pBufferContext) {
//    }
//    STDMETHOD_(void, OnVoiceError) (void* pBufferContext, HRESULT Error) {
//    }
//};


//SFCCallback* g_callback = nullptr;


static inline float frac(float value) {
    return value - (float)((long)value);
}

void make_square_wave_ex(
    float buf[],
    uint32_t len,
    float duty,
    float value) {

    for (uint32_t i = 0; i != len; ++i) {
        const float time = (float)i / (float)len;
        buf[i] = frac(time) >= duty ? value : -value;
    }
}

void make_triangle_wave_ex(
    float buf[],
    uint32_t len,
    float value) {

    for (uint32_t i = 0; i != len; ++i) {
        const float index = (float)i;
        const float time = index / len;

        const float now = frac(time) * 2.f;
        //buf[i] = value * (now >= 1.f ? (now * 2.f - 3.f) : (1.f - now * 2.f));
        if (now <= 0.5f)
            buf[i] = now * 2.f * value;
        else if (now > 1.5f)
            buf[i] = (now - 2.f) * 2.f * value;
        else
            buf[i] = (2.f - now * 2.f)* value;
    }
}

uint16_t lfsr_long(uint16_t v) noexcept {
    const uint16_t a = v & 1;
    const uint16_t b = (v >> 1) & 1;
    return uint16_t(v >> 1) | uint16_t((a ^ b) << 14);
}

uint16_t lfsr_short(uint16_t v) noexcept {
    const uint16_t a = v & 1;
    const uint16_t b = (v >> 6) & 1;
    return uint16_t(v >> 1) | uint16_t((a ^ b) << 14);
}

void make_noise_short(
    float buf[],
    uint32_t len,
    float value) {
    uint16_t lfsr = 1;
    for (uint32_t i = 0; i != len; ++i) {
        lfsr = lfsr_short(lfsr);
        buf[i] = (lfsr & 1) ? value : -value;
    }
}

void make_noise_long(
    float buf[],
    uint32_t len,
    float value) {
    uint16_t lfsr = 1;
    for (uint32_t i = 0; i != len; ++i) {
        lfsr = lfsr_long(lfsr);
        buf[i] = (lfsr & 1) ? value : -value;
    }
}

enum {
    SAMPLE_PER_SEC = 44100,
    BASE_FREQUENCY = 220,
};


/*
                          95.88
    square_out = -----------------------
                        8128
                 ----------------- + 100
                 square1 + square2


                          159.79
    tnd_out = ------------------------------
                          1
              ------------------------ + 100
              triangle   noise    dmc
              -------- + ----- + -----
                8227     12241   22638

*/

constexpr float base_quare_vol = 95.88f / (8128.f/15.f+100.f);
constexpr float base_triangle_vol = 159.79f / (100.f+1.f/(15.f/8227.f));
constexpr float base_noise_vol = 159.79f / (100.f + 1.f / (15.f / 12241.f));

//extern "C" void play_240() noexcept;




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

    fmt.cbSize = sizeof(fmt);
    fmt.wBitsPerSample = fmt.nBlockAlign * 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;

    // get length of buffer
    const uint32_t countlen = BASE_FREQUENCY * 4;
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

            constexpr uint32_t length_unit = BASE_FREQUENCY;
            make_square_wave_ex(ptr + length_unit * 0, length_unit, 0.875f, base_quare_vol);
            make_square_wave_ex(ptr + length_unit * 1, length_unit, 0.750f, base_quare_vol);
            make_square_wave_ex(ptr + length_unit * 2, length_unit, 0.500f, base_quare_vol);
            make_square_wave_ex(ptr + length_unit * 3, length_unit, 0.250f, base_quare_vol);
        }
        else hr = E_OUTOFMEMORY;
    }
    // 创建Source
    if (SUCCEEDED(hr)) {
        hr = g_xa2_data.xaudio2_7->CreateSourceVoice(
            &source,
            &fmt, 
            0,
            100.f
            //, g_callback
        );
    }
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


HRESULT xa2_create_clip_tri(sfc_ez_wave** output) noexcept {
    XAudio2::WAVEFORMATEX fmt = {};
    // CD
    fmt.nSamplesPerSec = SAMPLE_PER_SEC;
    // a block = a float var
    fmt.nBlockAlign = sizeof(float);
    // one channel
    fmt.nChannels = 1;
    // single float
    fmt.wFormatTag = Wave_IEEEFloat;


    fmt.cbSize = sizeof(fmt);
    fmt.wBitsPerSample = fmt.nBlockAlign * 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;

    // get length of buffer
    const uint32_t countlen = BASE_FREQUENCY;
    const uint32_t bytelen = countlen * sizeof(float) + sizeof(sfc_ez_wave);


    XAudio2::Ver2_7::IXAudio2SourceVoice* source = nullptr;
    sfc_ez_wave* ez_wave = nullptr;
    // 生成波


    constexpr float frequency = BASE_FREQUENCY;

    HRESULT hr = S_OK;

    // 生成波
    if (SUCCEEDED(hr)) {
        if (const auto buffer = std::malloc(bytelen)) {
            ez_wave = reinterpret_cast<sfc_ez_wave*>(buffer);
            const auto ptr = reinterpret_cast<float*>(ez_wave->wave);

            constexpr uint32_t length_unit = BASE_FREQUENCY;
            make_triangle_wave_ex(ptr, length_unit, base_triangle_vol);
        }
        else hr = E_OUTOFMEMORY;
    }
    // 创建Source
    if (SUCCEEDED(hr)) {
        hr = g_xa2_data.xaudio2_7->CreateSourceVoice(
            &source,
            &fmt,
            0,
            100.f
            //, g_callback
        );
    }
    // 提交缓冲区
    if (SUCCEEDED(hr)) {
        constexpr uint32_t length_unit = BASE_FREQUENCY * sizeof(float);

        XAudio2::XAUDIO2_BUFFER xbuffer = {};
        xbuffer.Flags = XAudio2::XAUDIO2_END_OF_STREAM;
        xbuffer.pAudioData = ez_wave->wave;
        xbuffer.AudioBytes = length_unit;
        xbuffer.LoopCount = XAudio2::XAUDIO2_LOOP_INFINITE;
        hr = source->SubmitSourceBuffer(&xbuffer);
    }
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


HRESULT xa2_create_clip_noi(sfc_ez_wave** output, int mode) noexcept {
    XAudio2::WAVEFORMATEX fmt = {};
    // CD
    fmt.nSamplesPerSec = SAMPLE_PER_SEC;
    // a block = a float var
    fmt.nBlockAlign = sizeof(float);
    // one channel
    fmt.nChannels = 1;
    // single float
    fmt.wFormatTag = Wave_IEEEFloat;


    fmt.cbSize = sizeof(fmt);
    fmt.wBitsPerSample = fmt.nBlockAlign * 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;

    constexpr uint32_t LEN = 0x8000;
    // get length of buffer
    const uint32_t countlen = LEN;
    const uint32_t bytelen = countlen * sizeof(float) + sizeof(sfc_ez_wave);


    XAudio2::Ver2_7::IXAudio2SourceVoice* source = nullptr;
    sfc_ez_wave* ez_wave = nullptr;
    // 生成波


    constexpr float frequency = BASE_FREQUENCY;


    HRESULT hr = S_OK;

    // 生成波
    if (SUCCEEDED(hr)) {
        if (const auto buffer = std::malloc(bytelen)) {
            ez_wave = reinterpret_cast<sfc_ez_wave*>(buffer);
            const auto ptr = reinterpret_cast<float*>(ez_wave->wave);
            if (mode)
                make_noise_short(ptr, LEN, base_noise_vol);
            else 
                make_noise_long(ptr, LEN, base_noise_vol);
        }
        else hr = E_OUTOFMEMORY;
    }
    // 创建Source
    if (SUCCEEDED(hr)) {
        hr = g_xa2_data.xaudio2_7->CreateSourceVoice(
            &source,
            &fmt
            //, g_callback
        );
    }
    // 提交缓冲区
    if (SUCCEEDED(hr)) {
        constexpr uint32_t length_unit = LEN * sizeof(float);
        XAudio2::XAUDIO2_BUFFER xbuffer = {};
        xbuffer.Flags = XAudio2::XAUDIO2_END_OF_STREAM;
        xbuffer.pAudioData = ez_wave->wave;
        xbuffer.AudioBytes = length_unit;
        xbuffer.LoopCount = XAudio2::XAUDIO2_LOOP_INFINITE;
        hr = source->SubmitSourceBuffer(&xbuffer);
    }
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


enum : uint32_t {
    SFC_DMC_MAX_LEN = 255 * 16 + 1 + 7
};

void xa2_dmc_decode(const uint8_t* data, uint32_t len, uint8_t init) noexcept {
    // 解码
    const auto decode_dpcm = [=](uint8_t v, uint8_t* output) noexcept {
        for (int i = 0; i != len; ++i) {
            uint8_t shifter = data[i];
            for (int j = 0; j != 8; ++j) {
                if (shifter & 1) {
                    // +2
                    if (v < 126) v += 2;
                }
                else {
                    // -2
                    if (v > 1) v -= 2;
                }
                shifter >>= 1;
                *output = v;
                ++output;
            }
        }
    };

}


HRESULT xa2_create_clip_dmc(sfc_ez_wave** output) noexcept {
    using output_t = uint8_t;
    XAudio2::WAVEFORMATEX fmt = {};
    fmt.nSamplesPerSec = 331434;
    fmt.nBlockAlign = sizeof(output_t);
    fmt.nChannels = 1;
    fmt.wFormatTag = Wave_PCM;
    fmt.cbSize = sizeof(fmt);
    fmt.wBitsPerSample = fmt.nBlockAlign * 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;


    const uint32_t countlen = SFC_DMC_MAX_LEN * 8;
    const uint32_t bytelen = countlen * sizeof(output_t) + SFC_DMC_MAX_LEN + sizeof(sfc_ez_wave);


    XAudio2::Ver2_7::IXAudio2SourceVoice* source = nullptr;
    sfc_ez_wave* ez_wave = nullptr;
    // 生成波


    HRESULT hr = S_OK;

    // 生成波
    if (SUCCEEDED(hr)) {
        if (const auto buffer = std::malloc(bytelen)) {
            ez_wave = reinterpret_cast<sfc_ez_wave*>(buffer);
            const auto ptr = reinterpret_cast<output_t*>(ez_wave->wave);
            // 解码
            //const auto decode_dpcm = [=](uint8_t v) noexcept {
            //    auto itr = ptr;
            //    const auto src = (uint8_t*)filebuffer;
            //    for (int i = 0; i != len; ++i) {
            //        uint8_t shifter = src[i];
            //        for (int j = 0; j != 8; ++j) {
            //            if (shifter & 1) {
            //                // +2
            //                if (v < 126) v += 2;
            //            }
            //            else {
            //                // -2
            //                if (v > 1) v -= 2;
            //            }
            //            shifter >>= 1;
            //            *itr = v;
            //            ++itr;
            //        }
            //    }
            //};
        }
        else hr = E_OUTOFMEMORY;
    }
    // 创建Source
    if (SUCCEEDED(hr)) {
        hr = g_xa2_data.xaudio2_7->CreateSourceVoice(
            &source,
            &fmt,
            0,
            2.f
            //, g_callback
        );
    }
    // 提交缓冲区
    //if (SUCCEEDED(hr)) {
    //    XAudio2::XAUDIO2_BUFFER xbuffer = {};
    //    xbuffer.Flags = XAudio2::XAUDIO2_END_OF_STREAM;
    //    xbuffer.pAudioData = ez_wave->wave;
    //    xbuffer.AudioBytes = countlen;
    //    xbuffer.LoopCount = XAudio2::XAUDIO2_LOOP_INFINITE;
    //    hr = source->SubmitSourceBuffer(&xbuffer);
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



extern "C" void xa2_play_square1(float frequency, uint16_t duty, uint16_t volume) noexcept {
    sfc_square_channel_state    state = {};
    state.frequency = frequency;
    state.duty = duty;
    state.volume = volume;

    auto& now64 = reinterpret_cast<uint64_t&>(state);
    auto& old64 = reinterpret_cast<uint64_t&>(g_xa2_data.square1_state);

    if (now64 == old64) return;

    //std::printf("%f - %d - %d\n", frequency, duty, volume);
    old64 = now64;



    const auto ez_wave = g_xa2_data.square1;
    const auto square = ez_wave->source;
    if (!volume) {
        square->ExitLoop();
        g_xa2_data.square1_stop = true;
        return;
    }

    if (g_xa2_data.square1_stop) {
        g_xa2_data.square1_stop = false;
        constexpr uint32_t length_unit = BASE_FREQUENCY * sizeof(float);
        XAudio2::XAUDIO2_BUFFER xbuffer = {};
        xbuffer.Flags = XAudio2::XAUDIO2_END_OF_STREAM;
        xbuffer.pAudioData = ez_wave->wave + length_unit * duty;
        xbuffer.AudioBytes = length_unit;
        xbuffer.LoopCount = XAudio2::XAUDIO2_LOOP_INFINITE;
        square->FlushSourceBuffers();
        square->SubmitSourceBuffer(&xbuffer);
    }

    //square->SetOutputVoices()

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
    if (!volume) { 
        square->ExitLoop();
        g_xa2_data.square2_stop = true;
        return;
    }

    if (g_xa2_data.square2_stop) {
        g_xa2_data.square2_stop = false;
        constexpr uint32_t length_unit = BASE_FREQUENCY * sizeof(float);
        XAudio2::XAUDIO2_BUFFER xbuffer = {};
        xbuffer.Flags = XAudio2::XAUDIO2_END_OF_STREAM;
        xbuffer.pAudioData = ez_wave->wave + length_unit * duty;
        xbuffer.AudioBytes = length_unit;
        xbuffer.LoopCount = XAudio2::XAUDIO2_LOOP_INFINITE;
        square->FlushSourceBuffers();
        square->SubmitSourceBuffer(&xbuffer);
    }

    square->SetVolume((float)volume / 15.f);
    square->SetFrequencyRatio(frequency / (float)BASE_FREQUENCY);
    square->Start();
}


extern "C" void xa2_play_triangle(float frequency) noexcept {
    if (frequency == g_xa2_data.triangle_frequency) return;
    g_xa2_data.triangle_frequency = frequency;

    const auto ez_wave = g_xa2_data.triangle;
    const auto triangle = ez_wave->source;
    // 40Hz - 10kHz认为有效
    if (frequency < 40 || frequency > 10000) { 
        triangle->ExitLoop();
        g_xa2_data.triangle_stop = true;
        return;
    }

    //std::printf("TRI: %f\n", frequency);

    if (g_xa2_data.triangle_stop) {
        g_xa2_data.triangle_stop = false;
        constexpr uint32_t length_unit = BASE_FREQUENCY * sizeof(float);
        XAudio2::XAUDIO2_BUFFER xbuffer = {};
        xbuffer.Flags = XAudio2::XAUDIO2_END_OF_STREAM;
        xbuffer.pAudioData = ez_wave->wave;
        xbuffer.AudioBytes = length_unit;
        xbuffer.LoopCount = XAudio2::XAUDIO2_LOOP_INFINITE;
        triangle->SubmitSourceBuffer(&xbuffer);
    }

    //triangle->Discontinuity();
    triangle->SetFrequencyRatio(frequency / (float)BASE_FREQUENCY);
    triangle->Start();

}

extern "C" void xa2_play_noise(uint16_t data, uint16_t volume) noexcept {
    union { uint16_t u16[2]; uint32_t u32; } udata;
    udata.u16[0] = volume;
    udata.u16[1] = data;
    if (udata.u32 == g_xa2_data.noise_data) return;
    g_xa2_data.noise_data = udata.u32;

    if (volume == 0) {
        if (!g_xa2_data.noise_l_stop)
            g_xa2_data.noise_l->source->Stop();
        if (!g_xa2_data.noise_s_stop)
            g_xa2_data.noise_s->source->Stop();
        g_xa2_data.noise_s_stop = true;
        g_xa2_data.noise_l_stop = true;
        return;
    }



    const auto ez_wave = (data & uint16_t(0x80)) 
        ? g_xa2_data.noise_s
        : g_xa2_data.noise_l
        ;
    auto& noise_stop = (data & uint16_t(0x80))
        ? g_xa2_data.noise_s_stop
        : g_xa2_data.noise_l_stop
        ;

    const auto noise = ez_wave->source;
    std::printf("NIS: 0x%02X  - @ %d\n", data, volume);

    const float ratio = SAMPLE_PER_SEC;
    const float f = NOSIE_FLIST[data & (uint16_t)0xF] / ratio;
    noise->SetFrequencyRatio(f*2.f);

    noise->SetVolume((float)volume / 15.f);

    if (noise_stop) {
        noise->Start();
        noise_stop = false;
    }
}


//extern "C" void xa2_delete_clip(void* p) noexcept {
//    const auto wave = reinterpret_cast<sfc_ez_wave*>(p);
//    if (wave) {
//        assert(wave->source);
//        wave->source->DestroyVoice();
//        std::free(wave);
//    }
//}




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

    //::CreateThread(0, 0, [](void*) ->DWORD {
    //    while (true) {
    //        play_240();
    //        Sleep(4);
    //    }
    //    return 0;
    //}, 0, 0, 0);
    
    // 创建方波#1
    if (SUCCEEDED(hr)) {
       hr = xa2_create_clip(&g_xa2_data.square1);
    }
    // 创建方波#2
    if (SUCCEEDED(hr)) {
        hr = xa2_create_clip(&g_xa2_data.square2);
    }
    // 创建三角波
    if (SUCCEEDED(hr)) {
        hr = xa2_create_clip_tri(&g_xa2_data.triangle);
    }
    // 创建噪音 Long模式
    if (SUCCEEDED(hr)) {
        hr = xa2_create_clip_noi(&g_xa2_data.noise_l, 0);
        g_xa2_data.noise_l_stop = true;
    }
    // 创建噪音 Short模式
    if (SUCCEEDED(hr)) {
        hr = xa2_create_clip_noi(&g_xa2_data.noise_s, 1);
        g_xa2_data.noise_s_stop = true;
    }
    // 创建DMC声道
    if (SUCCEEDED(hr)) {
        hr = xa2_create_clip_dmc(&g_xa2_data.dmc);
    }
    return !!SUCCEEDED(hr);
}


extern "C" void xa2_clean() noexcept {
    if (g_xa2_data.dmc) {
        g_xa2_data.dmc->source->DestroyVoice();
        std::free(g_xa2_data.dmc);
        g_xa2_data.dmc = nullptr;
    }
    if (g_xa2_data.noise_l) {
        g_xa2_data.noise_l->source->DestroyVoice();
        std::free(g_xa2_data.noise_l);
        g_xa2_data.noise_l = nullptr;
    }
    if (g_xa2_data.noise_s) {
        g_xa2_data.noise_s->source->DestroyVoice();
        std::free(g_xa2_data.noise_s);
        g_xa2_data.noise_s = nullptr;
    }
    if (g_xa2_data.triangle) {
        g_xa2_data.triangle->source->DestroyVoice();
        std::free(g_xa2_data.triangle);
        g_xa2_data.triangle = nullptr;
    }
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