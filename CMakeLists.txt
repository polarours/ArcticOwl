cmake_minimum_required(VERSION 3.16)

# Define the project
project(ArcticOwl VERSION 0.1.2 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2")

# search for required packages
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui imgcodecs videoio video features2d)
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network LinguistTools)

# include directories for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Generate version header file
configure_file(
    ${CMAKE_SOURCE_DIR}/include/arctic_owl/version.h.in
    ${CMAKE_BINARY_DIR}/generated/arctic_owl/version_config.h
    @ONLY
)

# header files
set(HEADERS
    src/core/video_capture.h
    src/core/video_processor.h
    src/modules/network/network_server.h
    src/modules/ui/main_window.h
)

# source files
set(SOURCES
    src/main.cpp
    src/core/video_capture.cpp
    src/core/video_processor.cpp
    src/modules/network/network_server.cpp
    src/modules/ui/main_window.cpp
)

# Define the executable
add_executable(ArcticOwl ${SOURCES} ${HEADERS})

qt_add_translations(ArcticOwl
    TS_FILES
        resources/translations/arcticowl_zh_CN.ts
    QM_FILES_OUTPUT_VARIABLE ArcticOwl_QM_FILES
)

foreach(qm_file IN LISTS ArcticOwl_QM_FILES)
    get_filename_component(qm_name "${qm_file}" NAME)
    set_source_files_properties("${qm_file}" PROPERTIES QT_RESOURCE_ALIAS "${qm_name}")
endforeach()

qt_add_resources(ArcticOwl arcticowl_translations
    PREFIX "/i18n"
    FILES ${ArcticOwl_QM_FILES}
)

# Include directories
target_include_directories(ArcticOwl
    PRIVATE
        ${CMAKE_BINARY_DIR}/generated
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(ArcticOwl
        ${OpenCV_LIBS}
        ${Boost_LIBRARIES}
        Qt6::Core Qt6::Widgets Qt6::Network
        pthread
)

# Install target
install(TARGETS ArcticOwl DESTINATION bin)