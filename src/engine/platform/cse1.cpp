#include "cse1.h"

#include "cse1_sync_utils.h"
#include "../engine.h"

#define CHIP_FREQBASE (1<<27)

#define MEMORY_SIZE (16*1024*1024)

void DivPlatformCSE1::acquire(short** buf, size_t len) {
    for (int i = 0; i < chans; i++) {
        oscBuf[i]->begin(len);
    }

    for (size_t i = 0; i < len; i++) {
        chip.clock(waveTable);
        int out[2] = {};

        for (unsigned char j = 0; j < chans; j++) {
            if (!isMuted[j]) {
              oscBuf[j]->putSample(i, chip.CSE1_GET_SAMPLE(j));

              auto& chip_channel = chip.CHANNELS.CHANNEL[j];

              chip_channel.OUT.PITCH = static_cast<CSE1_PACKED::CSE1_DOUBLE_REG>(chan[j].freq);

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
      int outLbuf = chan[i].vol < 255 ? chan[i].outL*chan[i].vol>>8 : chan[i].outL;
      int outRbuf = chan[i].vol < 255 ? chan[i].outR*chan[i].vol>>8 : chan[i].outR;
      chip.CHANNELS.CHANNEL[i].OUT.OUT_L = chan[i].outL2 < 0xffff ? outLbuf * chan[i].outL2>>16 : outLbuf;
      chip.CHANNELS.CHANNEL[i].OUT.OUT_R= chan[i].outR2 < 0xffff ? outRbuf * chan[i].outR2>>16 : outRbuf;
    }

    if (chan[i].freqChanged || chan[i].keyOn || chan[i].keyOff) {
      chan[i].freqChanged=false;
      chan[i].freq=chan[i].calcFreq();

      if (chan[i].keyOn) {
        chan[i].keyOn=false;
      }
      if (chan[i].keyOff) {
        for (auto& op : chip.CHANNELS.CHANNEL[i].OPS.OP)  {
          op.ENV_STATE.SET_ENV_ENUM(3);
        }
        chan[i].keyOff=false;
      }
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
      chan[c.chan].active=true;
      if (ins != nullptr) {
        switch (ins->type) {
          case DIV_INS_YMZ280B:
          case DIV_INS_ES5506:
          case DIV_INS_QSOUND:
          case DIV_INS_AMIGA: {
            auto& amiga = ins->amiga;

            if (c.value!=DIV_NOTE_NULL) {
              chan[c.chan].sample=amiga.getSample(c.value);
              chan[c.chan].pitchTable=samplePitchTable.get(chan[c.chan].sample);
              chan[c.chan].sampleNote=c.value;
              c.value=amiga.getFreq(c.value);
              chan[c.chan].sampleNoteDelta=c.value-chan[c.chan].sampleNote;
            } else if (chan[c.chan].sampleNote!=DIV_NOTE_NULL) {
              chan[c.chan].sample=amiga.getSample(chan[c.chan].sampleNote);
              chan[c.chan].pitchTable=samplePitchTable.get(chan[c.chan].sample);
              c.value=amiga.getFreq(chan[c.chan].sampleNote);
            }

            if (c.value!=DIV_NOTE_NULL) {
              chan[c.chan].baseFreq=chan[c.chan].calcBaseFreq(c.value);
            }

            if (c.value!=DIV_NOTE_NULL) {
              chan[c.chan].freqChanged=true;
              chan[c.chan].note=c.value;
            }

            chan[c.chan].keyOn=true;
            chan[c.chan].insChanged=false;

            auto cse1 = DivInstrumentCSE1();

            DivSample* s = parent->getSample(chan[c.chan].sample);
            if (s && s->samples > 0) {
              for (int op = 0; op < 2; op++) {
                if (s->loop) {
                  cse1.op[op].startP = sampleOff[chan[c.chan].sample] + s->getLoopStartPosition(DIV_SAMPLE_DEPTH_16BIT)/2;
                  cse1.op[op].endP = sampleOff[chan[c.chan].sample] + s->getLoopEndPosition(DIV_SAMPLE_DEPTH_16BIT)/2-1;
                  cse1.op[op].phase = sampleOff[chan[c.chan].sample];
                  cse1.op[op].wave = CSE1_PACKED::OPER_LOOP_SAMPLE;
                } else {
                  cse1.op[op].startP = sampleOff[chan[c.chan].sample];
                  cse1.op[op].endP = sampleOff[chan[c.chan].sample] + (s->getCurBufLen() + 1) / 2-1;
                  cse1.op[op].phase = cse1.op[op].startP;
                  cse1.op[op].wave = CSE1_PACKED::OPER_ONESHOT_SAMPLE;
                }
                cse1.op[op].adsr.ar = 0xffff;
                cse1.op[op].adsr.rr = 0xffff;
                cse1.op[op].env_divider = 0x1;
                cse1.out.inLeft[op] = 0xffff;
                cse1.out.inRight[op] = 0xffff;
                cse1.out.outLeft = 0xffff;
                cse1.out.outRight = 0xffff;
              }
            }
            chan[c.chan].outL = cse1.out.outLeft;
            chan[c.chan].outR = cse1.out.outRight;
            CSE1_REG_INS_SYNC::ins_to_reg(&cse1, &chip.CHANNELS.CHANNEL[c.chan]);
            break;
          }

          case DIV_INS_CSE1:
          default: {
            const auto& cse1 = ins->cse1;
            this->chan[c.chan].state.instrument = cse1;
            chan[c.chan].pitchTable = &pitchTable;

            CSE1_REG_INS_SYNC::ins_to_reg(&cse1, &chip.CHANNELS.CHANNEL[c.chan]);

            chan[c.chan].outL = cse1.out.outLeft;
            chan[c.chan].outR = cse1.out.outRight;
            if (c.value!=DIV_NOTE_NULL) {
              chan[c.chan].baseFreq=chan[c.chan].calcBaseFreq(c.value);
              chan[c.chan].freqChanged=true;
            }
            break;
          }
        }
      }
      break;
    }
    case DIV_CMD_NOTE_OFF: {
      chan[c.chan].active=false;
      chan[c.chan].sample=-1;
      chan[c.chan].active=false;
      chan[c.chan].keyOff=true;
      break;
    }
    case DIV_CMD_NOTE_OFF_ENV: {
      chan[c.chan].keyOff=true;
      break;
    }
    case DIV_CMD_INSTRUMENT:
      chan[c.chan].ins=c.value;
      break;
    case DIV_CMD_VOLUME:
      chan[c.chan].vol=c.value;
      if (chan[c.chan].vol>255) chan[c.chan].vol=255;
      break;
    case DIV_CMD_HINT_VOLUME:
      break;
    case DIV_CMD_PANNING:
      chan[c.chan].outL2=c.value;
      chan[c.chan].outR2=c.value2;
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
  for (int i=0; i<chans; i++) {
    chan[i].insChanged=true;
    chan[i].freqChanged=true;
    chan[i].sample=-1;
  }
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
  for (int i=0; i<chans; i++) {
    if (chan[i].ins==ins) {
      chan[i].insChanged=true;
    }
  }
}

const DivMemoryComposition *DivPlatformCSE1::getMemCompo(int index) {
  if (index!=0) return nullptr;
  return &memCompo;
}

const void *DivPlatformCSE1::getSampleMem(int index) {
  return index == 0 ? pcmMem : nullptr;
}

size_t DivPlatformCSE1::getSampleMemCapacity(int index) {
  return index == 0 ? MEMORY_SIZE : 0;
}

size_t DivPlatformCSE1::getSampleMemUsage(int index) {
  return index == 0 ? sampleMemLen : 0;
}

bool DivPlatformCSE1::isSampleLoaded(int index, int sample) {
  if (index!=0) return false;
  if (sample<0 || sample>32767) return false;
  return sampleLoaded[sample];
}

void DivPlatformCSE1::renderSamples(int sysID) {
    memset(pcmMem, 0, MEMORY_SIZE * sizeof(CSE1_PACKED::CSE1_REG));

    memset(sampleOff, 0, 32768 * sizeof(uint32_t));
    memset(sampleLoaded, 0, 32768 * sizeof(bool));

    memCompo = DivMemoryComposition();
    memCompo.name = "Sample RAM";

    size_t memPos = 0;
    for (int i = 0; i < parent->song.sampleLen; i++) {
        DivSample* s = parent->song.sample[i];

        if (!s->renderOn[0][sysID]) {
            sampleOff[i] = 0;
            continue;
        }

        const uint32_t length = s->getCurBufLen();
        const uint32_t wordLength = (length + 1) / 2;
        const auto* src = static_cast<unsigned char*>(s->getCurBuf());

        const uint32_t actualWordLength = MIN((getSampleMemCapacity(0) - memPos), wordLength);

        if (actualWordLength > 0) {

          for (size_t j = 0; j < actualWordLength; j++) {
            const uint16_t sample = (src[j * 2 + 1] << 8) | src[j * 2];
            pcmMem[memPos + j] = sample + 32768;
          }
          sampleOff[i] = memPos;
          memCompo.entries.push_back(DivMemoryEntry(
          DIV_MEMORY_SAMPLE, "Sample", i, memPos, memPos + actualWordLength
          ));
          memPos += actualWordLength;
        }

        if (actualWordLength < wordLength) {
            logW("out of CSE-1 PCM memory for sample %d!", i);
            break;
        }

        sampleLoaded[i] = true;
    }

    sysIDCache = sysID;

    sampleMemLen = memPos;
    memCompo.used = sampleMemLen;
    memCompo.capacity = getSampleMemCapacity(0);
}
void DivPlatformCSE1::reset() {
  chip.hard_reset();
  for (int i=0; i<chans; i++) {
    chan[i]=Channel(parent->song.compatFlags.linearPitch);
    chan[i].pitchTable=&pitchTable;
    chan[i].pitchTable=samplePitchTable.get(-1);
    chan[i].vol=255;
  }
}

void DivPlatformCSE1::notifyPitchTable(int sample) {
  pitchTable.init(
    parent->song.tuning,chipClock,
    CHIP_FREQBASE,
    0x7fffffff,
    false,
    parent->song.compatFlags.linearPitch);
  samplePitchTable.update<Channel>(
    chan,chans,parent->song.tuning,chipClock,
    1<<16,
    0x7fffffff,
    false,
    parent->song.compatFlags.linearPitch,
    sample);
}

unsigned int DivPlatformCSE1::getMaxFreq(int ch) {
  return 0xffffffff;
}

void DivPlatformCSE1::setFlags(const DivConfig& flags) {
  chipClock=192000;
  CHECK_CUSTOM_CLOCK
  chipClock = flags.getBool("quarterClock",false) ? chipClock / 4 : chipClock;
  rate=chipClock;
  for (int i = 0; i < chans; i++) {
    oscBuf[i]->setRate(rate);
  }
  switch (flags.getInt("defaultVolumeTableType",1)) {
    case 0:
      waveTable.default_volume_line = waveTable.old_js_expw.data();
      break;
    case 1:
      waveTable.default_volume_line = waveTable.real_volume_line.data();
      break;
    case 2:
      waveTable.default_volume_line = waveTable.linear_volume_line.data();
      break;
    case 3:
      waveTable.default_volume_line = waveTable.exp_volume_line.data();
      break;
    default:
      waveTable.default_volume_line = waveTable.old_js_expw.data();
      break;
  }
  waveTable.fastSpeed = flags.getInt("defaultVolumeTableSpeed",0x0100);;
  waveTable.reset();
  notifyPitchTable();
}

int DivPlatformCSE1::init(DivEngine* p, int channels, int sugRate, const DivConfig& flags) {
  parent=p;
  samplePitchTable.init(parent);
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

  sampleOff=new unsigned int[32768];
  sampleLoaded=new bool[32768];
  pcmMem=new CSE1_PACKED::CSE1_REG[getSampleMemCapacity(0)];
  sampleMemLen=0;
  waveTable.reset();

  waveTable.memPCM=pcmMem;

  setFlags(flags);
  reset();

  return channels;
}

void DivPlatformCSE1::quit() {
  for (int i=0; i<chans; i++) {
    delete oscBuf[i];
  }
  delete[] pcmMem;
  waveTable.memPCM = nullptr;
  samplePitchTable.destroy<Channel>(chan,CSE1_CHANNEL_NUMBER);
}

DivPlatformCSE1::~DivPlatformCSE1() = default;
