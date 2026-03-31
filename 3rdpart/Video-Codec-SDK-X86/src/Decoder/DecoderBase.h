//
// Created by 十七 on 2023/2/27.
//

#ifndef VIDEO_CODEC_SDK_X86_DECODERBASE_H
#define VIDEO_CODEC_SDK_X86_DECODERBASE_H

#include "codec_enum.h"
#include <cstdint>

namespace ja {
    namespace server {

        struct NvFrame {
            unsigned long flags;
            uint8_t *data;
            int size;
            unsigned int height;
            unsigned int width;
            time_t timestamp;
        };


        class DecoderBase {
        public:
            virtual ~DecoderBase() = default;

        public:
            virtual bool initial() = 0;

            virtual bool
            initial(CODEC_TYPE codecType, int iGpu = 0, bool bOutPlanar = true, bool bUseDeviceFrame = false,
                    bool bLowLatency = false,
                    bool bDeviceFramePitched = false) = 0;

            virtual void destroy() = 0;

        public:
            /**
             * 解码，送入h264或者h265的packet
             * @param pVideo h264/h265 包头地址
             * @param nVideoBytes h264/h265 包头长度
             * @return 返回值是当前可用的帧数，0则表示无帧可用，
             */
            virtual int decode(uint8_t *pVideo, int nVideoBytes) = 0;

            /**
             * 获取解码后的帧
             * @param pFrame
             * @param pFrameSize
             * @return 返回的指针需要手动调用 delete []手动释放
             */
            virtual bool getFrame(NvFrame *frame) = 0;

            virtual bool restart() = 0;

        };

        extern "C"
        {
            DecoderBase *get_decoder();
        }
    }

}


#endif //VIDEO_CODEC_SDK_X86_DECODERBASE_H
