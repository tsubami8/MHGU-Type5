#include "controls.h"

enum Type5Context type5_weapon_context(unsigned weapon, bool drawn, bool bow_ready,
                                       unsigned group, unsigned action)
{
    enum Type5Context context;
    switch (weapon) {
    case 0:  // Great Sword
    case 1:  // Sword and Shield
    case 2:  // Hammer
    case 3:  // Lance
    case 7:  // Long Sword
    case 8:  // Switch Axe
    case 9:  // Gunlance
    case 11: // Dual Blades
    case 12: // Hunting Horn
    case 13: // Insect Glaive
    case 14: // Charge Blade
        context = drawn ? CTX_MELEE_DRAWN : CTX_MELEE_SHEATHED;
        break;
    case 4:  // Heavy Bowgun
    case 6:  // Light Bowgun
        context = drawn ? CTX_BOWGUN_DRAWN : CTX_BOWGUN_SHEATHED;
        break;
    case 10: // Bow
        if (!bow_ready) return CTX_NATIVE;
        context = drawn ? CTX_BOW_DRAWN : CTX_BOW_SHEATHED;
        break;
    default:
        return CTX_NATIVE;
    }

    if (!drawn && group == 1 && action >= 85 && action <= 87) return CTX_DRINK;
    // The native group contains actual ledge jumps (action 0) and falls (action 1).
    // The game's weapon command tables choose whether the original chord has a move.
    if (!drawn && group == 4) return CTX_AIR_SHEATHED;
    if (group == 1 || group == 2 || group == 0x10 || group == 0x20 || group == 0x8010)
        return context;
    return CTX_FIELD_RECOVERY;
}

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
        state->draw_after_drink = false;
        return native;
    }

    if (previous_context == CTX_NATIVE || (scene_controls(previous_context) && context != previous_context))
        state->blocked = previous & (BTN_R | BTN_ZL | BTN_ZR);
    state->blocked &= native;

    const uint32_t held = native & ~state->blocked;
    const uint32_t pressed = held & ~previous;
    uint32_t mapped = held & ~(BTN_ZL | BTN_ZR);

    if (context == CTX_DRINK) {
        state->sheathe_requested = false;
        if (held & BTN_L) state->draw_after_drink = false;
        else if (pressed & BTN_ZR) state->draw_after_drink = true;
        return native & ~BTN_ZR;
    }
    const bool buffered_draw = state->draw_after_drink && previous_context == CTX_DRINK;
    state->draw_after_drink = false;
    if (context == CTX_FIELD_RECOVERY) {
        state->sheathe_requested = false;
        // Stay in the field input stream so a held R can carry through recovery.
        state->blocked |= held & (BTN_ZL | BTN_ZR);
        return native;
    }

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
        if (((pressed & BTN_ZR) || (buffered_draw && accepts_sheathe)) &&
            !(held & (BTN_L | BTN_Y))) {
            mapped |= BTN_R | BTN_X | BTN_A;
            // A buffered, aerial, or bow draw consumes the press that requested it.
            if (buffered_draw || context == CTX_BOW_SHEATHED || context == CTX_AIR_SHEATHED)
                state->blocked |= BTN_ZR;
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
