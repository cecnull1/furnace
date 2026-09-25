#include "cse1.h"

#include "cse1_sync_utils.h"
#include "../engine.h"

#define CHIP_FREQBASE (1<<27)

#define MEMORY_SIZE (16*1024*1024/2)

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
        this->chip.CHANNELS.CHANNEL[c.chan].OUT.OUT_L = chan[c.chan].vol*0x0101;
        this->chip.CHANNELS.CHANNEL[c.chan].OUT.OUT_R = chan[c.chan].vol*0x0101;
      }
      break;
    }
    case DIV_CMD_NOTE_OFF:
      chan[c.chan].active=false;
      for (auto& op : chip.CHANNELS.CHANNEL[c.chan].OPS.OP)  {
        op.ENV_STATE.SET_ENV_ENUM(3);
      }
      break;
    case DIV_CMD_INSTRUMENT:
      chan[c.chan].ins=c.value;
      break;
    case DIV_CMD_VOLUME:
      chan[c.chan].vol=c.value;
      if (chan[c.chan].vol>255) chan[c.chan].vol=255;
      this->chip.CHANNELS.CHANNEL[c.chan].OUT.OUT_L = chan[c.chan].vol*0x0101;
      this->chip.CHANNELS.CHANNEL[c.chan].OUT.OUT_R = chan[c.chan].vol*0x0101;
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
    // case DIV_CMD_ENV_RELEASE:
    //   for (auto& op : chip.CHANNELS.CHANNEL[c.chan].OPS.OP)  {
    //     op.ENV_STATE.SET_ENV_ENUM(3);
    //   }
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

    memset(sampleOff, 0, 32768 * sizeof(unsigned int));
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

        int length = s->getCurBufLen();
        int wordLength = (length + 1) / 2;
        auto* src = static_cast<unsigned char*>(s->getCurBuf());

        // 妫€鏌モ€滃唴瀛樷€?
        int actualWordLength = MIN((int)(getSampleMemCapacity(0) - memPos), wordLength);

        if (actualWordLength > 0) {
          if (s->depth == DIV_SAMPLE_DEPTH_16BIT) {
            for (int j = 0; j < actualWordLength; j++) {
              int16_t sample = (src[j * 2 + 1] << 8) | src[j * 2];
              pcmMem[memPos + j] = sample + 32768;
            }
          } else {
            // 8-bit 閲囨牱锛氬亸绉昏浆鎹?
            for (int j = 0; j < actualWordLength; j++) {
              int8_t sample = src[j];
              pcmMem[memPos + j] = (sample + 128) * 256;
            }
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
  chipClock=192000;
  CHECK_CUSTOM_CLOCK
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
