cmake_minimum_required(VERSION 3.16)
project(ArcticOwl)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译器选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2")

# 查找必要的库
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui imgcodecs videoio video features2d)
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

# 启用Qt的MOC、UIC和RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 头文件
set(HEADERS
    src/core/video_capture.h
    src/core/video_processor.h
    src/modules/network/network_server.h
    src/modules/ui/main_window.h
)

# 源文件
set(SOURCES
    src/main.cpp
    src/core/video_capture.cpp
    src/core/video_processor.cpp
    src/modules/network/network_server.cpp
    src/modules/ui/main_window.cpp
)

# 创建可执行文件
add_executable(ArcticOwl ${SOURCES} ${HEADERS})

# 包含目录
target_include_directories(ArcticOwl PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 链接库
target_link_libraries(ArcticOwl
        ${OpenCV_LIBS}
        ${Boost_LIBRARIES}
        Qt6::Core Qt6::Widgets Qt6::Network
        pthread
)

# 安装规则
install(TARGETS ArcticOwl DESTINATION bin)