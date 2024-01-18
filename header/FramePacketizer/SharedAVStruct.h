extern "C" {
#include <libavcodec/packet.h>
#include <libavutil/frame.h>
}

template <typename U>
using RequireSpecialType = typename std::enable_if<std::is_same<U, AVPacket*>::value ||
	std::is_same<U, AVFrame*>::value>::type;

template <typename T,RequireSpecialType<T>* = nullptr>
class SharedAVStruct
{
	
public:
	template <>
	SharedAVStruct()
	{
		data = av_packet_alloc();
	}

	template<>
	SharedAVStruct()
	{
		data = av_frame_alloc();
	}

	template<>
	~SharedAVStruct()
	{
		av_packet_free(&data);
	}

	template<>
	~SharedAVStruct()
	{
		av_frame_free(&data);
	}
private:
	T data;
};
