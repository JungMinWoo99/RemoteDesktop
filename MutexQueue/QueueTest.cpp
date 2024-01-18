#include "MutexQueue/MutexQueue.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

int main() {
    MutexQueue<int> myQueue;

    // Non-thread-safe operations
    std::cout << "Queue Empty: " << (myQueue.empty() ? "true" : "false") << std::endl;
    std::cout << "Queue Size: " << myQueue.size() << std::endl;

    myQueue.push(42);

    std::cout << "Front: " << myQueue.front() << std::endl;
    std::cout << "Back: " << myQueue.back() << std::endl;

    // Thread-safe operations
    std::vector<std::thread> threads;

    // Push threads
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([&myQueue, i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100 * i));
            myQueue.push(i);
            });
    }

    // Pop threads
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([&myQueue, i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100 * i));
            while (myQueue.empty());
                myQueue.pop();
            });
    }

    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }

    // Display final state
    std::cout << "Queue Empty: " << (myQueue.empty() ? "true" : "false") << std::endl;
    std::cout << "Queue Size: " << myQueue.size() << std::endl;

    return 0;
}
