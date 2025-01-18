#include "CppRtspServ/RtpMediaServ.h"

using namespace std;



AudioPacketProcessor::AudioPacketProcessor(RtpMediaServ& serv)
	:serv(serv)
{
}

void AudioPacketProcessor::PacketProcess(shared_ptr<SharedAVPacket> pkt)
{
	serv.RecvAudioPacket(pkt);
}

VideoPacketProcessor::VideoPacketProcessor(RtpMediaServ& serv)
	:serv(serv)
{
}
void VideoPacketProcessor::PacketProcess(shared_ptr<SharedAVPacket> pkt)
{
	serv.RecvVideoPacket(pkt);
}

inline RtpMediaServ::RtpMediaServ(string url)
	:url(url), audio_stream(NULL), video_stream(NULL), video_buf("RtpVideoPacketQueue"), audio_buf("RtpAudioPacketQueue")
{
	avformat_network_init();
	if (avformat_alloc_output_context2(&formatContext, nullptr, "rtp", url.c_str()) < 0)
	{
		cout << "avformat_alloc_output_context2 fail" << endl;
		exit(-2);
	}
	else
		cout << "formatContext alloc done" << endl;
}

inline bool RtpMediaServ::AddStream(const AVCodecContext* codec)
{
	int ret;
	if (codec->codec_type == AVMEDIA_TYPE_VIDEO)
	{
		// Video codec.
		video_stream = avformat_new_stream(formatContext, NULL);
		video_stream->id = formatContext->nb_streams - 1;
		video_stream->time_base = codec->time_base;
		/* copy the stream parameters to the muxer */
		ret = avcodec_parameters_from_context(video_stream->codecpar, codec);
		if (ret < 0)
		{
			fprintf(stderr, "Could not copy the stream parameters\n");
			exit(1);
		}
	}
	else if (codec->codec_type == AVMEDIA_TYPE_AUDIO)
	{
		// Audio codec.
		audio_stream = avformat_new_stream(formatContext, NULL);
		audio_stream->id = formatContext->nb_streams - 1;
		audio_stream->time_base = codec->time_base;
		/* copy the stream parameters to the muxer */
		ret = avcodec_parameters_from_context(audio_stream->codecpar, codec);
		if (ret < 0)
		{
			fprintf(stderr, "Could not copy the stream parameters\n");
			exit(1);
		}
	}
	else
	{
		// Codec of a different type, neither video nor audio.
		printf("Codec of a different type, neither video nor audio.\n");
		return false;
	}
}

inline bool RtpMediaServ::OpenServ()
{
	int ret;
	/* open the output file, if needed */
	if (!(formatContext->flags & AVFMT_NOFILE))
	{
		ret = avio_open(&formatContext->pb, url.c_str(), AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			// ERROR 
			char errorBuff[80];
			fprintf(stderr, "Could not open outfile '%s': %s", url.c_str(), av_make_error_string(errorBuff, 80, ret));
			exit(ret);
		}
	}

	/* Write the stream header, if any. */
	ret = avformat_write_header(formatContext, NULL);
	if (ret < 0) {
		char errorStr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, ret);
		cout << errorStr << endl;
		exit(ret);
	}

	/* Write the stream header, if any. */
	ret = avformat_write_header(formatContext, NULL);
	if (ret < 0) {
		char errorStr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, ret);
		cout << errorStr << endl;
		exit(ret);
	}
}

void RtpMediaServ::RunServ()
{
	is_running = true;
	serv_thr = thread(&RtpMediaServ::ServFunc, this);
}

inline void RtpMediaServ::RecvAudioPacket(shared_ptr<SharedAVPacket> input)
{
	input.get()->getPointer()->stream_index = audio_stream->index;
	input.get()->getPointer()->time_base = audio_stream->time_base;
	audio_buf.push(input);
}

inline void RtpMediaServ::RecvVideoPacket(shared_ptr<SharedAVPacket> input)
{
	input.get()->getPointer()->stream_index = video_stream->index;
	input.get()->getPointer()->time_base = video_stream->time_base;
	video_buf.push(input);
}

AVFormatContext* RtpMediaServ::getFmtContext()
{
	return formatContext;
}

inline void RtpMediaServ::EndServ()
{
	is_running = false;
	serv_thr.join();
	av_write_trailer(formatContext);
	cout << "end rtp server" << endl;
}

inline RtpMediaServ::~RtpMediaServ()
{
	avformat_free_context(formatContext);
}

inline void RtpMediaServ::ConsumePacket()
{
	bool ret;
	shared_ptr<SharedAVPacket> pkt;
	if (video_buf.size() != 0 &&
		(audio_buf.size() == 0 ||
			av_compare_ts(video_buf.front()->getPointer()->pts, video_buf.front()->getPointer()->time_base,
				audio_buf.front()->getPointer()->pts, audio_buf.front()->getPointer()->time_base) <= 0))
		ret = video_buf.pop(pkt);
	else
		ret = audio_buf.pop(pkt);

	if(ret)
	{
		int ret = av_interleaved_write_frame(formatContext, pkt->getPointer());
		if (ret < 0) 
		{
			char errorStr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
			av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, ret);
			cout << "av_interleaved_write_frame fail: " << errorStr << endl;
			exit(ret);
		}
	}
}

void RtpMediaServ::ServFunc()
{
	while (is_running)
	{
		ConsumePacket();
	}
}
