#pragma once
#include "MutexQueue/MutexQueue.h"
#include "ResourceMonitor/CountableResource.h"
#include "ScreenCapture/FrameData.h"
#include "FramePacketizer/AVStructPool.h"

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
            std::cout << CountableResource<FrameData>::PrintRemainResource()<< std::endl;
            std::cout << CountableResource<SharedAVFrame>::PrintRemainResource() << std::endl;
            std::cout << CountableResource<SharedAVPacket>::PrintRemainResource() << std::endl;
            std::this_thread::sleep_for(interval);  // �ֱ⸸ŭ ���
        }
    }
};