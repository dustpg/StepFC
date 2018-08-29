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

struct {

    HMODULE                                     dll_handle;

    XAudio2::Ver2_7::IXAudio2*                  xaudio2_7;
    XAudio2::Ver2_7::IXAudio2MasteringVoice*    xa_master;

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


struct sfc_ez_wave {
    XAudio2::Ver2_7::IXAudio2SourceVoice*   source;
    uint8_t                                 wave[0];
};


extern "C" void* xa2_create_clip() noexcept {
    XAudio2::WAVEFORMATEX fmt = {};
    // CD
    fmt.nSamplesPerSec = 44100;
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
    // 创建Source
    auto hr = g_xa2_data.xaudio2_7->CreateSourceVoice(
        &source,
        &fmt
    );
    // 生成波
    if (SUCCEEDED(hr)) {
        if (const auto buffer = std::malloc(bytelen)) {

            constexpr float pi = float(3.1415926536);
            constexpr float frequency = 400;

            ez_wave = reinterpret_cast<sfc_ez_wave*>(buffer);
            const auto ptr = reinterpret_cast<float*>(ez_wave->wave);
            for (uint32_t i = 0; i != countlen; ++i) {
                const auto time = float(i) / float(countlen);
                ptr[i] = std::sin(time * 2.f * pi * frequency);
            }
            XAudio2::XAUDIO2_BUFFER xbuffer = {};
            xbuffer.pAudioData = ez_wave->wave;
            xbuffer.AudioBytes = countlen * sizeof(float);
            xbuffer.LoopCount = XAudio2::XAUDIO2_LOOP_INFINITE;
            hr = source->SubmitSourceBuffer(&xbuffer);
        }
        else hr = E_OUTOFMEMORY;

    }
    // 播放
    if (SUCCEEDED(hr)) {
        hr = source->Start(0);
    }
    // 返回对象?
    if (SUCCEEDED(hr)) {
        ez_wave->source = source;
        return ez_wave;
    }
    else {
        if (source) source->DestroyVoice();
        std::free(ez_wave);
        return nullptr;
    }
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

    //const auto clip = xa2_create_clip();

    return !!SUCCEEDED(hr);
}


extern "C" void xa2_clean() noexcept {
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