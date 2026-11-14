//
// Created by Administrator on 2026/11/14.
//

#ifndef FURNACE_CTASIS_H
#define FURNACE_CTASIS_H
#include "../dispatch.h"
#include "sound/ctasis/ctasis.h"

#define MAX_CHANNEL 16

class DivPlatformTasis: public DivDispatch {
    struct Channel: public SharedChannel {
        explicit Channel(bool linear=true) : SharedChannel(0, linear) {
        }
    };

    Channel chan[MAX_CHANNEL];
    DivDispatchOscBuffer* oscBuf[MAX_CHANNEL];
    DivPitchTable pitchTable;
    DivPitchTableManager samplePitchTable;
    bool isMuted[MAX_CHANNEL];
    unsigned char chans;

    public:
    DivPlatformTasis();
    ~DivPlatformTasis() override;

    void reset() override;
    void tick(bool sysTick=true) override;
    int init(DivEngine* parent, int channels, int sugRate, const DivConfig& flags) override;
    void quit() override;
};

#endif //FURNACE_CTASIS_H
