#include "p_XAudio2_7.h"
#include "xa2_interface1.h"

#include <cstring>
#include <cstdlib>
#include <cassert>
#include <cstdio>
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

struct {

    HMODULE                                     dll_handle;

    XAudio2::Ver2_7::IXAudio2*                  xaudio2_7;
    XAudio2::Ver2_7::IXAudio2MasteringVoice*    xa_master;

    XAudio2::Ver2_7::IXAudio2SourceVoice*       source;

    uintptr_t                                   counter;

} g_xa2_data;


HRESULT xa2_create_source(uint32_t nSamplesPerSec) noexcept {
    XAudio2::WAVEFORMATEX fmt = {};
    // CD
    fmt.nSamplesPerSec = nSamplesPerSec;
    // a block = a float var
    fmt.nBlockAlign = sizeof(float);
    // one channel
    fmt.nChannels = 1;
    // single float
    fmt.wFormatTag = Wave_IEEEFloat;

    fmt.cbSize = sizeof(fmt);
    fmt.wBitsPerSample = fmt.nBlockAlign * 8;
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;

    HRESULT hr = S_OK;
    // 创建Source
    if (SUCCEEDED(hr)) {
        hr = g_xa2_data.xaudio2_7->CreateSourceVoice(
            &g_xa2_data.source,
            &fmt
        );
    }
    return hr;
}

FILE* g_record_file = nullptr;

void record_to_file(const float* buffer, uint32_t len) {
    if (!g_record_file) {
        g_record_file = std::fopen("record.flt", "wb");
        if (!g_record_file) return;
    }
    std::fwrite(buffer, sizeof(float), len, g_record_file);
}

extern "C" void xa2_submit_buffer(const float* buffer, uint32_t len) noexcept {
    //record_to_file(buffer, len);
    ++g_xa2_data.counter;
    XAudio2::XAUDIO2_BUFFER xbuffer = {};
    xbuffer.pAudioData = reinterpret_cast<const BYTE*>(buffer);
    xbuffer.AudioBytes = len * sizeof(float);
    xbuffer.pContext = reinterpret_cast<void*>(g_xa2_data.counter);
    const auto hr = g_xa2_data.source->SubmitSourceBuffer(&xbuffer);
    // TODO: 错误处理
    assert(SUCCEEDED(hr));
}


extern "C" void xa2_flush_buffer() noexcept {
    //g_xa2_data.source->Stop();
    const auto hr = g_xa2_data.source->FlushSourceBuffers();
    //g_xa2_data.source->Start();
    // TODO: 错误处理
    assert(SUCCEEDED(hr));
}

SFC_EXTERN_C unsigned xa2_buffer_left() SFC_NOEXCEPT {
    XAudio2::XAUDIO2_VOICE_STATE state = {};
    g_xa2_data.source->GetState(&state);
    if (state.pCurrentBufferContext) {
        const auto counter = reinterpret_cast<uintptr_t>(state.pCurrentBufferContext);
        return g_xa2_data.counter - counter;
    }
    return 0;
}


extern "C" int xa2_init(uint32_t sample) noexcept {
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
    // 创建源
    if (SUCCEEDED(hr)) {
       hr = xa2_create_source(sample);
    }
    // 尝试开始
    if (SUCCEEDED(hr)) {
        hr = g_xa2_data.source->Start();
    }
    // 提交测试用缓存
#if 0
    if (SUCCEEDED(hr)) {
        constexpr int buflen = 882;
        float buffer[buflen] = { 0.f };
        for (int i = 0; i != buflen; ++i)
            buffer[i] = ((i / 100) & 1) ? 0.2f : 0.0f;
        ::Sleep(200);

        xa2_flush_buffer();
        xa2_submit_buffer(buffer, buflen);
        xa2_submit_buffer(buffer, buflen);
        xa2_submit_buffer(buffer, buflen);
        while (true) {
            const auto left = xa2_buffer_left();
            printf("%d\n", left);
            if (!left)
                xa2_submit_buffer(buffer, buflen);
            xa2_submit_buffer(buffer, buflen);
            ::Sleep(20);
        }


        //XAudio2::XAUDIO2_VOICE_STATE state;
        //g_xa2_data.source->GetState(&state);

        xa2_submit_buffer(buffer, buflen);
        ::Sleep(10000);
    }
#endif
    return !!SUCCEEDED(hr);
}


extern "C" void xa2_clean() noexcept {
    if (g_record_file) {
        std::fclose(g_record_file);
    }
    if (g_xa2_data.source) {
        g_xa2_data.source->DestroyVoice();
        g_xa2_data.source = nullptr;
    }
    if (g_xa2_data.xa_master) {
        g_xa2_data.xa_master->DestroyVoice();
        g_xa2_data.xa_master = nullptr;
    }
    ::SafeRelease(g_xa2_data.xaudio2_7);
    ::FreeLibrary(g_xa2_data.dll_handle);
    ::CoUninitialize();
}

