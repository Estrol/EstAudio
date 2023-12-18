cmake_minimum_required(VERSION 3.0)
project(EstAudioTest)

set(CMAKE_CXX_STANDARD 17)

include_directories("../include")

set(SOURCES 
    "EstMain.cpp"
)

add_executable(EstAudioTest ${SOURCES})

target_link_libraries(EstAudioTest EstAudio)

if (WIN32)
    add_custom_command(TARGET EstAudioTest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:EstAudio>
        $<TARGET_FILE_DIR:EstAudioTest>
    )
endif()