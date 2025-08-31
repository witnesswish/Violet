cmake_minimum_required(VERSION 3.16)

project(Violet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Violet main.cpp
    mariadbhelper.h mariadbhelper.cpp
    server.h server.cpp
    unlogincenter.h unlogincenter.cpp
    common.h
    protocol.h
    protocol.cpp
    logincenter.h logincenter.cpp
    redishelper.h redishelper.cpp
    filecenter.h filecenter.cpp
    threadpoolcenter.h threadpoolcenter.cpp
    readme.me.md
    common.cpp
)

include(GNUInstallDirs)

find_library(MARIADBCPP_LIBRARY
    NAMES mariadbcpp
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
)

target_link_libraries(Violet PRIVATE mariadbcpp hiredis ssl crypto)

install(TARGETS Violet
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 条件编译test
option(TEST "Build test executable" OFF)
if(BUILD_TEST)
    message(STATUS "Building test executable")

    # 添加测试可执行文件
    add_executable(test
        mariadbhelper.h mariadbhelper.cpp
        redishelper.h redishelper.cpp
        test.cpp
    )

target_link_libraries(test PRIVATE mariadbcpp hiredis)
endif()

option(PORT_TEST "Build porttest executable" OFF)
if(PORT_TEST)
    message(STATUS "Building generate executable")

    # 添加测试可执行文件
    add_executable(porttest
        filecenter.h filecenter.cpp
        portTest.cpp
    )

target_link_libraries(porttest PRIVATE)
endif()
# mkdir build && cd build
# cmake -DBUILD_TEST=ON ..
# make test
