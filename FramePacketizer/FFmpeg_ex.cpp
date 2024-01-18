extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}

int main() {
    // Initialize FFmpeg
    av_register_all();
    avcodec_register_all();

    const char* outputFilename = "output.mp4";
    const int width = 1280;
    const int height = 720;
    const int framerate = 30;

    // Initialize output format context
    AVFormatContext* formatCtx = nullptr;
    int ret = avformat_alloc_output_context2(&formatCtx, nullptr, nullptr, outputFilename);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Error creating output context\n");
        return ret;
    }

    // Find video encoder
    AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        av_log(nullptr, AV_LOG_ERROR, "Codec not found\n");
        return AVERROR(EINVAL);
    }

    AVStream* stream = avformat_new_stream(formatCtx, codec);
    if (!stream) {
        av_log(nullptr, AV_LOG_ERROR, "Failed to create stream\n");
        return AVERROR(ENOMEM);
    }

    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        av_log(nullptr, AV_LOG_ERROR, "Failed to allocate codec context\n");
        return AVERROR(ENOMEM);
    }

    // Set codec parameters
    codecCtx->width = width;
    codecCtx->height = height;
    codecCtx->time_base = { 1, framerate };
    codecCtx->framerate = { framerate, 1 };
    codecCtx->gop_size = 10; // Keyframe interval
    codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

    if (formatCtx->oformat->flags & AVFMT_GLOBALHEADER)
        codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // Open codec
    ret = avcodec_open2(codecCtx, codec, nullptr);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Cannot open video codec\n");
        return ret;
    }

    // Initialize output stream
    ret = avio_open(&formatCtx->pb, outputFilename, AVIO_FLAG_WRITE);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Cannot open output file\n");
        return ret;
    }

    // Write format header
    ret = avformat_write_header(formatCtx, nullptr);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Error writing header\n");
        return ret;
    }

    // Create and fill AVFrame
    AVFrame* frame = av_frame_alloc();
    frame->format = codecCtx->pix_fmt;
    frame->width = codecCtx->width;
    frame->height = codecCtx->height;
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Failed to allocate frame data\n");
        return ret;
    }

    // Encode frames
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = nullptr;
    pkt.size = 0;

    for (int frameIndex = 0; frameIndex < 300; frameIndex++) {
        // Fill frame data with content (for example, load an image)
        // av_image_fill_arrays(...);

        frame->pts = frameIndex * codecCtx->time_base.num;

        // Encode the frame
        ret = avcodec_send_frame(codecCtx, frame);
        if (ret < 0) {
            av_log(nullptr, AV_LOG_ERROR, "Error sending frame for encoding\n");
            return ret;
        }

        while (ret >= 0) {
            ret = avcodec_receive_packet(codecCtx, &pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;
            else if (ret < 0) {
                av_log(nullptr, AV_LOG_ERROR, "Error receiving packet from encoder\n");
                return ret;
            }

            // Write packet to the output stream
            av_interleaved_write_frame(formatCtx, &pkt);
            av_packet_unref(&pkt);
        }
    }

    // Flush the encoder
    ret = avcodec_send_frame(codecCtx, nullptr);
    if (ret < 0) {
        av_log(nullptr, AV_LOG_ERROR, "Error flushing encoder\n");
        return ret;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(codecCtx, &pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            av_log(nullptr, AV_LOG_ERROR, "Error receiving packet from encoder\n");
            return ret;
        }

        // Write packet to the output stream
        av_interleaved_write_frame(formatCtx, &pkt);
        av_packet_unref(&pkt);
    }

    // Write trailer
    av_write_trailer(formatCtx);

    // Clean up
    av_frame_free(&frame);
    avcodec_free_context(&codecCtx);
    avio_close(formatCtx->pb);
    avformat_free_context(formatCtx);

    return 0;
}
