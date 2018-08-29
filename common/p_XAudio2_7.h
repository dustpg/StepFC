#pragma once
#include "p_XAudio2_base.h"

namespace XAudio2 { namespace Ver2_7 {
    struct WAVEFORMATEXTENSIBLE
    {
        WAVEFORMATEX Format;          // Base WAVEFORMATEX data
        union
        {
            WORD wValidBitsPerSample; // Valid bits in each sample container
            WORD wSamplesPerBlock;    // Samples per block of audio data; valid
                                        // if wBitsPerSample=0 (but rarely used).
            WORD wReserved;           // Zero if neither case above applies.
        } Samples;
        DWORD dwChannelMask;          // Positions of the audio channels
        GUID SubFormat;               // Format identifier GUID
    } ;

    typedef WAVEFORMATEXTENSIBLE *PWAVEFORMATEXTENSIBLE, *LPWAVEFORMATEXTENSIBLE;
    typedef const WAVEFORMATEXTENSIBLE *PCWAVEFORMATEXTENSIBLE, *LPCWAVEFORMATEXTENSIBLE;

    // interface list
    struct IXAudio2;
    struct IXAudio2Voice;
    struct IXAudio2SourceVoice;
    struct IXAudio2SubmixVoice;
    struct IXAudio2MasteringVoice;
    struct IXAudio2EngineCallback;
    struct IXAudio2VoiceCallback;

    enum XAUDIO2_DEVICE_ROLE
    {
        NotDefaultDevice = 0x0,
        DefaultConsoleDevice = 0x1,
        DefaultMultimediaDevice = 0x2,
        DefaultCommunicationsDevice = 0x4,
        DefaultGameDevice = 0x8,
        GlobalDefaultDevice = 0xf,
        InvalidDeviceRole = ~GlobalDefaultDevice
    } ;
#pragma pack(push, 1)

    // Returned by IXAudio2::GetDeviceDetails
    struct XAUDIO2_DEVICE_DETAILS
    {
        WCHAR DeviceID[256];                // String identifier for the audio device.
        WCHAR DisplayName[256];             // Friendly name suitable for display to a human.
        XAUDIO2_DEVICE_ROLE Role;           // Roles that the device should be used for.
        WAVEFORMATEXTENSIBLE OutputFormat;  // The device's native PCM audio output format.
    } ;

    // Returned by IXAudio2Voice::GetVoiceDetails
    struct XAUDIO2_VOICE_DETAILS
    {
        UINT32 CreationFlags;               // Flags the voice was created with.
        UINT32 InputChannels;               // Channels in the voice's input audio.
        UINT32 InputSampleRate;             // Sample rate of the voice's input audio.
    } ;

    // Used in XAUDIO2_VOICE_SENDS below
    struct XAUDIO2_SEND_DESCRIPTOR {
        UINT32 Flags;
        IXAudio2Voice* pOutputVoice;
    };

    // Used in the voice creation functions and in IXAudio2Voice::SetOutputVoices
    struct XAUDIO2_VOICE_SENDS {
        UINT32 SendCount;
        XAUDIO2_SEND_DESCRIPTOR* pSends;
    };
#pragma pack(pop)


    interface IXAudio2 : IUnknown {
        //STDMETHOD(QueryInterface) (REFIID riid, __deref_out void** ppvInterface) PURE;
        //STDMETHOD_(ULONG, AddRef) () PURE;
        //STDMETHOD_(ULONG, Release) () PURE;

        STDMETHOD(GetDeviceCount) (UINT32* pCount) PURE;
        STDMETHOD(GetDeviceDetails) (UINT32 Index, XAUDIO2_DEVICE_DETAILS* pDeviceDetails) PURE;
        STDMETHOD(Initialize) (UINT32 Flags = (0),
            XAUDIO2_PROCESSOR XAudio2Processor = (XAUDIO2_DEFAULT_PROCESSOR)) PURE;
        STDMETHOD(RegisterForCallbacks) (IXAudio2EngineCallback* pCallback) PURE;
        STDMETHOD_(void, UnregisterForCallbacks) (IXAudio2EngineCallback* pCallback) PURE;
        STDMETHOD(CreateSourceVoice) (IXAudio2SourceVoice** ppSourceVoice,
            const WAVEFORMATEX* pSourceFormat,
            UINT32 Flags = (0),
            float MaxFrequencyRatio = (XAUDIO2_DEFAULT_FREQ_RATIO),
            IXAudio2VoiceCallback* pCallback = (NULL),
            const XAUDIO2_VOICE_SENDS* pSendList = (NULL),
            const XAUDIO2_EFFECT_CHAIN* pEffectChain = (NULL)) PURE;
        STDMETHOD(CreateSubmixVoice) (IXAudio2SubmixVoice** ppSubmixVoice,
            UINT32 InputChannels, UINT32 InputSampleRate,
            UINT32 Flags = (0), UINT32 ProcessingStage = (0),
            const XAUDIO2_VOICE_SENDS* pSendList = (NULL),
            const XAUDIO2_EFFECT_CHAIN* pEffectChain = (NULL)) PURE;

        STDMETHOD(CreateMasteringVoice) (IXAudio2MasteringVoice** ppMasteringVoice,
            UINT32 InputChannels = (XAUDIO2_DEFAULT_CHANNELS),
            UINT32 InputSampleRate = (XAUDIO2_DEFAULT_SAMPLERATE),
            UINT32 Flags = (0), UINT32 DeviceIndex = (0),
            const XAUDIO2_EFFECT_CHAIN* pEffectChain = (NULL)) PURE;
        STDMETHOD(StartEngine) () PURE;
        STDMETHOD_(void, StopEngine) () PURE;
        STDMETHOD(CommitChanges) (UINT32 OperationSet) PURE;
        STDMETHOD_(void, GetPerformanceData) (XAUDIO2_PERFORMANCE_DATA* pPerfData) PURE;
        STDMETHOD_(void, SetDebugConfiguration) (const XAUDIO2_DEBUG_CONFIGURATION* pDebugConfiguration,
            void* pReserved = (NULL)) PURE;
    };


    /**************************************************************************
    *
    * IXAudio2Voice: Base voice management interface.
    *
    **************************************************************************/

    interface IXAudio2Voice {
        STDMETHOD_(void, GetVoiceDetails) (XAUDIO2_VOICE_DETAILS* pVoiceDetails) PURE;
        STDMETHOD(SetOutputVoices) (const XAUDIO2_VOICE_SENDS* pSendList) PURE;
        STDMETHOD(SetEffectChain) (const XAUDIO2_EFFECT_CHAIN* pEffectChain) PURE;
        STDMETHOD(EnableEffect) (UINT32 EffectIndex,
            UINT32 OperationSet = (XAUDIO2_COMMIT_NOW)) PURE;
        STDMETHOD(DisableEffect) (UINT32 EffectIndex,
            UINT32 OperationSet = (XAUDIO2_COMMIT_NOW)) PURE;
        STDMETHOD_(void, GetEffectState) (UINT32 EffectIndex, BOOL* pEnabled) PURE;
        STDMETHOD(SetEffectParameters) (UINT32 EffectIndex,
            const void* pParameters,
            UINT32 ParametersByteSize,
            UINT32 OperationSet = (XAUDIO2_COMMIT_NOW)) PURE;
        STDMETHOD(GetEffectParameters) (UINT32 EffectIndex,
            void* pParameters,
            UINT32 ParametersByteSize) PURE;
        STDMETHOD(SetFilterParameters) (const XAUDIO2_FILTER_PARAMETERS* pParameters,
            UINT32 OperationSet = (XAUDIO2_COMMIT_NOW)) PURE;
        STDMETHOD_(void, GetFilterParameters) (XAUDIO2_FILTER_PARAMETERS* pParameters) PURE;

        STDMETHOD(SetOutputFilterParameters) (IXAudio2Voice* pDestinationVoice,
            const XAUDIO2_FILTER_PARAMETERS* pParameters,
            UINT32 OperationSet = (XAUDIO2_COMMIT_NOW)) PURE;
        STDMETHOD_(void, GetOutputFilterParameters) (IXAudio2Voice* pDestinationVoice,
            XAUDIO2_FILTER_PARAMETERS* pParameters) PURE;

        STDMETHOD(SetVolume) (float Volume,
            UINT32 OperationSet = (XAUDIO2_COMMIT_NOW)) PURE;
        STDMETHOD_(void, GetVolume) (float* pVolume) PURE;
        STDMETHOD(SetChannelVolumes) (UINT32 Channels, const float* pVolumes,
            UINT32 OperationSet = (XAUDIO2_COMMIT_NOW)) PURE;

        STDMETHOD_(void, GetChannelVolumes) (UINT32 Channels, float* pVolumes) PURE;

        STDMETHOD(SetOutputMatrix) (IXAudio2Voice* pDestinationVoice,
            UINT32 SourceChannels, UINT32 DestinationChannels,
            const float* pLevelMatrix,
            UINT32 OperationSet = (XAUDIO2_COMMIT_NOW)) PURE;

        STDMETHOD_(void, GetOutputMatrix) (IXAudio2Voice* pDestinationVoice,
            UINT32 SourceChannels, UINT32 DestinationChannels,
            float* pLevelMatrix) PURE;
        STDMETHOD_(void, DestroyVoice) () PURE;
    };


    /**************************************************************************
    *
    * IXAudio2SourceVoice: Source voice management interface.
    *
    **************************************************************************/

    interface IXAudio2SourceVoice : IXAudio2Voice {
        STDMETHOD(Start) (UINT32 Flags = (0), UINT32 OperationSet = (XAUDIO2_COMMIT_NOW)) PURE;
        STDMETHOD(Stop) (UINT32 Flags = (0), UINT32 OperationSet = (XAUDIO2_COMMIT_NOW)) PURE;
        STDMETHOD(SubmitSourceBuffer) (const XAUDIO2_BUFFER* pBuffer, const XAUDIO2_BUFFER_WMA* pBufferWMA = (NULL)) PURE;
        STDMETHOD(FlushSourceBuffers) () PURE;
        STDMETHOD(Discontinuity) () PURE;
        STDMETHOD(ExitLoop) (UINT32 OperationSet = (XAUDIO2_COMMIT_NOW)) PURE;
        STDMETHOD_(void, GetState) (XAUDIO2_VOICE_STATE* pVoiceState) PURE;
        STDMETHOD(SetFrequencyRatio) (float Ratio,
            UINT32 OperationSet = (XAUDIO2_COMMIT_NOW)) PURE;
        STDMETHOD_(void, GetFrequencyRatio) (float* pRatio) PURE;
        STDMETHOD(SetSourceSampleRate) (UINT32 NewSourceSampleRate) PURE;
    };


    /**************************************************************************
    *
    * IXAudio2SubmixVoice: Submixing voice management interface.
    *
    **************************************************************************/

    interface IXAudio2SubmixVoice : IXAudio2Voice {

    };


    /**************************************************************************
    *
    * IXAudio2MasteringVoice: Mastering voice management interface.
    *
    **************************************************************************/

    interface IXAudio2MasteringVoice : IXAudio2Voice {

    };

    /**************************************************************************
    *
    * IXAudio2EngineCallback: Client notification interface for engine events.
    *
    * REMARKS: Contains methods to notify the client when certain events happen
    *          in the XAudio2 engine.  This interface should be implemented by
    *          the client.  XAudio2 will call these methods via the interface
    *          pointer provided by the client when it calls XAudio2Create or
    *          IXAudio2::Initialize.
    *
    **************************************************************************/

    interface IXAudio2EngineCallback {
        STDMETHOD_(void, OnProcessingPassStart) () PURE;
        STDMETHOD_(void, OnProcessingPassEnd) () PURE;
        STDMETHOD_(void, OnCriticalError) (HRESULT Error) PURE;
    };


    /**************************************************************************
    *
    * IXAudio2VoiceCallback: Client notification interface for voice events.
    *
    * REMARKS: Contains methods to notify the client when certain events happen
    *          in an XAudio2 voice.  This interface should be implemented by the
    *          client.  XAudio2 will call these methods via an interface pointer
    *          provided by the client in the IXAudio2::CreateSourceVoice call.
    *
    **************************************************************************/

    interface IXAudio2VoiceCallback {
        STDMETHOD_(void, OnVoiceProcessingPassStart) (UINT32 BytesRequired) PURE;
        STDMETHOD_(void, OnVoiceProcessingPassEnd) () PURE;
        STDMETHOD_(void, OnStreamEnd) () PURE;
        STDMETHOD_(void, OnBufferStart) (void* pBufferContext) PURE;
        STDMETHOD_(void, OnBufferEnd) (void* pBufferContext) PURE;
        STDMETHOD_(void, OnLoopEnd) (void* pBufferContext) PURE;
        STDMETHOD_(void, OnVoiceError) (void* pBufferContext, HRESULT Error) PURE;
    };


}}