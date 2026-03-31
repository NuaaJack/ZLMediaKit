//
// Created by 十七 on 2023/5/8.
//


#include "CUContext.h"
#include <iostream>

/**
*   @brief  Utility function to create CUDA context
*   @param  cuContext - Pointer to CUcontext. Updated by this function.
*   @param  iGpu      - Device number to get handle for
*/
static void createCudaContext(CUcontext* cuContext, int iGpu, unsigned int flags)
{
    CUdevice cuDevice = 0;
    ck(cuDeviceGet(&cuDevice, iGpu));
    char szDeviceName[80];
    ck(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), cuDevice));
    std::cout << "GPU in use: " << szDeviceName << " iGpu " << iGpu << std::endl;
    ck(cuCtxCreate(cuContext, flags, cuDevice));
}


ContextManager::ContextManager() {
    cuInit(0);
    createCudaContext(&m_cuContext, 0, 0);
}

ContextManager::~ContextManager() {
    ck(cuCtxDestroy(m_cuContext));
}

ContextManager* ContextManager::getContext()
{
   static ContextManager* contextManager = new ContextManager();
   return contextManager;
}
