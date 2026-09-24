//
// Created by Administrator on 2026/9/12.
//

#include "cse1.hpp"

namespace CSE1_PACKED {
    constexpr long double PI = 3.1415926535897932384626433832795028841971L;

    void CSE1::hard_reset() {
        this->CHANNELS = CSE1_CHANNELS();
    }

    void CSE1::clock(const WaveTable& wave_table) {
        this->CHANNELS.clock(wave_table, this);
    }

    CSE1_DOUBLE_SIG_REG CSE1::CSE1_GET_SAMPLE(uint8_t ch) const {
        int64_t ret = (static_cast<int64_t>(this->CHANNELS.OUTS_L[ch]) + static_cast<int64_t>(this->CHANNELS.OUTS_R[ch]));
        if (ret < INT16_MIN) ret = INT16_MIN;
        if (ret > INT16_MAX) ret = INT16_MAX;
        return static_cast<int32_t>(ret);
    }

    void CSE1_CHANNELS::clock(const WaveTable& wave_table, CSE1* ins) {
        for (size_t i = 0; i < CHANNEL.size(); i++) {
            auto& channel = CHANNEL[i];
            auto& filterState = FILTER_PRIVATE[i];
            OUTS_R[i]=OUTS_L[i]=0;
            channel.clock(wave_table, filterState, OUTS_L[i], OUTS_R[i]);
        }
    }

    void CSE1_CHANNEL_REGISTERS::clock(const WaveTable& wave_table, const std::array<CSE1_FILTER_PRIVATE, 3> &value, CSE1_DOUBLE_SIG_REG& LeftBuf, CSE1_DOUBLE_SIG_REG& RightBuf) {
        OPS.clock(wave_table, this, LeftBuf, RightBuf);
        LeftBuf = LeftBuf*OUT.OUT_L>>16;
        RightBuf = RightBuf*OUT.OUT_R>>16;
    }

    void CSE1_OPS::clock(const WaveTable &wave_table, CSE1_CHANNEL_REGISTERS* State, CSE1_DOUBLE_SIG_REG &LeftBuf, CSE1_DOUBLE_SIG_REG &RightBuf) {
        for (size_t opi = 0; opi < OP.size(); opi++) {
            OP[opi].clock(wave_table, State);
            LeftBuf += (((OP[opi].VFB-0x8000)*State->OUT.IN_L.MI[opi]>>16)*wave_table.default_volume_line[OP[opi].ENV_STATE.ENV_VP]>>16)>>3;
            RightBuf += (((OP[opi].VFB-0x8000)*State->OUT.IN_R.MI[opi]>>16)*wave_table.default_volume_line[OP[opi].ENV_STATE.ENV_VP]>>16)>>3;
        }
    }

    static constexpr CSE1_DOUBLE_REG MULT_CALC(const CSE1_DOUBLE_REG left, const uint8_t right) noexcept {
        return right == 0 ? left >> 1 : left * right;
    }

    static constexpr CSE1_REG BIT_EX(const bool v) noexcept {
        return ~(static_cast<CSE1_REG>(v)-1u);
    }

    void CSE1_OPER_STATE::clock(const WaveTable &wave_table, const CSE1_CHANNEL_REGISTERS *state) {
        if (FLAGS_A.GET_WAVE() < 6) {
            const auto o_pitch = this->FLAGS_A.GET_FIXED() ? this->PITCH : state->OUT.PITCH;
            PHASE += MULT_CALC(o_pitch + this->PITCH + (this->FLAGS_A.GET_OPN2_DETUNE()-4) * (o_pitch >> 10), this->FLAGS_A.GET_ML());
        }
        this->ENV_STATE.clock(this, this->ADSR);

        CSE1_REG add_phase = 0;
        for (size_t i = 0; i < state->OPS.OP.size(); i++) {
            auto& op = state->OPS.OP[i];
            add_phase += (static_cast<uint32_t>(op.VFB * wave_table.default_volume_line[op.ENV_STATE.ENV_VP]) >> 16) * this->MIS.MI[i] >> 13;
        }
        add_phase += PHASE>>16;
        if (FLAGS_A.GET_WAVE() < 6) {
            VFB = option_wavetable(wave_table, add_phase&0xffff);
        }
    }

    static uint16_t sat_add_u16(uint16_t a, uint16_t b) {
        uint32_t sum = (uint32_t)a + (uint32_t)b;
        return (sum > 0xFFFF) ? 0xFFFF : (uint16_t)sum;
    }

    inline uint16_t sat_sub_u16(uint16_t a, uint16_t b) {
        return (a < b) ? 0 : (a - b);
    }

    void CSE1_OPER_ENV_STATE::clock(const CSE1_OPER_STATE *state, CSE1_ADSR &adsr) {
        if (this->GET_ENV_DIVIDER_COUNT() == 0) {
            this->SET_ENV_DIVIDER_COUNT(state->FLAGS_B.GET_ENV_DIVIDER());
            switch(this->GET_ENV_ENUM()) {
                case 0: {
                    ENV_VP = sat_add_u16(ENV_VP, adsr.AR);
                    if (ENV_VP == 0xffff) { SET_ENV_ENUM(1); }
                    break;
                }
                case 1: {
                    ENV_VP = sat_sub_u16(ENV_VP, adsr.DR);
                    if (ENV_VP <= adsr.SL) { ENV_VP = adsr.SL, SET_ENV_ENUM(2); }
                    break;
                }
                case 2: {
                    ENV_VP = sat_sub_u16(ENV_VP, adsr.SR);
                    break;
                }
                case 3: {
                    ENV_VP = sat_sub_u16(ENV_VP, adsr.RR);
                    break;
                }
                default: ;
            }
        }
        this->SET_ENV_DIVIDER_COUNT(this->GET_ENV_DIVIDER_COUNT()-1);
    }

    CSE1_REG CSE1_OPER_STATE::spec_wavetable(const WaveTable &wave_table) {
        switch (this->FLAGS_A.GET_WAVE()) {
            case OPER_ONESHOT_SAMPLE: {
                return 0; // TODO: PCM
            }
            case OPER_LOOP_SAMPLE: {
                return 0; // TODO: PCM
            }
            default: return 0;
        }
    }

    CSE1_REG CSE1_OPER_STATE::option_wavetable(const WaveTable &wave_table, const CSE1_REG index) {
        switch (this->FLAGS_A.GET_WAVE()) {
            case SINE: {
                return wave_table.sine[index];
            }
            case TRIANGLE: {
                return wave_table.triangle[index];
            }
            case SQUARE: {
                return BIT_EX(index>this->DUTY);
            }
            case SAW: {
                return index;
            }
            case OPER_NOISE: {
                if (this->PHASE & 0x10000000) {
                    this->PHASE &= 0x0fffffff;
                    this->DUTY = (this->DUTY >> 1) | (((this->DUTY ^ (this->DUTY >> 2) ^ (this->DUTY >> 3) ^ (this->DUTY >> 5)) & 1) << 15);
                }
                return BIT_EX(this->DUTY & 1);
            }
            case OPER_WAVETABLE_SAMPLE: {
                return wave_table.memPCM[index+START_PHASE];
            }
            default: return 0;
        }
    }

    WaveTable::WaveTable(): default_volume_line(old_js_expw.data()), memPCM(nullptr) {
        for (size_t i = 0; i < sine.size(); i++) {
            sine[i] = static_cast<CSE1_REG>(std::sin(static_cast<long double>(i)*2*PI/65536.0L)*32767.0+32767.0)&0xffff;
        }
        for (size_t i = 0; i < triangle.size(); i++) {
            triangle[i] = static_cast<CSE1_REG>(std::asin(std::sin(static_cast<long double>(i)*2*PI/65536.0L))/PI*2*32767.0+32767.0)&0xffff;
        }

        for (size_t i = 0; i < old_js_expw.size(); i++) {
            old_js_expw[i] = static_cast<CSE1_REG>(
                (std::exp(i / 65535.0L * std::log(2.0L)) - 1.0L) * 65535.0L
            ) & 0xffff;
        }

        for (size_t i = 0; i < real_volume_line.size(); i++) {
            long double tl = (65535.0L-i) / 16383.0L;
            long double volume = std::pow(10.0L, -tl);
            real_volume_line[i] = static_cast<CSE1_REG>(volume * 65535.0L) & 0xffff;
        }
    }

}
