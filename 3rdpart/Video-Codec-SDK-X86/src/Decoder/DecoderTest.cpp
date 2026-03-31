//
// Created by 十七 on 2022/10/15.
//
#include "DecoderHW.h"
#include "../Utils/FFmpegDemuxer.h"
#include <fstream>
//simplelogger::Logger *logger = simplelogger::LoggerFactory::CreateConsoleLogger();


int main() {
    ja::server::DecoderBase *decoder = ja::server::get_decoder();
    decoder->initial(ja::server::H264);
    const char *szInFilePath = "/home/test.h264";
    const char *szInFilePath2 = "/home/infrared.h264";
    FFmpegDemuxer demuxer1(szInFilePath);
    FFmpegDemuxer demuxer2(szInFilePath2);

    int nVideoBytes = 0;
    uint8_t *pVideo = NULL;
    int nFrame;
    std::ofstream outFile("/home/720dec.yuv", std::ios::out | std::ios::binary);
    std::ofstream outFile2("/home/360dec.yuv", std::ios::out | std::ios::binary);

    do {
        demuxer1.Demux(&pVideo, &nVideoBytes);
        nFrame = decoder->decode(pVideo, nVideoBytes);
        if (nFrame == 0) {
            ja::server::NvFrame frame;
            int size;
            decoder->getFrame(&frame);
            std::cout << "frame.width: " << frame.width << " frame.height: " << frame.height << "\n";
            outFile.write(reinterpret_cast<char *>(frame.data), frame.size);
        }
    } while (nVideoBytes);
    std::cout << "resolution changed\n";

    decoder->restart();


    do {
        demuxer2.Demux(&pVideo, &nVideoBytes);
        nFrame = decoder->decode(pVideo, nVideoBytes);

        if (nFrame == 0) {
            ja::server::NvFrame frame;
            int size;
            decoder->getFrame(&frame);
            std::cout << "frame.width: " << frame.width << " frame.height: " << frame.height << " frame.size: "<< frame.size <<"\n";
            outFile2.write(reinterpret_cast<char *>(frame.data), frame.size);
        }
    } while (nVideoBytes);

    return 0;
}
