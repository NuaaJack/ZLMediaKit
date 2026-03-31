//
// Created by 十七 on 2022/10/16.
//
#include "EncoderHW.h"
#include <iostream>
#include <fstream>
//simplelogger::Logger *logger = simplelogger::LoggerFactory::CreateConsoleLogger();

std::ofstream fpOut("/home/encoder.h264", std::ios::out | std::ios::binary);

//void encode_callback(std::vector<std::vector<uint8_t>>& packets,void* userData)
//{
//    for(auto & i : packets)
//    {
//    }
//}

class EncodeCallbackImpl : public ja::server::EncodeCallback {
    void callback(uint8_t *data, bool, uint32_t size, void *user_data) {
        fpOut.write(reinterpret_cast<char *>(data), size);
    }
};

int main() {

    ja::server::EncoderBase *encoderHw = ja::server::get_encoder();
    EncodeCallbackImpl impl;
    encoderHw->initial(ja::server::H264, 1920, 1088, ja::server::I420, 4000000);
    encoderHw->setEncodeCallback(&impl, nullptr);
    std::ifstream fpIn1("/home/720dec.yuv", std::ios::in | std::ios::binary);
    std::ifstream fpIn2("/home/360dec.yuv", std::ios::in | std::ios::binary);
    int i = 1;
    int file = 0;
    while (true) {
        int size = encoderHw->getFrameSize();
        std::cout << "size " << size << std::endl;
        uint8_t frame[size];
        uint8_t frame2[size];

        std::streamsize nRead;
        if (file == 0) {
            std::cout << " read file 1\n";
            nRead = fpIn1.read(reinterpret_cast<char *>(frame), size).gcount();
        } else if(file == 1){
            std::cout << " read file 2\n";
            nRead = fpIn2.read(reinterpret_cast<char *>(frame), size).gcount();
        }

        std::cout << "nRead : " << nRead << std::endl;
        if (nRead == size) {
            std::cout << "begin encode\n";
            encoderHw->encode(frame, size);
        } else if (file == 0) {
            file = 1;
            std::cout << "respond changed\n";
            encoderHw->setResolution(1280, 720);
//            encoderHw->encode(frame, size);
//            std::cout << "encode done" << i << '\n';
//            break;
        } else {
            encoderHw->encode(frame, size);
            std::cout << "encode done" << i << '\n';
            break;
        }
        i++;
//        std::cout << "encode frame " << i++ <<'\n';
    }


    return 0;
}