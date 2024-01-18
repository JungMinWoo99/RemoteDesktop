#include "FramePacketizer/AVFrameManage.h"
#include <iostream>

using namespace std;

bool CreateAVFrame(AVFrame*& output_av, const AVCodecContext* c_context)
{
	//AVframe alloc
	if (output_av == NULL)
	{
		output_av = av_frame_alloc();
		if (output_av == NULL)
		{
			cout << "av_frame_alloc fail" << endl;
			exit(-1);
		}
	}

	//settint option
	output_av->width = c_context->width;
	output_av->height = c_context->height;
	output_av->format = c_context->pix_fmt;
	output_av->linesize[0] = output_av->width;
	output_av->linesize[1] = output_av->width / 2;
	output_av->linesize[2] = output_av->width / 2;

	int ret = av_frame_get_buffer(output_av, 0);
	if (ret != 0)
	{
		char errorStr[AV_ERROR_MAX_STRING_SIZE] = { 0 };
		av_make_error_string(errorStr, AV_ERROR_MAX_STRING_SIZE, ret);
		cout << "av_frame_get_buffer fail: " << errorStr << endl;
		exit(ret);
	}

	return true;
}

void CopyAVFrameToRaw(const AVFrame* src, std::shared_ptr<FrameData> dst)
{
	BYTE* frame_ptr = dst.get()->getMemPointer();
	size_t frame_size = dst.get()->getMemSize();
	memcpy(frame_ptr,
		src->data[0],
		frame_size * 2 / 3);
	memcpy(frame_ptr + frame_size * 2 / 3,
		src->data[1],
		frame_size / 6);
	memcpy(frame_ptr + frame_size * 2 / 3 + frame_size / 6,
		src->data[2],
		frame_size / 6);
}

void CopyRawToAVFrame(const std::shared_ptr<FrameData> src, AVFrame* dst)
{
	dst->pts = src.get()->getCaptureTime();

	//copy raw data
	BYTE* frame_ptr = src.get()->getMemPointer();
	size_t frame_size = src.get()->getMemSize();
	memcpy(dst->data[0],
		frame_ptr,
		frame_size * 2 / 3);
	memcpy(dst->data[1],
		frame_ptr + frame_size * 2 / 3,
		frame_size / 6);
	memcpy(dst->data[2],
		frame_ptr + frame_size * 2 / 3 + frame_size / 6,
		frame_size / 6);
}
