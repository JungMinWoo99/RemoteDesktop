#include "ScreenCapture/ScreenCapture.h"
#include <iostream>
#include <chrono>

using namespace std;

ScreenCapture::ScreenCapture(int color_bits, int pixel_width, int pixel_height)
	:pixel_width(pixel_width), pixel_height(pixel_height), color_bits(color_bits)
{
	static int obj_id_count = 0;

	obj_id = obj_id_count++;

	buf_byte_size = pixel_width * pixel_height * color_bits / 8;
}

int ScreenCapture::getWidth() const
{
	return pixel_width;
}

int ScreenCapture::getHeight() const
{
	return pixel_height;
}

int ScreenCapture::getFrameDataSize() const
{
	return buf_byte_size;
}

ScreenCapture::~ScreenCapture() {}