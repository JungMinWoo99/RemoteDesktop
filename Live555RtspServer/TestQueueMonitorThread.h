#pragma once
#include "MutexQueue/MutexQueue.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <functional>

class TestQueueMonitorThread {
public:
    // 생성자에서 주기적으로 호출할 함수와 주기를 설정합니다.
    TestQueueMonitorThread(std::chrono::seconds interval)
        :interval(interval), running(false) {}

    // 소멸자에서 쓰레드를 정리합니다.
    ~TestQueueMonitorThread() {
        stop();
    }

    // 쓰레드 시작
    void start() {
        if (!running) {
            running = true;
            thread = std::thread(&TestQueueMonitorThread::threadFunction, this);
        }
    }

    // 쓰레드 정지
    void stop() {
        if (running) {
            running = false;
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

private:
    std::chrono::seconds interval;
    std::thread thread;
    bool running;

    // 주기적으로 함수를 호출하는 쓰레드 함수
    void threadFunction() {
        while (running) {
            std::cout << std::endl;
            MutexQueueMonitor::getMonitor().PrintQueueList();  // 주기적으로 호출할 함수 실행
            std::cout << "remain FrameData: " << FrameData::getRemainFrame() << std::endl;
            std::cout << "remain AVFrame: " << SharedAVFrame::getRemainAVStruct() << std::endl;
            std::cout << "remain AVPacket: " << SharedAVPacket::getRemainAVStruct() << std::endl;
            std::this_thread::sleep_for(interval);  // 주기만큼 대기
        }
    }
};