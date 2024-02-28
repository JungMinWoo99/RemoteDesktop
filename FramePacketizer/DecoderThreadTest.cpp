#include "MultiThreadFrameGetter/PeriodicDataCollector.h"
#include "MemoryManage/PixFmtConverter.h"
#include "FramePacketizer/FrameDecoder.h"
#include "ScreenPrinter/WinScreenPrinter.h"
#include "MultiThreadFrameGetter/CaptureThread.h"
#include "FramePacketizer/CoderThread/DecoderThread.h"
#include "FramePacketizer/AVFrameManage.h"

#include <iostream>

using namespace std;

class TestFrameProcessor : public AVFrameProcessor
{
public:
    TestFrameProcessor(AVFormatContext** formatContext, const BITMAPINFO& bmi)
        :formatContext(formatContext), s_printer(DEFALUT_WIDTH, DEFALUT_HEIGHT, bmi, frame_ref)
    {
        // 파일 경로
        const char* filename = "output.avi";
        // AVFormatContext 생성
        *formatContext = avformat_alloc_context();

        // 파일 열기
        if (avformat_open_input(formatContext, filename, NULL, NULL) != 0) {
            fprintf(stderr, "Cannot open input file\n");
            exit(-1);
        }

        // 파일 정보 가져오기
        if (avformat_find_stream_info(*formatContext, NULL) < 0) {
            fprintf(stderr, "Cannot find stream information\n");
            exit(-1);
        }

        // 비디오 스트림 인덱스 찾기
        int videoStreamIndex = -1;
        for (int i = 0; i < (*formatContext)->nb_streams; i++) {
            if ((*formatContext)->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoStreamIndex = i;
                break;
            }
        }

        if (videoStreamIndex == -1) {
            fprintf(stderr, "Cannot find video stream\n");
            exit(-1);
        }

        yuv_frame_data = make_shared<VideoFrameData>(DEFALUT_WIDTH * DEFALUT_HEIGHT * 4 / 2 * 3 / 4);

        s_printer.StartPrint();
    }
    ~TestFrameProcessor()
    {
        s_printer.EndPrint();
        avformat_close_input(formatContext);
    }
    void FrameProcess(SharedAVFrame frame) override
    {
        auto avfrm = frame.get()->getPointer();
        std::this_thread::sleep_for(std::chrono::microseconds(1000000 / DEFALUT_FRAME_RATE));
        CopyAVFrameToRaw(avfrm, yuv_frame_data);
        frame_ref = cnv.ConvertYUVToBGR(yuv_frame_data);
    }
private:
    AVFormatContext** formatContext;
    WinScreenPrinter s_printer;
    shared_ptr<VideoFrameData> yuv_frame_data;
    PixFmtConverter cnv;
    shared_ptr<VideoFrameData> frame_ref;

};

int main() {
    //디코더 생성
    ScreenCapture capture_obj(DEFALUT_WIDTH, DEFALUT_HEIGHT);
    FrameDecoder decoder;
    AVFormatContext* formatContext;
    TestFrameProcessor frame_proc(&formatContext, capture_obj.getBMI());
    AVFrameHandlerThread dec_thr(decoder, frame_proc);
    PacketDecoderThread pkt_thr(decoder);

    dec_thr.StartHandle();
    pkt_thr.StartDecoding();
    // 파일에서 패킷 읽기
    while (true) {
        // AVPacket 할당

        SharedAVPacket packet = MakeSharedAVStruct<AVPacket*>();

        //읽을 프레임이 없는 경우
        if (av_read_frame(formatContext, packet.get()->getPointer()) < 0)
            break;

        // 패킷 처리

        // 여기에서 AVPacket을 사용하여 원하는 동작을 수행할 수 있습니다.
        // 예: 비디오 프레임 디코딩, 처리 등.
        pkt_thr.InputPacket(packet);
    }
    while (true);
    pkt_thr.EndDecoding();
    dec_thr.EndHandle();
   

    return 0;
}