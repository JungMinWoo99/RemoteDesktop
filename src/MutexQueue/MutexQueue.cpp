#include "MutexQueue/MutexQueue.h"

std::unique_ptr<MutexQueueMonitor> MutexQueueMonitor::instance = nullptr;
std::once_flag MutexQueueMonitor::initFlag;

std::ofstream MutexQueueMonitor::log_stream("mutex_queue_monitor_log.txt",std::ios::out | std::ios::trunc);