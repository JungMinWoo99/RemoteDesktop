#include "FramePacketizer/FrameDecoder.h"

class VideoDecoder : public FrameDecoder
{
public:
	VideoDecoder(	int w = DEFALUT_WIDTH, 
					int h = DEFALUT_HEIGHT, 
					int frame_rate = DEFALUT_FRAME_RATE, 
					AVCodecID coedec_id = AV_CODEC_ID_H264);

private:
	int w;
	int h;
	int frame_rate;
	AVCodecID coedec_id;
	
};