#include "ScreenCapture/DirectXScreenCapture.h"

#include <iostream>
#include <chrono>
#include <format>

/*
   This code was written with reference to the following source.
   source: https://www.sysnet.pe.kr/2/0/11385
*/

using namespace std;

DirectXScreenCapture::DirectXScreenCapture()
	:ScreenCapture(), log_file(std::format("DirectXScreenCapture{}.txt", obj_id)), log_stream(log_file.c_str(), std::ios::out | std::ios::trunc)
{
	std::ofstream output_file;
	static bool is_coInitialized = false;
	if (!is_coInitialized)
	{
		CoInitialize(NULL);
		is_coInitialized = true;
	}

	g_DXGIManager.SetCaptureSource(CaptureSource::CSMonitor2);

	g_DXGIManager.GetOutputRect(rcDim);

	HRESULT hr = spWICFactory.CoCreateInstance(CLSID_WICImagingFactory);
	if (FAILED(hr))
	{
		log_stream << "IWICImagingFactory Create fail" << endl;
		exit(hr);
	}

	pixel_width = rcDim.right - rcDim.left;
	pixel_height = rcDim.bottom - rcDim.top;

	color_bits = BYTE_PER_PIXEL * 8;

	buf_byte_size = pixel_width * pixel_height * color_bits / 8;
}

shared_ptr<FrameData> DirectXScreenCapture::CaptureCurrentScreen()
{
	static chrono::steady_clock::time_point start_tp = chrono::steady_clock::now();

	static shared_ptr<FrameData> prev_frame;

	shared_ptr<FrameData> pixel_data_buf = make_shared<FrameData>(this->buf_byte_size);

	if (!cap_mtx.try_lock())
	{
		log_stream << "DirectXScreenCapture " << obj_id << " : too busy" << endl;
		cap_mtx.lock();
	}

	chrono::steady_clock::time_point cap_tp = chrono::steady_clock::now();

	auto ret = Capture(rcDim, pixel_data_buf.get()->getMemPointer(), spWICFactory);
	if (ret != 0)
	{
		if (ret == DXGI_ERROR_WAIT_TIMEOUT)
			memcpy(pixel_data_buf.get()->getMemPointer(), prev_frame.get()->getMemPointer(), prev_frame.get()->getMemSize());
		else
		{
			log_stream << "DirectX Capture fail!" << endl;
			log_stream << "error code: "<< ret << endl;
			memcpy(pixel_data_buf.get()->getMemPointer(), prev_frame.get()->getMemPointer(), prev_frame.get()->getMemSize());
		}
	}
	else
		prev_frame = pixel_data_buf;

	chrono::nanoseconds captured_time = cap_tp - start_tp;
	pixel_data_buf.get()->setCaptureTime(captured_time);

	cap_mtx.unlock();

	return pixel_data_buf;
}

DirectXScreenCapture::~DirectXScreenCapture()
{
}

HRESULT DirectXScreenCapture::Capture(RECT& rcDim, BYTE* buf, CComPtr<IWICImagingFactory>& spWICFactory) {
	DWORD dwWidth = rcDim.right - rcDim.left;
	DWORD dwHeight = rcDim.bottom - rcDim.top;

	HRESULT hr = g_DXGIManager.GetOutputBits(buf, rcDim);
	if (FAILED(hr))
	{
		return hr;
	}

	return 0;
}