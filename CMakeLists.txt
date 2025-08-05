cmake_minimum_required(VERSION 3.16)

project(Violet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Violet main.cpp
    mariadbhelper.h mariadbhelper.cpp
    server.h server.cpp
    unlogincenter.h unlogincenter.cpp
    common.h)

include(GNUInstallDirs)
install(TARGETS Violet
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
