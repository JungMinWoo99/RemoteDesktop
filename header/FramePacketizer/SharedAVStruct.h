extern "C" {
#include <libavcodec/packet.h>
#include <libavutil/frame.h>
}

template <typename U>
using RequireSpecialType = typename std::enable_if<std::is_same<U, AVPacket*>::value ||
	std::is_same<U, AVFrame*>::value>::type;

template <typename T>
class SharedAVStruct
{
	
public:
	template <typename U = T, RequireSpecialType<U>* = nullptr>
	SharedAVStruct() {}
};

template <>
class SharedAVStruct<AVFrame*>
{

public:
	SharedAVStruct()
	{
		data = av_frame_alloc();
	}

	~SharedAVStruct()
	{
		av_frame_free(&data);
	}

	AVFrame* getPointer()
	{
		return data;
	}
private:
	AVFrame* data;
};

template <>
class SharedAVStruct<AVPacket*>
{

public:
	SharedAVStruct()
	{
		data = av_packet_alloc();
	}

	~SharedAVStruct()
	{
		av_packet_free(&data);
	}

private:
	AVPacket* data;
};
