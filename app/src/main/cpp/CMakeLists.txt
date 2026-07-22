cmake_minimum_required(VERSION 3.22.1)
project(guitarfx)

find_package(oboe REQUIRED CONFIG)

add_library(guitarfx SHARED
    NativeAudioEngine.cpp
    JniBridge.cpp
)

target_include_directories(guitarfx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(guitarfx oboe::oboe android log)
