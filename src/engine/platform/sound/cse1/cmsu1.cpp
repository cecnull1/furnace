//
// Created by Administrator on 2026/11/5.
//

#include "cmsu1.h"

#include "utils.h"

namespace CMSU1_PACKED {
    void CMSU1_MIXER::clock() noexcept {
        DELTA2 = bit_cast<CMSU1_DOUBLE_SIG_REG>(bit_cast<CMSU1_DOUBLE_REG>(DELTA2) + INPUT_DELTA_VOLUME);
        DELTA1 = bit_cast<CMSU1_DOUBLE_SIG_REG>(
            static_cast<uint32_t>((static_cast<int64_t>(DELTA1) * static_cast<uint64_t>(~CUTOFF1) >> 32) +
                (static_cast<int64_t>(DELTA2) * static_cast<uint64_t>(CUTOFF1) >> 32)));
        DELTA2 = bit_cast<int32_t>(static_cast<uint32_t>(static_cast<int64_t>(DELTA2) * static_cast<uint64_t>(CUTOFF2) >> 32));
        INPUT_DELTA_VOLUME = 0;
    }

    void CMSU1_MIXER::hard_reset() noexcept {
        DELTA1 = 0;
        DELTA2 = 0;
    }
}
