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
		nullptr,                     // ����� (nullptr�̸� �⺻ ����� ���)
		D3D_DRIVER_TYPE_HARDWARE,    // ����̹� Ÿ��
		nullptr,                     // ����Ʈ���� ��� �ڵ�
		0,                           // �÷���
		nullptr,                     // ���� ���� �迭
		0,                           // ���� ���� �迭�� ũ��
		D3D11_SDK_VERSION,           // SDK ����
		&swapChainDesc,				 // ���� ü�� ����
		&swapChain,                  // ���� ü��
		&d3dDevice,                     // ����̽�
		&featureLevel,               // ������ ������ ����̽��� ���� ����
		&d3dContext                     // ����̽� ���ؽ�Ʈ
	);
	if (FAILED(hr))
	{
		cout << "DeviceAndSwapChain create fail" << endl;
		// ���� ó�� �� �޽��� ���
		if (hr == DXGI_ERROR_INVALID_CALL) {
			// DXGI_ERROR_INVALID_CALL: �߸��� ȣ��� ���� ����
			OutputDebugString(L"Error: DXGI_ERROR_INVALID_CALL\n");
		}
		else if (hr == DXGI_ERROR_SDK_COMPONENT_MISSING) {
			// DXGI_ERROR_SDK_COMPONENT_MISSING: �ʿ��� ������Ʈ ����
			OutputDebugString(L"Error: DXGI_ERROR_SDK_COMPONENT_MISSING\n");
		}
		else if (hr == E_INVALIDARG) {
			// E_INVALIDARG: �߸��� ���ڷ� ���� ����
			OutputDebugString(L"Error: E_INVALIDARG\n");
		}
		else {
			// �� ���� ��쿡�� HRESULT �ڵ带 ���
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
		// ���� ó�� �� �޽��� ���
		if (hr == DXGI_ERROR_INVALID_CALL) {
			// DXGI_ERROR_INVALID_CALL: �߸��� ȣ��� ���� ����
			OutputDebugString(L"Error: DXGI_ERROR_INVALID_CALL\n");
		}
		else if (hr == DXGI_ERROR_SDK_COMPONENT_MISSING) {
			// DXGI_ERROR_SDK_COMPONENT_MISSING: �ʿ��� ������Ʈ ����
			OutputDebugString(L"Error: DXGI_ERROR_SDK_COMPONENT_MISSING\n");
		}
		else if (hr == E_INVALIDARG) {
			// E_INVALIDARG: �߸��� ���ڷ� ���� ����
			OutputDebugString(L"Error: E_INVALIDARG\n");
		}
		else {
			// �� ���� ��쿡�� HRESULT �ڵ带 ���
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
