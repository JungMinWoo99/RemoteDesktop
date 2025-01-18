#define _CRT_SECURE_NO_WARNINGS

#include "AudioCapture/WinAudioCapture.h"
#include "AudioPacketizer/AudioEncoder.h"
#include "MemoryManage/AVStructPool.h"

using namespace std;

#define RECORD_TIME 120
#define BYTE_PER_SAMPLE 4

// Write ADTS Header
constexpr int ADTS_HEADER_SIZE = 7;   // ADTS Header Size : 7 byte
constexpr int AUDIO_PROFILE_1 = 2;      // Audio Profile : AAC LC
constexpr int AUDIO_PROFILE_2 = 1;      // Audio Profile : AAC Main
constexpr int SAMPLE_RATE_1 = 4;        // Sampling Frequencies : 44100 Hz
constexpr int SAMPLE_RATE_2 = 3;        // Sampling Frequencies : 48000 Hz
constexpr int CHANNELS = 2;           // Channel Configurations : 2
uint8_t adts_data[7];

class MyAudioSink
{
public:
	MyAudioSink(WinAudioCapture& cap_obj, AudioEncoder& enc_obj)
		:cap_obj(cap_obj), enc_obj(enc_obj),raw_data_que(cap_obj.getDataBuffer())
	{
		const char* filename = "sample.aac";
		output = fopen(filename, "wb");
		if (!output) {
			fprintf(stderr, "Could not open %s\n", filename);
			exit(1);
		};
	}

	void RearrangeData()
	{
		static double record_time = 0;
		static shared_ptr<SharedAVFrame> frame = NULL;
		static AVFrame* frame_ptr;
		static size_t packet_blank_size;
		static size_t rest_frame_size;
		static size_t current_frame_size;
		static shared_ptr<AudioFrameData> raw_data;
		static uint8_t* tem_buf;
		auto enc_context = enc_obj.getEncCodecContext();
		tem_buf = new uint8_t[enc_context->frame_size * BYTE_PER_SAMPLE * enc_context->ch_layout.nb_channels];

		auto create_frame = [&]()
			{
				int ret;
				frame = empty_frame_buf.getEmptyObj();
				frame_ptr = frame.get()->getPointer();
				if (!frame) {
					fprintf(stderr, "Could not allocate audio frame\n");
					exit(1);
				}

				frame_ptr->nb_samples = enc_context->frame_size;
				frame_ptr->format = enc_context->sample_fmt;
				ret = av_channel_layout_copy(&frame_ptr->ch_layout, &enc_context->ch_layout);
				if (ret < 0)
					exit(1);

				ret = av_frame_get_buffer(frame_ptr, 0);
				if (ret < 0) {
					fprintf(stderr, "Could not allocate audio data buffers\n");
					exit(1);
				}

				packet_blank_size = enc_context->frame_size;
			};

		/* allocate the data buffers */
		int ret;
		int frame_size = cap_obj.getAudioSampleSize() * cap_obj.getAudioChannels();

		rest_frame_size = 0;
		create_frame();

		BYTE* current_frame_ptr;
		while (record_time < RECORD_TIME/*&& raw_data_que.size() != 0*/)
		{
			if (rest_frame_size == 0)
			{
				while (!raw_data_que.wait_and_pop_utill_not_empty(raw_data));
				int sample_num = raw_data.get()->getMemSize() / frame_size;
				record_time += (double)sample_num / cap_obj.getAudioSampleRate();
				current_frame_ptr = raw_data.get()->getMemPointer();
				current_frame_size = sample_num;
				rest_frame_size = current_frame_size;
			}

			current_frame_ptr = raw_data.get()->getMemPointer() + ((current_frame_size - rest_frame_size) * BYTE_PER_SAMPLE * frame_ptr->ch_layout.nb_channels);
			auto packet_blank_start_ptr = tem_buf + ((enc_context->frame_size - packet_blank_size) * BYTE_PER_SAMPLE * frame_ptr->ch_layout.nb_channels);

			if (packet_blank_size > rest_frame_size)
			{
				memcpy(packet_blank_start_ptr, current_frame_ptr, rest_frame_size * BYTE_PER_SAMPLE * frame_ptr->ch_layout.nb_channels);
				packet_blank_size -= rest_frame_size;
				rest_frame_size = 0;
			}
			else
			{
				memcpy(packet_blank_start_ptr, current_frame_ptr, packet_blank_size * BYTE_PER_SAMPLE * frame_ptr->ch_layout.nb_channels);
				rest_frame_size -= packet_blank_size;
				packet_blank_size = 0;

				// audio transform
				ret = swr_convert(enc_obj.swr_ctx, &frame_ptr->data[0], enc_context->frame_size, (const uint8_t**)&tem_buf, enc_context->frame_size);
				if (ret < 0) {
					fprintf(stderr, "data convert fail\n");
					exit(1);
				}

				enc_obj.EncodeFrame(frame);

				create_frame();
			}
		}

		delete[] tem_buf;
	}

	void EncodeData()
	{
		WriteAudioFile();

		//flush encoder
		enc_obj.FlushContext();

		WriteAudioFile();

		fclose(output);
	}

private:
	FILE* output;

	WinAudioCapture& cap_obj;
	AudioEncoder& enc_obj;

	AVStructPool<AVFrame*>& empty_frame_buf = AVStructPool<AVFrame*>::getInstance();

	MutexQueue<std::shared_ptr<AudioFrameData>>& raw_data_que;

	void WriteAudioFile()
	{
		/* read all the available output packets (in general there may be any
		 * number of them */
		int ret = 0;
		shared_ptr<SharedAVPacket> pkt;
		while (enc_obj.SendPacket(pkt)) {
			
			auto enced_data = pkt.get()->getPointer();

			// Write ADTS Header
			int sample_rate_header;
			int sample_rate = enc_obj.getEncCodecContext()->sample_rate;
			if (sample_rate == 48000)
				sample_rate_header = SAMPLE_RATE_2;
			else if (sample_rate == 44100)
				sample_rate_header = SAMPLE_RATE_1;
			else
			{
				cout << "This sample rate is not supported" << endl;
				exit(-1);
			}

			auto adts_size = enced_data->size + ADTS_HEADER_SIZE;
			adts_data[0] = static_cast<uint8_t>(0xFF);
			adts_data[1] = static_cast<uint8_t>(0xF9);
			adts_data[2] = static_cast<uint8_t>(((AUDIO_PROFILE_1 - 1) << 6) + (sample_rate_header << 2) + (CHANNELS >> 2));
			adts_data[3] = static_cast<uint8_t>(((CHANNELS & 3) << 6) + (adts_size >> 11));
			adts_data[4] = static_cast<uint8_t>((adts_size & 0x7FF) >> 3);
			adts_data[5] = static_cast<uint8_t>(((adts_size & 7) << 5) + 0x1F);
			adts_data[6] = static_cast<uint8_t>(0xFC);

			fwrite(adts_data, 1, ADTS_HEADER_SIZE, output);

			fwrite(enced_data->data, 1, enced_data->size, output);
		}
	}
};

int main(void)
{
	WinAudioCapture win_cap;
	AudioEncoder enc_obj(win_cap.getAudioChannels(), win_cap.getAudioSampleRate(), win_cap.getAudioSampleFormat(), AV_CODEC_ID_AAC);
	MyAudioSink audio_sink(win_cap, enc_obj);
	win_cap.StartCapture();
	audio_sink.RearrangeData();
	win_cap.EndCapture();
	audio_sink.EncodeData();
	return 0;
}