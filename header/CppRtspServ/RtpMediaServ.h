/*
   This code was written with reference to the following source.
   source 1: https://ffmpeg.org/doxygen/5.1/muxing_8c-example.html
*/
#pragma once

#include <thread>

#include "FramePacketizer/CoderThread/EncoderThread.h"

class RtpMediaServ;

class AudioPacketProcessor :public AVPacketProcessor
{
public:
	AudioPacketProcessor(RtpMediaServ& serv);
	void PacketProcess(std::shared_ptr<SharedAVPacket> pkt) override;
private:
	RtpMediaServ& serv;
};

class VideoPacketProcessor :public AVPacketProcessor
{
public:
	VideoPacketProcessor(RtpMediaServ& serv);
	void PacketProcess(std::shared_ptr<SharedAVPacket> pkt) override;
private:
	RtpMediaServ& serv;
};

class RtpMediaServ
{
public:
	RtpMediaServ(std::string url);

	bool AddStream(const AVCodecContext* codec);

	bool OpenServ();

	void RunServ();

	void RecvAudioPacket(std::shared_ptr<SharedAVPacket> input);

	void RecvVideoPacket(std::shared_ptr<SharedAVPacket> input);

	AVFormatContext* getFmtContext();

	void EndServ();

	~RtpMediaServ();

private:
	bool is_running;

	std::string url;
	AVFormatContext* formatContext;
	AVStream* audio_stream;
	AVStream* video_stream;

	MutexQueue< std::shared_ptr <SharedAVPacket>> video_buf;
	MutexQueue< std::shared_ptr <SharedAVPacket>> audio_buf;

	std::thread serv_thr;

	void ConsumePacket();

	void ServFunc();
};