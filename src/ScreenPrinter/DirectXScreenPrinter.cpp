#include <iostream>

#include "ScreenPrinter/DirectXScreenPrinter.h"

using namespace std;

LRESULT CALLBACK DirectXScreenPrinterWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

DirectXScreenPrinter::DirectXScreenPrinter(int width, int height, const std::shared_ptr<FrameData>& frame_ref)
	:ScreenPrinter(width, height), frame_ref(frame_ref)
{
	//set window class
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = DirectXScreenPrinterWndProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = L"MyWindowClass";
	RegisterClassEx(&wc);
	_main = CreateWindowEx(0, L"MyWindowClass", L"My Window", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, GetModuleHandle(NULL), NULL);

	//set Direct3D struct
	swapChainDesc = {};
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = width;
	swapChainDesc.BufferDesc.Height = height;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = _main; // hwnd is window handle
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Windowed = TRUE;

	auto hr = D3D11CreateDeviceAndSwapChain(
		nullptr,                     // 어댑터 (nullptr이면 기본 어댑터 사용)
		D3D_DRIVER_TYPE_HARDWARE,    // 드라이버 타입
		nullptr,                     // 소프트웨어 모듈 핸들
		0,                           // 플래그
		nullptr,                     // 피쳐 레벨 배열
		0,                           // 피쳐 레벨 배열의 크기
		D3D11_SDK_VERSION,           // SDK 버전
		&swapChainDesc,				 // 스왑 체인 설정
		&swapChain,                  // 스왑 체인
		&d3dDevice,                     // 디바이스
		&featureLevel,               // 실제로 생성된 디바이스의 피쳐 레벨
		&d3dContext                     // 디바이스 컨텍스트
	);
	if (FAILED(hr))
	{
		cout << "DeviceAndSwapChain create fail" << endl;
		// 오류 처리 및 메시지 출력
		if (hr == DXGI_ERROR_INVALID_CALL) {
			// DXGI_ERROR_INVALID_CALL: 잘못된 호출로 인한 오류
			OutputDebugString(L"Error: DXGI_ERROR_INVALID_CALL\n");
		}
		else if (hr == DXGI_ERROR_SDK_COMPONENT_MISSING) {
			// DXGI_ERROR_SDK_COMPONENT_MISSING: 필요한 컴포넌트 누락
			OutputDebugString(L"Error: DXGI_ERROR_SDK_COMPONENT_MISSING\n");
		}
		else if (hr == E_INVALIDARG) {
			// E_INVALIDARG: 잘못된 인자로 인한 오류
			OutputDebugString(L"Error: E_INVALIDARG\n");
		}
		else {
			// 그 외의 경우에는 HRESULT 코드를 출력
			wchar_t errorMsg[256];
			swprintf_s(errorMsg, L"Error: HRESULT = 0x%X\n", hr);
			OutputDebugString(errorMsg);
		}

		exit(-1);
	}

	//set Texture2D struct
	ZeroMemory(&textureDesc, sizeof(textureDesc));
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 32bit RGBA format used
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET;

	//set render struct
	rtvDesc = {};
	rtvDesc.Format = textureDesc.Format; // Set the format to match your texture's format
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D; // Assuming you are using a 2D texture
	rtvDesc.Texture2D.MipSlice = 0;

	is_printing = false;
}

void DirectXScreenPrinter::StartPrint()
{
	is_printing = true;
	ShowWindow(_main, SW_SHOWNORMAL);

	print_thread = thread(&DirectXScreenPrinter::PrintFunc, this);
}

void DirectXScreenPrinter::EndPrint()
{
	is_printing = false;
	print_thread.join();
	DestroyWindow(_main);
}

DirectXScreenPrinter::~DirectXScreenPrinter()
{
	// resource release
	texture->Release();
	d3dContext->Release();
	d3dDevice->Release();
	swapChain->Release();
}

void DirectXScreenPrinter::PrintFrame(shared_ptr<FrameData> frame)
{
	if(texture!= nullptr)
		texture->Release();
	texture = nullptr;

	D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory(&initData, sizeof(initData));
	initData.pSysMem = frame.get()->getMemPointer();
	initData.SysMemPitch = w * 4; // 4 means RGBA 4byte

	if (FAILED(d3dDevice->CreateTexture2D(&textureDesc, &initData, &texture)))
	{
		cout << "CreateTexture2D create fail" << endl;

		// resource release
		texture->Release();
		d3dContext->Release();
		d3dDevice->Release();
		swapChain->Release();

		exit(-1);
	}

	// Create Render Target View
	ID3D11RenderTargetView* renderTargetView = nullptr;
	auto hr = d3dDevice->CreateRenderTargetView(texture, &rtvDesc, &renderTargetView);
	if (FAILED(hr))
	{
		cout << "CreateRenderTargetView create fail" << endl;
		// 오류 처리 및 메시지 출력
		if (hr == DXGI_ERROR_INVALID_CALL) {
			// DXGI_ERROR_INVALID_CALL: 잘못된 호출로 인한 오류
			OutputDebugString(L"Error: DXGI_ERROR_INVALID_CALL\n");
		}
		else if (hr == DXGI_ERROR_SDK_COMPONENT_MISSING) {
			// DXGI_ERROR_SDK_COMPONENT_MISSING: 필요한 컴포넌트 누락
			OutputDebugString(L"Error: DXGI_ERROR_SDK_COMPONENT_MISSING\n");
		}
		else if (hr == E_INVALIDARG) {
			// E_INVALIDARG: 잘못된 인자로 인한 오류
			OutputDebugString(L"Error: E_INVALIDARG\n");
		}
		else {
			// 그 외의 경우에는 HRESULT 코드를 출력
			wchar_t errorMsg[256];
			swprintf_s(errorMsg, L"Error: HRESULT = 0x%X\n", hr);
			OutputDebugString(errorMsg);
		}

		// resource release
		texture->Release();
		d3dContext->Release();
		d3dDevice->Release();
		swapChain->Release();

		exit(-1);
	}
	d3dContext->OMSetRenderTargets(1, &renderTargetView, nullptr);
	d3dContext->UpdateSubresource(texture, 0, nullptr, frame.get()->getMemPointer(), swapChainDesc.BufferDesc.Width * 4, 0);
	swapChain->Present(1, 0);

	UpdateWindow(_main);
}

void DirectXScreenPrinter::PrintFunc()
{
	std::shared_ptr<FrameData> prev_frame;
	while (is_printing)
	{
		if (frame_ref != prev_frame)
		{
			prev_frame = frame_ref;
			PrintFrame(prev_frame);
		}
	}
}
