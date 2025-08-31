#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <mutex>

#define SERVER_WELCOME "Welcome you join to the chat room! Your chat ID is: Client #%d"
#define BUFFER_SIZE 0xFFFF
#define EPOLL_SIZE 5000

/**
 * @brief The ConnectionInfo class
 * 这个结构体是一个fd和ssl的映射
 * 主要作用是减少之前代码的更改，能快速查询fd对应的ssl，每个用户登录的时候，都应该注册这个结构体
 */
struct ConnectionInfo
{
    int fd;
    SSL *ssl;
};

extern std::unordered_map<int, SSL*> fdSslMap;
extern std::mutex fdSslMapMutex;

inline void addfd(int fd, int epfd, ConnectionInfo *ptr = nullptr, bool enable_et = true)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if(ptr != nullptr)
    {
        ev.data.ptr = ptr;
        std::lock_guard<std::mutex> lock(fdSslMapMutex);
        fdSslMap[fd] = ptr;
    }
    if (enable_et)
        ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    std::cout << "fd #" << fd << " added to epoll" << std::endl;
}
inline void removefd(int fd, int epfd, SSL *ssl = nullptr)
{
    if(ssl != nullptr)
    {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
}

#endif // COMMON_H

// 网上看来的流程，先记在这里，做一个对照
// 为这个新连接创建一个SSL对象
// SSL *ssl = SSL_new(ctx);
// if (!ssl) {
//     ERR_print_errors_fp(stderr);
//     close(client_fd); // 关闭TCP连接
//     continue;         // 继续处理下一个连接
// }

// // 将SSL对象与TCP socket关联起来
// if (SSL_set_fd(ssl, client_fd) != 1) {
//     ERR_print_errors_fp(stderr);
//     SSL_free(ssl);
//     close(client_fd);
//     continue;
// }

// // 在TCP连接之上进行SSL握手
// int accept_ret = SSL_accept(ssl);
// if (accept_ret <= 0) {
//     // 握手失败
//     int err = SSL_get_error(ssl, accept_ret);
//     fprintf(stderr, "SSL_accept failed with error %d\n", err);
//     ERR_print_errors_fp(stderr);

//     SSL_shutdown(ssl); // 尝试优雅关闭SSL
//     SSL_free(ssl);     // 释放SSL对象
//     close(client_fd);  // 关闭TCP连接
//     continue;
// }

// // 恭喜！到这里，SSL/TLS连接已经成功建立。
// // 现在所有通信都是加密的。
// printf("SSL connection established with client. Using cipher: %s\n", SSL_get_cipher(ssl));

// char buffer[1024];
// // 读取加密数据 (SSL_read会自动解密)
// int bytes_received = SSL_read(ssl, buffer, sizeof(buffer) - 1);
// if (bytes_received > 0) {
//     buffer[bytes_received] = '\0';
//     printf("Received encrypted message, decrypted as: %s\n", buffer);

//     // 发送加密响应 (SSL_write会自动加密)
//     const char *response = "Hello from SSL Server!";
//     int bytes_sent = SSL_write(ssl, response, strlen(response));
//     if (bytes_sent <= 0) {
//         ERR_print_errors_fp(stderr);
//     }
// } else {
// int err = SSL_get_error(ssl, bytes_received);
// // 处理错误或连接关闭
// }

// // 1. 通知对端SSL连接要关闭了 (双向关闭)
// SSL_shutdown(ssl);
// // 2. 释放SSL对象
// SSL_free(ssl);
// // 3. 关闭底层的TCP socket
// close(client_fd);

// // 释放SSL上下文
// SSL_CTX_free(ctx);
// 清理OpenSSL全局状态
// EVP_cleanup();
// ERR_free_strings();
