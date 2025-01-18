#define _CRT_SECURE_NO_WARNINGS

#include "MultiThreadFrameGetter/PeriodicDataCollector.h"
#include "AudioPacketizer/AudioDecoder.h"
#include "MultiThreadFrameGetter/CaptureThread.h"
#include "FramePacketizer/CoderThread/DecoderThread.h"
#include "MemoryManage/AVFrameManage.h"
#include "MemoryManage/AVStructPool.h"

#include <iostream>

using namespace std;

int data_size;

class TestFrameProcessor : public AVFrameProcessor
{
public:
    TestFrameProcessor(AVFormatContext** formatContext)
        :formatContext(formatContext)
    {
        //���� ����
        const char* output_file_name = "raw_data.bin";
        output = fopen(output_file_name, "wb");

        // AVFormatContext ����
        *formatContext = avformat_alloc_context();

        // ���� ����
        if (avformat_open_input(formatContext, "sample.aac", NULL, NULL) != 0) {
            fprintf(stderr, "Cannot open input file\n");
            exit(-1);
        }

        // ���� ���� ��������
        if (avformat_find_stream_info(*formatContext, NULL) < 0) {
            fprintf(stderr, "Cannot find stream information\n");
            exit(-1);
        }

        // ���� ��Ʈ�� �ε��� ã��
        int videoStreamIndex = -1;
        for (int i = 0; i < (*formatContext)->nb_streams; i++) {
            if ((*formatContext)->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                videoStreamIndex = i;
                break;
            }
        }

        if (videoStreamIndex == -1) {
            fprintf(stderr, "Cannot find video stream\n");
            exit(-1);
        }
    }
    ~TestFrameProcessor()
    {
        avformat_close_input(formatContext);
        fclose(output);
    }

    void FrameProcess(shared_ptr<SharedAVFrame> pkt) override
    {
        auto frame = pkt.get()->getPointer();
        for (int i = 0; i < frame->nb_samples; i++)
            for (int ch = 0; ch < 2; ch++)
                fwrite(frame->data[ch] + data_size * i, 1, data_size, output);
    }
private:
    FILE* output;
    AVFormatContext** formatContext;
    shared_ptr<VideoFrameData> yuv_frame_data;
    shared_ptr<VideoFrameData> frame_ref;

};

int main() {
    //���ڴ� ����
    AudioDecoder decoder(2,48000,AV_SAMPLE_FMT_FLTP,AV_CODEC_ID_AAC);
    data_size = av_get_bytes_per_sample(decoder.getDecCodecContext()->request_sample_fmt);

    AVFormatContext* formatContext;
    TestFrameProcessor frame_proc(&formatContext);
    AVFrameHandlerThread frm_thr(decoder, frame_proc);
    frm_thr.StartHandle();

    // ���Ͽ��� ��Ŷ �б�
    int i = 0;
    while (true) {
        // AVPacket �Ҵ�
        auto packet = AVStructPool<AVPacket*>::getInstance().getEmptyObj();

        //���� �������� ���� ���
        if (av_read_frame(formatContext, packet.get()->getPointer()) < 0)
            break;
        cout << i++ << endl;
        decoder.DecodePacket(packet);
    }
    Sleep(10000);

    frm_thr.EndHandle();
    return 0;
}