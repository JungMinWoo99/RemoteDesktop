#include <memory>
#include <thread>
#include <d3d11.h>
#include <windows.h>

#include "ScreenPrinter/ScreenPrinter.h"
#include "ScreenCapture/FrameData.h"

#define WIN32_LEAN_AND_MEAN

class DirectXScreenPrinter : ScreenPrinter
{
public:
	DirectXScreenPrinter(int width, int height, const std::shared_ptr<FrameData>& frame_ref);
	void StartPrint() override;
	void EndPrint() override;
	~DirectXScreenPrinter();

private:
	void PrintFrame(std::shared_ptr<FrameData> frame) override;

	void PrintFunc();
	std::thread print_thread;

	const std::shared_ptr<FrameData>& frame_ref;
	bool is_printing;

	// Direct3D
	ID3D11Device* d3dDevice;
	ID3D11DeviceContext* d3dContext;
	D3D_FEATURE_LEVEL featureLevel;
	IDXGISwapChain* swapChain;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;

	// Texture2D
	ID3D11Texture2D* texture;
	D3D11_TEXTURE2D_DESC textureDesc;

	// Render
	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;

	//window
	WNDCLASSEX wc = {};
	HWND _main;
};

