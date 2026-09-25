//
// Created by Administrator on 2026/9/20.
//

#include "cse1_sync_utils.h"

namespace CSE1_REG_INS_SYNC {
    void ins_to_reg(const DivInstrumentCSE1 *ins, CSE1_PACKED::CSE1_CHANNEL_REGISTERS *reg) {
        for (size_t a = 0; a < CSE1_OPER_NUMBER; a++) {
            for (size_t b = 0; b < CSE1_OPER_NUMBER; b++) {
                reg->OPS.OP[a].MIS.MI[b] = ins->op[a].mi[b];
            }
            auto& op = reg->OPS.OP[a];
            op.PHASE = ins->op[a].phase;
            op.FLAGS_A.SET_WAVE(ins->op[a].wave);
            op.DUTY = ins->op[a].duty;

            op.ADSR.AR = ins->op[a].adsr.ar;
            op.ADSR.DR = ins->op[a].adsr.dr;
            op.ADSR.SL = ins->op[a].adsr.sl;
            op.ADSR.SR = ins->op[a].adsr.sr;
            op.ADSR.RR = ins->op[a].adsr.rr;
            op.FLAGS_B.SET_ENV_DIVIDER(ins->op[a].env_divider);
            op.ENV_STATE.SET_ENV_ENUM(ins->op[a].adsr.adsrState);

            op.FLAGS_A.SET_ML(ins->op[a].ml);
            op.FLAGS_A.SET_OPN2_DETUNE(ins->op[a].dn);
            op.PITCH = (ins->op[a].hpitch<<16)+ins->op[a].lpitch;
            op.FLAGS_A.SET_FIXED(ins->op[a].fixed);

            op.FLAGS_A.SET_AM1(ins->op[a].am1);
            op.FLAGS_A.SET_AM2(ins->op[a].am2);
            op.FLAGS_A.SET_FM1(ins->op[a].fm1);
            op.FLAGS_A.SET_FM2(ins->op[a].fm2);
        }
        for (size_t o = 0; o < CSE1_OPER_NUMBER; o++) {
            reg->OUT.IN_L.MI[o] = ins->out.inLeft[o];
            reg->OUT.IN_R.MI[o] = ins->out.inRight[o];
        }

        reg->OUT.OUT_L = ins->out.outLeft;
        reg->OUT.OUT_R = ins->out.outRight;

        reg->SPEC.LFO_CONFIG.SET_FREQ(ins->special.lfo1.freq);
        reg->SPEC.LFO_CONFIG.SET_DEPTH(ins->special.lfo1.depth);
        reg->SPEC.LFO_CONFIG.SET_SHAPE(ins->special.lfo1.wave);

        reg->SPEC.LFO_CONFIG.SET_FREQ2(ins->special.lfo2.freq);
        reg->SPEC.LFO_CONFIG.SET_DEPTH2(ins->special.lfo2.depth);
        reg->SPEC.LFO_CONFIG.SET_SHAPE2(ins->special.lfo2.wave);
    }

    void reg_to_ins(const CSE1_PACKED::CSE1_CHANNEL_REGISTERS *reg, DivInstrumentCSE1 *ins) {
        for (size_t a = 0; a < CSE1_OPER_NUMBER; a++) {
            for (size_t b = 0; b < CSE1_OPER_NUMBER; b++) {
                ins->op[a].mi[b] = reg->OPS.OP[a].MIS.MI[b];
            }
            auto& op = reg->OPS.OP[a];
            ins->op[a].phase = op.PHASE;
            ins->op[a].wave = op.FLAGS_A.GET_WAVE();
            ins->op[a].duty = op.DUTY;

            ins->op[a].adsr.ar = op.ADSR.AR;
            ins->op[a].adsr.dr = op.ADSR.DR;
            ins->op[a].adsr.sl = op.ADSR.SL;
            ins->op[a].adsr.sr = op.ADSR.SR;
            ins->op[a].adsr.rr = op.ADSR.RR;
            ins->op[a].env_divider = op.FLAGS_B.GET_ENV_DIVIDER();
            ins->op[a].adsr.adsrState = op.ENV_STATE.GET_ENV_ENUM();

            ins->op[a].ml = op.FLAGS_A.GET_ML();
            ins->op[a].dn = op.FLAGS_A.GET_OPN2_DETUNE();
            ins->op[a].hpitch = op.PITCH>>16&0xffff;
            ins->op[a].lpitch = op.PITCH&0xffff;
            ins->op[a].fixed = op.FLAGS_A.GET_FIXED();

            ins->op[a].am1 = op.FLAGS_A.GET_AM1();
            ins->op[a].am2 = op.FLAGS_A.GET_AM2();
            ins->op[a].fm1 = op.FLAGS_A.GET_FM1();
            ins->op[a].fm2 = op.FLAGS_A.GET_FM2();
        }
        for (size_t o = 0; o < CSE1_OPER_NUMBER; o++) {
            ins->out.inLeft[o] = reg->OUT.IN_L.MI[o];
            ins->out.inRight[o] = reg->OUT.IN_R.MI[o];
        }

        ins->out.outLeft = reg->OUT.OUT_L;
        ins->out.outRight = reg->OUT.OUT_R;

        ins->special.lfo1.freq = reg->SPEC.LFO_CONFIG.GET_FREQ();
        ins->special.lfo1.depth = reg->SPEC.LFO_CONFIG.GET_DEPTH();
        ins->special.lfo1.wave = reg->SPEC.LFO_CONFIG.GET_SHAPE();

        ins->special.lfo2.freq = reg->SPEC.LFO_CONFIG.GET_FREQ2();
        ins->special.lfo2.depth = reg->SPEC.LFO_CONFIG.GET_DEPTH2();
        ins->special.lfo2.wave = reg->SPEC.LFO_CONFIG.GET_SHAPE2();
    }
}