#include "controls.h"

static bool scene_controls(enum Type5Context context)
{
    return context == CTX_MOUNT || context == CTX_BALLISTA;
}

uint32_t type5_map(Type5State *state, enum Type5Context context, uint32_t native,
                   bool accepts_sheathe)
{
    const uint32_t previous = state->previous_native;
    const enum Type5Context previous_context = state->previous_context;
    state->previous_native = native;
    state->previous_context = context;

    if (context == CTX_NATIVE) {
        state->blocked = 0;
        state->sheathe_requested = false;
        return native;
    }

    if (previous_context == CTX_NATIVE || (scene_controls(previous_context) && context != previous_context))
        state->blocked = previous & (BTN_R | BTN_ZL | BTN_ZR);
    state->blocked &= native;

    const uint32_t held = native & ~state->blocked;
    const uint32_t pressed = held & ~previous;
    uint32_t mapped = held & ~(BTN_ZL | BTN_ZR);

    const bool drawn = context == CTX_MELEE_DRAWN || context == CTX_BOWGUN_DRAWN ||
                       context == CTX_BOW_DRAWN;
    const uint32_t sheathe_conflicts = BTN_L | BTN_Y | BTN_ZR;
    if (!drawn || !(held & BTN_R) || (held & sheathe_conflicts)) {
        state->sheathe_requested = false;
    } else if ((pressed & BTN_R) && (!(held & BTN_B) || !accepts_sheathe)) {
        state->sheathe_requested = true;
    }

    if (context == CTX_MOUNT) {
        mapped &= ~BTN_R;
        if (held & BTN_ZR) mapped |= BTN_R;
        return mapped;
    }
    if (context == CTX_BALLISTA) {
        mapped &= ~(BTN_R | BTN_A);
        if (held & BTN_ZL) mapped |= BTN_R;
        if ((held & BTN_ZR) && !(held & BTN_L)) mapped |= BTN_A;
        return mapped;
    }

    if (!drawn) {
        if ((pressed & BTN_ZR) && !(held & BTN_L)) {
            mapped |= BTN_R | BTN_X | BTN_A;
            // A draw attack and a charge require separate presses of ZR.
            if (context == CTX_BOW_SHEATHED) state->blocked |= BTN_ZR;
        }
        return mapped;
    }

    mapped &= ~BTN_R;
    if (state->sheathe_requested && accepts_sheathe && !(held & BTN_B)) {
        state->sheathe_requested = false;
        mapped |= BTN_Y;
        // A held aim modifier must not interrupt the automatic Valor sheathe.
        if (context == CTX_BOW_DRAWN) state->blocked |= held & BTN_ZL;
        return mapped;
    }

    if (context == CTX_MELEE_DRAWN) {
        if (held & BTN_ZR) mapped |= BTN_R;
    } else if (context == CTX_BOWGUN_DRAWN) {
        if (held & BTN_ZL) mapped |= BTN_R;
        // Keep the original X+A stance commands and the item bar's A input.
        if (!(held & (BTN_L | BTN_X))) {
            mapped &= ~BTN_A;
            if (held & BTN_ZR) mapped |= BTN_A;
        }
    } else {
        if (held & BTN_ZL) mapped |= BTN_R;
        // Keep X+A to apply a coating and L+X/B to select one.
        if (!(held & BTN_L) && (held & (BTN_X | BTN_A)) != (BTN_X | BTN_A)) {
            mapped &= ~BTN_X;
            if (held & BTN_ZR) mapped |= BTN_X;
        }
    }
    return mapped;
}
