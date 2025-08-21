# <div align="center"><b><a href="README.md">简体中文</a> | <a href="README_EN.md">English</a></b></div>

# Violet 🚀


[![C++11](https://img.shields.io/badge/C++-11-blue.svg)](https://en.cppreference.com/)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/)

**工业级C++后端项目** | Reactor模型 · 双存储引擎 · mariadb连接池

## 简介
        高性能架构：基于Epoll ET模式+线程池，单机支持8000+并发连接

        零依赖部署：纯C++实现，无需容器化，make一键编译运行

        智能指针内存管理

        MariaDB连接池

        Redis 做离线消息缓存

        Reactor网络模型 

## 编译
```bash
git clone https://github.com/witnesswish/Violet.git
cd Violet && mkdir build && cd build
cmake .. && make -j4
./Violet
```
服务端使用了 hiredis 和 mariadb/conncpp, 编译前需要安装这两个库
需要进行模块测试，请修改test.cpp后参考以下命令，下面的所有命令都是基于build目录下运行：
```
cmake -DBUILD_TEST=ON .. && make test
```


# 架构
版本演化：
```mermaid
graph LR
    v1.0[基础版本] --> v1.1[+Mariadb做用户信息管理]
    v1.1 --> v1.2[+Redis做离线消息缓存]
    v1.2 --> v1.3[改进mariadb连接池]
    v1.3 --> v1.4[+文件上传+端口池]
    
```
```mermaid
graph TD
    客户端-->|TCP Socket|Epoll
    Epoll-->Protocol[协议解析]
    Protocol-->unloginRouter[匿名路由]
    Protocol-->loginRouter[有名路由]
    loginRouter-->|存储验证|MariaDB
    loginRouter-->|缓存|Redis
    unloginRouter-->|消息|转发
    登录用户的消息-->|ProtocolBuffer|协议解析
    协议解析-->|msg|普通聊天消息
    普通聊天消息-->|转发|根据请求好友转发
    协议解析-->|file|请求文件传输
    请求文件传输-->|查找端口|端口管理
    端口管理-->|成功|返回端口
    端口管理-->|失败|请求失败
    返回端口-->|成功|监听端口等待上传
```


✉️ 联系：violet@elveso.asia | [博客文章](https://elveso.asia/blog/)

