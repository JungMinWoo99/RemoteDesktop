#include "ScreenCapture/PixFmtConverter.h"
#include <iostream>

using namespace std;

PixFmtConverter::PixFmtConverter(int width, int height)
	:frame_width(width), frame_height(height)
{
	AVPixelFormat bgr_pix_fmt = AV_PIX_FMT_RGB32;

	AVPixelFormat yuv_pix_fmt = AV_PIX_FMT_YUV420P;

	bgr_stride[0] = frame_width * BYTE_PER_PIXEL;

	yuv_stride[0] = frame_width;
	yuv_stride[1] = frame_width / 2;
	yuv_stride[2] = frame_width / 2;

	rgb_to_yuv_ctx = sws_getContext(frame_width, frame_height, bgr_pix_fmt,
		frame_width, frame_height, yuv_pix_fmt,
		SWS_BICUBIC, nullptr, nullptr, nullptr);

	yuv_to_bgr_ctx = sws_getContext(frame_width, frame_height, yuv_pix_fmt,
		frame_width, frame_height, bgr_pix_fmt,
		SWS_BICUBIC, nullptr, nullptr, nullptr);

	if (rgb_to_yuv_ctx == NULL || yuv_to_bgr_ctx == NULL) {
		cout << "sws_getContext fail" << endl;
		exit(-1);
	}
}

shared_ptr<FrameData> PixFmtConverter::ConvertBGRToYUV(shared_ptr<FrameData> bgr_data)
{
	shared_ptr<FrameData> yuv_data = make_shared<FrameData>(bgr_data.get()->getMemSize() / 2 * 3 / BYTE_PER_PIXEL);
	yuv_data.get()->setCaptureTime(bgr_data.get()->getCaptureTime());

	const uint8_t* bgr_data_ptr[1] = { bgr_data.get()->getMemPointer() };
	uint8_t* yuv_data_ptr[3] = {
							yuv_data.get()->getMemPointer(),
							yuv_data.get()->getMemPointer() + yuv_data.get()->getMemSize() * 2 / 3,
							yuv_data.get()->getMemPointer() + yuv_data.get()->getMemSize() * 5 / 6
	};

	sws_scale(rgb_to_yuv_ctx, bgr_data_ptr, bgr_stride, 0, frame_height, yuv_data_ptr, yuv_stride);

	return yuv_data;
}

shared_ptr<FrameData> PixFmtConverter::ConvertYUVToBGR(shared_ptr<FrameData> yuv_data)
{

	shared_ptr<FrameData> bgr_data = make_shared<FrameData>(yuv_data.get()->getMemSize() / 3 * 2 * BYTE_PER_PIXEL);
	bgr_data.get()->setCaptureTime(yuv_data.get()->getCaptureTime());

	const uint8_t* yuv_data_ptr[3] = {
										yuv_data.get()->getMemPointer(),
										yuv_data.get()->getMemPointer() + yuv_data.get()->getMemSize() * 2 / 3,
										yuv_data.get()->getMemPointer() + yuv_data.get()->getMemSize() * 5 / 6
	};
	uint8_t* bgr_data_ptr[1] = { bgr_data.get()->getMemPointer() };

	sws_scale(yuv_to_bgr_ctx, yuv_data_ptr, yuv_stride, 0, frame_height, bgr_data_ptr, bgr_stride);

	return bgr_data;
}

PixFmtConverter::~PixFmtConverter()
{
	sws_freeContext(rgb_to_yuv_ctx);
}

void PixFmtConverter::FlipData(shared_ptr<FrameData> yuv_frame_data)
{
	uint8_t* pixel_data_buf = yuv_frame_data.get()->getMemPointer();
	int buf_row_size = frame_width * BYTE_PER_PIXEL;
	uint8_t* tem_row = new uint8_t[buf_row_size];

	for (int i = 0; i < frame_height / 2; i++) {
		int rowIndex1 = i * buf_row_size;
		int rowIndex2 = (frame_height - i - 1) * buf_row_size;

		memcpy(tem_row, pixel_data_buf + rowIndex1, buf_row_size);
		memcpy(pixel_data_buf + rowIndex1, pixel_data_buf + rowIndex2, buf_row_size);
		memcpy(pixel_data_buf + rowIndex2, tem_row, buf_row_size);
	}

	delete[] tem_row;
}