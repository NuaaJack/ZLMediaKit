//
// Created by 十七 on 2023/2/27.
//

#include "EncoderBase.h"
#include "EncoderHW.h"

namespace ja {
    namespace server {
        EncoderBase *get_encoder() {
            EncoderBase *encoder = new EncoderHW();
            return encoder;
        }
    }

}