#include "MultiThreadFrameGetter/CaptureThread.h"
#include "MultiThreadFrameGetter/PeriodicDataCollector.h"
#include "MemoryManage/PixFmtConverter.h"
#include "FramePacketizer/FrameDecoder.h"
#include "FramePacketizer/AVFrameManage.h"
#include "FramePacketizer/CoderThread/DecoderThread.h"
#include "ScreenPrinter/WinScreenPrinter.h"
#include "ScreenCapture/WinScreenCapture.h"

#include <iostream>

#define STREAM_ADDR_STR "RTPTest.sdp"

using namespace std;

class RTPFrameProcessor : public AVFrameProcessor
{
public:
	RTPFrameProcessor(AVFormatContext** formatContext, AVCodecContext** dec_codec_context, const BITMAPINFO& bmi)
		:formatContext(formatContext), dec_codec_context(dec_codec_context), s_printer(DEFALUT_WIDTH, DEFALUT_HEIGHT, bmi, frame_ref)
	{
		*formatContext = avformat_alloc_context();
		*dec_codec_context = avcodec_alloc_context3(NULL);

		const AVInputFormat* format = av_find_input_format("sdp");
		AVDictionary* formatOpts = nullptr;
		av_dict_set(&formatOpts, "protocol_whitelist", "file,udp,rtp", 0);

		// 파일 열기
		if (avformat_open_input(formatContext, STREAM_ADDR_STR, format, &formatOpts) != 0)
			fprintf(stderr, "avformat_open_input fail\n");
		else
			cout << "avformat_open_input done" << endl;

		// 파일 정보 가져오기
		if (avformat_find_stream_info(*formatContext, NULL) < 0)
			fprintf(stderr, "Cannot find stream information\n");
		else
			cout << "avformat_find_stream_info done" << endl;

		// 비디오 스트림 인덱스 찾기
		int videoStreamIndex = -1;
		for (int i = 0; i < (*formatContext)->nb_streams; i++) {
			if ((*formatContext)->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
				videoStreamIndex = i;
				break;
			}
		}

		if (videoStreamIndex == -1)
			cout << "Cannot find video stream" << endl;
		
		const AVCodec* codec = NULL;
		codec = avcodec_find_decoder((*formatContext)->streams[videoStreamIndex]->codecpar->codec_id);
		if (!codec) 
			cout << "Cannot find decoder avcodec" << endl;
		

		if (avcodec_open2(*dec_codec_context, codec, NULL) < 0)
			cout << "Cannot open codec" << endl;

		yuv_frame_data = make_shared<VideoFrameData>(DEFALUT_WIDTH * DEFALUT_HEIGHT * 4 / 2 * 3 / 4);
		s_printer.StartPrint();
	}

	~RTPFrameProcessor()
	{
		avformat_free_context(*formatContext);
	}

	void FrameProcess(AVFrame* frame) override
	{
		//std::this_thread::sleep_for(std::chrono::microseconds(1000000 / DEFALUT_FRAME_RATE));
		CopyAVFrameToRaw(frame, yuv_frame_data);
		frame_ref = cnv.ConvertYUVToBGR(yuv_frame_data);
	}

	void EndEncoding()
	{
		avformat_close_input(formatContext);
		s_printer.EndPrint();
	}
private:
	AVFormatContext** formatContext;
	AVCodecContext** dec_codec_context;
	WinScreenPrinter s_printer;
	shared_ptr<VideoFrameData> yuv_frame_data;
	PixFmtConverter cnv;
	shared_ptr<VideoFrameData> frame_ref;
};


int main() {
	//디코드 및 출력
	AVFormatContext* formatContext=nullptr;
	AVCodecContext* dec_codec_context= nullptr;
	WinScreenCapture capture_obj;
	FrameDecoder decoding_obj;
	RTPFrameProcessor prc_obj(&formatContext, &dec_codec_context, capture_obj.getBMI());
	AVFrameHandlerThread frm_thr(decoding_obj, prc_obj);
	PacketDecoderThread dec_thr(decoding_obj);

	

	//Decoding thread set
	dec_thr.StartDecoding();
	frm_thr.StartHandle();

	auto packet = AVStructPool<AVPacket*>::getInstance().getEmptyObj();

	do {
		av_read_frame(formatContext, packet.get()->getPointer());
	} while (!(packet.get()->getPointer()->flags & AV_PKT_FLAG_KEY));
	dec_thr.InputPacket(packet);
	cout << "recv key frame done" << endl;

	
	while (true) {
		auto packet = AVStructPool<AVPacket*>::getInstance().getEmptyObj();
		if (packet == NULL)
		{
			fprintf(stderr, "av_packet_alloc fail\n");
			return -1;
		}

		if (av_read_frame(formatContext, packet.get()->getPointer()) == 0)
		{
			dec_thr.InputPacket(packet);
		}
		else
		{
			cout << "av_read_frame fail" << endl;
		}
	}
	frm_thr.EndHandle();
	dec_thr.EndDecoding();

	return 0;
}