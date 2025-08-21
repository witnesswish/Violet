# <div align="center"><b><a href="README.md">简体中文</a> | <a href="README_EN.md">English</a></b></div>

# Violet 🚀

[![C++11](https://img.shields.io/badge/C++-11-blue.svg)](https://en.cppreference.com/)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/)

## introduction
        High Concurrency：Epoll ET + Thread Pool

        share_memory: shared_ptr + atomic

        MariaDB Connection Pool 

        Redis 

## compile
```bash
git clone https://github.com/witnesswish/Violet.git
cd Violet && mkdir build && cd build
cmake .. && make -j4
./Violet
```
I am using hiredis and mariadb/conncpp, you need to install them before compiling
If you need to test functions, write your code on test.cpp and use the following command， all command below are based on build directory
```
cmake -DBUILD_TEST=ON .. && make test
```


# 架构
```mermaid
graph TD
    client-->|TCP Socket|Epoll
    Epoll-->|dispatch|ProtocolAnalysis
    ProtocolAnalysis-->unloginRouter[unloginResolve]
    ProtocolAnalysis-->loginRouter[loginResolve]
    loginRouter-->|database|MariaDB
    loginRouter-->|cache|Redis
    unloginRouter-->|message|transfer
```


✉️ 联系：violet@elveso.asia | [博客文章](https://elveso.asia/blog/)

