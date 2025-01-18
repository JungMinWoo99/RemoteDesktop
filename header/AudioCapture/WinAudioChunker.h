#pragma once

#include <thread>

#include "Constant/VideoConstants.h"
#include "AudioCapture/WinAudioCapture.h"
#include "MemoryManage/FrameData.h"

using namespace std;

class WinAudioChunker
{
public:
	WinAudioChunker(WinAudioCapture& cap_obj, size_t chunk_size);

	void StartProc();

	void EndProc();

private:
	WinAudioCapture& cap_obj;
	size_t chunk_size;
	MutexQueue< std::shared_ptr<AudioFrameData>> chunked_data_buf;
	std::thread proc_thr;
	bool is_proc;

	void MakeChunk(std::shared_ptr<AudioFrameData> input);

	void ProcFunc();
};

