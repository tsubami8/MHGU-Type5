.syntax unified
.arm
.text

// Substitute the native R3 preset only at the target-camera input and HUD reads.
// The instruction at each resume address immediately sets the game flags again.
.macro target_camera_site name, index, dest, options, field, next, base, offset
    .global type5_target_camera_\name
    .hidden type5_target_camera_\name
    .type type5_target_camera_\name, %function
type5_target_camera_\name:
    ldrb \dest, [\options, \field]
    push {r12, lr}
    ldr r12, 1f
0:  ldr r12, [pc, r12]
    cmp r12, #0
    movne \dest, #2
    ldrb \next, [\base, #\offset]
    ldr r12, 3f
2:  ldr r12, [pc, r12]
    str r12, [sp, #4]
    pop {r12, pc}
1:  .word type5_target_camera_active - (0b + 8)
3:  .word type5_target_camera_resume + (4 * \index) - (2b + 8)
    .size type5_target_camera_\name, . - type5_target_camera_\name
.endm

.hidden type5_target_camera_active
.hidden type5_target_camera_resume
target_camera_site input, 0, r4, r1, r2, r1, r6, 0xa8e
target_camera_site hud,   1, r6, r0, r1, r0, r4, 0x54
