// modified example to overlay tester

#include <imgui.h>
#include "imgui_impl_dx11.h"
#include "imgui_impl_dx11.cpp"
#include <d3d11.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>

/////////////////////////////////////////////////////////////////////////////////
#include <boost/filesystem.hpp>
#include <Windows.h>
typedef int(*TModUnInit)(ImGuiContext* context);
typedef int(*TModRender)(ImGuiContext* context);
typedef int(*TModInit)(ImGuiContext* context);
typedef void(*TModTextureData)(unsigned char** out_pixels, int* out_width, int* out_height, int* out_bytes_per_pixel);
typedef void(*TModSetTexture)(void* texture);
typedef bool(*TModUpdateFont)(ImGuiContext* context);
typedef bool(*TModMenu)(bool* show);

TModUnInit modUnInit = nullptr;
TModRender modRender = nullptr;
TModInit modInit = nullptr;
TModTextureData modTextureData = nullptr;
TModSetTexture modSetTexture = nullptr;
TModUpdateFont modUpdateFont = nullptr;
TModMenu modMenu = nullptr;
HMODULE mod;
/////////////////////////////////////////////////////////////////////////////////

// Data
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;

void CreateRenderTarget()
{
    DXGI_SWAP_CHAIN_DESC sd;
    g_pSwapChain->GetDesc(&sd);

    // Create the render target
    ID3D11Texture2D* pBackBuffer;
    D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
    ZeroMemory(&render_target_view_desc, sizeof(render_target_view_desc));
    render_target_view_desc.Format = sd.BufferDesc.Format;
    render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, &render_target_view_desc, &g_mainRenderTargetView);
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

HRESULT CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    {
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = 2;
        sd.BufferDesc.Width = 0;
        sd.BufferDesc.Height = 0;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    }

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[1] = { D3D_FEATURE_LEVEL_11_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 1, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return E_FAIL;

    CreateRenderTarget();

    return S_OK;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

extern LRESULT ImGui_ImplDX11_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplDX11_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            ImGui_ImplDX11_InvalidateDeviceObjects();
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
            ImGui_ImplDX11_CreateDeviceObjects();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

static ID3D11ShaderResourceView*g_pModTextureView = NULL;
static void ImGui_ImplDX11_CreateModTexture()
{
	if (!modTextureData)
		return;

	if (g_pModTextureView)
	{
		g_pModTextureView->Release();
		g_pModTextureView = nullptr;
		if (modSetTexture)
			modSetTexture(nullptr);
	}
	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();
	unsigned char* pixels;
	int width, height;
	int bpp;
	modTextureData(&pixels, &width, &height, &bpp);
	if (pixels == nullptr)
		return;

	// Upload texture to graphics system
	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;

		ID3D11Texture2D *pTexture = NULL;
		D3D11_SUBRESOURCE_DATA subResource;
		subResource.pSysMem = pixels;
		subResource.SysMemPitch = desc.Width * 4;
		subResource.SysMemSlicePitch = 0;
		g_pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);

		// Create texture view
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = desc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		g_pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &g_pModTextureView);
		if(pTexture)
			pTexture->Release();
	}

	// Store our identifier
	//io.Fonts->TexID = (void *)g_pFontTextureView;

	//// Create texture sampler
	//{
	//	D3D11_SAMPLER_DESC desc;
	//	ZeroMemory(&desc, sizeof(desc));
	//	desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	//	desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	//	desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	//	desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	//	desc.MipLODBias = 0.f;
	//	desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	//	desc.MinLOD = 0.f;
	//	desc.MaxLOD = 0.f;
	//	g_pd3dDevice->CreateSamplerState(&desc, &g_pFontSampler);
	//}

	if (modSetTexture)
		modSetTexture(g_pModTextureView);
}

int main(int argc, char** argv)
{

	/////////////////////////////////////////////////////////////////////////////////
	wchar_t szPath[1024] = { 0, };
	GetModuleFileNameW(NULL, szPath, 1023);
	boost::filesystem::path p(szPath);
	if (argc > 1)
	{
		if (boost::filesystem::exists(p.parent_path() / argv[1]))
			p = p.parent_path() / argv[1];
		else
			p = argv[1];
	}
	else
	{
		p = p.parent_path() / "ActWebSocketImguiOverlay.dll";
	}
	mod = LoadLibraryW(p.wstring().c_str());
	if (mod)
	{
		modInit = (TModInit)GetProcAddress(mod, "ModInit");
		modRender = (TModRender)GetProcAddress(mod, "ModRender");
		modUnInit = (TModUnInit)GetProcAddress(mod, "ModUnInit");
		modTextureData = (TModTextureData)GetProcAddress(mod, "ModTextureData");
		modSetTexture = (TModSetTexture)GetProcAddress(mod, "ModSetTexture");
		modUpdateFont = (TModUpdateFont)GetProcAddress(mod, "ModUpdateFont");
	}
	/////////////////////////////////////////////////////////////////////////////////

    // Create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), NULL, NULL, _T("ImGui Example"), NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(_T("ImGui Example"), _T("ImGui DirectX11 Example"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

	/////////////////////////////////////////////////////////////////////////////////
	if (modInit)
	{
		modInit(ImGui::GetCurrentContext());
	}
	/////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////
	if (modUnInit)
	{
		modUnInit(ImGui::GetCurrentContext());
	}
	/////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////
	if (modInit)
	{
		modInit(ImGui::GetCurrentContext());
	}
	/////////////////////////////////////////////////////////////////////////////////
    // Initialize Direct3D
    if (CreateDeviceD3D(hwnd) < 0)
    {
        CleanupDeviceD3D();
        UnregisterClass(_T("ImGui Example"), wc.hInstance);
        return 1;
    }

    // Show the window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // Setup ImGui binding
    ImGui_ImplDX11_Init(hwnd, g_pd3dDevice, g_pd3dDeviceContext);
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;

    // Load Fonts
    // (there is a default font, this is only if you want to change it. see extra_fonts/README.txt for more details)
    //ImGuiIO& io = ImGui::GetIO();
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyClean.ttf", 13.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

    bool show_test_window = true;
    bool show_another_window = false;
    ImVec4 clear_col = ImColor(114, 144, 154);

    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

		if (modUpdateFont)
		{
			if (modUpdateFont(ImGui::GetCurrentContext()))
			{
				if (g_pFontTextureView)
					g_pFontTextureView->Release();
				g_pFontTextureView = nullptr;
				if (g_pFontSampler)
					g_pFontSampler->Release();
				g_pFontSampler = nullptr;
				ImGui_ImplDX11_CreateFontsTexture();
			}
		}

		if (!g_pModTextureView) {
			ImGui_ImplDX11_CreateModTexture();
		}
        ImGui_ImplDX11_NewFrame();

		/////////////////////////////////////////////////////////////////////////////////
		if (modRender)
		{
			modRender(ImGui::GetCurrentContext());
		}
		/////////////////////////////////////////////////////////////////////////////////

        // Rendering
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, (float*)&clear_col);
        ImGui::Render();
        g_pSwapChain->Present(0, 0);
    }

    ImGui_ImplDX11_Shutdown();
    CleanupDeviceD3D();
    UnregisterClass(_T("ImGui Example"), wc.hInstance);

	/////////////////////////////////////////////////////////////////////////////////
	if (modUnInit)
	{
		modUnInit(ImGui::GetCurrentContext());
	}
	/////////////////////////////////////////////////////////////////////////////////

    return 0;
}
