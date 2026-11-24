#ifndef TYPE5_CONTROLS_H
#define TYPE5_CONTROLS_H

#include <stdbool.h>
#include <stdint.h>

// Masks in the pad word before the game derives pressed/held/released.
enum Type5Button {
    BTN_L3 = 0x2, BTN_R3 = 0x4, BTN_L = 0x100, BTN_R = 0x200, BTN_ZL = 0x400,
    BTN_ZR = 0x800, BTN_X = 0x1000, BTN_A = 0x2000, BTN_B = 0x4000,
    BTN_Y = 0x8000,
};

enum Type5Context {
    CTX_NATIVE, CTX_MELEE_SHEATHED, CTX_MELEE_DRAWN,
    CTX_BOWGUN_SHEATHED, CTX_BOWGUN_DRAWN, CTX_BOW_SHEATHED, CTX_BOW_DRAWN,
    CTX_MOUNT, CTX_BALLISTA,
};

typedef struct {
    uint32_t previous_native;
    enum Type5Context previous_context;
    uint32_t blocked;
    bool sheathe_requested;
} Type5State;

uint32_t type5_map(Type5State *state, enum Type5Context context, uint32_t native,
                   bool accepts_sheathe);

#endif
