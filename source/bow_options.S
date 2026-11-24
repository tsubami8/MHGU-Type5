.syntax unified
.arm
.text

// Reproduce the game's load and comparison and leave its registers and flags intact.
// Forge enters these stubs by a jump, so the game's LR is left untouched.
.macro bow_option_site index, reg, offset, compare
    .global type5_bow_option_\index
    .hidden type5_bow_option_\index
    .type type5_bow_option_\index, %function
type5_bow_option_\index:
    ldrb r\reg, [r\reg, #\offset]
    push {r12, lr}
    ldr r12, 1f
0:  ldr r12, [pc, r12]
    cmp r12, #0
    movne r\reg, #0
    ldr r12, 3f
2:  ldr r12, [pc, r12]
    str r12, [sp, #4]
    cmp r\reg, #\compare
    pop {r12, pc}
1:  .word type5_bow_options_active - (0b + 8)
3:  .word type5_bow_option_resume + (4 * \index) - (2b + 8)
    .size type5_bow_option_\index, . - type5_bow_option_\index
.endm

.hidden type5_bow_options_active
.hidden type5_bow_option_resume
#define BOW_OPTION_SITE(index, address, reg, offset, compare) bow_option_site index, reg, offset, compare
#include "bow_option_sites.def"
#undef BOW_OPTION_SITE
