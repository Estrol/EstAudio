cmake_minimum_required(VERSION 3.0)
project(EstAudio)

set(CMAKE_CXX_STANDARD 17)

include_directories("./include")

file(GLOB_RECURSE HEADERS "./include/*.h" "./include/*.hpp")

set(SOURCES 
    "src/EstAudio.cpp"
    "src/EstMain.cpp"
    "src/third-party/miniaudio/miniaudio-decoders.cpp"
)

if (WIN32)
    set(SOURCES "${SOURCES}" "src/Win32/resource.rc")
endif() 

add_compile_definitions("EST_EXPORT")

add_library(EstAudio SHARED ${HEADERS} ${SOURCES})

set_target_properties(EstAudio PROPERTIES OUTPUT_NAME "EstAudio")

find_package(OpusFile CONFIG REQUIRED)
find_package(Vorbis CONFIG REQUIRED)

target_link_libraries(EstAudio PRIVATE 
    OpusFile::opusfile
    Vorbis::vorbisfile 
)