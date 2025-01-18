#include "AudioCapture/WinAudioChunker.h"

inline WinAudioChunker::WinAudioChunker(WinAudioCapture& cap_obj, size_t chunk_size)
	:cap_obj(cap_obj), chunk_size(chunk_size), chunked_data_buf("WinAudioChunkerBuf"), is_proc(false)
{
}

inline void WinAudioChunker::StartProc()
{
	is_proc = true;
	proc_thr = thread(&WinAudioChunker::ProcFunc, this);
}

inline void WinAudioChunker::EndProc()
{
	chunked_data_buf.Close();
	is_proc = false;
	proc_thr.join();
}

inline void WinAudioChunker::MakeChunk(std::shared_ptr<AudioFrameData> input)
{
	static size_t used_buf_size = 0;
	static shared_ptr<AudioFrameData> buf = make_shared<AudioFrameData>(chunk_size);

	/* allocate the data buffers */
	int ret;

	size_t rest_buf_size = buf.get()->getMemSize() - used_buf_size;
	BYTE* packet_blank_start_ptr = buf.get()->getMemPointer() + used_buf_size;

	if (rest_buf_size > input.get()->getMemSize())
	{
		memcpy(packet_blank_start_ptr, input.get()->getMemPointer(), input.get()->getMemSize());
		used_buf_size += input.get()->getMemSize();
	}
	else
	{
		memcpy(packet_blank_start_ptr, input.get()->getMemPointer(), rest_buf_size);
		chunked_data_buf.push(buf);
		buf = make_shared<AudioFrameData>(chunk_size);
		used_buf_size = 0;

		size_t rest_input_data_size = input.get()->getMemSize() - rest_buf_size;
		if (rest_input_data_size > 0)
		{
			std::shared_ptr<AudioFrameData> rest_data = make_shared<AudioFrameData>(rest_input_data_size);
			memcpy(rest_data.get()->getMemPointer(), input.get()->getMemPointer() + rest_buf_size, rest_input_data_size);
			MakeChunk(rest_data);
		}
	}
}

inline void WinAudioChunker::ProcFunc()
{
	while (is_proc)
	{
		shared_ptr<AudioFrameData> pkt;
		if (chunked_data_buf.wait_and_pop_utill_not_empty(pkt))
			MakeChunk(pkt);
	}
}
