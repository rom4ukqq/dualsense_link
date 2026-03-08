cmake_minimum_required(VERSION 3.26)

project(dualsense-link VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DSL_BUILD_DRIVER "Build KMDF + VHF driver skeleton target." ON)
option(DSL_BUILD_UI "Expose WinUI 3 skeleton target." ON)

add_library(dsl_core STATIC
    core/crc32.cpp
    core/dualsense_parser.cpp
    core/packet_translation.cpp
)

target_include_directories(dsl_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(dsl_core PUBLIC cxx_std_20)

add_library(dsl_ipc STATIC
    ipc/pipe_server.cpp
)

target_include_directories(dsl_ipc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dsl_ipc PUBLIC dsl_core)

add_executable(dsl_service
    service/main.cpp
    service/bluetooth_manager.cpp
    service/controller_proxy.cpp
)

target_include_directories(dsl_service PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dsl_service PRIVATE dsl_core dsl_ipc)

if(WIN32)
    target_link_libraries(dsl_service PRIVATE setupapi hid)
else()
    message(FATAL_ERROR "DualSense Link targets Windows 10/11 only.")
endif()

if(DSL_BUILD_DRIVER)
    add_subdirectory(driver)
endif()

if(DSL_BUILD_UI)
    add_subdirectory(ui)
endif()
