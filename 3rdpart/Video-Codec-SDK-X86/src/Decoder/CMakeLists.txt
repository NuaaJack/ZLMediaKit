# Copyright 2020-2021 NVIDIA Corporation.  All rights reserved.
#
# Please refer to the NVIDIA end user license agreement (EULA) associated
# with this source code for terms and conditions that govern your use of
# this software. Any use, reproduction, disclosure, or distribution of
# this software and related documentation outside the terms of the EULA
# is strictly prohibited.

# 3.7 is required for FindVulkan module support in CMake.
cmake_minimum_required(VERSION 3.7)

project(DecoderHW)


set(JA_DEC_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/DecoderHW.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/DecoderBase.cpp
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


source_group( "headers" FILES ${NV_DEC_HDRS})
source_group( "sources" FILES ${APP_SOURCES} ${NV_DEC_SOURCES} )

find_package(CUDA)

set(CUDA_HOST_COMPILER ${CMAKE_CXX_COMPILER})

set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-gencode arch=compute_50,code=\"sm_50,compute_50\")
if ( CMAKE_COMPILER_IS_GNUCC )
    if(NOT "${CUDA_NVCC_FLAGS}" MATCHES "-std=c\\+\\+11" )
        list(APPEND CUDA_NVCC_FLAGS -std=c++11)
    endif()
endif()

cuda_add_library(${PROJECT_NAME} SHARED ${NV_DEC_SOURCES} ${NV_DEC_HDRS} ${JA_DEC_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES CUDA_SEPARABLE_COMPILATION ON)


target_include_directories(${PROJECT_NAME} PUBLIC
        ${CUDA_INCLUDE_DIRS}
        ${NVCODEC_PUBLIC_INTERFACE_DIR}
        ${NVCODEC_UTILS_DIR}
        ${NV_DEC_DIR}
        ${NV_APPDEC_COMMON_DIR}
        ${NV_CODEC_DIR}
        )

target_link_libraries(${PROJECT_NAME} ${CUDA_CUDA_LIBRARY} ${CMAKE_DL_LIBS} ${NVENCODEAPI_LIB} ${CUVID_LIB})

#if(ENABLE_CODEC_SERVER_TEST)
#    set(APP_SOURCES
#            ${CMAKE_CURRENT_SOURCE_DIR}/DecoderTest.cpp
#            )
#
#    cuda_add_executable(deocdertest  ${APP_SOURCES} ${NV_DEC_SOURCES} ${NV_DEC_HDRS} ${JA_DEC_SOURCES})
#    set_target_properties(deocdertest PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
#    target_include_directories(deocdertest PUBLIC ${CUDA_INCLUDE_DIRS}
#            ${NVCODEC_PUBLIC_INTERFACE_DIR}
#            ${NVCODEC_UTILS_DIR}
#            ${NV_DEC_DIR}
#            ${NV_APPDEC_COMMON_DIR}
#            ${NV_FFMPEG_HDRS}
#            ${NV_CODEC_DIR}
#            )
#    target_link_libraries(deocdertest ${PROJECT_NAME} ${AVCODEC_LIB}
#            ${AVFORMAT_LIB} ${AVUTIL_LIB} ${SWRESAMPLE_LIB})
#
#endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${NVCODEC_SAMPLES_INSTALL_DIR})
