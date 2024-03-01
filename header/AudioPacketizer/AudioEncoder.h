#include <thread>

#include "AudioCapture/WinAudioCapture.h"
#include "MemoryManage/AVStructPool.h"

class AudioEncoder
{
public:
	AudioEncoder(WinAudioCapture& cap_obj);

	_Check_return_ bool EncodeFrame(std::shared_ptr<SharedAVFrame> input);

	_Check_return_ bool SendPacket(std::shared_ptr<SharedAVPacket>& packet);

	_Check_return_ bool SendPacketBlocking(std::shared_ptr<SharedAVPacket>& packet);

	void FlushContext();

	const AVCodec* getEncCodec();

	AVCodecContext* getEncCodecContext();

	size_t getBufferSize();

	~AudioEncoder();
private:
	_Check_return_ bool FillPacketBuf();

	const AVCodec* enc_codec;
	AVCodecContext* enc_context;

	static std::ofstream log_stream;

	SwrContext* swr_ctx;
	WinAudioCapture& cap_obj;

	int audio_channels;
	int sample_rate;
	int bit_rate;
	int bit_per_sample;
	int sample_per_block;
	AVSampleFormat raw_data_format;

	AVStructPool<AVPacket*>& empty_packet_buf = AVStructPool<AVPacket*>::getInstance();

	MutexQueue<std::shared_ptr<SharedAVPacket>> enced_packet_buf;

	std::mutex encoder_mtx;
};