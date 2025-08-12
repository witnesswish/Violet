cmake_minimum_required(VERSION 3.16)

project(Violet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(MariaDB REQUIRED)

add_executable(Violet main.cpp
    mariadbhelper.h mariadbhelper.cpp
    server.h server.cpp
    unlogincenter.h unlogincenter.cpp
    common.h
    protocol.h
    protocol.cpp)

target_link_libraries(Violet
    PRIVATE MariaDB::MariaDB
)

include(GNUInstallDirs)
install(TARGETS Violet
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 添加条件编译测试模块的选项
option(BUILD_TEST "Build test executable" OFF)
if(BUILD_TEST)
    message(STATUS "Building test executable")

    # 添加测试可执行文件
    add_executable(test
        mariadbhelper.h mariadbhelper.cpp
        test.cpp
    )

    # 链接 MariaDB 库到测试程序
    target_link_libraries(test
        PRIVATE MariaDB::MariaDB
    )
endif()
# mkdir build && cd build
# cmake -DBUILD_TEST=ON ..
# make
