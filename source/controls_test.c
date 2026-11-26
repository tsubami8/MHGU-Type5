#include "controls.h"

#define CHECK(condition) do { if (!(condition)) return __LINE__; } while (0)
#define MAP(context, keys, legal) type5_map(&state, context, keys, legal)

int type5_test(void)
{
    Type5State state = {0};
    CHECK(MAP(CTX_NATIVE, BTN_ZR, true) == BTN_ZR);
    CHECK(MAP(CTX_MELEE_SHEATHED, BTN_ZR, true) == 0);
    CHECK(MAP(CTX_MELEE_SHEATHED, 0, true) == 0);
    CHECK(MAP(CTX_MELEE_SHEATHED, BTN_ZR, true) == (BTN_R | BTN_X | BTN_A));
    CHECK(MAP(CTX_MELEE_DRAWN, BTN_ZR, true) == BTN_R);
    CHECK(MAP(CTX_MELEE_DRAWN, 0, true) == 0);
    CHECK(MAP(CTX_MELEE_DRAWN, BTN_R, true) == BTN_Y);
    CHECK(MAP(CTX_MELEE_DRAWN, BTN_R, true) == 0);
    CHECK(MAP(CTX_MELEE_SHEATHED, BTN_R, true) == BTN_R);
    CHECK(MAP(CTX_MELEE_SHEATHED, BTN_R3, true) == BTN_R3);
    CHECK(MAP(CTX_MELEE_DRAWN, BTN_L3 | BTN_R3, true) == (BTN_L3 | BTN_R3));
    CHECK(MAP(CTX_MELEE_DRAWN, BTN_L | BTN_R, true) == BTN_L);
    CHECK(MAP(CTX_MELEE_DRAWN, BTN_Y, true) == BTN_Y);
    CHECK(MAP(CTX_MELEE_DRAWN, BTN_R, false) == 0);
    CHECK(MAP(CTX_NATIVE, BTN_R | BTN_ZL, true) == (BTN_R | BTN_ZL));

    state = (Type5State){0};
    CHECK(MAP(CTX_MELEE_DRAWN, BTN_R, false) == 0);
    CHECK(MAP(CTX_MELEE_DRAWN, BTN_R, false) == 0);
    CHECK(MAP(CTX_MELEE_DRAWN, BTN_R, true) == BTN_Y);
    CHECK(MAP(CTX_MELEE_DRAWN, BTN_R, true) == 0);
    CHECK(MAP(CTX_MELEE_SHEATHED, BTN_R, true) == BTN_R);
    CHECK(MAP(CTX_MELEE_DRAWN, 0, false) == 0);
    CHECK(MAP(CTX_MELEE_DRAWN, BTN_R, false) == 0);
    CHECK(MAP(CTX_MELEE_DRAWN, 0, false) == 0);
    CHECK(MAP(CTX_MELEE_DRAWN, 0, true) == 0);
    CHECK(MAP(CTX_MELEE_DRAWN, BTN_R, false) == 0);
    CHECK(MAP(CTX_MELEE_DRAWN, BTN_R | BTN_Y, true) == BTN_Y);
    CHECK(MAP(CTX_MELEE_DRAWN, BTN_R, true) == 0);

    state = (Type5State){0};
    CHECK(MAP(CTX_BOWGUN_DRAWN, BTN_B, false) == BTN_B);
    CHECK(MAP(CTX_BOWGUN_DRAWN, BTN_B | BTN_R, false) == BTN_B);
    CHECK(MAP(CTX_BOWGUN_DRAWN, BTN_B | BTN_R, true) == BTN_B);
    CHECK(MAP(CTX_BOWGUN_DRAWN, BTN_R, true) == BTN_Y);
    CHECK(MAP(CTX_BOWGUN_DRAWN, BTN_R, true) == 0);

    state = (Type5State){0};
    CHECK(MAP(CTX_BOWGUN_DRAWN, BTN_ZL | BTN_ZR, true) == (BTN_R | BTN_A));
    CHECK(MAP(CTX_BOWGUN_DRAWN, BTN_ZL | BTN_R3, true) == (BTN_R | BTN_R3));
    CHECK(MAP(CTX_BOWGUN_DRAWN, BTN_A, true) == 0);
    CHECK(MAP(CTX_BOWGUN_DRAWN, BTN_X | BTN_A, true) == (BTN_X | BTN_A));
    CHECK(MAP(CTX_BOWGUN_DRAWN, BTN_X | BTN_ZR, true) == BTN_X);
    CHECK(MAP(CTX_BOWGUN_DRAWN, BTN_L | BTN_B | BTN_ZR, true) == (BTN_L | BTN_B));
    CHECK(MAP(CTX_BOWGUN_DRAWN, BTN_ZL | BTN_R, true) == BTN_Y);

    state = (Type5State){0};
    CHECK(MAP(CTX_BOW_DRAWN, BTN_ZL, true) == BTN_R);
    CHECK(MAP(CTX_BOW_DRAWN, 0, true) == 0);
    CHECK(MAP(CTX_BOW_DRAWN, BTN_ZL | BTN_ZR, true) == (BTN_R | BTN_X));
    CHECK(MAP(CTX_BOW_DRAWN, BTN_ZR, true) == BTN_X);
    CHECK(MAP(CTX_BOW_DRAWN, 0, true) == 0);
    CHECK(MAP(CTX_BOW_DRAWN, BTN_X | BTN_A, true) == (BTN_X | BTN_A));
    CHECK(MAP(CTX_BOW_DRAWN, BTN_L | BTN_X, true) == (BTN_L | BTN_X));
    CHECK(MAP(CTX_BOW_DRAWN, BTN_L | BTN_B, true) == (BTN_L | BTN_B));
    CHECK(MAP(CTX_BOW_DRAWN, BTN_L | BTN_ZR, true) == BTN_L);
    CHECK(MAP(CTX_BOW_DRAWN, BTN_R, false) == 0);
    CHECK(MAP(CTX_BOW_DRAWN, BTN_R, true) == BTN_Y);
    CHECK(MAP(CTX_BOW_SHEATHED, BTN_R, true) == BTN_R);
    CHECK(MAP(CTX_BOW_SHEATHED, BTN_L | BTN_X, true) == (BTN_L | BTN_X));
    CHECK(MAP(CTX_BOW_SHEATHED, BTN_L | BTN_B, true) == (BTN_L | BTN_B));
    CHECK(MAP(CTX_BOW_SHEATHED, BTN_ZR, true) == (BTN_R | BTN_X | BTN_A));
    CHECK(MAP(CTX_BOW_DRAWN, BTN_ZR, true) == 0);
    CHECK(MAP(CTX_BOW_DRAWN, 0, true) == 0);
    CHECK(MAP(CTX_BOW_DRAWN, BTN_ZR, true) == BTN_X);
    CHECK(MAP(CTX_BOW_DRAWN, BTN_ZR | BTN_A, true) == (BTN_X | BTN_A));
    CHECK(MAP(CTX_BOW_DRAWN, 0, true) == 0);
    CHECK(MAP(CTX_BOW_DRAWN, BTN_Y | BTN_ZR, true) == (BTN_Y | BTN_X));
    CHECK(MAP(CTX_BOW_DRAWN, BTN_ZL, true) == BTN_R);
    CHECK(MAP(CTX_BOW_DRAWN, BTN_ZL | BTN_R, true) == BTN_Y);
    CHECK(MAP(CTX_BOW_DRAWN, BTN_ZL | BTN_R, false) == 0);
    CHECK(MAP(CTX_BOW_SHEATHED, BTN_ZL | BTN_R, false) == BTN_R);
    CHECK(MAP(CTX_BOW_SHEATHED, 0, false) == 0);

    state = (Type5State){0};
    CHECK(MAP(CTX_MOUNT, BTN_ZR | BTN_R3 | BTN_X | BTN_A, false) == (BTN_R | BTN_R3 | BTN_X | BTN_A));
    CHECK(MAP(CTX_MELEE_DRAWN, BTN_ZR, true) == 0);
    CHECK(MAP(CTX_MELEE_DRAWN, 0, true) == 0);
    CHECK(MAP(CTX_MELEE_DRAWN, BTN_ZR, true) == BTN_R);
    CHECK(MAP(CTX_BALLISTA, BTN_ZL | BTN_ZR, false) == (BTN_R | BTN_A));
    CHECK(MAP(CTX_BALLISTA, BTN_L | BTN_B | BTN_ZR, false) == (BTN_L | BTN_B));
    CHECK(MAP(CTX_BALLISTA, BTN_B, false) == BTN_B);

    state = (Type5State){0};
    CHECK(MAP(CTX_MELEE_DRAWN, 0, true) == 0);
    CHECK(MAP(CTX_FIELD_RECOVERY, BTN_R, false) == BTN_R);
    CHECK(MAP(CTX_MELEE_SHEATHED, BTN_R, true) == BTN_R);
    CHECK(MAP(CTX_DRINK, BTN_ZR, false) == 0);
    CHECK(MAP(CTX_DRINK, 0, false) == 0);
    CHECK(MAP(CTX_MELEE_SHEATHED, 0, true) == (BTN_R | BTN_X | BTN_A));
    CHECK(MAP(CTX_MELEE_SHEATHED, 0, true) == 0);
    CHECK(MAP(CTX_DRINK, BTN_ZR, false) == 0);
    CHECK(MAP(CTX_MELEE_SHEATHED, BTN_ZR, true) == (BTN_R | BTN_X | BTN_A));
    CHECK(MAP(CTX_MELEE_DRAWN, BTN_ZR, true) == 0);
    CHECK(MAP(CTX_MELEE_DRAWN, 0, true) == 0);
    CHECK(MAP(CTX_DRINK, BTN_ZR, false) == 0);
    CHECK(MAP(CTX_FIELD_RECOVERY, BTN_ZR, false) == BTN_ZR);
    CHECK(MAP(CTX_MELEE_SHEATHED, BTN_ZR, true) == 0);
    CHECK(MAP(CTX_MELEE_SHEATHED, 0, true) == 0);
    CHECK(MAP(CTX_AIR_SHEATHED, BTN_ZR, false) == (BTN_R | BTN_X | BTN_A));
    CHECK(MAP(CTX_AIR_SHEATHED, BTN_ZR, false) == 0);

    const unsigned weapons[] = {0, 1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    for (unsigned i = 0; i < sizeof weapons / sizeof weapons[0]; ++i) {
        const unsigned weapon = weapons[i];
        const enum Type5Context ground = type5_weapon_context(weapon, false, true, 2, 2);
        const enum Type5Context ledge = type5_weapon_context(weapon, false, true, 4, 0);
        CHECK(ledge == CTX_AIR_SHEATHED);
        CHECK(type5_weapon_context(weapon, false, true, 4, 1) == ledge);
        CHECK(type5_weapon_context(weapon, true, true, 4, 0) == CTX_FIELD_RECOVERY);
        state = (Type5State){0};
        CHECK(MAP(ground, BTN_R, false) == BTN_R);
        CHECK(MAP(ledge, BTN_R | BTN_ZR, false) == (BTN_R | BTN_X | BTN_A));
        CHECK(MAP(ledge, BTN_R | BTN_ZR, false) == BTN_R);
        CHECK(MAP(ground, BTN_R | BTN_ZR, false) == BTN_R);
    }
    CHECK(type5_weapon_context(5, false, true, 4, 0) == CTX_NATIVE);
    CHECK(type5_weapon_context(15, false, true, 4, 0) == CTX_NATIVE);
    return 0;
}
