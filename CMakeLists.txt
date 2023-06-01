cmake_minimum_required(VERSION 3.16)
project(diskio)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -no-pie -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(Qt5_DIR ./vendor/usr/lib/x86_64-linux-gnu/cmake/Qt5)
find_package(Qt5 REQUIRED Core Widgets Gui Charts)

# the -I flag in gcc
include_directories(${PROJECT_SOURCE_DIR}/include/, ${PROJECT_SOURCE_DIR}/vendor/include/) 

set(CMAKE_BUILD_WITH_INSTALL_RPATH on)
set(CMAKE_INSTALL_RPATH "./vendor/lib" "./lib" "./vendor/usr/lib/x86_64-linux-gnu")
link_directories(./vendor/lib)
link_directories(./vendor/usr/lib/x86_64-linux-gnu)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.h" "${PROJECT_SOURCE_DIR}/src/*.hpp" "${PROJECT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE VENDOR_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/vendor/src/*.h" "${PROJECT_SOURCE_DIR}/vendor/src/*.hpp" "${PROJECT_SOURCE_DIR}/vendor/src/*.cpp")

add_executable(${PROJECT_NAME} ${APP_SOURCES} ${VENDOR_SOURCES})
target_compile_options(${PROJECT_NAME} PRIVATE -fPIC)
target_link_libraries(${PROJECT_NAME} 
    pthread
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Charts
)