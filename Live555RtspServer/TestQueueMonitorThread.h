#pragma once
#include "MutexQueue/MutexQueue.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <functional>

class TestQueueMonitorThread {
public:
    // �����ڿ��� �ֱ������� ȣ���� �Լ��� �ֱ⸦ �����մϴ�.
    TestQueueMonitorThread(std::chrono::seconds interval)
        :interval(interval), running(false) {}

    // �Ҹ��ڿ��� �����带 �����մϴ�.
    ~TestQueueMonitorThread() {
        stop();
    }

    // ������ ����
    void start() {
        if (!running) {
            running = true;
            thread = std::thread(&TestQueueMonitorThread::threadFunction, this);
        }
    }

    // ������ ����
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

    // �ֱ������� �Լ��� ȣ���ϴ� ������ �Լ�
    void threadFunction() {
        while (running) {
            std::cout << std::endl;
            MutexQueueMonitor::getMonitor().PrintQueueList();  // �ֱ������� ȣ���� �Լ� ����
            std::cout << "remain FrameData: " << FrameData::getRemainFrame() << std::endl;
            std::cout << "remain AVFrame: " << SharedAVFrame::getRemainAVStruct() << std::endl;
            std::cout << "remain AVPacket: " << SharedAVPacket::getRemainAVStruct() << std::endl;
            std::this_thread::sleep_for(interval);  // �ֱ⸸ŭ ���
        }
    }
};