/*
   This code was written with reference to the following source.
   source 1: https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording?redirectedfrom=MSDN
   source 2: https://www.hagulu.com/150
   source 3: https://ffmpeg.org/doxygen/trunk/encode_audio_8c-example.html#a27
*/

//-----------------------------------------------------------
// Record an audio stream from the default audio capture
// device. The RecordAudioStream function allocates a shared
// buffer big enough to hold one second of PCM audio data.
// The function uses this buffer to stream data from the
// capture device. The main loop runs every 1/2 second.
//-----------------------------------------------------------

// REFERENCE_TIME time units per second and per millisecond

#define _CRT_SECURE_NO_WARNINGS

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

#include <iostream>
#include <queue>

#define RECORD_TIME 30
#define BYTE_PER_SAMPLE 4

#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto Exit; }
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

// Write ADTS Header
constexpr int ADTS_HEADER_SIZE = 7;   // ADTS Header Size : 7 byte
constexpr int AUDIO_PROFILE_1 = 2;      // Audio Profile : AAC LC
constexpr int AUDIO_PROFILE_2 = 1;      // Audio Profile : AAC Main
constexpr int SAMPLE_RATE_1 = 4;        // Sampling Frequencies : 44100 Hz
constexpr int SAMPLE_RATE_2 = 3;        // Sampling Frequencies : 48000 Hz
constexpr int CHANNELS = 2;           // Channel Configurations : 2
uint8_t adts_data[7];

FILE* raw;

using namespace std;

class MyAudioSink
{
public:
	MyAudioSink()
	{
		enc_codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
		if (enc_codec == NULL)
		{
			cout << "find_encoder fail" << endl;
			exit(-1);
		}

		enc_context = avcodec_alloc_context3(enc_codec);
		if (enc_context == NULL)
		{
			cout << "alloc encoder context fail" << endl;
			exit(-1);
		}

		const char* filename = "sample.aac";
		output = fopen(filename, "wb");
		if (!output) {
			fprintf(stderr, "Could not open %s\n", filename);
			exit(1);
		};
	}

	HRESULT SetFormat(WAVEFORMATEX* pwfx)
	{
		audio_channels = pwfx->nChannels;
		sample_rate = pwfx->nSamplesPerSec;
		bit_rate = pwfx->nAvgBytesPerSec * 8;
		bit_per_sample = pwfx->wBitsPerSample;

		int ret;

		if (pwfx->wFormatTag == WAVE_FORMAT_PCM)
		{
			cout << "this source is pcm format" << endl;
			raw_data_format = AV_SAMPLE_FMT_S16;

		}
		else if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
		{
			WAVEFORMATEXTENSIBLE* pWfxExtensible = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pwfx);
			pWfxExtensible->Format.wFormatTag;
			bit_per_sample = pWfxExtensible->Samples.wValidBitsPerSample;
			sample_per_block = pWfxExtensible->Samples.wSamplesPerBlock;
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

		/* put sample parameters */
		enc_context->bit_rate = 576000 * 2;

		/* check that the encoder supports float input */
		enc_context->sample_fmt = AV_SAMPLE_FMT_FLTP;
		if (!check_sample_fmt(enc_codec, enc_context->sample_fmt)) {
			fprintf(stderr, "Encoder does not support sample format %s",
				av_get_sample_fmt_name(enc_context->sample_fmt));
			exit(1);
		}

		/* select other audio parameters supported by the encoder */
		enc_context->sample_rate = select_sample_rate(enc_codec, sample_rate);
		ret = select_channel_layout(enc_codec, &enc_context->ch_layout, audio_channels);
		if (ret < 0)
			exit(1);

		/* open it */
		if (avcodec_open2(enc_context, enc_codec, NULL) < 0) {
			fprintf(stderr, "Could not open codec\n");
			exit(1);
		}

		// Input audio parameters
		AVSampleFormat in_sample_fmt = raw_data_format;
		int in_sample_rate = sample_rate;
		int in_channels = audio_channels;

		// Output audio parameters
		AVSampleFormat out_sample_fmt = enc_context->sample_fmt;
		int out_sample_rate = sample_rate;
		int out_channels = audio_channels;

		// Create audio conversion context
		ret = swr_alloc_set_opts2(&swr_ctx,
			&enc_context->ch_layout,
			out_sample_fmt,
			out_sample_rate,
			&enc_context->ch_layout,
			in_sample_fmt,
			in_sample_rate,
			0,
			NULL);
		swr_init(swr_ctx);

		return ret;
	}

	HRESULT CopyData(BYTE* data, UINT32 frame_num, BOOL* done)
	{
		static UINT32 sample_sum = 0;
		sample_sum += frame_num;
		if (sample_sum / enc_context->sample_rate >= RECORD_TIME)
			*done = TRUE;
		else
			*done = FALSE;

		//caputre raw data
		fwrite(data, 1, frame_num * BYTE_PER_SAMPLE * audio_channels, raw);

		static uint8_t* buf = NULL;
		int buf_size = frame_num * BYTE_PER_SAMPLE * audio_channels;
		buf = new uint8_t[buf_size];
		memcpy(buf, data, buf_size);

		raw_data_que.push(make_pair(buf, (int)frame_num));

		return 0;
	}

	void RearrangeData()
	{
		static AVFrame* frame = NULL;
		static size_t packet_blank_size;
		static size_t rest_frame_size;
		static size_t current_frame_size;
		static uint8_t* current_frame_ptr = NULL;
		static uint8_t* tem_buf;
		tem_buf = new uint8_t[enc_context->frame_size * BYTE_PER_SAMPLE * audio_channels];

		auto create_frame = [&]()
			{
				int ret;
				frame = av_frame_alloc();
				if (!frame) {
					fprintf(stderr, "Could not allocate audio frame\n");
					exit(1);
				}

				frame->nb_samples = enc_context->frame_size;
				frame->format = enc_context->sample_fmt;
				ret = av_channel_layout_copy(&frame->ch_layout, &enc_context->ch_layout);
				if (ret < 0)
					exit(1);

				ret = av_frame_get_buffer(frame, 0);
				if (ret < 0) {
					fprintf(stderr, "Could not allocate audio data buffers\n");
					exit(1);
				}

				packet_blank_size = enc_context->frame_size;
			};

		/* allocate the data buffers */
		int ret;

		rest_frame_size = 0;
		create_frame();

		while (!raw_data_que.empty())
		{
			if (rest_frame_size == 0)
			{
				if (current_frame_ptr != NULL)
				{
					delete[] raw_data_que.front().first;
					raw_data_que.pop();
					if (raw_data_que.empty())
						break;
				}
				current_frame_ptr = raw_data_que.front().first;
				current_frame_size = raw_data_que.front().second;
				rest_frame_size = current_frame_size;
			}

			current_frame_ptr = raw_data_que.front().first + ((current_frame_size - rest_frame_size) * BYTE_PER_SAMPLE * audio_channels);
			auto packet_blank_start_ptr = tem_buf + ((enc_context->frame_size - packet_blank_size) * BYTE_PER_SAMPLE * audio_channels);

			if (packet_blank_size > rest_frame_size)
			{
				memcpy(packet_blank_start_ptr, current_frame_ptr, rest_frame_size * BYTE_PER_SAMPLE * audio_channels);
				packet_blank_size -= rest_frame_size;
				rest_frame_size = 0;
			}
			else
			{
				memcpy(packet_blank_start_ptr, current_frame_ptr, packet_blank_size * BYTE_PER_SAMPLE * audio_channels);
				rest_frame_size -= packet_blank_size;
				packet_blank_size = 0;
				// audio transform
				ret = swr_convert(swr_ctx, &frame->data[0], enc_context->frame_size, (const uint8_t**)&tem_buf, enc_context->frame_size);
				if (ret < 0) {
					fprintf(stderr, "data convert fail\n");
					exit(1);
				}

				frame_que.push(frame);

				create_frame();
			}
		}

		delete[] tem_buf;
	}

	void EncodeData()
	{
		RearrangeData();

		int ret = 0;

		/* send the frame for encoding */
		while (!frame_que.empty())
		{
			auto frame = frame_que.front();
			ret = avcodec_send_frame(enc_context, frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);
			else if (ret < 0) {
				fprintf(stderr, "Error sending the frame to the encoder\n");
				exit(1);
			}
			else
			{
				frame_que.pop();
				av_frame_free(&frame);
			}

			WriteAudioFile();
		}

		//flush encoder
		ret = avcodec_send_frame(enc_context, NULL);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);
		else if (ret < 0) {
			fprintf(stderr, "Error sending the frame to the encoder\n");
			exit(1);
		}

		WriteAudioFile();

		fclose(output);
	}

private:
	FILE* output;

	const AVCodec* enc_codec;
	AVCodecContext* enc_context;

	SwrContext* swr_ctx;

	int audio_channels;
	int sample_rate;
	int bit_rate;
	int bit_per_sample;
	int sample_per_block;
	AVSampleFormat raw_data_format;

	queue<pair<uint8_t*, int>> raw_data_que;
	queue<AVFrame*> frame_que;

	void WriteAudioFile()
	{
		/* packet for holding encoded output */
		static AVPacket* pkt = NULL;
		if (pkt == NULL)
		{
			pkt = av_packet_alloc();
			if (!pkt) {
				fprintf(stderr, "could not allocate the packet\n");
				exit(1);
			}
		}

		/* read all the available output packets (in general there may be any
		 * number of them */
		int ret = 0;
		while (ret >= 0) {
			ret = avcodec_receive_packet(enc_context, pkt);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			else if (ret < 0) {
				fprintf(stderr, "Error encoding audio frame\n");
				exit(1);
			}

			// Write ADTS Header
			int sample_rate_header;
			if (sample_rate == 48000)
				sample_rate_header = SAMPLE_RATE_2;
			else if (sample_rate == 44100)
				sample_rate_header = SAMPLE_RATE_1;
			else
			{
				cout << "This sample rate is not supported" << endl;
				exit(-1);
			}

			auto adts_size = pkt->size + ADTS_HEADER_SIZE;
			adts_data[0] = static_cast<uint8_t>(0xFF);
			adts_data[1] = static_cast<uint8_t>(0xF9);
			adts_data[2] = static_cast<uint8_t>(((AUDIO_PROFILE_1 - 1) << 6) + (sample_rate_header << 2) + (CHANNELS >> 2));
			adts_data[3] = static_cast<uint8_t>(((CHANNELS & 3) << 6) + (adts_size >> 11));
			adts_data[4] = static_cast<uint8_t>((adts_size & 0x7FF) >> 3);
			adts_data[5] = static_cast<uint8_t>(((adts_size & 7) << 5) + 0x1F);
			adts_data[6] = static_cast<uint8_t>(0xFC);

			fwrite(adts_data, 1, ADTS_HEADER_SIZE, output);

			fwrite(pkt->data, 1, pkt->size, output);
			av_packet_unref(pkt);
		}
	}

	/* check that a given sample format is supported by the encoder */
	static int check_sample_fmt(const AVCodec* codec, enum AVSampleFormat sample_fmt)
	{
		const enum AVSampleFormat* p = codec->sample_fmts;

		while (*p != AV_SAMPLE_FMT_NONE) {
			if (*p == sample_fmt)
				return 1;
			p++;
		}
		return 0;
	}

	/* just pick the highest supported samplerate */
	static int select_sample_rate(const AVCodec* codec, int spec_rate = 0)
	{
		const int* p;
		int best_samplerate = 0;

		if (!codec->supported_samplerates)
			return 44100;

		p = codec->supported_samplerates;
		while (*p) {
			if (!best_samplerate || abs(44100 - *p) < abs(44100 - best_samplerate))
				best_samplerate = *p;
			if (spec_rate == best_samplerate)
				break;
			p++;
		}
		if (best_samplerate != spec_rate)
			cout << "Encoder does not support sample rate " << spec_rate << endl;

		return best_samplerate;
	}

	/* select layout with the highest channel count */
	static int select_channel_layout(const AVCodec* codec, AVChannelLayout* dst, int spec_channels = 0)
	{
		const AVChannelLayout* p;
		const AVChannelLayout* best_ch_layout = NULL;
		int best_nb_channels = 0;


		if (!codec->ch_layouts)
		{
			AVChannelLayout av_audio_layout = AV_CHANNEL_LAYOUT_STEREO;
			return av_channel_layout_copy(dst, &av_audio_layout);
		}


		p = codec->ch_layouts;
		while (p->nb_channels) {
			int nb_channels = p->nb_channels;

			if (nb_channels > best_nb_channels) {
				best_ch_layout = p;
				best_nb_channels = nb_channels;
			}
			p++;
		}

		if (spec_channels != 0)
		{
			if (spec_channels <= best_nb_channels)
				best_nb_channels = spec_channels;
			else
				cout << "Encoder does not support channels " << spec_channels << endl;
		}

		return av_channel_layout_copy(dst, best_ch_layout);
	}
};

HRESULT RecordAudioStream(MyAudioSink* pMySink)
{
	HRESULT hr;
	REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
	REFERENCE_TIME hnsActualDuration;
	UINT32 bufferFrameCount;
	UINT32 numFramesAvailable;
	IMMDeviceEnumerator* pEnumerator = NULL;
	IMMDevice* pDevice = NULL;
	IAudioClient* pAudioClient = NULL;
	IAudioCaptureClient* pCaptureClient = NULL;
	WAVEFORMATEX* pwfx = NULL;
	UINT32 packetLength = 0;
	BOOL bDone = FALSE;
	BYTE* pData;
	DWORD flags;

	hr = CoInitialize(NULL);
	EXIT_ON_ERROR(hr)

		hr = CoCreateInstance(
			CLSID_MMDeviceEnumerator, NULL,
			CLSCTX_ALL, IID_IMMDeviceEnumerator,
			(void**)&pEnumerator);
	EXIT_ON_ERROR(hr)

		hr = pEnumerator->GetDefaultAudioEndpoint(
			eRender, eConsole, &pDevice);
	EXIT_ON_ERROR(hr)

		hr = pDevice->Activate(
			IID_IAudioClient, CLSCTX_ALL,
			NULL, (void**)&pAudioClient);
	EXIT_ON_ERROR(hr)

		hr = pAudioClient->GetMixFormat(&pwfx);
	EXIT_ON_ERROR(hr)

		hr = pAudioClient->Initialize(
			AUDCLNT_SHAREMODE_SHARED,
			AUDCLNT_STREAMFLAGS_LOOPBACK,
			hnsRequestedDuration,
			0,
			pwfx,
			NULL);
	EXIT_ON_ERROR(hr)

		// Get the size of the allocated buffer.
		hr = pAudioClient->GetBufferSize(&bufferFrameCount);
	EXIT_ON_ERROR(hr)

		hr = pAudioClient->GetService(
			IID_IAudioCaptureClient,
			(void**)&pCaptureClient);
	EXIT_ON_ERROR(hr)

		// Notify the audio sink which format to use.
		hr = pMySink->SetFormat(pwfx);
	EXIT_ON_ERROR(hr)

		// Calculate the actual duration of the allocated buffer.
		hnsActualDuration = (double)REFTIMES_PER_SEC *
		bufferFrameCount / pwfx->nSamplesPerSec;

	hr = pAudioClient->Start();  // Start recording.
	EXIT_ON_ERROR(hr)

		// Each loop fills about half of the shared buffer.
		while (bDone == FALSE)
		{
			// Sleep for half the buffer duration.
			Sleep(hnsActualDuration / REFTIMES_PER_MILLISEC / 2);

			hr = pCaptureClient->GetNextPacketSize(&packetLength);
			EXIT_ON_ERROR(hr)

				while (packetLength != 0)
				{
					// Get the available data in the shared buffer.
					hr = pCaptureClient->GetBuffer(
						&pData,
						&numFramesAvailable,
						&flags, NULL, NULL);
					EXIT_ON_ERROR(hr)

						if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
						{
							pData = NULL;  // Tell CopyData to write silence.
						}

					// Copy the available capture data to the audio sink.
					hr = pMySink->CopyData(
						pData, numFramesAvailable, &bDone);
					EXIT_ON_ERROR(hr)

						hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
					EXIT_ON_ERROR(hr)

						hr = pCaptureClient->GetNextPacketSize(&packetLength);
					EXIT_ON_ERROR(hr)
				}
		}

	hr = pAudioClient->Stop();  // Stop recording.
	EXIT_ON_ERROR(hr)
		pMySink->EncodeData();

Exit:
	CoTaskMemFree(pwfx);
	SAFE_RELEASE(pEnumerator)
		SAFE_RELEASE(pDevice)
		SAFE_RELEASE(pAudioClient)
		SAFE_RELEASE(pCaptureClient)

		return hr;
}

int main()
{
	const char* filename = "raw_data.bin";
	raw = fopen(filename, "wb");
	MyAudioSink audio;
	auto ret = RecordAudioStream(&audio);
	fclose(raw);
	return ret;
}