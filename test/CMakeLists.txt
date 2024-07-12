cmake_minimum_required(VERSION 3.5)
project(EstAudioTest)

set(CMAKE_CXX_STANDARD 20)

include_directories("../include")

set(SOURCES 
    "EstMain.cpp"
)

add_executable(EstAudioTest ${SOURCES})

target_link_libraries(EstAudioTest EstAudio)

if (WIN32)
add_custom_target(CopyEstAudio ALWAYS
    COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_FILE:EstAudio>
    $<TARGET_FILE_DIR:EstAudioTest>
)
endif()