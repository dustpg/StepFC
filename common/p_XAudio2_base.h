#pragma once
#include <cstdint>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <Unknwn.h>

#pragma pack(push, 1)

namespace XAudio2 {
    // WAVEFORMATEX
    struct WAVEFORMATEX {
        uint16_t        wFormatTag;
        uint16_t        nChannels;
        uint32_t        nSamplesPerSec;
        uint32_t        nAvgBytesPerSec;
        uint16_t        nBlockAlign;
        uint16_t        wBitsPerSample;
        uint16_t        cbSize;
    };
    // XAUDIO2_FILTER_TYPE
    enum XAUDIO2_FILTER_TYPE {
        LowPassFilter,                      // Attenuates frequencies above the cutoff frequency (state-variable filter).
        BandPassFilter,                     // Attenuates frequencies outside a given range      (state-variable filter).
        HighPassFilter,                     // Attenuates frequencies below the cutoff frequency (state-variable filter).
        NotchFilter,                        // Attenuates frequencies inside a given range       (state-variable filter).
        LowPassOnePoleFilter,               // Attenuates frequencies above the cutoff frequency (one-pole filter, XAUDIO2_FILTER_PARAMETERS.OneOverQ has no effect)
        HighPassOnePoleFilter,              // Attenuates frequencies below the cutoff frequency (one-pole filter, XAUDIO2_FILTER_PARAMETERS.OneOverQ has no effect)

        XAUDIO2_DEFAULT_FILTER_TYPE = LowPassFilter
    };
    // constant list
    enum : uint32_t {
        XAUDIO2_MAX_BUFFER_BYTES = 0x80000000,
        XAUDIO2_MAX_QUEUED_BUFFERS = 64,
        XAUDIO2_MAX_BUFFERS_SYSTEM = 2,
        XAUDIO2_MAX_AUDIO_CHANNELS = 64,
        XAUDIO2_MIN_SAMPLE_RATE = 1000,
        XAUDIO2_MAX_SAMPLE_RATE = 200000,
        XAUDIO2_MAX_LOOP_COUNT = 254,
        XAUDIO2_MAX_INSTANCES = 8,

        XAUDIO2_MAX_RATIO_TIMES_RATE_XMA_MONO = 600000,
        XAUDIO2_MAX_RATIO_TIMES_RATE_XMA_MULTICHANNEL = 300000,

        XAUDIO2_COMMIT_NOW = 0,
        XAUDIO2_COMMIT_ALL = 0,
        XAUDIO2_INVALID_OPSET = static_cast<uint32_t>(-1),
        XAUDIO2_NO_LOOP_REGION = 0,
        XAUDIO2_LOOP_INFINITE = 255,
        XAUDIO2_DEFAULT_CHANNELS = 0,
        XAUDIO2_DEFAULT_SAMPLERATE = 0,

        XAUDIO2_VOICE_NOPITCH = 0x0002,        // Used in IXAudio2::CreateSourceVoice
        XAUDIO2_VOICE_NOSRC = 0x0004,        // Used in IXAudio2::CreateSourceVoice
        XAUDIO2_VOICE_USEFILTER = 0x0008,        // Used in IXAudio2::CreateSource/SubmixVoice
        XAUDIO2_PLAY_TAILS = 0x0020,        // Used in IXAudio2SourceVoice::Stop
        XAUDIO2_END_OF_STREAM = 0x0040,        // Used in XAUDIO2_BUFFER.Flags
        XAUDIO2_SEND_USEFILTER = 0x0080,        // Used in XAUDIO2_SEND_DESCRIPTOR.Flags
        XAUDIO2_VOICE_NOSAMPLESPLAYED = 0x0100,        // Used in IXAudio2SourceVoice::GetState

        XAUDIO2_QUANTUM_NUMERATOR = 1,
        XAUDIO2_QUANTUM_DENOMINATOR = 100,

        FACILITY_XAUDIO2 = 0x896,
        XAUDIO2_E_INVALID_CALL = 0x88960001,    // An API call or one of its arguments was illegal
        XAUDIO2_E_XMA_DECODER_ERROR = 0x88960002,    // The XMA hardware suffered an unrecoverable error
        XAUDIO2_E_XAPO_CREATION_FAILED = 0x88960003,    // XAudio2 failed to initialize an XAPO effect
        XAUDIO2_E_DEVICE_INVALIDATED = 0x88960004,    // An audio device became unusable (unplugged, etc)

        XAUDIO2_LOG_ERRORS = 0x0001,   // For handled errors with serious effects.
        XAUDIO2_LOG_WARNINGS = 0x0002,   // For handled errors that may be recoverable.
        XAUDIO2_LOG_INFO = 0x0004,   // Informational chit-chat (e.g. state changes).
        XAUDIO2_LOG_DETAIL = 0x0008,   // More detailed chit-chat.
        XAUDIO2_LOG_API_CALLS = 0x0010,   // Public API function entries and exits.
        XAUDIO2_LOG_FUNC_CALLS = 0x0020,   // Internal function entries and exits.
        XAUDIO2_LOG_TIMING = 0x0040,   // Delays detected and other timing data.
        XAUDIO2_LOG_LOCKS = 0x0080,   // Usage of critical sections and mutexes.
        XAUDIO2_LOG_MEMORY = 0x0100,   // Memory heap usage information.
        XAUDIO2_LOG_STREAMING = 0x1000,   // Audio streaming information.

    };

    // float
    static constexpr float XAUDIO2_MAX_VOLUME_LEVEL = 16777216.0f;
    static constexpr float XAUDIO2_MIN_FREQ_RATIO = (1 / 1024.0f);
    static constexpr float XAUDIO2_MAX_FREQ_RATIO = 1024.0f;
    static constexpr float XAUDIO2_DEFAULT_FREQ_RATIO = 2.0f;
    static constexpr float XAUDIO2_MAX_FILTER_ONEOVERQ = 1.5f;
    static constexpr float XAUDIO2_MAX_FILTER_FREQUENCY = 1.0f;
    static constexpr float XAUDIO2_DEFAULT_FILTER_FREQUENCY = XAUDIO2_MAX_FILTER_FREQUENCY;
    static constexpr float XAUDIO2_DEFAULT_FILTER_ONEOVERQ = 1.0f;
    static constexpr float XAUDIO2_QUANTUM_MS = (1000.0f * XAUDIO2_QUANTUM_NUMERATOR / XAUDIO2_QUANTUM_DENOMINATOR);

    // Used in XAudio2Create, specifies which CPU(s) to use.
    enum XAUDIO2_PROCESSOR : uint32_t {
        Processor1 = 0x00000001,
        Processor2 = 0x00000002,
        Processor3 = 0x00000004,
        Processor4 = 0x00000008,
        Processor5 = 0x00000010,
        Processor6 = 0x00000020,
        Processor7 = 0x00000040,
        Processor8 = 0x00000080,
        Processor9 = 0x00000100,
        Processor10 = 0x00000200,
        Processor11 = 0x00000400,
        Processor12 = 0x00000800,
        Processor13 = 0x00001000,
        Processor14 = 0x00002000,
        Processor15 = 0x00004000,
        Processor16 = 0x00008000,
        Processor17 = 0x00010000,
        Processor18 = 0x00020000,
        Processor19 = 0x00040000,
        Processor20 = 0x00080000,
        Processor21 = 0x00100000,
        Processor22 = 0x00200000,
        Processor23 = 0x00400000,
        Processor24 = 0x00800000,
        Processor25 = 0x01000000,
        Processor26 = 0x02000000,
        Processor27 = 0x04000000,
        Processor28 = 0x08000000,
        Processor29 = 0x10000000,
        Processor30 = 0x20000000,
        Processor31 = 0x40000000,
        Processor32 = 0x80000000,

        XAUDIO2_ANY_PROCESSOR = 0xffffffff,
        XAUDIO2_DEFAULT_PROCESSOR = Processor1,
    };
    // Returned by IXAudio2Voice::GetVoiceDetails
    struct XAUDIO2_VOICE_DETAILS {
        UINT32 CreationFlags;
        UINT32 ActiveFlags;
        UINT32 InputChannels;
        UINT32 InputSampleRate;
    };



    // Used in XAUDIO2_EFFECT_CHAIN below
    struct XAUDIO2_EFFECT_DESCRIPTOR {
        IUnknown* pEffect;
        BOOL InitialState;
        UINT32 OutputChannels;
    };

    // Used in the voice creation functions and in IXAudio2Voice::SetEffectChain
    struct XAUDIO2_EFFECT_CHAIN {
        UINT32 EffectCount;
        XAUDIO2_EFFECT_DESCRIPTOR* pEffectDescriptors;
    };

    // Used in IXAudio2Voice::Set/GetFilterParameters and Set/GetOutputFilterParameters
    struct XAUDIO2_FILTER_PARAMETERS {
        XAUDIO2_FILTER_TYPE Type;
        float Frequency;
        float OneOverQ;
    };
    // Used in IXAudio2SourceVoice::SubmitSourceBuffer
    struct XAUDIO2_BUFFER {
        UINT32 Flags;
        UINT32 AudioBytes;
        const BYTE* pAudioData;
        UINT32 PlayBegin;
        UINT32 PlayLength;
        UINT32 LoopBegin;
        UINT32 LoopLength;
        UINT32 LoopCount;
        void* pContext;
    };


    struct XAUDIO2_BUFFER_WMA {
        const UINT32* pDecodedPacketCumulativeBytes;
        UINT32 PacketCount;

    };

    // Returned by IXAudio2SourceVoice::GetState
    struct XAUDIO2_VOICE_STATE {
        void* pCurrentBufferContext;
        UINT32 BuffersQueued;
        UINT64 SamplesPlayed;
    };

    // Returned by IXAudio2::GetPerformanceData
    struct XAUDIO2_PERFORMANCE_DATA
    {
        UINT64 AudioCyclesSinceLastQuery;
        UINT64 TotalCyclesSinceLastQuery;
        UINT32 MinimumCyclesPerQuantum;
        UINT32 MaximumCyclesPerQuantum;
        UINT32 MemoryUsageInBytes;
        UINT32 CurrentLatencyInSamples;
        UINT32 GlitchesSinceEngineStarted;
        UINT32 ActiveSourceVoiceCount;
        UINT32 TotalSourceVoiceCount;
        UINT32 ActiveSubmixVoiceCount;

        UINT32 ActiveResamplerCount;
        UINT32 ActiveMatrixMixCount;


        UINT32 ActiveXmaSourceVoices;
        UINT32 ActiveXmaStreams;
    };

    // Used in IXAudio2::SetDebugConfiguration
    struct XAUDIO2_DEBUG_CONFIGURATION {
        UINT32 TraceMask;
        UINT32 BreakMask;
        BOOL LogThreadID;
        BOOL LogFileline;
        BOOL LogFunctionName;
        BOOL LogTiming;
    };
    enum AUDCLNT_SHAREMODE {
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_SHAREMODE_EXCLUSIVE
    };

    enum AUDIO_STREAM_CATEGORY {
        AudioCategory_Other = 0,
        AudioCategory_ForegroundOnlyMedia,
        AudioCategory_BackgroundCapableMedia,
        AudioCategory_Communications,
        AudioCategory_Alerts,
        AudioCategory_SoundEffects,
        AudioCategory_GameEffects,
        AudioCategory_GameMedia,
    };
}
#pragma pack(pop)