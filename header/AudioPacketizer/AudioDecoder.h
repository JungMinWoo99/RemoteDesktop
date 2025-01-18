#include "FramePacketizer/FrameDecoder.h"

class AudioDecoder : public FrameDecoder
{
public:
	AudioDecoder(	int audio_channels,
					int sample_rate,
					AVSampleFormat raw_data_format,
					AVCodecID codec_id);
private:
	int audio_channels;
	int sample_rate;
	AVSampleFormat raw_data_format;
	AVCodecID codec_id;
};