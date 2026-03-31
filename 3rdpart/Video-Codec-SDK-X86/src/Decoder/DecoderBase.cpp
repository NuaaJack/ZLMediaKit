//
// Created by 十七 on 2023/2/27.
//

#include "DecoderHW.h"


namespace ja {
    namespace server {

        DecoderBase *get_decoder() {
            return new DecoderHW();
        }
    }
}