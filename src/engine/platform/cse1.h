#ifndef CSE1_H_PLATFORM_CSE1
#define CSE1_H_PLATFORM_CSE1

#include "../dispatch.h"
#include "sound/cse1/cse1.hpp"

class DivPlatformCSE1 : public DivDispatch {
    struct Channel: SharedChannel {
        struct {
            DivInstrumentCSE1 instrument;
        } state{};
        Channel(bool linear=true):
          SharedChannel(0,linear) {}
    };
    Channel chan[CSE1_CHANNEL_NUMBER];
    DivDispatchOscBuffer* oscBuf[CSE1_CHANNEL_NUMBER];
    DivPitchTable pitchTable;
    DivPitchTableManager samplePitchTable;
    bool isMuted[CSE1_CHANNEL_NUMBER];
    unsigned char chans;
    CSE1_PACKED::CSE1 chip;
    CSE1_PACKED::WaveTable waveTable;
    DivMemoryComposition memCompo;
    CSE1_PACKED::CSE1_REG* pcmMem;
    unsigned int* sampleOff;
    bool* sampleLoaded;
    size_t sampleMemLen;
    int sysIDCache;

    friend void putDispatchChip(void*,int);
    friend void putDispatchChan(void*,int,int);
public:
    void acquire(short** buf, size_t len) override;
    int getOutputCount() override;
    void muteChannel(int ch, bool mute) override;
    int dispatch(DivCommand c) override;
    void notifyInsDeletion(void* ins) override;

    void forceIns() override;

    void notifyInsChange(int ins) override;

    void renderSamples(int sysID) override;

    void notifyPitchTable(int sample=-1) override;
    unsigned int getMaxFreq(int ch) override;
    SharedChannel* getChanState(int chan) override;
    DivDispatchOscBuffer* getOscBuffer(int chan) override;
    void setFlags(const DivConfig& flags) override;

    unsigned char *getRegisterPool() override;

    int getRegisterPoolSize() override;

    int getRegisterPoolDepth() override;

    const void* getSampleMem(int index) override;

    size_t getSampleMemCapacity(int index) override;

    size_t getSampleMemUsage(int index) override;

    bool isSampleLoaded(int index, int sample) override;

    const DivMemoryComposition *getMemCompo(int index) override;

    void reset() override;
    void tick(bool sysTick=true) override;
    int init(DivEngine* parent, int channels, int sugRate, const DivConfig& flags) override;
    void quit() override;
    ~DivPlatformCSE1() override;
};
#endif // CSE1_H_PLATFORM_CSE1
