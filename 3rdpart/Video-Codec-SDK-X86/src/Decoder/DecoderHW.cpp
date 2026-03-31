//
// Created by 十七 on 2022/10/15.
//

#include "DecoderHW.h"
#include "CUContext.h"

#define TMP_FRAME_SZIE 167771216
namespace ja {
    namespace server {

        cudaVideoCodec aCodec[] = {
                cudaVideoCodec_H264,
                cudaVideoCodec_HEVC
        };


        static void ConvertSemiplanarToPlanar(uint8_t *pHostFrame, int nWidth, int nHeight, int nBitDepth) {
            if (nBitDepth == 8) {
                // nv12->iyuv
                YuvConverter <uint8_t> converter8(nWidth, nHeight);
                converter8.UVInterleavedToPlanar(pHostFrame);
            } else {
                // p016->yuv420p16
                YuvConverter <uint16_t> converter16(nWidth, nHeight);
                converter16.UVInterleavedToPlanar((uint16_t *) pHostFrame);
            }
        }

        DecoderHW::DecoderHW() {
            ck(cuInit(0));
            m_frames = std::queue<NvFrame>();
            m_tFrame = new uint8_t[TMP_FRAME_SZIE];
        }

        DecoderHW::~DecoderHW() {
 	    if(m_tFrame != nullptr){
     		delete[] m_tFrame;
                m_tFrame = nullptr;
            }


            if(m_tmpFrame != nullptr){
                delete[] m_tmpFrame;
                m_tmpFrame = nullptr;
            }
        }

        bool DecoderHW::initial() {
            // 创建解码器实例，初始化解码环境
            bool ret = true;
            int nGpu = 0;
            m_nvDecoder = nullptr;
            ck(cuDeviceGetCount(&nGpu));
            if (m_iGpu < 0 || m_iGpu >= nGpu) {
                std::cout << "GPU ordinal out of range. Should be within [" << 0 << ", " << nGpu - 1 << "]"
                          << std::endl;
                return 1;
            }

//            createCudaContext(&m_cuContext, m_iGpu, 0);
            m_cuContext = ContextManager::getContext()->m_cuContext;

            m_nvDecoder = std::make_unique<NvDecoder>(m_cuContext, m_bUseDeviceFrame, aCodec[m_codecType],
                                                      m_bLowLatency, m_bDeviceFramePitched);
            m_nvDecoder->SetOperatingPoint(false, false);

            m_initFlag = ret;

            return ret;
        }

        bool DecoderHW::initial(CODEC_TYPE codecType, int iGpu, bool bOutPlanar, bool bUseDeviceFrame, bool bLowLatency,
                                bool bDeviceFramePitched) {
            bool ret = true;
            m_nvDecoder = nullptr;
            m_codecType = codecType;
            m_iGpu = iGpu;
            m_bUseDeviceFrame = bUseDeviceFrame;
            m_bOutPlanar = bOutPlanar;
            m_bLowLatency = bLowLatency;
            m_bDeviceFramePitched = bDeviceFramePitched;
            // GPU序号数量
            int nGpu = 0;
            ck(cuDeviceGetCount(&nGpu));
            if (m_iGpu < 0 || m_iGpu >= nGpu) {
                std::cout << "GPU ordinal out of range. Should be within [" << 0 << ", " << nGpu - 1 << "]"
                          << std::endl;
                return 1;
            }

//            createCudaContext(&m_cuContext, m_iGpu, 0);
            m_cuContext = ContextManager::getContext()->m_cuContext;

            switch (m_codecType) {
                case H264:
                    m_nvDecoder = std::make_unique<NvDecoder>(m_cuContext, m_bUseDeviceFrame, cudaVideoCodec_H264,
                                                              m_bLowLatency, m_bDeviceFramePitched);
                    std::cout << "set codec type H264\n";
                    // 只有对AV1 SVC 有效，所以在h264,H265编码设置为false
                    m_nvDecoder->SetOperatingPoint(false, false);
                    break;
                case HEVC:
                    m_nvDecoder = std::make_unique<NvDecoder>(m_cuContext, m_bUseDeviceFrame, cudaVideoCodec_HEVC,
                                                              m_bLowLatency, m_bDeviceFramePitched);
                    std::cout << "set codec type HEVC\n";
                    m_nvDecoder->SetOperatingPoint(false, false);
                    break;
                default:
                    std::cout << " require a codec type!!!\n";
                    ret = false;
                    break;
            }

            m_initFlag = ret;
            return ret;
        }

        void DecoderHW::destroy() {
           if (m_initFlag) {
                std::queue<NvFrame> empty_queue;
                swap(empty_queue, m_frames);
                m_nvDecoder = nullptr;
                m_initFlag = false;
//                ck(cuCtxDestroy(m_cuContext));
           }
        }


        int DecoderHW::decode(uint8_t *pVideo, int nVideoBytes) {
            int nFrameReturned;
            nFrameReturned = m_nvDecoder->Decode(pVideo, nVideoBytes);

            bool bDecodeOutSemiPlanar = false;
            uint8_t *pFrame = nullptr;
            if (nFrameReturned) {

                bDecodeOutSemiPlanar = (m_nvDecoder->GetOutputFormat() == cudaVideoSurfaceFormat_NV12) ||
                                       (m_nvDecoder->GetOutputFormat() == cudaVideoSurfaceFormat_P016);

                for (int i = 0; i < nFrameReturned; i++) {
                    pFrame = m_nvDecoder->GetFrame();

                    if (m_bOutPlanar && bDecodeOutSemiPlanar) {
                        ConvertSemiplanarToPlanar(pFrame, m_nvDecoder->GetWidth(), m_nvDecoder->GetHeight(),
                                                  m_nvDecoder->GetBitDepth());
                    }

                    // 如果尚未初始化，初始化
                    if(!m_tmpFrame)
                    {
                        m_tmpFrame = new uint8_t[m_nvDecoder->GetFrameSize()];
                        last_frame_size = m_nvDecoder->GetFrameSize();
                    }else if(m_nvDecoder->GetFrameSize() != last_frame_size)
                    {
                        delete[] m_tmpFrame;
                        m_tmpFrame = nullptr;
                        m_tmpFrame = new uint8_t[m_nvDecoder->GetFrameSize()];
                        last_frame_size = m_nvDecoder->GetFrameSize();
                    }

                    if (m_nvDecoder->GetWidth() == m_nvDecoder->GetDecodeWidth()) {

                        memcpy(m_tmpFrame, pFrame, m_nvDecoder->GetFrameSize());
                        NvFrame frame;
                        frame.data = m_tmpFrame;
                        frame.size = m_nvDecoder->GetFrameSize();
                        frame.height = m_nvDecoder->GetHeight();
                        frame.width = m_nvDecoder->GetWidth();

                        m_frames.push(frame);
                    } else {
                        std::cout << " odd" << std::endl;
                        // 4:2:0 output width is 2 byte aligned. If decoded width is odd , luma has 1 pixel padding
                        // Remove padding from luma while dumping it to disk
                        // dump luma
                        int offset = 0;
                        for (auto i = 0; i < m_nvDecoder->GetHeight(); i++) {
                            memcpy(m_tFrame + offset, pFrame, m_nvDecoder->GetDecodeWidth() * m_nvDecoder->GetBPP());
                            pFrame += m_nvDecoder->GetWidth() * m_nvDecoder->GetBPP();
                            offset += m_nvDecoder->GetWidth() * m_nvDecoder->GetBPP();
                        }
                        // dump Chroma
                        memcpy(m_tFrame + offset, pFrame, m_nvDecoder->GetChromaPlaneSize());
                        offset += m_nvDecoder->GetChromaPlaneSize();
                        memcpy(m_tmpFrame, m_tFrame, offset);
                        NvFrame frame;
                        frame.data = m_tmpFrame;
                        frame.height = m_nvDecoder->GetHeight();
                        frame.width = m_nvDecoder->GetWidth();
                        frame.size = offset;
                        m_frames.push(frame);
                    }

                }
            }
            // 0 表示正常解码。-1表示没有解码出图片
            return nFrameReturned > 0 ? 0 : -1;
        }

        bool DecoderHW::getFrame(NvFrame *frame) {
            if (m_frames.empty()) {
                std::cout << "there is no more frames in decoder queue\n";
                return false;
            }
            *frame = m_frames.front();
            m_frames.pop();
//        *pFrame = packet.first;
//        *pFrameSize = packet.second;
            return true;
        }

      bool DecoderHW::restart() {
            // 清空队列
            std::queue<NvFrame> empty_queue;
            swap(empty_queue, m_frames);
            m_nvDecoder = nullptr;
            destroy();
            initial();
            return true;
        }

    }

}
