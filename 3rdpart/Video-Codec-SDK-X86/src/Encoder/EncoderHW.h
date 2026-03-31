//
// Created by 十七 on 2022/10/16.
//

#ifndef VIDEO_CODEC_SDK_11_1_5_ENCODER_HW_H
#define VIDEO_CODEC_SDK_11_1_5_ENCODER_HW_H

#include <iostream>
#include <memory>
#include <cuda.h>
#include "NvCodecUtils.h"
#include "NvEncoder/NvEncoderCuda.h"
#include "NvEncoder/NvEncoderOutputInVidMemCuda.h"
#include "Logger.h"
#include "NvEncoderCLIOptions.h"
#include "codec_enum.h"
#include "EncoderBase.h"

namespace ja {
    namespace server {

        typedef std::unique_ptr<NvEncoderCuda> NvEncoderPtr;

        class EncoderHW : public EncoderBase {
        public:
            EncoderHW();

            ~EncoderHW() override;

            /**
             * 调用初始化函数
             * @param codecType 编码类型，H264 , HEVC
             * @param width 分辨率宽度
             * @param height 分辨率高度
             * @param pixelFormat 色彩格式
             * @param bitrate 码率
             * @param iGpu 指定GPU编码
             * @param fps 帧率
             * @param gop gop大小
             * @param profile 编码品质
             * @param preset 编码质量和速度等级
             * @param rc 码率控制模式
             * @return true表示初始化成功，false表示初始化失败
             */
            bool initial(CODEC_TYPE codecType, int width, int height, PIXEL_FORMAT pixelFormat,
                         int bitrate = 400000,
                         int iGpu = 0,
                         int fps = 25,
                         int gop = 25,
                         PROFILE profile = MAIN,
                         PRESET preset = P5,
                         RC rc = VBR
            );

            void destroy();

        public:
            /**
             * 输入一张图片及图片大小，通过回调会返回编码后的文件
             * @param data 图片原始地址
             * @param size 图片的大小
             * @return true表示编码成功，false表示编码失败
             */
            bool encode(uint8_t data[], int size);

            /**
             * 设置编码回调函数
             * @param encodeCallback 回调函数
             * @param userData 自定义数据
             */
            void setEncodeCallback(EncodeCallback *encodeCallback, void *userData);

            /**
             * 动态设置码率
             * @param bitrate 码率
             * @return true表示设置成功
             */
            bool setBitrate(int bitrate);

            /**
             * 动态设置分辨率
             * @param width 宽度
             * @param height 高度
             * @return true表示设置成功
             */
            bool setResolution(int width, int height);

            int getFrameSize();

        private:
            int m_iGpu = 0;
            CUcontext m_cuContext = nullptr;
            CUdevice m_cuDevice = 0;
            NvEncoderPtr m_nvEncoder;
            NV_ENC_INITIALIZE_PARAMS m_initializeParams = {NV_ENC_INITIALIZE_PARAMS_VER};
            NV_ENC_CONFIG m_encodeConfig = {NV_ENC_CONFIG_VER};

            int m_fps;
            int m_bitrate;
            int m_gop;
            PROFILE m_profile;
            PRESET m_preset;
            CODEC_TYPE m_codecType;
            PIXEL_FORMAT m_pixFormat;
            RC m_rc;

            // 回调函数设置
            EncodeCallback *m_encodeCallback;
            void *m_userData;

            bool m_initFlag = false;
        };
    }
}


#endif //VIDEO_CODEC_SDK_11_1_5_ENCODER_HW_H
