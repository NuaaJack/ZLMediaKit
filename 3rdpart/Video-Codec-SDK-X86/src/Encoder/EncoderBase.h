//
// Created by 十七 on 2023/2/27.
//

#ifndef VIDEO_CODEC_SDK_X86_ENCODERBASE_H
#define VIDEO_CODEC_SDK_X86_ENCODERBASE_H

#include "codec_enum.h"
#include <cstdint>

namespace ja {
    namespace server {

        /**
         * packets里含有多个包，每个包是一个vector数组，按顺序读取。每个vector读取对应size大小即可
         */
        typedef class EncodeCallback {
        public:
            virtual void callback(uint8_t *data, bool, uint32_t size, void *user_data) = 0;
        };

        class EncoderBase {
        public:
            virtual ~EncoderBase() = default;

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
            virtual bool initial(CODEC_TYPE codecType, int width, int height, PIXEL_FORMAT pixelFormat,
                                 int bitrate,
                                 int iGpu = 0,
                                 int fps = 25,
                                 int gop = 25,
                                 PROFILE profile = MAIN,
                                 PRESET preset = P5,
                                 RC rc = VBR
            ) = 0;

            virtual void destroy() = 0;

        public:
            /**
             * 输入一张图片及图片大小，通过回调会返回编码后的文件
             * @param data 图片原始地址
             * @param size 图片的大小
             * @return true表示编码成功，false表示编码失败
             */
            virtual bool encode(uint8_t data[], int size) = 0;

            /**
             * 设置编码回调函数
             * @param encodeCallback 回调函数
             * @param userData 自定义数据
             */
            virtual void setEncodeCallback(EncodeCallback *encodeCallback, void *userData) = 0;

            /**
             * 动态设置码率
             * @param bitrate 码率
             * @return true表示设置成功
             */
            virtual bool setBitrate(int bitrate) = 0;

            /**
             * 动态设置分辨率
             * @param width 宽度
             * @param height 高度
             * @return true表示设置成功
             */
            virtual bool setResolution(int width, int height) = 0;

            virtual int getFrameSize() = 0;

        public:
            int width;
            int height;

        };

        extern "C"
        {
            EncoderBase *get_encoder();
        }
    }
}


#endif //VIDEO_CODEC_SDK_X86_ENCODERBASE_H
