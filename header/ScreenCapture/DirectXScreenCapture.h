#pragma once

/*
   This code was written with reference to the following source.
   source: https://www.sysnet.pe.kr/2/0/11385
*/

#include <string>
#include <fstream>

#include "ScreenCapture/ScreenCapture.h"
#include "ExternCode/DXGIManager/DXGIManager.h"

class DirectXScreenCapture : public ScreenCapture
{
public:
	DirectXScreenCapture();
	
	std::shared_ptr<VideoFrameData> CaptureCurrentScreen() override;

	~DirectXScreenCapture() override;

private:
	std::string log_file;
	std::ofstream log_stream;

	DXGIManager g_DXGIManager;
	CComPtr<IWICImagingFactory> spWICFactory;
	RECT rcDim;

	HRESULT Capture(RECT& rcDim, BYTE* buf, CComPtr<IWICImagingFactory>& spWICFactory);
};