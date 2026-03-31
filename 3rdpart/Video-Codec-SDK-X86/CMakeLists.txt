cmake_minimum_required(VERSION 3.21)
project(Video_Codec_SDK_X86)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(NVCODEC_PUBLIC_INTERFACE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/Interface)
set(NVCODEC_UTILS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/Utils)
set(NV_CODEC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/NvCodec)
set(NV_ENC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/NvCodec/NvEncoder)
set(NV_DEC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/NvCodec/NvDecoder)
set(NV_APPENC_COMMON_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/src/Common)
set(NV_APPDEC_COMMON_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/src/Common)
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")  # Debug模式下的编译指令
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")         # Release模式下的编译指令

set(CMAKE_INSTALL_PREFIX .)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(NVCODEC_SAMPLES_INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build)

find_library(CUVID_LIB NAMES libnvcuvid.so.1 PATHS /usr/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu)
find_library(NVENCODEAPI_LIB NAMES libnvidia-encode.so.1 PATHS /usr/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu)

option(ENABLE_CODEC_SERVER_TEST "enable server codec test." OFF)


#add_subdirectory(src/Decoder)
#add_subdirectory(src/Encoder)

find_package(CUDA)
message(STATUS "CUDA_CUDA_LIBRARY: ${CUDA_CUDA_LIBRARY}")
message(STATUS "CUDA_TOOLKIT_ROOT_DIR: ${CUDA_TOOLKIT_ROOT_DIR}")

set(CUDA_HOST_COMPILER ${CMAKE_CXX_COMPILER})

set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-gencode arch=compute_50,code=\"sm_50,compute_50\")
if ( CMAKE_COMPILER_IS_GNUCC )
    if(NOT "${CUDA_NVCC_FLAGS}" MATCHES "-std=c\\+\\+11" )
        list(APPEND CUDA_NVCC_FLAGS -std=c++11)
    endif()
endif()


set(JA_ENC_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Encoder/EncoderHW.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Encoder/EncoderBase.cpp
        )

set(NV_ENC_SOURCES
        ${NV_ENC_DIR}/NvEncoder.cpp
        ${NV_ENC_DIR}/NvEncoderCuda.cpp
        ${NV_ENC_DIR}/NvEncoderOutputInVidMemCuda.cpp
        )

set(NV_ENC_HDRS
        ${NV_ENC_DIR}/NvEncoder.h
        ${NV_ENC_DIR}/NvEncoderCuda.h
        ${NV_ENC_DIR}/NvEncoderOutputInVidMemCuda.h
        ${NVCODEC_PUBLIC_INTERFACE_DIR}/nvEncodeAPI.h
        ${NVCODEC_UTILS_DIR}/NvCodecUtils.h
        ${NVCODEC_UTILS_DIR}/NvEncoderCLIOptions.h
        ${NV_APPENC_COMMON_DIR}/codec_enum.h
        )

set(NV_ENC_CUDA_UTILS
        ${NVCODEC_UTILS_DIR}/crc.cu
        )


set(JA_DEC_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Decoder/DecoderHW.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Decoder/DecoderBase.cpp
        )

set(NV_DEC_SOURCES
        ${NV_DEC_DIR}/NvDecoder.cpp
        )

set(NV_DEC_HDRS
        ${NV_DEC_DIR}/NvDecoder.h
        ${NVCODEC_PUBLIC_INTERFACE_DIR}/cuviddec.h
        ${NVCODEC_PUBLIC_INTERFACE_DIR}/nvcuvid.h
        ${NVCODEC_UTILS_DIR}/NvCodecUtils.h
        ${NVCODEC_UTILS_DIR}/FFmpegDemuxer.h
        ${NV_APPDEC_COMMON_DIR}/AppDecUtils.h
        ${CMAKE_CURRENT_SOURCE_DIR}/DecoderHW.h
        ${NV_APPDEC_COMMON_DIR}/codec_enum.h
        )

set(LOGGER_SRC
        ${NVCODEC_UTILS_DIR}/SimpleLogger.cpp)

set(CONTEXT_SRC
        ${NVCODEC_UTILS_DIR}/CUContext.cpp)

source_group( "headers" FILES ${NV_ENC_HDRS} ${NV_DEC_HDRS})
source_group( "sources" FILES ${APP_SOURCES} ${NV_ENC_SOURCES} ${NV_ENC_CUDA_UTILS} ${NV_DEC_SOURCES})


cuda_add_library(NvCodecHW SHARED
        ${JA_ENC_SOURCES} ${NV_ENC_SOURCES} ${NV_ENC_CUDA_UTILS} ${NV_ENC_HDRS}
        ${NV_DEC_SOURCES} ${JA_DEC_SOURCES}
        ${LOGGER_SRC}
        ${CONTEXT_SRC})

set_target_properties(NvCodecHW PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

target_include_directories(NvCodecHW PUBLIC
        ${CUDA_INCLUDE_DIRS}
        ${NVCODEC_PUBLIC_INTERFACE_DIR}
        ${NVCODEC_UTILS_DIR}
        ${NV_ENC_DIR}
        ${NV_APPENC_COMMON_DIR}
        ${NV_CODEC_DIR}
        ${NV_DEC_DIR}
        ${NV_APPDEC_COMMON_DIR}
        )

target_link_libraries(NvCodecHW  ${CUDA_CUDA_LIBRARY} ${CMAKE_DL_LIBS} ${NVENCODEAPI_LIB} ${CUVID_LIB} )

if(ENABLE_CODEC_SERVER_TEST)
    set(APP_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/Encoder/EncoderTest.cpp
            )

    cuda_add_executable(encoderTest  ${APP_SOURCES} ${NV_ENC_SOURCES} ${NV_ENC_CUDA_UTILS})

    set_target_properties(encoderTest PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

    target_include_directories(encoderTest PUBLIC ${CUDA_INCLUDE_DIRS}
            ${NVCODEC_PUBLIC_INTERFACE_DIR}
            ${NVCODEC_UTILS_DIR}
            ${NV_ENC_DIR}
            ${NV_CODEC_DIR}
            )

    target_link_libraries(encoderTest  NvCodecHW ${CMAKE_DL_LIBS})

    set(APP_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/Decoder/DecoderTest.cpp
            )
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswresample)
    cuda_add_executable(deocdertest  ${APP_SOURCES} ${NV_DEC_SOURCES} ${JA_DEC_SOURCES})
    set_target_properties(deocdertest PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
    target_include_directories(deocdertest PUBLIC ${CUDA_INCLUDE_DIRS}
            ${NVCODEC_PUBLIC_INTERFACE_DIR}
            ${NVCODEC_UTILS_DIR}
            ${NV_DEC_DIR}
            ${NV_APPDEC_COMMON_DIR}
            ${FFMPEG_INCLUDE_DIRS}
            ${NV_CODEC_DIR}
            )
    link_directories(${FFMPEG_LIBRARY_DIRS})
    target_link_libraries(deocdertest NvCodecHW ${CMAKE_DL_LIBS} ${FFMPEG_LIBRARIES})

endif()
install(TARGETS NvCodecHW  RUNTIME DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR})

