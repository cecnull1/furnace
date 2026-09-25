//
// Created by Administrator on 2026/9/12.
//

#include "cse1.hpp"

namespace CSE1_PACKED {
    constexpr long double PI = 3.1415926535897932384626433832795028841971L;

    void CSE1::hard_reset() {
        this->CHANNELS = CSE1_CHANNELS();
    }

    void CSE1::clock(const WaveTable& wave_table) noexcept {
        this->CHANNELS.clock(wave_table);
    }

    CSE1_DOUBLE_SIG_REG CSE1::CSE1_GET_SAMPLE(uint8_t ch) const noexcept {
        int64_t ret = (static_cast<int64_t>(this->CHANNELS.OUTS_L[ch]) + static_cast<int64_t>(this->CHANNELS.OUTS_R[ch]));
        if (ret < INT16_MIN) ret = INT16_MIN;
        if (ret > INT16_MAX) ret = INT16_MAX;
        return static_cast<int32_t>(ret);
    }

    void CSE1_CHANNELS::clock(const WaveTable& wave_table) noexcept {
        for (size_t i = 0; i < CHANNEL.size(); i++) {
            auto& channel = CHANNEL[i];
            auto& filterState = FILTER_PRIVATE[i];
            OUTS_R[i]=OUTS_L[i]=0;
            channel.clock(wave_table, filterState, OUTS_L[i], OUTS_R[i]);
        }
    }

    void CSE1_CHANNEL_REGISTERS::clock(const WaveTable& wave_table, std::array<CSE1_FILTER_PRIVATE, 3>& value, CSE1_DOUBLE_SIG_REG& LeftBuf, CSE1_DOUBLE_SIG_REG& RightBuf) noexcept {
        CSE1_DOUBLE_SIG_REG LeftBuf2 = 0;
        CSE1_DOUBLE_SIG_REG RightBuf2 = 0;
        OPS.clock(wave_table, this, LeftBuf2, RightBuf2);
        SPEC.LFO1.PHASE+=SPEC.LFO_CONFIG.GET_FREQ();
        SPEC.LFO2.PHASE+=SPEC.LFO_CONFIG.GET_FREQ2();
        OUT.PITCH += SPEC.SWEEP_FREQS[CSE1_OPER_NUMBER].FREQ_SPEED;
        CSE1_DOUBLE_SIG_REG LeftBuf3 = 0;
        CSE1_DOUBLE_SIG_REG RightBuf3 = 0;
        for (size_t f = 0; f < SPEC.FILTERS.size(); f++) {
            SPEC.FILTERS[f].clock(wave_table, this, value[f], value, LeftBuf2, RightBuf2);
            LeftBuf3 += value[f].VFB_L;
            RightBuf3 += value[f].VFB_R;
        }

        LeftBuf = (LeftBuf2 * wave_table.default_volume_line[OUT.OUT_L] >> 16) + LeftBuf3;
        RightBuf = (RightBuf2 * wave_table.default_volume_line[OUT.OUT_R] >> 16) + RightBuf3;
    }

    void CSE1_FILTER::clock(
        const WaveTable& wave_table,
        const CSE1_CHANNEL_REGISTERS* state,
        CSE1_FILTER_PRIVATE& filter_private,
        const std::array<CSE1_FILTER_PRIVATE, 3>& value,
        const CSE1_DOUBLE_SIG_REG in1,
        const CSE1_DOUBLE_SIG_REG in2) const {
        // TODO: Filter
    }

    void CSE1_OPS::clock(const WaveTable &wave_table, const CSE1_CHANNEL_REGISTERS* State, CSE1_DOUBLE_SIG_REG &LeftBuf, CSE1_DOUBLE_SIG_REG &RightBuf) noexcept {

        const auto fm1pitch = static_cast<uint64_t>(static_cast<CSE1_SIG_REG>(CSE1_LFO_CONFIG::option_wavetable(State->SPEC.LFO_CONFIG.GET_SHAPE(), wave_table, State->SPEC.LFO1.PHASE)-0x8000)
                    *State->SPEC.LFO_CONFIG.GET_DEPTH());
        const auto fm2pitch = static_cast<uint64_t>(static_cast<CSE1_SIG_REG>(CSE1_LFO_CONFIG::option_wavetable(State->SPEC.LFO_CONFIG.GET_SHAPE2(), wave_table, State->SPEC.LFO2.PHASE)-0x8000)
                    *State->SPEC.LFO_CONFIG.GET_DEPTH2());

        for (size_t opi = 0; opi < OP.size(); opi++) {
            OP[opi].clock(wave_table, State, fm1pitch, fm2pitch);
            OP[opi].PITCH+=State->SPEC.SWEEP_FREQS[opi].FREQ_SPEED;
            LeftBuf +=  (((OP[opi].VFB-0x8000)*State->OUT.IN_L.MI[opi]>>16)*wave_table.default_volume_line[OP[opi].ENV_STATE.ENV_VP]>>16)>>3;
            RightBuf += (((OP[opi].VFB-0x8000)*State->OUT.IN_R.MI[opi]>>16)*wave_table.default_volume_line[OP[opi].ENV_STATE.ENV_VP]>>16)>>3;
        }
    }

    constexpr CSE1_DOUBLE_REG MULT_CALC(const CSE1_DOUBLE_REG left, const uint8_t right) noexcept {
        return right == 0 ? left >> 1 : left * right;
    }

    constexpr CSE1_REG BIT_EX(const bool v) noexcept {
        return ~(static_cast<CSE1_REG>(v)-1u);
    }

    constexpr CSE1_DOUBLE_REG BIT_EX_32(const bool v) noexcept {
        return  ~(static_cast<CSE1_DOUBLE_REG>(v)-1u);
    }

    void CSE1_OPER_STATE::clock(const WaveTable &wave_table, const CSE1_CHANNEL_REGISTERS *state, const uint64_t ExtFM1AddPitch, const uint64_t ExtFM2AddPitch) noexcept {
        if (FLAGS_A.GET_WAVE() < 6) {
            const auto o_pitch = this->FLAGS_A.GET_FIXED() ? this->PITCH : state->OUT.PITCH;
            const auto depth = o_pitch;
            PHASE += MULT_CALC(
                o_pitch +
                this->PITCH +
                (this->FLAGS_A.GET_OPN2_DETUNE()-4) * (o_pitch >> 10) +
                (FLAGS_A.GET_FM1() ? ExtFM1AddPitch * depth >> 24: 0) +
                (FLAGS_A.GET_FM2() ? ExtFM2AddPitch * depth >> 24: 0),
                this->FLAGS_A.GET_ML()
                );
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

    static uint16_t sat_add_u16(uint16_t a, uint16_t b) noexcept {
        uint32_t sum = static_cast<uint32_t>(a) + static_cast<uint32_t>(b);
        return sum > 0xFFFF ? 0xFFFF : static_cast<uint16_t>(sum);
    }

    static uint16_t sat_sub_u16(uint16_t a, uint16_t b) noexcept {
        return a < b ? 0 : a - b;
    }

    void CSE1_OPER_ENV_STATE::clock(const CSE1_OPER_STATE *state, const CSE1_ADSR &adsr) noexcept {
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

    // CSE1_REG CSE1_OPER_STATE::spec_wavetable(const WaveTable &wave_table) noexcept {
    //     switch (this->FLAGS_A.GET_WAVE()) {
    //         case OPER_ONESHOT_SAMPLE: {
    //             return 0; // TODO: PCM
    //         }
    //         case OPER_LOOP_SAMPLE: {
    //             return 0; // TODO: PCM
    //         }
    //         default: return 0;
    //     }
    // }

    constexpr CSE1_REG CSE1_LFO_CONFIG::option_wavetable(const uint8_t shape, const WaveTable &wave_table,
        const CSE1_REG index) noexcept {
        switch (shape) {
            case SINE: {
                return wave_table.sine[index];
            }
            case TRIANGLE: {
                return wave_table.triangle[index];
            }
            case SQUARE: {
                return BIT_EX(index&0x8000);
            }
            case SAW: {
                return index;
            }
            default: return 0;
        }
    }

    CSE1_REG CSE1_OPER_STATE::option_wavetable(const WaveTable &wave_table, const CSE1_REG index) noexcept {
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

    WaveTable::WaveTable(): default_volume_line(old_js_expw.data()), memPCM(nullptr), fastSpeed(0x4000) {
        for (size_t i = 0; i < sine.size(); i++) {
            sine[i] = static_cast<CSE1_REG>(std::sin(static_cast<long double>(i)*2*PI/65536.0L)*32767.0+32767.0)&0xffff;
        }
        for (size_t i = 0; i < triangle.size(); i++) {
            triangle[i] = static_cast<CSE1_REG>(std::asin(std::sin(static_cast<long double>(i)*2*PI/65536.0L))/PI*2*32767.0+32767.0)&0xffff;
        }

        for (size_t i = 0; i < divider_table.size(); i++) {
            divider_table[i] = i != 0 ? 65535/i: 65535;
        }

        for (size_t i = 0; i < tan_table.size(); i++) {
            long double angle = static_cast<long double>(i) / 65535.0L * (PI / 2.0L);
            long double t = std::tan(angle);
            if (t > 1.0e10L) {
                tan_table[i] = 65535;
            } else {
                long double normalized = t / (1.0L + t);
                tan_table[i] = static_cast<CSE1_REG>(normalized * 65535.0L) & 0xffff;
            }
        }
    }

    void WaveTable::reset() noexcept {
        for (size_t i = 0; i < old_js_expw.size(); i++) {
            old_js_expw[i] = fastSpeed != 0 ? static_cast<CSE1_REG>(
                (std::exp(i/65535.0L * std::log(static_cast<long double>(fastSpeed)*fastSpeed+1.0L)) - 1.0L) * 65535.0L / (static_cast<long double>(fastSpeed)*fastSpeed)
            ) & 0xffff : i;
        }

        for (size_t i = 0; i < real_volume_line.size(); i++) {
            long double tl = (65535.0L-i) / 16383.0L*fastSpeed/16384.0L;
            long double volume = std::pow(10.0L, -tl);
            real_volume_line[i] = static_cast<CSE1_REG>(volume * 65535.0L) & 0xffff;
        }

        for (size_t i = 0; i < exp_volume_line.size(); i++) {
            long double tl = (65535.0L-i) / 8191.0L*fastSpeed/16384.0L;
            long double volume = std::exp(-tl);
            exp_volume_line[i] = static_cast<CSE1_REG>(volume * 65535.0L) & 0xffff;
        }

        for (size_t i = 0; i < linear_volume_line.size(); i++) {
            linear_volume_line[i] = i;
        }
    }

}
