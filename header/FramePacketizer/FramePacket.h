#include <stdint.h>

struct FramePacket
{
	int64_t pts;
	int64_t dts;

	int size;
	uint8_t* data;

	int stream_index;
	int flags;
};