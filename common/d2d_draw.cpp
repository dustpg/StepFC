#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d2d1_1.h>
//#include <unknwn.h>
#include <d3d11.h>
//#include <d3dcompiler.h>
//#include <DirectXMath.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <cassert>
#include <iterator>
#include <algorithm>
#include "d2d_interface.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")

struct alignas(sizeof(float)*4) GlobalData {
    IDXGISwapChain*         swap_chain;
    ID3D11Device*           device;
    ID3D11DeviceContext*    device_context;

    ID2D1Factory1*          d2d_factory;
    ID2D1Device*            d2d_device;
    ID2D1DeviceContext*     d2d_context;
    ID2D1Bitmap1*           d2d_target;
    ID2D1Bitmap1*           d2d_bg;
    ID2D1SolidColorBrush*   d2d_brush;
} g_data = {  };

enum { WINDOW_WIDTH = 1280, WINDOW_HEIGHT = 720 };
static const wchar_t WINDOW_TITLE[] = L"D2D Draw";
//static bool doit = true;

LRESULT CALLBACK ThisWndProc(HWND , UINT , WPARAM , LPARAM ) noexcept;
void DoRender(uint32_t sync) noexcept;
bool InitD3D(HWND) noexcept;
void ClearD3D() noexcept;
HRESULT CreateColoredCube() noexcept;

template<class Interface>
inline void SafeRelease(Interface *&pInterfaceToRelease) {
    if (pInterfaceToRelease != nullptr) {
        pInterfaceToRelease->Release();
        pInterfaceToRelease = nullptr;
    }
}

uint32_t g_sync = 1;
uint32_t g_bg_data[256 * 256 + 256];

extern "C" void main_cpp() noexcept {
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
    if (::InitD3D(hwnd)) {
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
    'J', 'K', 'U', 'I', 'W', 'S', 'A', 'D',
    // A, B, Select, Start, Up, Down, Left, Right
    VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD5, VK_NUMPAD6, 
    VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT,
};

LRESULT CALLBACK ThisWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
    switch (msg)
    {
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


void DoRender(uint32_t sync) noexcept {
    main_render(g_bg_data);
    const auto hr0 = g_data.d2d_bg->CopyFromMemory(nullptr, g_bg_data, 256 * 4);
    assert(SUCCEEDED(hr0));
    // D2D
    {
        const auto ctx = g_data.d2d_context;
        ctx->BeginDraw();
        ctx->Clear(D2D1::ColorF(1.f, 1.f, 1.f, 1.f));
        ctx->SetTransform(D2D1::Matrix3x2F::Scale({ 2.f, 2.f }));
        ctx->DrawBitmap(g_data.d2d_bg, nullptr, 1.f, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
        if (sub_render(g_bg_data)) {
            const auto hr0 = g_data.d2d_bg->CopyFromMemory(nullptr, g_bg_data, 256 * 4);
            assert(SUCCEEDED(hr0));
            D2D1_RECT_F des_rect = { 256, 0, 512, 240 };
            ctx->DrawBitmap(g_data.d2d_bg, &des_rect, 1.f, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
        }


        const auto hr1 = ctx->EndDraw();
        assert(SUCCEEDED(hr1));
        const auto hr2 = g_data.swap_chain->Present(sync, 0);
        assert(SUCCEEDED(hr2));
    }
}

bool InitD3D(HWND hwnd) noexcept {
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
    // 创建纯色笔刷
    if (SUCCEEDED(hr)) {
        hr = g_data.d2d_context->CreateSolidColorBrush(
            D2D1::ColorF(0.f, 0.f, 0.f, 1.f),
            &g_data.d2d_brush
        );
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
    ::SafeRelease(g_data.d2d_brush);
    ::SafeRelease(g_data.d2d_bg);
    ::SafeRelease(g_data.d2d_target); 
    ::SafeRelease(g_data.d2d_context);
    ::SafeRelease(g_data.d2d_device);
    ::SafeRelease(g_data.d2d_factory);
    
    ::SafeRelease(g_data.device_context);
    ::SafeRelease(g_data.device);
    ::SafeRelease(g_data.swap_chain);
}


