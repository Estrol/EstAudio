cmake_minimum_required(VERSION 3.5)
project(EstAudio)

set(CMAKE_CXX_STANDARD 20)

include_directories("./include")

file(GLOB_RECURSE HEADERS "./include/*.h" "./include/*.hpp")

set(SOURCES 
    "src/Audio/Device.cpp"
    "src/Audio/Error.cpp"
    "src/Audio/Sample/SampleAttributes.cpp"
    "src/Audio/Sample/SampleFileIO.cpp"
    "src/Audio/Channel/ChannelControl.cpp"
    "src/Audio/Channel/ChannelConverter.cpp"
    "src/Audio/Channel/ChannelFileIO.cpp"
    "src/Audio/Channel/ChannelInternal.cpp"
    "src/Audio/Channel/ChannelAttributes.cpp"
    "src/Audio/FX/FXAttributes.cpp"
    "src/Audio/FX/FXFileIO.cpp"
    "src/Encoder/EncoderAttributes.cpp"
    "src/Encoder/EncoderProcessor.cpp"
    "src/Encoder/EncoderFileIO.cpp"
    "src/Encoder/EncoderExport.cpp"
    "src/third-party/miniaudio/miniaudio-decoders.cpp"
    "src/third-party-impl/impl.cpp"

    "src/Utils/IO.cpp"
    "src/Utils/MD5.cpp"
    "src/Unknown.cpp"
)

if (WIN32)
    set(SOURCES "${SOURCES}" "src/Win32/resource.rc")
endif() 

add_compile_definitions("EST_EXPORT")
if(MSVC)
    add_compile_options("/WX")
    # replace /W3 to /W4 with regex
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
else()
    add_compile_options("-Wall" "-Wextra" "-Wpedantic" "-Werror")
endif()

add_library(EstAudio SHARED ${HEADERS} ${SOURCES})

set_target_properties(EstAudio PROPERTIES OUTPUT_NAME "EstAudio")

find_package(OpusFile CONFIG REQUIRED)
find_package(Vorbis CONFIG REQUIRED)
find_package(kissfft CONFIG REQUIRED)

target_link_libraries(EstAudio PRIVATE 
    OpusFile::opusfile
    Vorbis::vorbisfile 
    kissfft::kissfft-float
)

add_subdirectory(test)