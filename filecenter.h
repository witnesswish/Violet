#ifndef FILECENTER_H
#define FILECENTER_H

#include <stdint.h>
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <future>
#include <filesystem>

#include "protocol.h"
#include "threadpoolcenter.h"

class FileCenter
{
public:
    FileCenter();
    int getAvailablePort(int &port, int rangeStart=46386, int rangEnd=56386, int chunkSize=512000);
    ssize_t vuploadFile(int fd, std::string fileName, uint32_t fileSize, uint32_t chunk, uint32_t chunkSize=512000);
    void vdownloadFile(int fd, std::string fileName, uint32_t fileSize, uint32_t chunk, uint32_t chunkSize=512000);
private:
    VioletProtHeader header;
};

class PortPoolCenter
{
public:
    PortPoolCenter(int minPort = 46837, int maxPort = 56836);
    ~PortPoolCenter();
    void init(int portSize);
    int getPort(int timeout=5000);
    int availablePortCount();
    void startProducerWorker();
private:
    std::queue<int> m_availablePorts;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_stopSignal{false};
    const int m_minPort;
    const int m_maxPort;
    std::atomic<int> m_nextPortToScan;
    ThreadPoolCenter threadpool;
    bool isPortAvailable(int port);
    void producerWorker(int batchSize=10);
};

#endif // FILECENTER_H
