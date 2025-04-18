#include "MemoryManage/AVPacketManage.h"

using namespace std;

std::shared_ptr<PacketData> ConvertAVPacketToRawWithoutHeader(AVPacket* src)
{
	auto data_ptr = src->data;
	auto data_size = src->size;
	BYTE* p_data = data_ptr + NAL_HEADER_LEN;
	size_t p_size = data_size - NAL_HEADER_LEN;

	auto ret = make_shared< PacketData>(p_size);
	memcpy(ret.get()->getMemPointer(), p_data, p_size);

	return ret;
}

shared_ptr<PacketData> ConvertAVPacketToRawWithoutHeader(std::shared_ptr <SharedAVPacket> src)
{
	auto data_ptr = src.get()->getPointer()->data;
	auto data_size = src.get()->getPointer()->size;
	BYTE* p_data = data_ptr + NAL_HEADER_LEN;
	size_t p_size = data_size - NAL_HEADER_LEN;

	auto ret = make_shared< PacketData>(p_size);
	memcpy(ret.get()->getMemPointer(), p_data, p_size);

	return ret;
}

std::shared_ptr<SharedAVPacket> ConvertRawToAVPacketWithHeader(std::shared_ptr<PacketData> src)
{
	auto data_ptr = src.get()->getMemPointer();
	auto data_size = src.get()->getMemSize();
	auto ret = AVStructPool<AVPacket*>::getInstance().getEmptyObj();
	
	av_new_packet(ret.get()->getPointer(), data_size);

	memcpy(ret.get()->getPointer()->data , data_ptr, data_size );

	return ret;
}

const BYTE* FindNextNAL(const BYTE* data_start, const BYTE* data_end)
{
	BYTE nal_header[NAL_HEADER_LEN] = { 0x00 ,0x00 ,0x00 ,0x01 };
	const BYTE* p = data_start;

	// Simply lookup "0x000001" pattern
	while (p <= (data_end - NAL_HEADER_LEN + 1))
	{
		if (p[0] == nal_header[0] && p[1] == nal_header[1]&& p[2] == nal_header[2]&& p[3] == nal_header[3])
		{
			return p;
		}
		else
		{
			++p;
		}
	}

	//no more NALU
	return data_end;
}

bool isSPSNALU(const BYTE* packet_data)
{
	auto nalu_type_p = packet_data + NAL_HEADER_LEN;
	if (((*nalu_type_p) & 0x1F) == 0x07)
		return true;
	else
		return false;
}

bool isPPSNALU(const BYTE* packet_data)
{
	auto nalu_type_p = packet_data + NAL_HEADER_LEN;
	if (((*nalu_type_p) & 0x1F) == 0x08)
		return true;
	else
		return false;
}

void CopyNALU(const BYTE* nalu_start, const BYTE* nalu_end, shared_ptr<PacketData>& buf)
{
	size_t nalu_size = nalu_end - nalu_start - NAL_HEADER_LEN;
	shared_ptr<PacketData> ret = make_shared< PacketData>(nalu_size);

	memcpy(ret.get()->getMemPointer(), nalu_start + NAL_HEADER_LEN, ret.get()->getMemSize());
	buf = ret;
}


