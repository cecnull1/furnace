#include "cse1.h"

#include "cse1_sync_utils.h"
#include "../engine.h"

#define CHIP_FREQBASE (1<<27)

void DivPlatformCSE1::acquire(short** buf, size_t len) {
    for (int i = 0; i < chans; i++) {
        oscBuf[i]->begin(len);
    }

    for (size_t i = 0; i < len; i++) {
        int out[2] = {};

        for (unsigned char j = 0; j < chans; j++) {
            if (!isMuted[j]) {
              oscBuf[j]->putSample(i, chip.CSE1_GET_SAMPLE(j));

              auto& chip_channel = chip.CHANNELS.CHANNEL[j];

              auto& ins_chan = this->chan[j].state.instrument;

              chip_channel.OUT.PITCH = static_cast<CSE1_PACKED::CSE1_DOUBLE_REG>(chan[j].calcFreq());

              out[0] += chip.CHANNELS.OUTS_L[j];
              out[1] += chip.CHANNELS.OUTS_R[j];
            } else {
              oscBuf[j]->putSample(i, 0);
            }
        }

        if (out[0] < -32768) out[0] = -32768;
        if (out[0] > 32767) out[0] = 32767;
        buf[0][i] = out[0];

        if (out[1] < -32768) out[1] = -32768;
        if (out[1] > 32767) out[1] = 32767;
        buf[1][i] = out[1];

        chip.clock(waveTable);
    }

    for (int i = 0; i < chans; i++) {
        oscBuf[i]->end(len);
    }
}

int DivPlatformCSE1::getOutputCount() {
    return 2;
}


void DivPlatformCSE1::muteChannel(int ch, bool mute) {
  isMuted[ch]=mute;
}

void DivPlatformCSE1::tick(bool sysTick) {
  for (unsigned char i=0; i<chans; i++) {
    if (sysTick) {
      // chan[i].amp-=7;
      // if (chan[i].noise) {
      //   if (chan[i].amp<0) chan[i].amp=0;
      // } else {
      //   if (chan[i].amp<15) chan[i].amp=15;
      // }
    }

    if (chan[i].freqChanged) {
      chan[i].freqChanged=false;
      chan[i].freq=chan[i].calcFreq();
    }
  }
}

SharedChannel* DivPlatformCSE1::getChanState(int ch) {
  return &chan[ch];
}

DivDispatchOscBuffer* DivPlatformCSE1::getOscBuffer(int ch) {
  return oscBuf[ch];
}

int DivPlatformCSE1::dispatch(DivCommand c) {
  switch (c.cmd) {
    case DIV_CMD_NOTE_ON: {
      DivInstrument* ins=parent->getIns(chan[c.chan].ins,DIV_INS_CSE1);
      if (c.value!=DIV_NOTE_NULL) {
        chan[c.chan].baseFreq=chan[c.chan].calcBaseFreq(c.value);
        chan[c.chan].freqChanged=true;
      }
      //chan[c.chan].noise=(ins->std.dutyMacro.len>0 && ins->std.dutyMacro.val[0]==1);
      chan[c.chan].active=true;
      //chan[c.chan].amp=64;
      if (ins != nullptr) {
        const auto& cse1 = ins->cse1;
        this->chan[c.chan].state.instrument = cse1;
        CSE1_REG_INS_SYNC::ins_to_reg(&cse1, &chip.CHANNELS.CHANNEL[c.chan]);
      }
      break;
    }
    case DIV_CMD_NOTE_OFF:
      chan[c.chan].active=false;
      break;
    case DIV_CMD_INSTRUMENT:
      chan[c.chan].ins=c.value;
      break;
    case DIV_CMD_VOLUME:
      chan[c.chan].vol=c.value;
      if (chan[c.chan].vol>255) chan[c.chan].vol=255;
      break;
    case DIV_CMD_GET_VOLUME:
      return chan[c.chan].vol;
      break;
    case DIV_CMD_PITCH:
      chan[c.chan].pitch=c.value;
      chan[c.chan].freqChanged=true;
      break;
    case DIV_CMD_NOTE_PORTA: {
      int destFreq=chan[c.chan].calcBaseFreq(c.value2);
      bool return2=false;
      if (destFreq>chan[c.chan].baseFreq) {
        chan[c.chan].baseFreq+=c.value;
        if (chan[c.chan].baseFreq>=destFreq) {
          chan[c.chan].baseFreq=destFreq;
          return2=true;
        }
      } else {
        chan[c.chan].baseFreq-=c.value;
        if (chan[c.chan].baseFreq<=destFreq) {
          chan[c.chan].baseFreq=destFreq;
          return2=true;
        }
      }
      chan[c.chan].freqChanged=true;
      if (return2) return 2;
      break;
    }
    case DIV_CMD_LEGATO:
      chan[c.chan].baseFreq=chan[c.chan].calcBaseFreq(c.value);
      chan[c.chan].freqChanged=true;
      break;
    case DIV_CMD_GET_VOLMAX:
      return 255;
      break;
    default:
      break;
  }
  return 1;
}

void DivPlatformCSE1::notifyInsDeletion(void* ins) {
  // nothing
}

void DivPlatformCSE1::forceIns() {

}

unsigned char *DivPlatformCSE1::getRegisterPool() {
  return reinterpret_cast<uint8_t*>(&chip.CHANNELS.CHANNEL);
}

int DivPlatformCSE1::getRegisterPoolDepth() {
  return 16;
}

int DivPlatformCSE1::getRegisterPoolSize() {
  return sizeof(CSE1_PACKED::CSE1_CHANNEL_REGISTERS) * CSE1_CHANNEL_NUMBER / 2;
}

void DivPlatformCSE1::notifyInsChange(int ins) {

}

void DivPlatformCSE1::renderSamples(int sysID) {

}

void DivPlatformCSE1::reset() {
  chip.hard_reset();
  for (int i=0; i<chans; i++) {
    chan[i]=Channel(parent->song.compatFlags.linearPitch);
    chan[i].pitchTable=&pitchTable;
    chan[i].vol=255;
  }
}

void DivPlatformCSE1::notifyPitchTable(int sample) {
  pitchTable.init(parent->song.tuning,chipClock,CHIP_FREQBASE,0x7fffffff,false,parent->song.compatFlags.linearPitch);
}

unsigned int DivPlatformCSE1::getMaxFreq(int ch) {
  return 0xffffffff;
}

void DivPlatformCSE1::setFlags(const DivConfig& flags) {
  CHECK_CUSTOM_CLOCK else {
    chipClock=192000;
  }
  rate=chipClock;
  for (int i = 0; i < chans; i++) {
    oscBuf[i]->setRate(rate);
  }
  notifyPitchTable();
}

int DivPlatformCSE1::init(DivEngine* p, int channels, int sugRate, const DivConfig& flags) {
  parent=p;
  dumpWrites=false;
  skipRegisterWrites=false;
  for (int i=0; i<DIV_MAX_CHANS; i++) {
    isMuted[i]=false;
    if (i<channels) {
      oscBuf[i]=new DivDispatchOscBuffer;
      oscBuf[i]->setRate(192000);
    }
  }
  rate=192000;
  chipClock=192000;
  notifyPitchTable();
  chans=channels;
  setFlags(flags);
  reset();
  return channels;
}

void DivPlatformCSE1::quit() {
  for (int i=0; i<chans; i++) {
    delete oscBuf[i];
  }
}

DivPlatformCSE1::~DivPlatformCSE1() = default;
