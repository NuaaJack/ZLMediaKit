//
// Created by 十七 on 2022/10/15.
//

#ifndef VIDEO_CODEC_SDK_11_1_5_CODEC_ENUM_H
#define VIDEO_CODEC_SDK_11_1_5_CODEC_ENUM_H


namespace ja {
    namespace server {

        enum PIXEL_FORMAT {
            I420 = 0,
            NV12 = 1,
            RGBA = 2,

            UNKNOWN_PIXEL_FORMAT
        };

        enum CODEC_TYPE {
            H264 = 0,
            HEVC = 1,

        };

        enum PRESET {
            P1 = 0,
            P2 = 1,
            P3 = 2,
            P4 = 3,
            P5 = 4,
            P6 = 5,
            P7 = 6,

            UNKNOWN_PRESET
        };

        enum PROFILE {
            BASELINE,
            MAIN,
            HIGH,
            HIGH444,
            MAIN10,
            FREXT,

            UNKNOWN_PROFILE
        };

        // Rate Control
        enum RC {
            CONSTQP,
            VBR,
            CBR
        };
    }

}

#endif //VIDEO_CODEC_SDK_11_1_5_CODEC_ENUM_H
