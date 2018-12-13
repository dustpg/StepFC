#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d2d1_1.h>
//#include <unknwn.h>
#include <d3d11.h>
#include <dwrite_1.h>
#include <wincodec.h>
//#include <d3dcompiler.h>
//#include <DirectXMath.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cassert>
#include <iterator>
#include <algorithm>
#include "d2d_interface3.h"

#include <mmsystem.h>

namespace FAKE_IID {

    /// <summary>
    /// The iid identifier write factory1
    /// </summary>
    const GUID IID_IDWriteFactory1 = {
        0x30572f99, 0xdac6, 0x41db,
        { 0xa1, 0x6e, 0x04, 0x86, 0x30, 0x7e, 0x60, 0x6a }
    };
    /// <summary>
    /// The iid identifier write text layout1
    /// </summary>
    const GUID IID_IDWriteTextLayout1 = {
        0x9064d822, 0x80a7, 0x465c, {
        0xa9, 0x86, 0xdf, 0x65, 0xf7, 0x8b, 0x8f, 0xeb }
    };
}

#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "windowscodecs.lib")



/// <summary>
/// 
/// </summary>
enum {
    // [2A03] 方波#1
    SFC_2A03_Square1 = 0,
    // [2A03] 方波#2
    SFC_2A03_Square2,
    // [2A03] 三角波
    SFC_2A03_Triangle,
    // [2A03] 噪声
    SFC_2A03_Noise,
    // [2A03] DMC
    SFC_2A03_DMC,
    // [MMC5] 方波#1
    SFC_MMC5_Square1,
    // [MMC5] 方波#2
    SFC_MMC5_Square2,
    // [MMC5] PCM声道
    SFC_MMC5_PCM,
    // [VRC6] 方波#1
    SFC_VRC6_Square1,
    // [VRC6] 方波#2
    SFC_VRC6_Square2,
    // [VRC6] 锯齿波
    SFC_VRC6_Saw,
    // [VRC7] FM声道#0
    SFC_VRC7_FM0,
    // [VRC7] FM声道#1
    SFC_VRC7_FM1,
    // [VRC7] FM声道#2
    SFC_VRC7_FM2,
    // [VRC7] FM声道#3
    SFC_VRC7_FM3,
    // [VRC7] FM声道#4
    SFC_VRC7_FM4,
    // [VRC7] FM声道#5
    SFC_VRC7_FM5,
    // [FDS1] 波形声道
    SFC_FDS1_Wavefrom,
    // [N163] 波形#0
    SFC_N163_Wavefrom0,
    // [N163] 波形#1
    SFC_N163_Wavefrom1,
    // [N163] 波形#2
    SFC_N163_Wavefrom2,
    // [N163] 波形#3
    SFC_N163_Wavefrom3,
    // [N163] 波形#4
    SFC_N163_Wavefrom4,
    // [N163] 波形#5
    SFC_N163_Wavefrom5,
    // [N163] 波形#6
    SFC_N163_Wavefrom6,
    // [N163] 波形#7
    SFC_N163_Wavefrom7,
    // [FME7] 声道A
    SFC_FME7_ChannelA,
    // [FME7] 声道B
    SFC_FME7_ChannelB,
    // [FME7] 声道C
    SFC_FME7_ChannelC,
    // 总声道数量
    SFC_CHANNEL_COUNT
};

struct SFCChannelInfo {
    IDWriteTextLayout*      chn_name;
    IDWriteTextLayout*      freqnote;
    IDWriteTextLayout*      volinfo;
    float                   freq;

    uint16_t                volume;

    void Release();
    void UpdateFreq(float, unsigned offset = 0);
    void UpdateVol(unsigned i, uint16_t);
    HRESULT MakeChnName(const wchar_t*, uint32_t);
};


enum {
    vrc7_text_custom = 0,
    vrc7_text_a = 16,
    vrc7_text_d,
    vrc7_text_s,
    vrc7_text_r,
    vrc7_text_am,
    vrc7_text_fm,
    vrc7_text_count
};


struct alignas(sizeof(float)*4) GlobalData {
    uint32_t                scale_fac;
    uint32_t                shader_length;
    uint8_t*                shader_buffer;


    IWICImagingFactory*     wic_factory;

    IDXGISwapChain*         swap_chain;
    ID3D11Device*           device;
    ID3D11DeviceContext*    device_context;

    ID2D1Factory1*          d2d_factory;
    ID2D1Device*            d2d_device;
    ID2D1DeviceContext*     d2d_context;
    ID2D1Bitmap1*           d2d_target;
    ID2D1Bitmap1*           d2d_bg;
    ID2D1Bitmap1*           d2d_res;
    ID2D1SolidColorBrush*   d2d_brush;
    ID2D1SolidColorBrush*   d2d_white;
    ID2D1SolidColorBrush*   d2d_grey;
    ID2D1SolidColorBrush*   d2d_common;
    ID2D1SolidColorBrush*   d2d_red;
    ID2D1SolidColorBrush*   d2d_6cf;

    IDWriteFactory1*        dw_factory;
    IDWriteTextFormat*      dw_basetf;
    IDWriteTextLayout*      dw_frequnit;

    IDWriteTextLayout*      vrc7_name[vrc7_text_count];

    IDWriteTextLayout*      fme7_tone;
    IDWriteTextLayout*      fme7_noise;
    IDWriteTextLayout*      fme7_env;
    IDWriteTextLayout*      fme7_s5b_noview;
    D2D1_POINT_2F           dw_unit_offset;
    D2D1_POINT_2F           s5b_noview_offset;

    IDWriteTextLayout*      noise_mode0;
    IDWriteTextLayout*      noise_mode1;
    IDWriteTextLayout*      dmc_dpcm;


    ID2D1Effect*            d2d_effect;
    ID2D1Image*             d2d_output;


    uint32_t                time_tick;
    float                   float_time;
    float                   time_per_frame;



    SFCChannelInfo          infos[SFC_CHANNEL_COUNT];

    const sfc_visualizers_t*visualizer_data;
    const float*            channel_buffer;
    const float*            n163_wavtable;
    const float*            fds1_wavtable;
    const float*            vrc7_wavtable;
    
    unsigned                visualizer_len;
    unsigned                channel_spf;

    unsigned                vrc7_tablelen;


    void*                   timelines;

    uint8_t                 all_keyboard_mask[SFC_CHANNEL_COUNT];

} g_data = { 1, 0 };



// 自定义 顺序
const unsigned d2d_custom_index[] = {
    SFC_2A03_Square1,
    SFC_2A03_Square2,
    SFC_2A03_Triangle,
    SFC_2A03_Noise,
    SFC_2A03_DMC,
    SFC_VRC6_Square1,
    SFC_VRC6_Square2,
    SFC_VRC6_Saw,
    SFC_VRC7_FM0,
    SFC_VRC7_FM1,
    SFC_VRC7_FM2,
    SFC_VRC7_FM3,
    SFC_VRC7_FM4,
    SFC_VRC7_FM5,
    SFC_FDS1_Wavefrom,
    SFC_MMC5_Square1,
    SFC_MMC5_Square2,
    SFC_MMC5_PCM,
    SFC_N163_Wavefrom7,
    SFC_N163_Wavefrom6,
    SFC_N163_Wavefrom5,
    SFC_N163_Wavefrom4,
    SFC_N163_Wavefrom3,
    SFC_N163_Wavefrom2,
    SFC_N163_Wavefrom1,
    SFC_N163_Wavefrom0,
    SFC_FME7_ChannelA,
    SFC_FME7_ChannelB,
    SFC_FME7_ChannelC,
};


// {B8AF3834-4CBE-47BF-8ECD-1F6CF0A9D43A}
static const GUID CLSID_DustPG_FamicomAE = {
    0xb8af3834, 0x4cbe, 0x47bf, { 0x8e, 0xcd, 0x1f, 0x6c, 0xf0, 0xa9, 0xd4, 0x3a }
};


enum { WINDOW_WIDTH = 1280, WINDOW_HEIGHT = 720 };
enum { FONT_SIZE = 14 };
static const wchar_t WINDOW_TITLE[] = L"D2D Draw";
//static bool doit = true;

LRESULT CALLBACK ThisWndProc(HWND , UINT , WPARAM , LPARAM ) noexcept;
void DoRender(uint32_t sync) noexcept;
bool InitD3D(HWND, const char* res) noexcept;
void ClearD3D() noexcept;
void DoVisualizer() noexcept;
void Resize(HWND) noexcept;
auto FamicomAE__Register(ID2D1Factory1* factory) noexcept->HRESULT;
auto LoadBitmapFromMemory(
    ID2D1DeviceContext* pRenderTarget,
    IWICImagingFactory* pIWICFactory,
    uint8_t* buf,
    size_t len,
    ID2D1Bitmap1** ppBitmap
) noexcept->HRESULT;

void TickTime() noexcept;

template<typename T> void GetShaderDataOnce(T call) {
    call(g_data.shader_buffer, g_data.shader_length);
    std::free(g_data.shader_buffer);
    g_data.shader_length = 0;
    g_data.shader_buffer = nullptr;
}

auto CreateFreqUnitTL() noexcept->HRESULT;


uint32_t GetScaleFac() noexcept {
    return g_data.scale_fac;
}

template<class Interface>
inline void SafeRelease(Interface *&pInterfaceToRelease) {
    if (pInterfaceToRelease != nullptr) {
        pInterfaceToRelease->Release();
        pInterfaceToRelease = nullptr;
    }
}

//#include <vector>

uint32_t g_sync = 1;
uint32_t g_bg_data[256 * 256 + 256];




// ----------------------------------------------------------------------------
//                                Windows API
// ----------------------------------------------------------------------------

extern "C" void main_cpp(
    const char* shader_bin_file_name,
    const char* shader_res_file_name) noexcept {
    std::memset(g_data.all_keyboard_mask, -1, sizeof(g_data.all_keyboard_mask));

    g_data.all_keyboard_mask[SFC_2A03_Noise] = 0;
    g_data.all_keyboard_mask[SFC_2A03_DMC] = 0;
    g_data.all_keyboard_mask[SFC_MMC5_PCM] = 0;

    // 目前假定为60FPS
    g_data.time_per_frame = 1.f / 60.f;
    // 读取着色器数据
    if (const auto file = std::fopen(shader_bin_file_name, "rb")) {
        std::fseek(file, 0, SEEK_END);
        const auto len = std::ftell(file);
        std::fseek(file, 0, SEEK_SET);
        if (const auto ptr = std::malloc(len)) {
            std::fread(ptr, 1, len, file);
            g_data.shader_length = len;
            g_data.shader_buffer = reinterpret_cast<uint8_t*>(ptr);
        }
        std::fclose(file);
        if (const auto substr = std::strstr(shader_bin_file_name, "x.cso")) {
            const auto ch = substr[-1];
            // 1~8倍有效
            if (ch > '0' && ch < '9')  g_data.scale_fac = ch - '0';
        }
    }
    else std::printf("file<%s> not found.\n", shader_bin_file_name);
    // DPIAware
    ::SetProcessDPIAware();
    // 注册窗口
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = ThisWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(LONG_PTR);
    wcex.hInstance = ::GetModuleHandleW(nullptr);
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"DemoWindowClass";
    wcex.hIcon = nullptr;
    ::RegisterClassExW(&wcex);
    // 计算窗口大小
    RECT window_rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    DWORD window_style = WS_OVERLAPPEDWINDOW;
    AdjustWindowRect(&window_rect, window_style, FALSE);
    window_rect.right -= window_rect.left;
    window_rect.bottom -= window_rect.top;
    window_rect.left = (::GetSystemMetrics(SM_CXFULLSCREEN) - window_rect.right) / 2;
    window_rect.top = (::GetSystemMetrics(SM_CYFULLSCREEN) - window_rect.bottom) / 2;
    // 创建窗口
    const auto hwnd = ::CreateWindowExW(
        0,
        wcex.lpszClassName, WINDOW_TITLE, window_style,
        window_rect.left, window_rect.top, window_rect.right, window_rect.bottom,
        0, 0, ::GetModuleHandleW(nullptr) , nullptr
    );
    if (!hwnd) return;
    ::ShowWindow(hwnd, SW_NORMAL);
    ::UpdateWindow(hwnd);
    if (::InitD3D(hwnd, shader_res_file_name)) {
        MSG msg = { 0 };
        while (msg.message != WM_QUIT) {
            // 获取消息
            if (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                ::TranslateMessage(&msg);
                ::DispatchMessageW(&msg);
            }
            else DoRender(g_sync);
        }
    }
    ::ClearD3D();
    return;
}

static const unsigned sc_key_map[16] = {
    // A, B, Select, Start, Up, Down, Left, Right
    'K', 'J', 'U', 'I', 'W', 'S', 'A', 'D',
    // A, B, Select, Start, Up, Down, Left, Right
    VK_NUMPAD3, VK_NUMPAD2, VK_NUMPAD5, VK_NUMPAD6,
    VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT,
};

LRESULT CALLBACK ThisWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
    switch (msg)
    {
    case WM_SIZE:
        ::Resize(hwnd);
        break;
    case WM_CLOSE:
        ::ClearD3D();
        ::DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    //case WM_RBUTTONDOWN:
    //    doit = true;
    //    return 0;
    case WM_KEYDOWN:
        if (!(lParam & LPARAM(1 << 30))) {
        case WM_KEYUP:
            const auto itr = std::find(std::begin(sc_key_map), std::end(sc_key_map), unsigned(wParam));
            if (itr != std::end(sc_key_map)) {
                const int index = itr - std::begin(sc_key_map);
                ::user_input(index, msg == WM_KEYDOWN);
            }
            else if (unsigned(wParam) == VK_F1) {
                qsave();
            }
            else if (unsigned(wParam) == VK_F2) {
                qload();
            }
            else if (unsigned(wParam) == VK_SPACE) {
                g_sync = msg != WM_KEYDOWN;
            }

        }
        return 0;
    }
    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}


SFC_EXTERN_C void d2d_submit_wave(const float* data, unsigned len) SFC_NOEXCEPT {
    if (!len) return;
    if (g_data.visualizer_len) return;

    const unsigned end = len - 1;
    const auto ctx = g_data.d2d_context;
    const auto make_point = [=](unsigned i) {
        D2D1_POINT_2F point;
        point.x = float(i) + 1.f;
        point.y = (100.f + 1.f) - (data[i] * 100.f);
        return point;
    };

    for (unsigned i = 0; i != end; ++i) {
        ctx->DrawLine(
            make_point(i), make_point(i+1),
            g_data.d2d_brush
        );
    }
}


// ----------------------------------------------------------------------------
//                                Direct2D
// ----------------------------------------------------------------------------


void DoRender(uint32_t sync) noexcept {

    // D2D
    {
        const auto ctx = g_data.d2d_context;
        ctx->BeginDraw();
        ctx->Clear(D2D1::ColorF(1.f, 1.f, 1.f, 1.f));
        ctx->SetTransform(D2D1::Matrix3x2F::Identity());

        TickTime();

        if (!g_data.visualizer_len) {

            ctx->SetTransform(
                D2D1::Matrix3x2F::Translation({ 0.f, 150.f })
            );

            if (g_data.d2d_output) {
                ctx->DrawImage(
                    g_data.d2d_output
                );
            }
            else {
                //ctx->SetTransform(D2D1::Matrix3x2F::Scale(3, 3));
                ctx->DrawBitmap(
                    g_data.d2d_bg,
                    nullptr,
                    1.f,
                    D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR
                );
            }
        }

        ctx->SetTransform(D2D1::Matrix3x2F::Identity());
        DoVisualizer();
        const auto hr1 = ctx->EndDraw();
        assert(SUCCEEDED(hr1));
        const auto hr2 = g_data.swap_chain->Present(sync, 0);
        assert(SUCCEEDED(hr2));
    }
}



void TickTime() noexcept {
    const uint32_t time = ::timeGetTime();
    const uint32_t delta = time - g_data.time_tick;
    // 没有经过一毫秒
    if (!delta) return;
    g_data.time_tick = time;
    // 超过200毫秒就算了
    if (delta > 200) return;

    g_data.float_time += float(delta) * 0.001f;

    // 超过半数就算一次
    const float half_juster = g_data.float_time < g_data.time_per_frame ? 0.5f : 0.0f;
    const long count = static_cast<long>(g_data.float_time / g_data.time_per_frame + half_juster);
    if (count > 0) {
        for (int i = 1; i != count; ++i) main_render(nullptr);
        main_render(g_bg_data);

        if (!g_data.visualizer_len) {
            const auto hr0 = g_data.d2d_bg->CopyFromMemory(nullptr, g_bg_data, 256 * 4);
            assert(SUCCEEDED(hr0));
        }
        g_data.float_time -= float(count) * g_data.time_per_frame;
    }
}


void Resize(HWND hwnd) noexcept {
    RECT rect; ::GetClientRect(hwnd, &rect);
    const uint32_t width = rect.right - rect.left;
    const uint32_t height = rect.bottom - rect.top;
    if (width && height && g_data.d2d_target) {
        const auto cur = g_data.d2d_target->GetPixelSize();
        if (cur.width == width && cur.height == height) return;
        HRESULT hr = S_OK;
        IDXGISurface* dxgibuffer = nullptr;
        g_data.d2d_context->SetTarget(nullptr);
        g_data.d2d_target->Release();
        g_data.d2d_target = nullptr;
        // 重置交换链尺寸
        if (SUCCEEDED(hr)) {
            hr = g_data.swap_chain->ResizeBuffers(
                2, width, height,
                DXGI_FORMAT_R8G8B8A8_UNORM,
                0
            );
        }
        // 利用交换链获取Dxgi表面
        if (SUCCEEDED(hr)) {
            hr = g_data.swap_chain->GetBuffer(
                0,
                IID_IDXGISurface,
                reinterpret_cast<void**>(&dxgibuffer)
            );
        }
        // 利用Dxgi表面创建位图
        if (SUCCEEDED(hr)) {
            D2D1_BITMAP_PROPERTIES1 bitmap_properties = D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
            );
            hr = g_data.d2d_context->CreateBitmapFromDxgiSurface(
                dxgibuffer,
                &bitmap_properties,
                &g_data.d2d_target
            );
            g_data.d2d_context->SetTarget(g_data.d2d_target);
        }
        // 重建失败?
        ::SafeRelease(dxgibuffer);

        // TODO: 错误处理
        assert(SUCCEEDED(hr));
    }
}

bool InitD3D(HWND hwnd, const char* res) noexcept {
    HRESULT hr = S_OK;
    IDXGIDevice1* dxgi_device = nullptr;
    IDXGISurface* dxgi_surface = nullptr;
    // 创建D3D设备与交换链
    if (SUCCEEDED(hr)) {
        // D3D11 创建flag 
        // 一定要有D3D11_CREATE_DEVICE_BGRA_SUPPORT
        // 否则创建D2D设备上下文会失败
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if !defined(NDEBUG)
        // Debug状态 有D3D DebugLayer就可以取消注释
        creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        const D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1
        };
        DXGI_SWAP_CHAIN_DESC sd = { 0 };
        sd.BufferCount = 2;
        sd.BufferDesc.Width = WINDOW_WIDTH;
        sd.BufferDesc.Height = WINDOW_HEIGHT;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hwnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        hr = ::D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            creationFlags,
            featureLevels,
            sizeof(featureLevels) / sizeof(featureLevels[0]),
            D3D11_SDK_VERSION,
            &sd,
            &g_data.swap_chain,
            &g_data.device,
            nullptr,
            &g_data.device_context
        );
    }
    // 获取后备缓存作为IDXGISurface
    if (SUCCEEDED(hr)) {
        hr = g_data.swap_chain->GetBuffer(0, IID_IDXGISurface, (void**)&dxgi_surface);
    }
    // 创建 WIC 工厂.
    if (SUCCEEDED(hr)) {
        hr = ::CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_IWICImagingFactory,
            reinterpret_cast<void**>(&g_data.wic_factory)
        );
    }
    // 创建DWrite工厂
    if (SUCCEEDED(hr)) {
        hr = ::DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED,
            FAKE_IID::IID_IDWriteFactory1,
            reinterpret_cast<IUnknown**>(&g_data.dw_factory)
        );
    }
    // 创建DWrite工厂
    if (SUCCEEDED(hr)) {
        hr = g_data.dw_factory->CreateTextFormat(
            //L"Arial" , nullptr,  
            L"Courier New", nullptr, 
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            float(FONT_SIZE - 1),
            L"",
            &g_data.dw_basetf
        );
    }
    // 创建单位
    if (SUCCEEDED(hr)) {
        g_data.dw_basetf->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        hr = ::CreateFreqUnitTL();
    }
    // 创建D2D工厂
    if (SUCCEEDED(hr)) {
        hr = ::D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED,
            IID_ID2D1Factory1,
            (void**)&g_data.d2d_factory
        );
    }
    // 创建 IDXGIDevice
    if (SUCCEEDED(hr)) {
        hr = g_data.device->QueryInterface(
            IID_IDXGIDevice1,
            reinterpret_cast<void**>(&dxgi_device)
        );
    }
    // 创建 D2D设备
    if (SUCCEEDED(hr)) {
        hr = g_data.d2d_factory->CreateDevice(dxgi_device, &g_data.d2d_device);
    }
    // 创建 D2D设备上下文
    if (SUCCEEDED(hr)) {
        hr = g_data.d2d_device->CreateDeviceContext(
            D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
            &g_data.d2d_context
        );
    }
    // 利用DXGI表面创建D2D渲染承载位图
    if (SUCCEEDED(hr)) {
        D2D1_BITMAP_PROPERTIES1 bitmap_properties = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );
        hr = g_data.d2d_context->CreateBitmapFromDxgiSurface(
            dxgi_surface,
            &bitmap_properties,
            &g_data.d2d_target
        );
    }
    // 创建纯色笔刷
    if (SUCCEEDED(hr)) {
        D2D1_BITMAP_PROPERTIES1 properties = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_NONE,
            D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)
        );
        hr = g_data.d2d_context->CreateBitmap(
            D2D1_SIZE_U{ 32 * 8, 30 * 8},
            nullptr, 0,
            &properties,
            &g_data.d2d_bg
        );
    }
    // 创建着色器资源
    if (SUCCEEDED(hr)) {
        if (const auto file = std::fopen(res, "rb")) {
            std::fseek(file, 0, SEEK_END);
            const auto len = std::ftell(file);
            std::fseek(file, 0, SEEK_SET);
            if (const auto ptr = std::malloc(len)) {
                std::fread(ptr, 1, len, file);
                hr = LoadBitmapFromMemory(
                    g_data.d2d_context,
                    g_data.wic_factory,
                    reinterpret_cast<uint8_t*>(ptr),
                    len,
                    &g_data.d2d_res
                );
                std::free(ptr);
            }
            std::fclose(file);
        }
    }
    // 创建纯色笔刷
    if (SUCCEEDED(hr)) {
        hr = g_data.d2d_context->CreateSolidColorBrush(
            D2D1::ColorF(0.f, 0.f, 0.f, 1.f),
            &g_data.d2d_brush
        );
    }
    // 创建纯色笔刷
    if (SUCCEEDED(hr)) {
        hr = g_data.d2d_context->CreateSolidColorBrush(
            D2D1::ColorF(1.f, 1.f, 1.f, 1.f),
            &g_data.d2d_white
        );
    }
    // 创建纯色笔刷
    if (SUCCEEDED(hr)) {
        hr = g_data.d2d_context->CreateSolidColorBrush(
            D2D1::ColorF(1.f, 1.f, 1.f, 1.f),
            &g_data.d2d_common
        );
    }
    // 创建纯色笔刷
    if (SUCCEEDED(hr)) {
        hr = g_data.d2d_context->CreateSolidColorBrush(
            D2D1::ColorF(1.f, 0.f, 0.f, 1.f),
            &g_data.d2d_red
        );
    }
    // 创建纯色笔刷
    if (SUCCEEDED(hr)) {
        hr = g_data.d2d_context->CreateSolidColorBrush(
            D2D1::ColorF(0x66ccff),
            &g_data.d2d_6cf
        );
    }
    // 创建纯色笔刷
    if (SUCCEEDED(hr)) {
        hr = g_data.d2d_context->CreateSolidColorBrush(
            D2D1::ColorF(0.5f, 0.5f, 0.5f, 1.f),
            &g_data.d2d_grey
        );
    }
    // 注册特效
    if (SUCCEEDED(hr)) {
        hr = FamicomAE__Register(g_data.d2d_factory);
    }
    // 创建特效
    if (SUCCEEDED(hr)) {
        const auto result = g_data.d2d_context->CreateEffect(
            CLSID_DustPG_FamicomAE,
            &g_data.d2d_effect
        );
        // 成功的话
        if (SUCCEEDED(result)) {
            g_data.d2d_effect->GetOutput(&g_data.d2d_output);
            g_data.d2d_effect->SetInput(0, g_data.d2d_bg);
            if (g_data.d2d_res)
                g_data.d2d_effect->SetInput(1, g_data.d2d_res);
        }
    }
    // 设置为输出目标
    if (SUCCEEDED(hr)) {
        g_data.d2d_context->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
        g_data.d2d_context->SetTarget(g_data.d2d_target);
    }
    ::SafeRelease(dxgi_surface);
    ::SafeRelease(dxgi_device);
    return SUCCEEDED(hr);
}

void ClearD3D() noexcept {
    ::SafeRelease(g_data.d2d_res);
    ::SafeRelease(g_data.d2d_effect);
    ::SafeRelease(g_data.d2d_output);
    
    ::SafeRelease(g_data.d2d_brush);
    ::SafeRelease(g_data.d2d_white);
    ::SafeRelease(g_data.d2d_common);
    ::SafeRelease(g_data.d2d_red);
    ::SafeRelease(g_data.d2d_6cf);
    ::SafeRelease(g_data.d2d_grey);
    
    ::SafeRelease(g_data.d2d_bg);
    ::SafeRelease(g_data.d2d_target); 
    ::SafeRelease(g_data.d2d_context);
    ::SafeRelease(g_data.d2d_device);
    ::SafeRelease(g_data.d2d_factory);
    
    ::SafeRelease(g_data.device_context);
    ::SafeRelease(g_data.device);
    ::SafeRelease(g_data.swap_chain);


    for (auto& x : g_data.infos) {
        x.Release();
    }

    ::SafeRelease(g_data.dw_frequnit);
    ::SafeRelease(g_data.fme7_tone);
    ::SafeRelease(g_data.fme7_noise);
    ::SafeRelease(g_data.fme7_env);
    ::SafeRelease(g_data.fme7_s5b_noview);
    ::SafeRelease(g_data.noise_mode0);
    ::SafeRelease(g_data.noise_mode1);
    ::SafeRelease(g_data.dmc_dpcm);

    for (auto& x : g_data.vrc7_name)
        ::SafeRelease(x);

    ::SafeRelease(g_data.dw_basetf);
    ::SafeRelease(g_data.dw_factory);
    ::SafeRelease(g_data.wic_factory);
    
}


// ----------------------------------------------------------------------------
//                                图像滤镜
// ----------------------------------------------------------------------------

#include <d2d1effectauthor.h>
#include <atomic>

// FC后期特效
class FamicomAE final : public ID2D1EffectImpl, public ID2D1DrawTransform {
public:
    // ID2D1EffectImpl
    IFACEMETHODIMP Initialize(ID2D1EffectContext* pContextInternal, ID2D1TransformGraph* pTransformGraph) noexcept override;
    IFACEMETHODIMP PrepareForRender(D2D1_CHANGE_TYPE changeType) noexcept override;
    IFACEMETHODIMP SetGraph(ID2D1TransformGraph* pGraph) noexcept override { assert("unsupported!"); return E_NOTIMPL; }
    // IUnknown
    IFACEMETHODIMP_(ULONG) AddRef() noexcept override { return ++m_cRef; }
    IFACEMETHODIMP_(ULONG) Release() noexcept override;
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppOutput) noexcept override;
    // ID2D1Transform
    IFACEMETHODIMP MapInputRectsToOutputRect(const D2D1_RECT_L* pInputRects,
        const D2D1_RECT_L* pInputOpaqueSubRects,
        UINT32 inputRectCount,
        D2D1_RECT_L* pOutputRect,
        D2D1_RECT_L* pOutputOpaqueSubRect) noexcept override;
    IFACEMETHODIMP MapOutputRectToInputRects(const D2D1_RECT_L* pOutputRect,
        D2D1_RECT_L* pInputRects,
        UINT32 inputRectCount) const noexcept override;
    IFACEMETHODIMP MapInvalidRect(UINT32 inputIndex,
        D2D1_RECT_L invalidInputRect,
        D2D1_RECT_L* pInvalidOutputRect) const noexcept override;
    // ID2D1TransformNode
    IFACEMETHODIMP_(UINT32) GetInputCount() const noexcept override { return 2; }
    // ID2D1DrawTransform
    IFACEMETHODIMP SetDrawInfo(ID2D1DrawInfo *pDrawInfo) noexcept override;
public:
    // 构造函数
    FamicomAE() noexcept { m_cRef = 1; }
    // 析构函数
    ~FamicomAE() noexcept { ::SafeRelease(m_pDrawInfo); }
private:
    // 刻画信息
    ID2D1DrawInfo*              m_pDrawInfo = nullptr;
    // 引用计数器
    std::atomic<uint32_t>       m_cRef;
    // 放大倍数
    uint32_t                    m_cScale = GetScaleFac();
    // 输入矩形
    D2D1_RECT_L                 m_inputRect = D2D1::RectL();
};


// {88A1F5A7-E47F-4240-BA79-A54C2EE53D39}
static const GUID GUID_FamicomAE_PS = { 
    0x88a1f5a7, 0xe47f, 0x4240, { 0xba, 0x79, 0xa5, 0x4c, 0x2e, 0xe5, 0x3d, 0x39 } 
};


// 注册径向模糊特效
auto FamicomAE__Register(ID2D1Factory1* factory) noexcept ->HRESULT {
    assert(factory && "bad argment");
    const WCHAR* pszXml = LR"xml(<?xml version = "1.0" ?>
<Effect>
    <Property name = "DisplayName" type = "string" value = "FamicomAE" />
    <Property name = "Author" type = "string" value = "dustpg" />
    <Property name = "Category" type = "string" value = "Transform" />
    <Property name = "Description" type = "string" value = "径向模糊" />
    <Inputs>
        <Input name = "Source" />
        <Input name = "Source" />
    </Inputs>
</Effect>
)xml";
    // 创建
    struct create_this {
        static HRESULT WINAPI call(IUnknown** effect) noexcept {
            assert(effect && "bad argment");
            ID2D1EffectImpl* obj = new(std::nothrow) FamicomAE();
            *effect = obj;
            return obj ? S_OK : E_OUTOFMEMORY;
        };
        
    };
    // 注册
    return factory->RegisterEffectFromString(
        CLSID_DustPG_FamicomAE,
        pszXml,
        nullptr, 0,
        create_this::call
    );
}

// 初始化对象 Create 创建后 会调用此方法
IFACEMETHODIMP FamicomAE::Initialize(
    ID2D1EffectContext* pEffectContext,
    ID2D1TransformGraph* pTransformGraph) noexcept {
    // 参数检查
    assert(pEffectContext && pTransformGraph && "bad arguments");
    if (!pEffectContext || !pTransformGraph) return E_INVALIDARG;
    HRESULT hr = S_FALSE;
    // 载入shader文件
    if (SUCCEEDED(hr)) {
        // 检查是否已经可以
        BYTE buf[4] = { 0 };
        hr = pEffectContext->LoadPixelShader(GUID_FamicomAE_PS, buf, 0);
        // 失败时重载
        if (FAILED(hr)) {
            GetShaderDataOnce([&hr, pEffectContext](const uint8_t* data, uint32_t len) noexcept {
                if (!data) return;
                hr = pEffectContext->LoadPixelShader(GUID_FamicomAE_PS, data, len);
            });
        }
    }
    // 连接
    if (SUCCEEDED(hr)) {
        hr = pTransformGraph->SetSingleTransformNode(this);
    }
    return hr;
}


// 准备渲染
IFACEMETHODIMP FamicomAE::PrepareForRender(D2D1_CHANGE_TYPE changeType) noexcept {
    if (changeType == D2D1_CHANGE_TYPE_NONE) return S_OK;
    m_pDrawInfo->SetPixelShader(GUID_FamicomAE_PS);
    m_pDrawInfo->SetInputDescription(0, { D2D1_FILTER_MIN_MAG_MIP_POINT, 0 });
    return S_OK;
}

// 实现 IUnknown::Release
IFACEMETHODIMP_(ULONG) FamicomAE::Release() noexcept {
    if ((--m_cRef) == 0) {
        delete this;
        return 0;
    }
    else {
        return m_cRef;
    }
}

// 实现 IUnknown::QueryInterface
IFACEMETHODIMP FamicomAE::QueryInterface(REFIID riid, _Outptr_ void** ppOutput) noexcept {
    *ppOutput = nullptr;
    HRESULT hr = S_OK;
    // 获取 ID2D1EffectImpl
    if (riid == IID_ID2D1EffectImpl) {
        *ppOutput = static_cast<ID2D1EffectImpl*>(this);
    }
    // 获取 ID2D1DrawTransform
    else if (riid == IID_ID2D1DrawTransform) {
        *ppOutput = static_cast<ID2D1DrawTransform*>(this);
    }
    // 获取 ID2D1Transform
    else if (riid == IID_ID2D1Transform) {
        *ppOutput = static_cast<ID2D1Transform*>(this);
    }
    // 获取 ID2D1TransformNode
    else if (riid == IID_ID2D1TransformNode) {
        *ppOutput = static_cast<ID2D1TransformNode*>(this);
    }
    // 获取 IUnknown
    else if (riid == IID_IUnknown) {
        *ppOutput = this;
    }
    // 没有接口
    else {
        hr = E_NOINTERFACE;
    }
    if (*ppOutput != nullptr) {
        AddRef();
    }
    return hr;
}


// 设置刻画信息
IFACEMETHODIMP FamicomAE::SetDrawInfo(_In_ ID2D1DrawInfo *drawInfo) noexcept {
    ::SafeRelease(m_pDrawInfo);
    if (drawInfo) {
        drawInfo->AddRef();
        m_pDrawInfo = drawInfo;
    }
    return S_OK;
}

// 映射无效矩形区
IFACEMETHODIMP FamicomAE::MapInvalidRect(
    UINT32 inputIndex,
    D2D1_RECT_L invalidInputRect,
    _Out_ D2D1_RECT_L* pInvalidOutputRect
) const noexcept {
    *pInvalidOutputRect = m_inputRect;
    return S_OK;
}

// 映射输出矩形到输入矩形数组
IFACEMETHODIMP FamicomAE::MapOutputRectToInputRects(
    _In_ const D2D1_RECT_L* pOutputRect,
    _Out_writes_(inputRectCount) D2D1_RECT_L* pInputRects,
    UINT32 inputRectCount
) const noexcept {
    // 虽说是数组 这里就一个
    //if (inputRectCount != 1) return E_INVALIDARG;
    // 映射
    for (uint32_t i = 0; i != inputRectCount; ++i)
        pInputRects[i] = m_inputRect;
    return S_OK;
}

// 映射输入矩形数组到输出输出矩形
IFACEMETHODIMP FamicomAE::MapInputRectsToOutputRect(
    _In_reads_(inputRectCount) const D2D1_RECT_L* pInputRects,
    _In_reads_(inputRectCount) const D2D1_RECT_L* pInputOpaqueSubRects,
    UINT32 inputRectCount,
    _Out_ D2D1_RECT_L* pOutputRect,
    _Out_ D2D1_RECT_L* pOutputOpaqueSubRect
) noexcept {
    //if (inputRectCount != 1) return E_INVALIDARG;

    *pOutputRect = pInputRects[0];
    pOutputRect->right *= m_cScale;
    pOutputRect->bottom *= m_cScale;

    m_inputRect = pInputRects[0];
    *pOutputOpaqueSubRect = *pOutputRect;
    return S_OK;
}




// 从内存文件读取位图
auto LoadBitmapFromMemory(
    ID2D1DeviceContext* pRenderTarget,
    IWICImagingFactory* pIWICFactory,
    uint8_t* buf,
    size_t len,
    ID2D1Bitmap1** ppBitmap
) noexcept -> HRESULT {
    IWICBitmapDecoder *pDecoder = nullptr;
    IWICBitmapFrameDecode *pSource = nullptr;
    IWICStream *pStream = nullptr;
    IWICFormatConverter *pConverter = nullptr;
    IWICBitmapScaler *pScaler = nullptr;

    IWICStream* stream = nullptr;
    HRESULT hr = S_OK;
    // 创建内存流
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateStream(&stream);
    }
    // 创建内存流
    if (SUCCEEDED(hr)) {
        hr = stream->InitializeFromMemory(buf, len);
    }
    // 创建解码器
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateDecoderFromStream(
            stream,
            nullptr,
            WICDecodeMetadataCacheOnLoad,
            &pDecoder
        );
    }
    if (SUCCEEDED(hr)) {
        hr = pDecoder->GetFrame(0, &pSource);
    }
    if (SUCCEEDED(hr)) {
        hr = pIWICFactory->CreateFormatConverter(&pConverter);
    }
    if (SUCCEEDED(hr)) {
        hr = pConverter->Initialize(
            pSource,
            GUID_WICPixelFormat32bppBGRA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.f,
            WICBitmapPaletteTypeMedianCut
        );
    }

    D2D1_SIZE_U size = {};
    // 获取数据
    if (SUCCEEDED(hr)) {
        hr = pConverter->GetSize(&size.width, &size.height);
    }
    const auto sizeof_rgba = static_cast<uint32_t>(sizeof(uint32_t));
    const auto bylen = size.width * size.height * sizeof_rgba;
    const auto bypch = size.width * sizeof_rgba;
    uint32_t* ptr = nullptr;
    // 申请空间
    if (SUCCEEDED(hr)) {
        ptr = (uint32_t*)std::malloc(bylen);
        if (!ptr) hr = E_OUTOFMEMORY;
    }
    // 复制数据
    if (SUCCEEDED(hr)) {
        hr = pConverter->CopyPixels(nullptr, bypch, bylen, (BYTE*)ptr);
    }
    if (SUCCEEDED(hr)) {
        D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_NONE,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            96.0f,
            96.0f
        );
        hr = pRenderTarget->CreateBitmap(
            size,
            ptr, bypch,
            &bitmapProperties,
            ppBitmap
        );
    }
    std::free(ptr);
    ::SafeRelease(stream);
    ::SafeRelease(pDecoder);
    ::SafeRelease(pSource);
    ::SafeRelease(pStream);
    ::SafeRelease(pConverter);
    ::SafeRelease(pScaler);
    return hr;
}


// ----------------------------------------------------------------------------
//                                音频可视化
// ----------------------------------------------------------------------------


enum {
    // 白键高度
    KB_WHITE_HEIGHT = 120,
    // 白键宽度
    KB_WHITE_WIDTH = 32,
    // 黑键高度
    KB_BLACK_HEIGHT = 100,
    // 黑键宽度
    KB_BLACK_WIDTH = 20,
    // 白键间隙
    KB_WHITE_SPACE = 1,
    // 黑键起始
    KB_BLACK_START = KB_WHITE_WIDTH - KB_BLACK_WIDTH / 2,
    // 一节宽度
    KB_SEC_WIDTH = KB_WHITE_WIDTH * 7,
    // 标记大小
    KB_MARK_SIZE = 12,
    // A4C4提示高度
    KB_A4C4_HINT_HEIGHT = 4,



    NOTE_C = 0,
    NOTE_CS,
    NOTE_D,
    NOTE_DS,
    NOTE_E,

    NOTE_F,
    NOTE_FS,
    NOTE_G,
    NOTE_GS,
    NOTE_A,
    NOTE_AS,
    NOTE_B,
};

struct RectWH { float x, y, w, h; };

// 布局 
const RectWH kb_layout[] = {
    // C
    { KB_WHITE_WIDTH * 0, 0, KB_WHITE_WIDTH - KB_WHITE_SPACE, KB_WHITE_HEIGHT },
    // C#
    { KB_WHITE_WIDTH * 0 + KB_BLACK_START, 0, KB_BLACK_WIDTH, KB_BLACK_HEIGHT },
    // D
    { KB_WHITE_WIDTH * 1, 0, KB_WHITE_WIDTH - KB_WHITE_SPACE, KB_WHITE_HEIGHT },
    // D#
    { KB_WHITE_WIDTH * 1 + KB_BLACK_START, 0, KB_BLACK_WIDTH, KB_BLACK_HEIGHT },
    // E
    { KB_WHITE_WIDTH * 2, 0, KB_WHITE_WIDTH - KB_WHITE_SPACE, KB_WHITE_HEIGHT },
    // F
    { KB_WHITE_WIDTH * 3, 0, KB_WHITE_WIDTH - KB_WHITE_SPACE, KB_WHITE_HEIGHT },
    // F#
    { KB_WHITE_WIDTH * 3 + KB_BLACK_START, 0, KB_BLACK_WIDTH, KB_BLACK_HEIGHT },
    // G
    { KB_WHITE_WIDTH * 4, 0, KB_WHITE_WIDTH - KB_WHITE_SPACE, KB_WHITE_HEIGHT },
    // G#
    { KB_WHITE_WIDTH * 4 + KB_BLACK_START, 0, KB_BLACK_WIDTH, KB_BLACK_HEIGHT },
    // A
    { KB_WHITE_WIDTH * 5, 0, KB_WHITE_WIDTH - KB_WHITE_SPACE, KB_WHITE_HEIGHT },
    // A#
    { KB_WHITE_WIDTH * 5 + KB_BLACK_START, 0, KB_BLACK_WIDTH, KB_BLACK_HEIGHT },
    // B
    { KB_WHITE_WIDTH * 6, 0, KB_WHITE_WIDTH - KB_WHITE_SPACE, KB_WHITE_HEIGHT },
};


// 渲染键盘一节
void RenderKBSection(float x, float y) {
    const auto ctx = g_data.d2d_context;
    const auto blk = g_data.d2d_brush;
    const auto wht = g_data.d2d_white;
    // 七枚白键
    for (const auto& rect : kb_layout) {
        if (rect.h < KB_WHITE_HEIGHT) continue;
        const D2D1_RECT_F draw = {
            x + rect.x, y + rect.y,
            x + rect.x + rect.w,
            y + rect.y + rect.h
        };
        ctx->FillRectangle(&draw, wht);
    }
    // 五枚黑键
    for (const auto& rect : kb_layout) {
        if (rect.h < KB_WHITE_HEIGHT) {

            const D2D1_RECT_F draw = {
                x + rect.x, y + rect.y,
                x + rect.x + rect.w,
                y + rect.y + rect.h
            };
            ctx->FillRectangle(&draw, blk);
        }
    }
}


__declspec(noinline)
/// <summary>
/// 以C4为基础计算键ID
/// </summary>
/// <param name="freq">The freq.</param>
/// <returns></returns>
inline int calc_key_id_c4(float freq) {
    /*
    换底公式
                 log(c, b)
    log(a, b) = ------------
                 log(c, a)
    */
    const float a4 = 440.f;
    const float code = logf(freq / a4) * 17.3123404907f + 9.f;
    //return static_cast<int>(floorf(code + 0.5f));
    const float adj = code < 0.f ? -0.5f : 0.5f;
    return int(code + adj);
}


// 渲染键盘
void RenderKeyboard(float x, float y) noexcept {
    const auto ptr = g_data.visualizer_data;
    const auto len = g_data.visualizer_len;
    const auto brush = g_data.d2d_common;


    const D2D1_RECT_F zone = { 0, y-4, 2048, 1024 };
    const D2D1_RECT_F rect = { x, y, 2048, 1024 };
    g_data.d2d_context->FillRectangle(&zone, g_data.d2d_brush);

    for (int i = -4; i != 5; ++i) {
        RenderKBSection(rect.left + float(i * KB_SEC_WIDTH), rect.top);
    }

    // 标出A4
    g_data.d2d_context->FillRectangle({ 
        rect.left + kb_layout[NOTE_A].x, rect.top + KB_WHITE_HEIGHT - KB_A4C4_HINT_HEIGHT,
        rect.left + kb_layout[NOTE_A].x + KB_WHITE_WIDTH,
        rect.top + KB_WHITE_HEIGHT }, g_data.d2d_6cf);
    // 标出C4
    g_data.d2d_context->FillRectangle({
        rect.left, rect.top + KB_WHITE_HEIGHT - KB_A4C4_HINT_HEIGHT,
        rect.left + KB_WHITE_WIDTH,
        rect.top + KB_WHITE_HEIGHT }, g_data.d2d_red);




    for (unsigned i = 0; i != len; ++i) {
        if (ptr[i].key_on & g_data.all_keyboard_mask[i]) {

            brush->SetColor(reinterpret_cast<const D2D_COLOR_F*>(ptr[i].color));
            const int id = calc_key_id_c4(ptr[i].freq);

            // 获取音符
            const int note = (id % 12 + 12) % 12;
            const int section = (id - note) / 12;


            // 获取键盘位置
            const auto bkr = kb_layout[note];
            // 确认标记位置
            const auto posy = rect.top + bkr.y + bkr.h - float(KB_MARK_SIZE);
            const auto posx = rect.left + bkr.x + bkr.w * 0.5f + float(section * KB_SEC_WIDTH);
            const D2D1_RECT_F mark = {
                posx - float(KB_MARK_SIZE) * 0.5f,
                posy - float(KB_MARK_SIZE) * 0.5f,
                posx + float(KB_MARK_SIZE) * 0.5f,
                posy + float(KB_MARK_SIZE) * 0.5f
            };
            g_data.d2d_context->FillRectangle(&mark, brush);
        }
    }
}


enum {
    // 声道详情区域高度
    CH_ZONE_HEIGHT = 90,
    // 波形窗口宽度
    WW_WIDTH = 300,
    // 波形窗口高度
    WW_HEIGHT = CH_ZONE_HEIGHT - 20,
    // 波形窗口左起始点
    WW_LSTART = 20,
    // 波形窗口单位: A1 = 55Hz A2 = 110Hz
    WW_UNIT = 110,
    // 波形窗口内间距
    WW_PADDING = WW_HEIGHT / 10,
    // 内容实际高度
    WW_CONT_HEIGHT = WW_HEIGHT - WW_PADDING * 2,
    // 波形窗口上起始点
    WW_TSTART = (CH_ZONE_HEIGHT - WW_HEIGHT) / 2,
    // 最大显示波形段数
    WW_MAX_WAVESECTION = 32,


    // 频率标记大小
    WW_MARKER_SIZE = 4
};

// 波形窗口
void DrawWaveWindow(float y) noexcept {
    // 边框
    const D2D1_RECT_F rect = {
        WW_LSTART - 0.5f, WW_TSTART - 0.5f + y,
        WW_LSTART + WW_WIDTH + 0.5f, WW_TSTART + WW_HEIGHT + y + 0.5f
    };
    g_data.d2d_context->DrawRectangle(&rect, g_data.d2d_white);
    // 上频率尺

}


static const D2D1_POINT_2F d2d_2a03_sq[] = {
    // 12.5%
    { WW_WIDTH * 0.875f, WW_WIDTH * 0.125f },
    // 25.0%
    { WW_WIDTH * 0.750f, WW_WIDTH * 0.250f },
    // 50.0%
    { WW_WIDTH * 0.500f, WW_WIDTH * 0.500f },
    // 75.0%
    { WW_WIDTH * 0.250f, WW_WIDTH * 0.750f },
};

// 2A03 方波
void Draw2A03SquareWave(float y, const sfc_visualizers_t& d) noexcept {
    const auto ctx = g_data.d2d_context;
    const auto brush = g_data.d2d_common;
    D2D1_COLOR_F color = *reinterpret_cast<const D2D_COLOR_F*>(d.color);
    color.a = 1.f;
    brush->SetColor(color);
    constexpr float offx = WW_LSTART;
    constexpr float offy = WW_TSTART + WW_CONT_HEIGHT + WW_PADDING;
    constexpr float wwww = WW_WIDTH;
    // K-ON才算
    if (d.key_on) {
        // 12.5% 25% 50% 75%
        /*
                  -
           -------
        */
        const auto data = d2d_2a03_sq[d.ud.sq_duty];
        // 缩放率
        const auto count = d.freq / float(WW_UNIT) ;
        const auto len = std::min(int(count + 1.f), int(WW_MAX_WAVESECTION));
        const auto rate = 1.f / count;
        const auto scw = rate * wwww;

        D2D1_POINT_2F p1, p2, p3, p4, p5;
        p1 = { offx, offy + y };
        p2 = { offx + data.x * rate, offy + y };
        p3 = { p2.x, offy + y - (float(d.volume) / 15.f) * float(WW_CONT_HEIGHT) };
        p4 = { p3.x + data.y * rate, p3.y };
        p5 = { p4.x, p1.y };

        ctx->PushAxisAlignedClip({
            offx, WW_PADDING +y, offx+wwww, offy + y
            }, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        for (int i = 0; i != len; ++i) {
            ctx->DrawLine(p1, p2, brush);
            ctx->DrawLine(p2, p3, brush);
            ctx->DrawLine(p3, p4, brush);
            ctx->DrawLine(p4, p5, brush);
            p1.x += scw; p2.x += scw; p3.x += scw; p4.x += scw; p5.x += scw;
        }

        ctx->PopAxisAlignedClip();
    }
    // 否则就是一条直线
    else ctx->DrawLine({ offx, offy + y }, { offx + wwww, offy + y }, brush);
}



// 2A03 三角波
void Draw2A03TriangleWave(float y, const sfc_visualizers_t& d) noexcept {
    const auto ctx = g_data.d2d_context;
    const auto brush = g_data.d2d_common;
    brush->SetColor(reinterpret_cast<const D2D_COLOR_F*>(d.color));
    constexpr float offx = WW_LSTART;
    constexpr float offy = WW_TSTART + WW_PADDING;
    constexpr float wwww = WW_WIDTH;
    // K-ON才算
    if (d.key_on) {
        /*
          \   /
           \ /
            V
        */
        // 缩放率
        const auto count = d.freq / float(WW_UNIT);
        const auto len = std::min(int(count + 1.f), int(WW_MAX_WAVESECTION));
        const auto rate = 1.f / count;
        const auto scw = rate * wwww;
        ctx->PushAxisAlignedClip({
                offx, WW_PADDING + y, offx + wwww, offy + WW_CONT_HEIGHT + y
            }, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        D2D1_POINT_2F p1, p2, p3;
        p1 = { offx, offy + y };
        p2 = { offx + scw * 0.5f, offy + WW_CONT_HEIGHT + y };
        p3 = { offx + scw, offy + y };


        for (int i = 0; i != len; ++i) {
            ctx->DrawLine(p1, p2, brush);
            ctx->DrawLine(p2, p3, brush);
            p1.x += scw; p2.x += scw; p3.x += scw;
        }

        ctx->PopAxisAlignedClip();
    }
    // 否则就是一条直线
    else {
        const float voly = (1.f - (float)d.volume / 15.f) * float(WW_CONT_HEIGHT);
        ctx->DrawLine({ offx, offy + voly + y }, { offx + wwww, offy + voly + y }, brush);
    }
}




void DrawNormalPCM(float y, const sfc_visualizers_t& d, const float *const base, float max, float half) noexcept {
    const auto ctx = g_data.d2d_context;
    const auto brush = g_data.d2d_common;
    D2D1_COLOR_F color = *reinterpret_cast<const D2D_COLOR_F*>(d.color);
    color.a = 1.f;
    brush->SetColor(color);
    const auto len = g_data.channel_spf - 1;
    D2D1_POINT_2F p1, p2;
    const float xplus = (float)WW_WIDTH / (float)g_data.channel_spf;
    p1 = { WW_LSTART, base[0] }; p2 = { WW_LSTART + xplus, base[1] };


    const auto mapper = [max, y, half](D2D1_POINT_2F pt) {
        constexpr float offy = WW_TSTART + WW_PADDING;
        constexpr float cont = WW_CONT_HEIGHT;
        pt.y = offy + y + (1.f - (pt.y / max)) * cont * half;
        return pt;
    };
    const float line_width = 1.f;

    float x = WW_LSTART + xplus;
    for (int i = 1; i != len; ++i, x+= xplus) {
        // 斜率一致则合并
        if ((base[i + 1] - base[i + 0]) == (p2.y - p1.y)) {
            p2 = { x, base[i] };
        }
        else {
            ctx->DrawLine(mapper(p1), mapper(p2), brush, line_width);
            p1 = p2;
            p2 = { x, base[i] };
        }
    }

    p2 = { x, base[len] };
    ctx->DrawLine(mapper(p1), mapper(p2), brush, line_width);
}


// VRC6 方波
void DrawVRC6SquareWave(float y, const sfc_visualizers_t& d) noexcept {
    const auto ctx = g_data.d2d_context;
    const auto brush = g_data.d2d_common;
    D2D1_COLOR_F color = *reinterpret_cast<const D2D_COLOR_F*>(d.color);
    color.a = 1.f;
    brush->SetColor(color);
    constexpr float offx = WW_LSTART;
    constexpr float offy = WW_TSTART + WW_CONT_HEIGHT + WW_PADDING;
    constexpr float wwww = WW_WIDTH;
    // K-ON才算
    if (d.key_on) {
        // 1/16 - 15/16
        if (d.ud.sq_duty < 15) {
            const float low = (float)(d.ud.sq_duty + 1) / 16.f;
            const D2D1_POINT_2F data = { wwww - low * wwww, low * wwww };
            // 缩放率
            const auto count = d.freq / float(WW_UNIT);
            const auto len = std::min(int(count + 1.f), int(WW_MAX_WAVESECTION));
            const auto rate = 1.f / count;
            const auto scw = rate * wwww;


            D2D1_POINT_2F p1, p2, p3, p4, p5;
            p1 = { offx, offy + y };
            p2 = { offx + data.x * rate, offy + y };
            p3 = { p2.x, offy + y - (float(d.volume) / 15.f) * float(WW_CONT_HEIGHT) };
            p4 = { p3.x + data.y * rate, p3.y };
            p5 = { p4.x, p1.y };

            ctx->PushAxisAlignedClip({
                offx, WW_PADDING + y, offx + wwww, offy + y
                }, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

            for (int i = 0; i != len; ++i) {
                ctx->DrawLine(p1, p2, brush);
                ctx->DrawLine(p2, p3, brush);
                ctx->DrawLine(p3, p4, brush);
                ctx->DrawLine(p4, p5, brush);
                p1.x += scw; p2.x += scw; p3.x += scw; p4.x += scw; p5.x += scw;
            }

            ctx->PopAxisAlignedClip();
        }
        // 16/16
        else {
            // 还是一条直线
            y -= (float(d.volume) / 15.f) * float(WW_CONT_HEIGHT);
            ctx->DrawLine({ offx, offy + y }, { offx + wwww, offy + y }, brush);
        }
    }
    // 否则就是一条直线
    else ctx->DrawLine({ offx, offy + y }, { offx + wwww, offy + y }, brush);
}


// VRC6 锯齿波
void DrawVRC6SawWave(float y, const sfc_visualizers_t& d) noexcept {
    const auto ctx = g_data.d2d_context;
    const auto brush = g_data.d2d_common;
    D2D1_COLOR_F color = *reinterpret_cast<const D2D_COLOR_F*>(d.color);
    color.a = 1.f;
    brush->SetColor(color);
    constexpr float offx = WW_LSTART;
    constexpr float offy = WW_TSTART + WW_PADDING;
    constexpr float wwww = WW_WIDTH;
    //printf("%f\n", d.freq);
    // K-ON才算
    if (d.key_on) {
        /*    /|
             / |
            /  | /
           /   |/
        */
        // 缩放率
        const auto count = d.freq / float(WW_UNIT);
        const auto len = std::min(int(count + 1.f), int(WW_MAX_WAVESECTION));
        const auto rate = 1.f / count;
        const auto scw = rate * wwww;
        ctx->PushAxisAlignedClip({
                offx, WW_PADDING + y, offx + wwww, offy + WW_CONT_HEIGHT + y
            }, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        const float vol = 1.f - (d.volume * 6.f / 255.f);

        D2D1_POINT_2F p1, p2, p3;
        p1 = { offx, offy + WW_CONT_HEIGHT + y };
        p2 = { offx + scw, offy + WW_CONT_HEIGHT * vol + y };
        p3 = { offx + scw, offy + WW_CONT_HEIGHT + y };


        for (int i = 0; i != len; ++i) {
            ctx->DrawLine(p1, p2, brush);
            ctx->DrawLine(p2, p3, brush);
            p1.x += scw; p2.x += scw; p3.x += scw;
        }

        ctx->PopAxisAlignedClip();
    }
    // 否则就是一条直线
    else {
        const float voly = /*(1.f - (float)d.volume / 15.f) **/ float(WW_CONT_HEIGHT);
        ctx->DrawLine({ offx, offy + voly + y }, { offx + wwww, offy + voly + y }, brush);
    }
}


// 波形表
void DrawWaveTable(
    float y, 
    const sfc_visualizers_t& d, 
    const float wt[],
    const unsigned len_raw, 
    float vmax, float wmax,
    float half
) {

    if (!len_raw) return;

    constexpr float offx = WW_LSTART;
    constexpr float offy = WW_TSTART + WW_PADDING;
    constexpr float wwww = WW_WIDTH;
    const float line_width = 1.f;
    const auto ctx = g_data.d2d_context;
    const auto brush = g_data.d2d_common;
    D2D1_COLOR_F color = *reinterpret_cast<const D2D_COLOR_F*>(d.color);
    color.a = 1.f;
    brush->SetColor(color);

    // 渲染直线
    if (!d.key_on) {
        ctx->DrawLine(
            { offx, offy + WW_CONT_HEIGHT * half + y},
            { offx + wwww, offy + WW_CONT_HEIGHT * half + y},
            brush, line_width
        );
        return;
    }


    // 波形窗口区

    const auto base = wt;
    const auto len = len_raw - 1;
    const float vol = (float)d.volume * vmax;

    const auto count = d.freq / float(WW_UNIT);
    const auto repeat = std::min(int(count * (float)len_raw + 1.f), int(WW_MAX_WAVESECTION * len_raw));
    const auto rate = 1.f / count;

    //printf("%5.1fHz  - %f\n", d.freq, count);

    const float xplus = wwww * rate / (float)len_raw;


    const auto mapper = [wmax, vol, y, half](D2D1_POINT_2F pt) {
        constexpr float offy = WW_TSTART + WW_PADDING;
        constexpr float cont = WW_CONT_HEIGHT;
        pt.y = offy + y + (1.f - (pt.y * vol * wmax)) * cont * half;
        return pt;
    };
    D2D1_POINT_2F p1, p2;
    p1 = { WW_LSTART, base[0] }; p2 = { WW_LSTART + xplus, base[1] };
    float x = WW_LSTART + xplus;

    ctx->PushAxisAlignedClip({
        offx, WW_PADDING + y, offx + wwww, offy + WW_CONT_HEIGHT + y
        }, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);


    for (int i = 1, j = 1; j != repeat; ++j) {
        x += xplus;
        const int next_i = i == len_raw - 1 ? 0 : i + 1;
        // 斜率一致则合并
        if ((base[next_i] - base[i]) == (p2.y - p1.y)) {
            p2 = { x, base[i] };
        }
        else {
            ctx->DrawLine(mapper(p1), mapper(p2), brush, line_width);
            p1 = p2;
            p2 = { x, base[i] };
        }
        i = next_i;
    }

    p2 = { x, base[len] };
    ctx->DrawLine(mapper(p1), mapper(p2), brush, line_width);

    ctx->PopAxisAlignedClip();


    // 波形窗口区
    constexpr float wave_shape_x = WW_LSTART + WW_WIDTH + FONT_SIZE;
    constexpr float wave_shape_w = WW_WIDTH / 3;
    const float shape_xp = wave_shape_w / float(len_raw);
    p1 = { wave_shape_x, base[0] };  p2 = { wave_shape_x + shape_xp, base[1] };

    const auto mapper2 = [wmax, y, half](D2D1_POINT_2F pt) {
        constexpr float offy = WW_TSTART + WW_HEIGHT / 2;
        constexpr float cont = WW_HEIGHT / 2;
        pt.y = offy + y + (1.f - (pt.y * wmax)) * cont * half;
        return pt;
    };
    x = wave_shape_x + shape_xp;
    for (int i = 1; i != len_raw - 1; ++i) {
        x += shape_xp;
        // 斜率一致则合并
        if ((base[i+1] - base[i]) == (p2.y - p1.y)) {
            p2 = { x, base[i] };
        }
        else {
            ctx->DrawLine(mapper2(p1), mapper2(p2), brush, line_width);
            p1 = p2;
            p2 = { x, base[i] };
        }
    }
    p2 = { x, base[len_raw-1] };
    ctx->DrawLine(mapper2(p1), mapper2(p2), brush, line_width);
}


void DrawFreqMarker(float y, float freq) {
    constexpr float offx = WW_LSTART;
    constexpr float offy = WW_TSTART - WW_MARKER_SIZE / 2;
    constexpr float size = WW_MARKER_SIZE;

    const float ox = offx + (freq ? WW_WIDTH / (freq / float(WW_UNIT)) : 0.f) ;
    // 太长
    if (ox <= offx + (float)WW_WIDTH * 1.5) {
        g_data.d2d_context->DrawLine(
            { ox - size, y + offy - size },
            { ox, y + offy },
            g_data.d2d_red
        );
        g_data.d2d_context->DrawLine(
            { ox + size, y + offy - size },
            { ox, y + offy },
            g_data.d2d_red
        );

    }
    // 显示单位


    auto offset = g_data.dw_unit_offset;
    offset.x += offx + WW_WIDTH;
    offset.y += offy + y;

    g_data.d2d_context->DrawTextLayout(
        offset, g_data.dw_frequnit, g_data.d2d_white
    );
}


void DrawCommonInfo(float y, const sfc_visualizers_t& d, unsigned i) {
    auto& info = g_data.infos[i];
    constexpr float offx = WW_LSTART * 2 + WW_WIDTH;
    // 声道名称
    const auto ctx = g_data.d2d_context;
    const auto brush = g_data.d2d_white;
    const auto bgrey = g_data.d2d_grey;
    {
        const auto chnamey = y - FONT_SIZE / 2;
        ctx->DrawTextLayout({ WW_LSTART , chnamey }, info.chn_name, brush);
    }
    // 音高
    const auto freq = d.freq;
    if (freq > 0.f) {
        info.UpdateFreq(freq, i >= SFC_2A03_Noise && i <= SFC_2A03_DMC ? 4 : 0);
        const auto notey = y + FONT_SIZE / 2;
        if (info.freqnote)
            ctx->DrawTextLayout({ offx , notey }, info.freqnote, brush);
    }
    // 音量
    info.UpdateVol(i, d.volume);
    {
        const auto  voly = y + FONT_SIZE * 3 / 2;
        if (info.volinfo)
            ctx->DrawTextLayout({ offx , voly }, info.volinfo, d.key_on ? brush : bgrey);
    }

}

void DrawFME7NoView(float y, const sfc_visualizers_t& d) {
    const auto ctx = g_data.d2d_context;
    const auto brush = g_data.d2d_common;
    D2D1_COLOR_F color = *reinterpret_cast<const D2D_COLOR_F*>(d.color);
    color.a = 1.f;
    brush->SetColor(color);
    constexpr float offx = WW_LSTART;
    constexpr float offy = WW_TSTART + WW_CONT_HEIGHT / 2;

    auto offset = g_data.s5b_noview_offset;
    offset.x += offx + WW_WIDTH / 2;
    offset.y += offy + y;

    ctx->DrawTextLayout(offset, g_data.fme7_s5b_noview, brush);
}


void Draw2A03NoiseInfo(float y, const sfc_visualizers_t& d) {
    constexpr float offx = WW_LSTART + WW_WIDTH;
    constexpr float offy = WW_TSTART + WW_CONT_HEIGHT;
    const auto ctx = g_data.d2d_context;

    ctx->DrawTextLayout(
        { offx + FONT_SIZE , offy + y }, 
        d.ud.noi_mode ? g_data.noise_mode1 : g_data.noise_mode0,
        g_data.d2d_white
    );
}


void Draw2A03DMCInfo(float y, const sfc_visualizers_t& d) {
    constexpr float offx = WW_LSTART + WW_WIDTH;
    constexpr float offy = WW_TSTART + WW_CONT_HEIGHT;
    const auto ctx = g_data.d2d_context;

    ctx->DrawTextLayout(
        { offx + FONT_SIZE , offy + y },
        g_data.dmc_dpcm,
        d.key_on ? g_data.d2d_white : g_data.d2d_grey
    );
}

void DrawFME7Info(float y, const sfc_visualizers_t& d) {
    const auto t = d.ex.fme7.tone ? g_data.d2d_white : g_data.d2d_grey;
    const auto n = d.ex.fme7.noi ? g_data.d2d_white : g_data.d2d_grey;
    const auto e = d.ex.fme7.env ? g_data.d2d_white : g_data.d2d_grey;
    constexpr float offx = WW_LSTART + WW_WIDTH;
    constexpr float offy = WW_TSTART + WW_CONT_HEIGHT ;
    const auto ctx = g_data.d2d_context;

    ctx->DrawTextLayout({ offx + FONT_SIZE * 1, offy + y }, g_data.fme7_tone, t);
    ctx->DrawTextLayout({ offx + FONT_SIZE * 4, offy + y }, g_data.fme7_noise, n);
    ctx->DrawTextLayout({ offx + FONT_SIZE * 7, offy + y }, g_data.fme7_env, e);
}


void DrawVRC7Info(float y, const sfc_visualizers_t& d) {
    D2D1_POINT_2F pt;
    constexpr float offx = WW_LSTART + WW_WIDTH;
    constexpr float offy = WW_TSTART + WW_CONT_HEIGHT;
    const auto ctx = g_data.d2d_context;
    const auto bon = g_data.d2d_white;
    const auto bof = g_data.d2d_grey;
    const auto brush_a = [bon, bof](uint8_t a) { return a == 1 ? bon : bof; };
    const auto brush_d = [bon, bof](uint8_t a) { return a == 2 ? bon : bof; };
    const auto brush_s = [bon, bof](uint8_t a) { return a == 3 ? bon : bof; };
    const auto brush_r = [bon, bof](uint8_t a) { return a == 4 ? bon : bof; };
    const auto brush_m = [bon, bof](uint8_t a) { return a ? bon : bof; };
    // 乐器
    ctx->DrawTextLayout({ offx + FONT_SIZE , offy + y }, g_data.vrc7_name[d.ex.vrc7.instrument], bon);
    // 载波器
    pt = { offx - FONT_SIZE * 3 / 2, WW_TSTART + y };
    ctx->DrawTextLayout(pt, g_data.vrc7_name[vrc7_text_fm], brush_m(d.ex.vrc7.car_fm));
    pt.x -= FONT_SIZE * 2;
    ctx->DrawTextLayout(pt, g_data.vrc7_name[vrc7_text_am], brush_m(d.ex.vrc7.car_am));
    pt.x -= FONT_SIZE * 2;
    ctx->DrawTextLayout(pt, g_data.vrc7_name[vrc7_text_r], brush_r(d.ex.vrc7.car_state));
    pt.x -= FONT_SIZE;
    ctx->DrawTextLayout(pt, g_data.vrc7_name[vrc7_text_s], brush_s(d.ex.vrc7.car_state));
    pt.x -= FONT_SIZE;
    ctx->DrawTextLayout(pt, g_data.vrc7_name[vrc7_text_d], brush_d(d.ex.vrc7.car_state));
    pt.x -= FONT_SIZE;
    ctx->DrawTextLayout(pt, g_data.vrc7_name[vrc7_text_a], brush_a(d.ex.vrc7.car_state));
    // 调制器
    pt = { offx - FONT_SIZE * 3 / 2, WW_TSTART + WW_CONT_HEIGHT + y };
    ctx->DrawTextLayout(pt, g_data.vrc7_name[vrc7_text_fm], brush_m(d.ex.vrc7.mod_fm));
    pt.x -= FONT_SIZE * 2;
    ctx->DrawTextLayout(pt, g_data.vrc7_name[vrc7_text_am], brush_m(d.ex.vrc7.mod_am));
    pt.x -= FONT_SIZE * 2;
    ctx->DrawTextLayout(pt, g_data.vrc7_name[vrc7_text_r], brush_r(d.ex.vrc7.mod_state));
    pt.x -= FONT_SIZE;
    ctx->DrawTextLayout(pt, g_data.vrc7_name[vrc7_text_s], brush_s(d.ex.vrc7.mod_state));
    pt.x -= FONT_SIZE;
    ctx->DrawTextLayout(pt, g_data.vrc7_name[vrc7_text_d], brush_d(d.ex.vrc7.mod_state));
    pt.x -= FONT_SIZE;
    ctx->DrawTextLayout(pt, g_data.vrc7_name[vrc7_text_a], brush_a(d.ex.vrc7.mod_state));
}

void RenderWaveWindow(float y) noexcept {
    const auto ptr = g_data.visualizer_data;
    const auto len = g_data.visualizer_len;
    int y_count = 0;
    // 波形窗口必须存在掩码才渲染
    for (unsigned j = 0; j != len; ++j) {
        const auto i = d2d_custom_index[j];
        if (ptr[i].mask) {
            DrawWaveWindow(y);
            switch (i)
            {
            case SFC_FME7_ChannelA:
            case SFC_FME7_ChannelB:
            case SFC_FME7_ChannelC:
                DrawFME7Info(y, ptr[i]);
                if (!(ptr[i].ex.fme7.tone && !(ptr[i].ex.fme7.noi | ptr[i].ex.fme7.env))) {
                    DrawFME7NoView(y, ptr[i]);
                    break;
                }
            case SFC_2A03_Square1:
            case SFC_2A03_Square2:
            case SFC_MMC5_Square1:
            case SFC_MMC5_Square2:
                Draw2A03SquareWave(y, ptr[i]);
                break;
            case SFC_2A03_Triangle:
                Draw2A03TriangleWave(y, ptr[i]);
                break;
            case SFC_2A03_Noise:
                Draw2A03NoiseInfo(y, ptr[i]);
                DrawNormalPCM(
                    y, ptr[i],
                    g_data.channel_buffer + g_data.channel_spf * SFC_VIS_PCM_NOISE, 
                    15.f, 1.f
                );
                break;
            case SFC_2A03_DMC:
                Draw2A03DMCInfo(y, ptr[i]);
                DrawNormalPCM(y, ptr[i], 
                    g_data.channel_buffer + g_data.channel_spf * SFC_VIS_PCM_DMC,
                    127.f, 1.f);
                break;
            case SFC_MMC5_PCM:
                DrawNormalPCM(y, ptr[i], 
                    g_data.channel_buffer + g_data.channel_spf * SFC_VIS_PCM_PCM,
                    255.f, 1.f);
                break;
            case SFC_VRC6_Square1:
            case SFC_VRC6_Square2:
                DrawVRC6SquareWave(y, ptr[i]);
                break;
            case SFC_VRC6_Saw:
                DrawVRC6SawWave(y, ptr[i]);
                break;
            case SFC_VRC7_FM0:
            case SFC_VRC7_FM1:
                //DrawNormalPCM(y, ptr[SFC_VRC7_FM0], SFC_VRC7_FM0, 0.125f, 0.5f);
                //break;
            case SFC_VRC7_FM2:
            case SFC_VRC7_FM3:
            case SFC_VRC7_FM4:
            case SFC_VRC7_FM5:
                DrawWaveTable(
                    y, ptr[i],
                    g_data.vrc7_wavtable + g_data.vrc7_tablelen * ptr[i].ex.vrc7.instrument,
                    g_data.vrc7_tablelen,
                    2.f / 15.f, 2.f,
                    0.5f
                );
                DrawVRC7Info(y, ptr[i]);
                //DrawNormalPCM(y, ptr[i], i, 0.125f, 0.5f);
                break;
            case SFC_FDS1_Wavefrom:
                DrawWaveTable(y, ptr[i], g_data.fds1_wavtable, 64, 1.f / 32.f / 30.f, 1.f / 63.f, 1.f);
                break;
            case SFC_N163_Wavefrom0:
            case SFC_N163_Wavefrom1:
            case SFC_N163_Wavefrom2:
            case SFC_N163_Wavefrom3:
            case SFC_N163_Wavefrom4:
            case SFC_N163_Wavefrom5:
            case SFC_N163_Wavefrom6:
            case SFC_N163_Wavefrom7:
                DrawWaveTable(y, ptr[i],
                    g_data.n163_wavtable + ptr[i].ex.n163.wavtbl_off,
                    ptr[i].ex.n163.wavtbl_len, 
                    1.f / 15.f, 1.f / 15.f,
                    1.f
                );
                break;
            }

            DrawCommonInfo(y, ptr[i], i);
            DrawFreqMarker(y, ptr[i].freq);

            y += float(CH_ZONE_HEIGHT);
            ++y_count;
            if (!g_data.timelines) {
                constexpr int count_per_col = 5;
                if (y_count == count_per_col) {
                    y_count = 0;
                    y -= float(CH_ZONE_HEIGHT * count_per_col);
                    D2D1_MATRIX_3X2_F matrix;
                    g_data.d2d_context->GetTransform(&matrix);
                    matrix._31 += float(WW_WIDTH) * 1.5f * matrix._11;
                    g_data.d2d_context->SetTransform(&matrix);
                }
            }
        }
    }
}


// 音频可视化
void DoVisualizer() noexcept {
    const auto len = g_data.visualizer_len;
    if (!len) return; //return;
    const float c4 = 512 + 256;
    const float y = 4;
    RenderKeyboard(c4, y);
    g_data.d2d_context->SetTransform(D2D1::Matrix3x2F::Scale({ 1.3f, 1.3f }));
    //g_data.d2d_context->SetTransform(D2D1::Matrix3x2F::Scale({ 1.7f, 1.7f }));
    RenderWaveWindow(y + KB_WHITE_HEIGHT + 20);
}


SFC_EXTERN_C void d2d_set_visualizers(
    const sfc_visualizers_t* d, unsigned len,
    const float* n163_wave_table,
    const float* fds1_wave_table,
    const float* vrc7_wave_table, unsigned vrc7_wtlen,
    const float* chbuf, unsigned sample_per_frame
) SFC_NOEXCEPT {
    g_data.visualizer_data = d;
    g_data.visualizer_len = len;
    g_data.n163_wavtable = n163_wave_table;
    g_data.fds1_wavtable = fds1_wave_table;
    g_data.vrc7_wavtable = vrc7_wave_table;
    g_data.vrc7_tablelen = vrc7_wtlen;
    g_data.channel_buffer = chbuf;
    g_data.channel_spf = sample_per_frame;
}



const wchar_t chn_name[] = {
    L"2A03-SQ1"
    L"2A03-SQ2"
    L"2A03-TRI"
    L"2A03-NOI"
    L"2A03-DMC"
    L"MMC5-SQ1"
    L"MMC5-SQ2"
    L"MMC5-PCM"
    L"VRC6-SQ1"
    L"VRC6-SQ2"
    L"VRC6-SAW"
    L"VRC7-FM0"
    L"VRC7-FM1"
    L"VRC7-FM2"
    L"VRC7-FM3"
    L"VRC7-FM4"
    L"VRC7-FM5"
    L"FDS1-WAV"
    L"N163-WF0"
    L"N163-WF1"
    L"N163-WF2"
    L"N163-WF3"
    L"N163-WF4"
    L"N163-WF5"
    L"N163-WF6"
    L"N163-WF7"
    L"FME7-ChA"
    L"FME7-ChB"
    L"FME7-ChC"
};


const wchar_t vol_unit[] = {
    L"P-Linear x"
    L"P-Linear x"
    L"P-Linear x"
    L"P-Linear x"
    L"P-Linear x"

    L"P-Linear x"
    L"P-Linear x"
    L"P-Linear ?"

    L"  Linear x"
    L"  Linear x"
    L"  Linear x"

    L"  3.00dB x"
    L"  3.00dB x"
    L"  3.00dB x"
    L"  3.00dB x"
    L"  3.00dB x"
    L"  3.00dB x"

    L"L 1/1000 x"

    L"  Linear x"
    L"  Linear x"
    L"  Linear x"
    L"  Linear x"
    L"  Linear x"
    L"  Linear x"
    L"  Linear x"
    L"  Linear x"

    L"  3.00dB x"
    L"  3.00dB x"
    L"  3.00dB x"
};

const wchar_t* const vrc7_name_table[] = {
    L"0-Custom",
    L"1-Buzzy Bell",
    L"2-Guitar",
    L"3-Wurly",
    L"4-Flute",
    L"5-Clarinet",
    L"6-Synth",
    L"7-Trumpet",
    L"8-Organ",
    L"9-Bells",
    L"A-Vibes",
    L"B-Vibraphone",
    L"C-Tutti",
    L"D-Fretless",
    L"E-Synth Bass",
    L"F-Sweep",

    L"A",
    L"D",
    L"S",
    L"R",
    L"AM",
    L"FM"

};

SFC_EXTERN_C void d2d_n163_wavtbl_changed(unsigned i)  SFC_NOEXCEPT {

}

auto CreateFreqUnitTL() noexcept->HRESULT {
    auto hr = g_data.dw_factory->CreateTextLayout(
        //L" 55Hz", 5,
        L"110Hz", 5,
        g_data.dw_basetf,
        0, 0,
        &g_data.dw_frequnit
    );
    if (SUCCEEDED(hr)) {
        DWRITE_TEXT_METRICS m;
        g_data.dw_frequnit->GetMetrics(&m);
        g_data.dw_unit_offset = { m.width * -0.5f, -m.height };
    }
    if (SUCCEEDED(hr)) {
        hr = g_data.dw_factory->CreateTextLayout(
            L"Tone", 4,
            g_data.dw_basetf,
            0, 0,
            &g_data.fme7_tone
        );
    }
    if (SUCCEEDED(hr)) {
        hr = g_data.dw_factory->CreateTextLayout(
            L"Noi", 3,
            g_data.dw_basetf,
            0, 0,
            &g_data.fme7_noise
        );
    }
    if (SUCCEEDED(hr)) {
        hr = g_data.dw_factory->CreateTextLayout(
            L"Env", 3,
            g_data.dw_basetf,
            0, 0,
            &g_data.fme7_env
        );
    }
    if (SUCCEEDED(hr)) {
        hr = g_data.dw_factory->CreateTextLayout(
            L"Sunsoft 5B - No View", 20,
            g_data.dw_basetf,
            0, 0,
            &g_data.fme7_s5b_noview
        );
    }
    if (SUCCEEDED(hr)) {
        DWRITE_TEXT_METRICS m;
        g_data.fme7_s5b_noview->GetMetrics(&m);
        g_data.s5b_noview_offset = { m.width * -0.5f, 0 };

        for (unsigned i = 0; i != SFC_CHANNEL_COUNT; ++i) {
            const auto code = g_data.infos[i].MakeChnName(chn_name + 8 * i, 8);
            if (FAILED(code)) return code;
        }
    }
    if (SUCCEEDED(hr)) {
        hr = g_data.dw_factory->CreateTextLayout(
            L"Mode: Long", 10,
            g_data.dw_basetf,
            0, 0,
            &g_data.noise_mode0
        );
    }
    if (SUCCEEDED(hr)) {
        hr = g_data.dw_factory->CreateTextLayout(
            L"Mode: Short", 11,
            g_data.dw_basetf,
            0, 0,
            &g_data.noise_mode1
        );
    }
    if (SUCCEEDED(hr)) {
        hr = g_data.dw_factory->CreateTextLayout(
            L"ΔPCM", 4,
            g_data.dw_basetf,
            0, 0,
            &g_data.dmc_dpcm
        );
    }
    for (int i = 0; i != vrc7_text_count; ++i) {
        if (SUCCEEDED(hr)) {
            const auto ptr = vrc7_name_table[i];
            hr = g_data.dw_factory->CreateTextLayout(
                ptr, std::wcslen(ptr),
                g_data.dw_basetf,
                0, 0,
                &g_data.vrc7_name[i]
            );
        }
    }
    return hr;
}

// ------------------------------------------------------------


#include <cwchar>

const wchar_t note_name[] = L"C-C#D-D#E-F-F#G-G#A-A#B-";

void SFCChannelInfo::Release() {
    ::SafeRelease(this->freqnote);
    ::SafeRelease(this->chn_name);
    ::SafeRelease(this->volinfo);
    
}

void SFCChannelInfo::UpdateVol(unsigned i , uint16_t v) {
    if (v == this->volume) return;
    ::SafeRelease(this->volinfo);
    this->volume = v;
    wchar_t buf[32];
    constexpr int vol_unit_name = 10;
    memcpy(buf, vol_unit + i * vol_unit_name, vol_unit_name * sizeof(wchar_t));
    std::swprintf(
        buf + vol_unit_name, 32 - vol_unit_name, 
        L" %3d", v 
    );
    const auto hr = g_data.dw_factory->CreateTextLayout(
        buf, std::wcslen(buf),
        g_data.dw_basetf,
        0, 0,
        &this->volinfo
    );
}

void SFCChannelInfo::UpdateFreq(float freq, unsigned offset) {
    // TODO: 同一个键范围都算?
    if (freq == this->freq) return;
    ::SafeRelease(this->freqnote);
    wchar_t buf[32];
    uint32_t len = 1;
    if (freq > 0.f) {
        this->freq = freq;
        const int id = calc_key_id_c4(freq);
        const int note = (id % 12 + 12) % 12;
        const int section = (id - note) / 12;
        buf[0] = note_name[note * 2 + 0];
        buf[1] = note_name[note * 2 + 1];
        if (freq > 1e4f) 
            std::swprintf(buf + 2, 30, L"%2d @%6.2fkHz", section + 4, freq / 1000.f);
        else
            std::swprintf(buf + 2, 30, L"%2d @ %6.1fHz", section + 4, freq);
        len = std::wcslen(buf);
    }
    else {
        buf[0] = 0;
        offset = 0;
    };

    const auto hr = g_data.dw_factory->CreateTextLayout(
        buf + offset, len - offset,
        g_data.dw_basetf,
        0, 0,
        &this->freqnote
    );
}


HRESULT SFCChannelInfo::MakeChnName(const wchar_t* name, uint32_t len) {
    return g_data.dw_factory->CreateTextLayout(
        name, len,
        g_data.dw_basetf,
        0, 0,
        &this->chn_name
    );
}
