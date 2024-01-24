#include "MutexQueue/MutexQueue.h"

std::unique_ptr<MutexQueueMonitor> MutexQueueMonitor::instance = nullptr;
std::once_flag MutexQueueMonitor::initFlag;