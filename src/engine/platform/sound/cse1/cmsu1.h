//
// Created by Administrator on 2026/11/5.
//

#ifndef FURNACE_CMSU1_H
#define FURNACE_CMSU1_H

namespace CMSU1_PACKED { // CECNULL1 MICRO SOUND UNIT
    typedef uint16_t CMSU1_REG;
    typedef uint32_t CMSU1_DOUBLE_REG;
    typedef int16_t CMSU1_SIG_REG;
    typedef int32_t CMSU1_DOUBLE_SIG_REG;

    struct CMSU1_MIXER {
        CMSU1_DOUBLE_SIG_REG DELTA1, DELTA2;
        CMSU1_DOUBLE_REG CUTOFF1, CUTOFF2;
        CMSU1_DOUBLE_REG INPUT_DELTA_VOLUME;
        CMSU1_REG LOW_VOLUME;
        CMSU1_REG HIGH_VOLUME;

        void clock() noexcept;
        void hard_reset() noexcept;
    };

    struct CMSU1_CHANNEL {
        CMSU1_MIXER MIXER;
        CMSU1_REG ORIGIN[2];
        CMSU1_REG FLAGS[2];
        CMSU1_REG DELTA_ADDITION[2];
        CMSU1_REG DUTY[2];
        CMSU1_DOUBLE_REG PHASE[2];
        CMSU1_DOUBLE_REG PITCH[2];
        CMSU1_DOUBLE_REG START_PHASE;
        CMSU1_DOUBLE_REG END_PHASE;
    };
}

#endif //FURNACE_CMSU1_H
