#pragma once

#include <string>
#include <fstream>

#include "ScreenCapture/ScreenCapture.h"
#include "ExternCode/DXGIManager/DXGIManager.h"

class DirectXScreenCapture : public ScreenCapture
{
public:
	DirectXScreenCapture();
	
	std::shared_ptr<FrameData> CaptureCurrentScreen() override;

	~DirectXScreenCapture() override;

private:
	std::string log_file;
	std::ofstream log_stream;

	DXGIManager g_DXGIManager;
	CComPtr<IWICImagingFactory> spWICFactory;
	RECT rcDim;

	HRESULT Capture(RECT& rcDim, BYTE* buf, CComPtr<IWICImagingFactory>& spWICFactory);
};