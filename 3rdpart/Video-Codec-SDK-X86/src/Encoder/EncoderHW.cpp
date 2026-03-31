//
// Created by 十七 on 2022/10/16.
//

#include "EncoderHW.h"
#include "CUContext.h"

namespace ja {
    namespace server {

        NV_ENC_BUFFER_FORMAT aFormat[] = {
                NV_ENC_BUFFER_FORMAT_IYUV,
                NV_ENC_BUFFER_FORMAT_NV12,
                NV_ENC_BUFFER_FORMAT_ARGB,
                NV_ENC_BUFFER_FORMAT_UNDEFINED,
        };

        std::string aCodecTypes[] = {
                "h264",
                "hevc"
        };

        std::string aPrset[] = {
                "p1",
                "p2",
                "p3",
                "p4",
                "p5",
                "p6",
                "p7",
        };

        std::string aProfile[] = {
                "baseline",
                "main",
                "high",
                "high444",
                "main10",
                "frext"
        };

        std::string aRc[] = {
                "constqp",
                "vbr",
                "cbr"
        };

        EncoderHW::EncoderHW() {
            ck(cuInit(0));
        }

        EncoderHW::~EncoderHW() {
            destroy();
        }

        bool EncoderHW::initial(CODEC_TYPE codecType, int width, int height, PIXEL_FORMAT pixelFormat,
                                int bitrate,
                                int iGpu,
                                int fps,
                                int gop,
                                PROFILE profile,
                                PRESET preset,
                                RC rc) {
            m_codecType = codecType;
            m_iGpu = iGpu;
            this->width = width;
            this->height = height;
            m_pixFormat = pixelFormat;
            m_fps = fps;
            m_gop = gop;
            m_bitrate = bitrate;
            m_profile = profile;
            m_preset = preset;
            m_rc = rc;

            int nGpu = 0;
            ck(cuDeviceGetCount(&nGpu));
            if (m_iGpu < 0 || m_iGpu >= nGpu) {
                std::cout << "GPU ordinal out of range. Should be within [" << 0 << ", " << nGpu - 1 << "]"
                          << std::endl;
                return false;
            }

            ck(cuDeviceGet(&m_cuDevice, m_iGpu));
            char szDeviceName[80];
            ck(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), m_cuDevice));
            std::cout << "GPU in use: " << szDeviceName << " m_iGpu " << m_iGpu << std::endl;

//            ck(cuCtxCreate(&m_cuContext, 0, m_cuDevice));
            m_cuContext = ContextManager::getContext()->m_cuContext;

            // 设置编码参数
            std::ostringstream oss;
            oss << "-codec " << aCodecTypes[m_codecType] << " -preset " << aPrset[preset]
                << " -profile " << aProfile[profile] << " -bitrate " << m_bitrate << " -maxbitrate " << m_bitrate
                << " -fps " << m_fps << " -gop " << m_gop << " -bf 0"
                << " -rc " << aRc[m_rc];
            std::cout << "init encode params : " << oss.str() << '\n';
            NvEncoderInitParam encodeCLIOptions;
            encodeCLIOptions = NvEncoderInitParam(oss.str().c_str());

            // 创建编码器
            m_nvEncoder = std::unique_ptr<NvEncoderCuda>(
                    new NvEncoderCuda(m_cuContext, this->width, this->height, aFormat[m_pixFormat]));

            // 初始化编码器
            if (m_initializeParams.encodeGUID == NV_ENC_CODEC_H264_GUID) {
                m_encodeConfig.encodeCodecConfig.h264Config.repeatSPSPPS = 1;
            } else if (m_initializeParams.encodeGUID == NV_ENC_CODEC_HEVC_GUID) {
                m_encodeConfig.encodeCodecConfig.hevcConfig.repeatSPSPPS = 1;
            }
            m_initializeParams.encodeConfig = &m_encodeConfig;
            m_nvEncoder->CreateDefaultEncoderParams(&m_initializeParams, encodeCLIOptions.GetEncodeGUID(),
                                                    encodeCLIOptions.GetPresetGUID(),
                                                    encodeCLIOptions.GetTuningInfo());
            encodeCLIOptions.SetInitParams(&m_initializeParams, aFormat[m_pixFormat]);


            m_nvEncoder->CreateEncoder(&m_initializeParams);
            std::cout << "编码器初始化成功\n";
            m_initFlag = true;
            return true;
        }

        void EncoderHW::destroy() {
            if (m_initFlag) {
		std::vector<std::vector<uint8_t>> vPacket;
                m_nvEncoder->EndEncode(vPacket);
                m_nvEncoder->DestroyEncoder();
                m_nvEncoder = nullptr;
                m_initFlag = false;
               // ck(cuCtxDestroy(m_cuContext));
            }
        }

        bool EncoderHW::encode(uint8_t data[], int size) {
            int nFrameSize = m_nvEncoder->GetFrameSize();

            std::vector<std::vector<uint8_t>> vPacket;
            if (size == nFrameSize) {
                // 获取GPU地址，将内容从CPU拷贝到GPU
                const NvEncInputFrame *encoderInputFrame = m_nvEncoder->GetNextInputFrame();
                NvEncoderCuda::CopyToDeviceFrame(m_cuContext, data, 0, (CUdeviceptr) encoderInputFrame->inputPtr,
                                                 (int) encoderInputFrame->pitch,
                                                 m_nvEncoder->GetEncodeWidth(),
                                                 m_nvEncoder->GetEncodeHeight(),
                                                 CU_MEMORYTYPE_HOST,
                                                 encoderInputFrame->bufferFormat,
                                                 encoderInputFrame->chromaOffsets,
                                                 encoderInputFrame->numChromaPlanes);

                m_nvEncoder->EncodeFrame(vPacket);
            } else {
                m_nvEncoder->EncodeFrame(vPacket);
            }
            for (int i = 0; i < vPacket.size(); i++) {
                m_encodeCallback->callback((uint8_t *) vPacket[i].data(), true, vPacket[i].size(), m_userData);
            }
            return true;
        }

        void EncoderHW::setEncodeCallback(EncodeCallback *encodeCallback, void *userData) {
            m_encodeCallback = encodeCallback;
            m_userData = userData;
        }

        int EncoderHW::getFrameSize() {
            return m_nvEncoder->GetFrameSize();
        }

        bool EncoderHW::setBitrate(int bitrate) {
            std::cout << "change bitrate " << bitrate << '\n';
            m_bitrate = bitrate;
            NV_ENC_RECONFIGURE_PARAMS reconfigureParams = {NV_ENC_RECONFIGURE_PARAMS_VER};
            memcpy(&reconfigureParams.reInitEncodeParams, &m_initializeParams, sizeof(m_initializeParams));
            NV_ENC_CONFIG reInitCodecConfig = {NV_ENC_CONFIG_VER};
            memcpy(&reInitCodecConfig, m_initializeParams.encodeConfig, sizeof(reInitCodecConfig));
            reconfigureParams.reInitEncodeParams.encodeConfig = &reInitCodecConfig;
            reconfigureParams.reInitEncodeParams.encodeConfig->rcParams.averageBitRate = bitrate;
            reconfigureParams.reInitEncodeParams.encodeConfig->rcParams.maxBitRate = bitrate;
//        reconfigureParams.reInitEncodeParams.encodeConfig->rcParams.vbvBufferSize =
//                reconfigureParams.reInitEncodeParams.encodeConfig->rcParams.averageBitRate *
//                reconfigureParams.reInitEncodeParams.frameRateDen / reconfigureParams.reInitEncodeParams.frameRateNum;
//        reconfigureParams.reInitEncodeParams.encodeConfig->rcParams.vbvInitialDelay = reconfigureParams.reInitEncodeParams.encodeConfig->rcParams.vbvBufferSize;
            m_nvEncoder->Reconfigure(&reconfigureParams);
            NvEncoderInitParam encodeCLIOptions;

            std::cout << encodeCLIOptions.MainParamToString(&reconfigureParams.reInitEncodeParams) << std::endl;
//        memcpy(&m_initializeParams,&reconfigureParams.reInitEncodeParams,sizeof(reconfigureParams.reInitEncodeParams));
            return true;
        }

        bool EncoderHW::setResolution(int width, int height) {
            if (width == this->width && height == this->height) {
                std::cout << "no need to reconfigure\n";
                return true;
            }
            this->width = width;
            this->height = height;

            NV_ENC_RECONFIGURE_PARAMS reconfigureParams = {NV_ENC_RECONFIGURE_PARAMS_VER};
            memcpy(&reconfigureParams.reInitEncodeParams, &m_initializeParams, sizeof(m_initializeParams));
            NV_ENC_CONFIG reInitCodecConfig = {NV_ENC_CONFIG_VER};
            memcpy(&reInitCodecConfig, m_initializeParams.encodeConfig, sizeof(reInitCodecConfig));
            reconfigureParams.reInitEncodeParams.encodeConfig = &reInitCodecConfig;

            reconfigureParams.reInitEncodeParams.encodeWidth = this->width;
            reconfigureParams.reInitEncodeParams.encodeHeight = this->height;
            reconfigureParams.reInitEncodeParams.darWidth = reconfigureParams.reInitEncodeParams.encodeWidth;
            reconfigureParams.reInitEncodeParams.darHeight = reconfigureParams.reInitEncodeParams.encodeHeight;
            reconfigureParams.forceIDR = true;

            m_nvEncoder->Reconfigure(&reconfigureParams);
            NvEncoderInitParam encodeCLIOptions;

            std::cout << encodeCLIOptions.MainParamToString(&reconfigureParams.reInitEncodeParams) << std::endl;
            return true;
        }
    }

}
