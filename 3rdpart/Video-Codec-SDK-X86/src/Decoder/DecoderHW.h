//
// Created by 十七 on 2022/10/15.
//

#ifndef VIDEO_CODEC_SDK_11_1_5_DECODER_HW_H
#define VIDEO_CODEC_SDK_11_1_5_DECODER_HW_H

#include <iostream>
#include <algorithm>
#include <thread>
#include <cuda.h>
#include <memory>
#include <queue>
#include "NvDecoder/NvDecoder.h"
#include "NvCodecUtils.h"
#include "AppDecUtils.h"
#include "codec_enum.h"
#include "DecoderBase.h"

namespace ja {
    namespace server {

        typedef std::unique_ptr<NvDecoder> NvDecoderPtr;

        class DecoderHW : public DecoderBase {

        public:
            DecoderHW();

            ~DecoderHW();

            bool initial();

            bool initial(CODEC_TYPE codecType, int iGpu = 0, bool bOutPlanar = true, bool bUseDeviceFrame = false,
                         bool bLowLatency = false,
                         bool bDeviceFramePitched = false);

            void destroy();

        public:
            /**
             * 解码，送入h264或者h265的packet
             * @param pVideo h264/h265 包头地址
             * @param nVideoBytes h264/h265 包头长度
             * @return 0表示有正常的解码真，-1 表示暂无帧可用
             */
            int decode(uint8_t *pVideo, int nVideoBytes);

            /**
             * 获取解码后的帧
             * @param pFrame
             * @param pFrameSize
             * @return 返回的指针需要手动调用 delete []手动释放
             */
            bool getFrame(NvFrame *frame);

            bool restart();

        private:
            // 指定解码GPU序号
            int m_iGpu = 0;
            CUcontext m_cuContext = nullptr;
            NvDecoderPtr m_nvDecoder;
            bool m_bOutPlanar = true;
            bool m_bUseDeviceFrame = false;
            bool m_bLowLatency = false;
            bool m_bDeviceFramePitched = false;

            uint8_t *m_tFrame;
            uint8_t* m_tmpFrame = nullptr;

            CODEC_TYPE m_codecType;

            // frame和frame size
            std::queue<NvFrame> m_frames;

            bool m_initFlag = false;

            unsigned int last_frame_size = 0;
        };
    }

}


#endif //VIDEO_CODEC_SDK_11_1_5_DECODER_HW_H
