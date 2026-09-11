//
// Created by Administrator on 2026/9/12.
//

#include "../dispatch.h"
#include "sound/cse1/cse1.h"

class DivPlatformCSE1 : public DivDispatch {
    struct Channel: public SharedChannel {
        DivInstrumentCSE1 state;


    };
};