#include "FramePacketizer/FramePacketizer.h"

using namespace std;

shared_ptr<FramePacket> FramePack(AVPacket* avpkt)
{
    shared_ptr<FramePacket> pkt = make_shared<FramePacket>();

    pkt.get()->pts = avpkt->pts;
    pkt.get()->dts = avpkt->dts;

    pkt.get()->size = avpkt->size;
    pkt.get()->data = avpkt->data;

    pkt.get()->stream_index = avpkt->stream_index;
    pkt.get()->flags = avpkt->flags;

    return pkt;
}
