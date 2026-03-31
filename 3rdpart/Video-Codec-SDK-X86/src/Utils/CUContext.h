//
// Created by 十七 on 2023/5/8.
//

#ifndef VIDEO_CODEC_SDK_X86_CUCONTEXT_H
#define VIDEO_CODEC_SDK_X86_CUCONTEXT_H

#include <cuda.h>
#include "NvCodecUtils.h"

class ContextManager
{
public:
    ContextManager(const ContextManager&) = delete;
    ContextManager& operator=(const ContextManager&) = delete;
    ~ContextManager();

    static ContextManager* getContext();

private:
    ContextManager();
public:
    CUcontext m_cuContext;
};

#endif //VIDEO_CODEC_SDK_X86_CUCONTEXT_H
