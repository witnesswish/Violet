#include "filecenter.h"


FileCenter::FileCenter()
{
    header = {};
}

int FileCenter::vuploadFile(int port)
{
    namespace fs = std::filesystem;
    std::fstream file;
    std::string fileName;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        perror("sock error");
        return -1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt error");
        return -1;
    }
    if (bind(sock, (const struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind error");
        return -1;
    }
    if (listen(sock, 1) < 0)
    {
        perror("listen error");
        return -1;
    }
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLength = sizeof(struct sockaddr_in);
    while(1)
    {
        int client = accept(sock, (struct sockaddr *)&clientAddr, &clientAddrLength);
        if(client < 0)
        {
            perror("accept client");
            continue;
        }
        while(1)
        {
            ssize_t len = recv(client, &header, sizeof(header), MSG_PEEK);
            if(len < 0)
            {
                close(client);
                break;
            }
            if(len == 0)
            {
                close(client);
                break;
            }
            // 如果收到的数据长度没有超过协议，那么认为是捣乱的，直接丢掉并关闭，正常传输完会在type字段返回77
            if(len < sizeof(header))
            {
                close(client);
                break;
            }
            // 传输文件的时候，需要magic，length，type，需要每次都带头，每次传输8kb的整体数据，验证数据的合法性，如果不合法，直接丢弃
            // 只有在第一次传输的时候带上neck，neck的pass表示文件名，header的type 1表示带neck不传内容，2表示不带neck传内容，3表示最后一次传输
            uint32_t contentLen = ntohl(header.length);
            if(ntohl(header.magic) != 0x43484154)
            {
                std::cout<< "magic error, continue" <<std::endl;
                continue;
            }
            int totaLen = sizeof(header)+contentLen;
            std::vector<char> recvBuffer(totaLen);
            recv(client, recvBuffer.data(), recvBuffer.size(), 0);
            if(ntohl(header.type) == 1)
            {
                memcpy(&neck, recvBuffer.data()+sizeof(header)+sizeof(neck), sizeof(neck));
                fileName = std::string(neck.pass);
                continue;
            }
            std::vector<char> content(contentLen);
            memcpy(content.data(), recvBuffer.data()+sizeof(header), content.size());


            file.open(fileName.c_str(), std::ios::trunc|std::ios::app);
            file.write(content.data()+sizeof(header), contentLen);
            file.close();
            if(fs::file_size(fileName.c_str()) == header.length)
            {
                close(sock);
                return 0;
            }
            // header.magic = htonl(0x43484154); // "CHAT"
            // header.version = htons(1);
            // header.type = htons(0);
            // header.length = htonl(44);
            // header.timestamp = htonl(static_cast<uint32_t>(time(nullptr)));
        }
    }
    close(sock);
    return -1;
}

PortPoolCenter::PortPoolCenter(int minPort, int maxPort) : m_minPort(minPort), m_maxPort(maxPort), m_nextPortToScan(minPort)
{
    init(10);
    startProducerWorker();
}

PortPoolCenter::~PortPoolCenter()
{
    m_stopSignal = true;
    m_condition.notify_all();
    // 端口生产者永远不会停止，当析构函数调用，线程池自动析构join所有线程
}

void PortPoolCenter::init(int portSize)
{
    std::vector<int> ports;
    int scanned = 0;
    int portToCheck = m_minPort;
    while (ports.size() < portSize && portToCheck <= m_maxPort)
    {
        if(isPortAvailable(portToCheck))
        {
            ports.push_back(portToCheck);
        }
        portToCheck++;
        scanned++;
    }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (int port : ports)
        {
            m_availablePorts.push(port);
        }
    }
    m_nextPortToScan = portToCheck;
}

int PortPoolCenter::getPort(int timeout)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if(m_availablePorts.empty())
    {
        if(m_condition.wait_for(lock, std::chrono::milliseconds(timeout), [this]{return !m_availablePorts.empty()||!m_stopSignal;}))
        {
            if(m_stopSignal)
            {
                return -1;
            }
            else
            {
                return -2;
            }
        }
    }
    int port = m_availablePorts.front();
    m_availablePorts.pop();
    return port;
}

void PortPoolCenter::startProducerWorker()
{
    threadpool.enqueue([this](){return this->producerWorker();});
}

int PortPoolCenter::availablePortCount()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_availablePorts.size();
}

bool PortPoolCenter::isPortAvailable(int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        return false;
    }

    // 设置 SO_REUSEADDR 以便更好地检测端口
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    bool available = (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == 0);
    close(sockfd);
    return available;
}

void PortPoolCenter::producerWorker(int batchSize)
{
    while (!m_stopSignal)
    {
        std::vector<int> newPorts;
        int scanned = 0;
        while (scanned < batchSize && !m_stopSignal)
        {
            int portToCheck = m_nextPortToScan++;

            if (portToCheck > m_maxPort) {
                if (m_nextPortToScan.load() > m_maxPort) {
                    m_nextPortToScan = m_minPort; // 循环扫描
                }
                continue;
            }

            if (isPortAvailable(portToCheck)) {
                newPorts.push_back(portToCheck);
            }
            scanned++;
        }
        if (!newPorts.empty() && !m_stopSignal) {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                for (int port : newPorts) {
                    m_availablePorts.push(port);
                }
            }
            // 通知等待的消费者
            m_condition.notify_all();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
