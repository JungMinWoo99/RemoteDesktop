/*
   This code was written with reference to the following source.
   source 1: https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording?redirectedfrom=MSDN
*/
#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}


#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

#include <thread>
#include <memory>

#include "MemoryManage/FrameData.h"
#include "MutexQueue/MutexQueue.h"

class WinAudioCapture
{
public:
    WinAudioCapture();

	void StartCapture();

	MutexQueue<std::shared_ptr<AudioFrameData>>& getDataBuffer();

	int getAudioChannels();
	int getAudioSampleRate();
	int getAudioSampleSize();
	AVSampleFormat getAudioSampleFormat();


	void EndCapture();

	~WinAudioCapture();
private:
    HRESULT hr;
	UINT32 max_buffer_frame_count;
	IMMDeviceEnumerator* pEnumerator = NULL;
	IMMDevice* pDevice = NULL;
	IAudioClient* pAudioClient = NULL;
	IAudioCaptureClient* pCaptureClient = NULL;

	int audio_channels;
	int sample_rate;
	int byte_per_sample;
	int vaild_bit_per_sample;
	AVSampleFormat raw_data_format;

	std::thread cap_thread;

	MutexQueue<std::shared_ptr<AudioFrameData>> raw_data_que;

	bool continue_capture;

    void CheckError(HRESULT hr);

	void CaptureFunc();
};


