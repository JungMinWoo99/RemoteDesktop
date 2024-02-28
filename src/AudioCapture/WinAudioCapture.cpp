/*
   This code was written with reference to the following source.
   source: https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording?redirectedfrom=MSDN
*/

#include "AudioCapture/WinAudioCapture.h"

#include <iostream>

#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000
#define AUDIO_DATA_DURATION REFTIMES_PER_SEC / 10

#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

using namespace std;
WinAudioCapture::WinAudioCapture()
	:raw_data_que("AudioRawDataBuffer"), continue_capture(false)
{
	const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
	const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
	const IID IID_IAudioClient = __uuidof(IAudioClient);
	const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

	WAVEFORMATEX* pwfx = NULL;

	hr = CoInitialize(NULL);
	CheckError(hr);

	hr = CoCreateInstance(
		CLSID_MMDeviceEnumerator, NULL,
		CLSCTX_ALL, IID_IMMDeviceEnumerator,
		(void**)&pEnumerator);
	CheckError(hr);

	hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
	CheckError(hr);

	hr = pDevice->Activate(
		IID_IAudioClient, CLSCTX_ALL,
		NULL, (void**)&pAudioClient);
	CheckError(hr);

	hr = pAudioClient->GetMixFormat(&pwfx);
	CheckError(hr);

	audio_channels = pwfx->nChannels;
	sample_rate = pwfx->nSamplesPerSec;
	byte_per_sample = pwfx->wBitsPerSample / 8;

	if (pwfx->wFormatTag == WAVE_FORMAT_PCM)
	{
		cout << "this source is pcm format" << endl;
		raw_data_format = AV_SAMPLE_FMT_S16;

	}
	else if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
	{
		WAVEFORMATEXTENSIBLE* pWfxExtensible = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pwfx);
		pWfxExtensible->Format.wFormatTag;
		vaild_bit_per_sample = pWfxExtensible->Samples.wValidBitsPerSample;
		if (IsEqualGUID(pWfxExtensible->SubFormat, KSDATAFORMAT_SUBTYPE_PCM))
		{
			cout << "this source is pcm format" << endl;
			raw_data_format = AV_SAMPLE_FMT_S16;
		}
		else if (IsEqualGUID(pWfxExtensible->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
		{
			cout << "this source is not pcm format: KSDATAFORMAT_SUBTYPE_IEEE_FLOAT" << endl;
			raw_data_format = AV_SAMPLE_FMT_FLT;
		}
		else
		{
			cout << "this source is not pcm format: unknown" << endl;
			exit(-1);
		}
	}
	else
	{
		cout << "this source is not pcm format: unknown" << endl;
		exit(-1);
	}

	hr = pAudioClient->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		AUDCLNT_STREAMFLAGS_LOOPBACK,
		AUDIO_DATA_DURATION,
		0,
		pwfx,
		NULL);
	CheckError(hr);

	// Get the size of the allocated buffer.
	hr = pAudioClient->GetBufferSize(&max_buffer_frame_count);
	CheckError(hr);

	hr = pAudioClient->GetService(
		IID_IAudioCaptureClient,
		(void**)&pCaptureClient);
	CheckError(hr);

	CoTaskMemFree(pwfx);
}

void WinAudioCapture::StartCapture()
{
	continue_capture = true;
	hr = pAudioClient->Start();  // Start recording.
	CheckError(hr);
	cap_thread = thread(&WinAudioCapture::CaptureFunc, this);
}

MutexQueue<std::shared_ptr<AudioFrameData>>& WinAudioCapture::getDataBuffer()
{
	return raw_data_que;
}

int WinAudioCapture::getAudioChannels()
{
	return audio_channels;
}

int WinAudioCapture::getAudioSampleRate()
{
	return sample_rate;
}

int WinAudioCapture::getAudioSampleSize()
{
	return byte_per_sample;
}

AVSampleFormat WinAudioCapture::getAudioSampleFormat()
{
	return raw_data_format;
}

void WinAudioCapture::EndCapture()
{
	continue_capture = false;
	cap_thread.join();
	hr = pAudioClient->Stop();  // Stop recording.
	CheckError(hr);
}

WinAudioCapture::~WinAudioCapture()
{
	if (continue_capture)
	{
		cout << "WinAudioCapture is broken while capture is still running." << endl;
		EndCapture();
	}

	SAFE_RELEASE(pEnumerator);
	SAFE_RELEASE(pDevice);
	SAFE_RELEASE(pAudioClient);
	SAFE_RELEASE(pCaptureClient);
}

void WinAudioCapture::CheckError(HRESULT hr)
{
	if (FAILED(hr))
		exit(hr);
}

void WinAudioCapture::CaptureFunc()
{
	UINT32 packetLength;
	BYTE* pData;//GetBuffer return 
	UINT32 numFramesAvailable;//GetBuffer return 
	DWORD flags;//GetBuffer result flag 

	// Each loop fills about half of the shared buffer.
	while (continue_capture)
	{
		// Sleep for half the buffer duration.
		Sleep(AUDIO_DATA_DURATION/ REFTIMES_PER_MILLISEC/2);

		hr = pCaptureClient->GetNextPacketSize(&packetLength);//The client calls GetNextPacketSize before each pair of calls to GetBuffer and ReleaseBuffer until GetNextPacketSize reports a packet size of 0
		CheckError(hr);

			while (packetLength != 0)
			{
				// Get the available data in the shared buffer.
				hr = pCaptureClient->GetBuffer(
					&pData,
					&numFramesAvailable,
					&flags, NULL, NULL);
				CheckError(hr);

				// Copy the available capture data to the audio sink.
				int buf_size = numFramesAvailable * byte_per_sample * audio_channels;
				uint8_t* buf = (BYTE*)malloc(buf_size);
				if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
					memset(buf, 0, buf_size);  // write silence.
				else
					memcpy(buf, pData, buf_size);
				raw_data_que.push(make_shared<AudioFrameData>(buf, (int)buf_size));

				hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);//Release the buffer obtained by GetBuffer
				CheckError(hr);

				hr = pCaptureClient->GetNextPacketSize(&packetLength);//The client calls GetNextPacketSize before each pair of calls to GetBuffer and ReleaseBuffer until GetNextPacketSize reports a packet size of 0
				CheckError(hr);
			}
	}
}
