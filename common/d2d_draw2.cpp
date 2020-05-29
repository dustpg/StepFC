#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d2d1_1.h>
//#include <unknwn.h>
#include <d3d11.h>
#include <wincodec.h>
//#include <d3dcompiler.h>
//#include <DirectXMath.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cassert>
#include <iterator>
#include <algorithm>
#include "d2d_interface2.h"

#include <mmsystem.h>

#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "windowscodecs.lib")

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

    ID2D1Effect*            d2d_effect;
    ID2D1Image*             d2d_output;


    uint32_t                time_tick;
    float                   float_time;
    float                   time_per_frame;

} g_data = { 1, 0 };


// {B8AF3834-4CBE-47BF-8ECD-1F6CF0A9D43A}
static const GUID CLSID_DustPG_FamicomAE = {
    0xb8af3834, 0x4cbe, 0x47bf, { 0x8e, 0xcd, 0x1f, 0x6c, 0xf0, 0xa9, 0xd4, 0x3a }
};


enum { WINDOW_WIDTH = 1280, WINDOW_HEIGHT = 720 };
static const wchar_t WINDOW_TITLE[] = L"D2D Draw";
//static bool doit = true;

LRESULT CALLBACK ThisWndProc(HWND , UINT , WPARAM , LPARAM ) noexcept;
void DoRender(uint32_t sync) noexcept;
bool InitD3D(HWND, const char* res) noexcept;
void ClearD3D() noexcept;
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

extern "C" void main_cpp(
    const char* shader_bin_file_name,
    const char* shader_res_file_name) noexcept {
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
                if (msg == WM_KEYDOWN) qsave();
            }
            else if (unsigned(wParam) == VK_F2) {
                if (msg == WM_KEYDOWN) qload();
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

void DoRender(uint32_t sync) noexcept {

    // D2D
    {
        const auto ctx = g_data.d2d_context;
        ctx->BeginDraw();
        ctx->Clear(D2D1::ColorF(1.f, 1.f, 1.f, 1.f));
        ctx->SetTransform(D2D1::Matrix3x2F::Identity());


        TickTime();

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
        const auto hr0 = g_data.d2d_bg->CopyFromMemory(nullptr, g_bg_data, 256 * 4);
        assert(SUCCEEDED(hr0));
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
    ::SafeRelease(g_data.d2d_bg);
    ::SafeRelease(g_data.d2d_target); 
    ::SafeRelease(g_data.d2d_context);
    ::SafeRelease(g_data.d2d_device);
    ::SafeRelease(g_data.d2d_factory);
    
    ::SafeRelease(g_data.device_context);
    ::SafeRelease(g_data.device);
    ::SafeRelease(g_data.swap_chain);

    ::SafeRelease(g_data.wic_factory);

}


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