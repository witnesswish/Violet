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
    readme.md
    redishelper.h redishelper.cpp
)

include(GNUInstallDirs)

find_library(MARIADBCPP_LIBRARY
    NAMES mariadbcpp
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu
)

target_link_libraries(Violet PRIVATE mariadbcpp hiredis)

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

option(GENERATE_DATA "Build generate executable" OFF)
if(BUILD_TEST)
    message(STATUS "Building generate executable")

    # 添加测试可执行文件
    add_executable(generate
        mariadbhelper.h mariadbhelper.cpp
        redishelper.h redishelper.cpp
        generatedata.cpp
    )

target_link_libraries(generate PRIVATE mariadbcpp hiredis)
endif()
# mkdir build && cd build
# cmake -DBUILD_TEST=ON ..
# make test
