#ifndef TYPE5_NOTES_TEXT_H
#define TYPE5_NOTES_TEXT_H

#define NOTES_HEADING(text) "<COL 008000FF>" text "</COL>\r\n"
#define NOTES_BUTTON(button) "<COL E6E6E6FF><ICON " button "></COL>"
#define NOTES_L NOTES_BUTTON("PAD_L")
#define NOTES_R NOTES_BUTTON("PAD_R")
#define NOTES_ZL NOTES_BUTTON("PAD_ZL")
#define NOTES_ZR NOTES_BUTTON("PAD_ZR")
#define NOTES_X NOTES_BUTTON("PAD_X")
#define NOTES_Y NOTES_BUTTON("PAD_Y")
#define NOTES_A NOTES_BUTTON("PAD_A")
#define NOTES_B NOTES_BUTTON("PAD_B")
#define NOTES_L3 NOTES_BUTTON("ANALOG_LB")
#define NOTES_R3 NOTES_BUTTON("ANALOG_RB")
#define NOTES_DRAW NOTES_R "+" NOTES_X "+" NOTES_A

enum { TYPE5_NOTES_PAGES = 4 };

static const char *const type5_notes_chinese[TYPE5_NOTES_PAGES] = {
    NOTES_HEADING("【操作类型5】")
    "最大程度还原从怪物猎人：世界起的作品的手柄操作的操作类型。\r\n"
    NOTES_HEADING("【特色操作】")
    "･长按" NOTES_L "打开道具栏。选中怪物后，按目标视角设置使用" NOTES_L "对准。\r\n"
    "･按住" NOTES_R "并推动摇杆冲刺。拔刀时按下可收刀，继续按住可接冲刺。\r\n"
    "･" NOTES_ZL "用于远程瞄准。" NOTES_ZR "用于武器动作，收刀时取代" NOTES_DRAW "。\r\n"
    "･按下左摇杆" NOTES_L3 "缩放地图。\r\n"
    "･按下右摇杆" NOTES_R3 "锁定或轮换大型怪物目标，再按可轮换到取消选中。\r\n"
    "･使用 CraftWheel 0.50 时，展开的轮盘优先处理左右摇杆按下。",

    NOTES_HEADING("【特色操作】")
    NOTES_HEADING("近战")
    "･所有近战武器持武时，原" NOTES_R "键动作及组合技改用" NOTES_ZR "。\r\n"
    "･收刀时按" NOTES_ZR "，使出原" NOTES_DRAW "对应的武器动作。\r\n"
    "･持武按" NOTES_R "触发短按" NOTES_Y "的收刀，动作可派生时才会执行。\r\n"
    "･英勇风格使用最短英勇纳刀。主动按住" NOTES_Y "可使用原生英勇纳刀。\r\n"
    NOTES_HEADING("轻弩和重弩")
    "･拔刀时，短按" NOTES_ZL "切换开镜，长按进行快速瞄准。按" NOTES_ZR "射击。\r\n"
    "･按住" NOTES_L "打开弹药列表，用游戏原生按键选择弹药。\r\n"
    "･勇气重弩奔跑中按住" NOTES_R "，回到可纳刀的状态后再收刀并冲刺。\r\n"
    "･蹲射继续使用原生架势指令，按" NOTES_ZR "开火。",

    NOTES_HEADING("【特色操作】")
    NOTES_HEADING("弓")
    "･收刀时按" NOTES_ZR "，拔刀使出近战攻击。松开后可再次按下拉弦。\r\n"
    "･持弓时按住" NOTES_ZR "拉弦蓄力，松开发射。\r\n"
    "･按住" NOTES_ZL "进入瞄准，松开退出。\r\n"
    "･按住" NOTES_L "，用" NOTES_X "/" NOTES_B "切换弓瓶，同时按" NOTES_X "+" NOTES_A "装瓶。\r\n"
    "･持弓按" NOTES_R "收刀。英勇风格执行最短英勇纳刀，按住并推动摇杆可接冲刺。",

    NOTES_HEADING("【特色操作】")
    NOTES_HEADING("适用范围与锁定选项")
    "･武器改键适用于全部14种猎人武器。\r\n"
    "･使用武器改键时，将「R摇杆的动作」设为类型1。\r\n"
    "･骑乘与固定弩台使用游戏原生操作。\r\n"
    "･为避免冲突，以下选项在类型5下置灰并锁定。\r\n"
    "  L/R键的动作、L摇杆的动作、弩炮瞄准类型、弓操作类型、弓瞄准取消。\r\n"
    "･切回类型1～4后，恢复原先保存的设置。",
};

static const char *const type5_notes_english[TYPE5_NOTES_PAGES] = {
    NOTES_HEADING("Control Type 5")
    "A layout inspired by Monster Hunter: World and later games.\r\n"
    NOTES_HEADING("Unique Controls")
    "Hold " NOTES_L " for the item bar. Use " NOTES_L " to face the selected target.\r\n"
    "Hold " NOTES_R " and move to sprint. With a weapon drawn, press it to sheathe first.\r\n"
    NOTES_ZL " aims with ranged weapons. " NOTES_ZR " uses a weapon action or fires.\r\n"
    "While sheathed, " NOTES_ZR " replaces " NOTES_DRAW ".\r\n"
    "Click the left stick " NOTES_L3 " to zoom the map.\r\n"
    "Click the right stick " NOTES_R3 " to select, cycle, or clear a large monster target.\r\n"
    "With CraftWheel 0.50 open, stick clicks go to the shortcut wheel.",

    NOTES_HEADING("Unique Controls")
    NOTES_HEADING("Melee")
    "For all drawn melee weapons, " NOTES_ZR " replaces " NOTES_R " in actions and combos.\r\n"
    "While sheathed, " NOTES_ZR " performs the original " NOTES_DRAW " action.\r\n"
    "Press " NOTES_R " to sheathe when the current action allows it.\r\n"
    "Valor uses the shortest sheathe. Hold " NOTES_Y " for the native Valor sheathe.\r\n"
    NOTES_HEADING("Light and Heavy Bowgun")
    "Tap " NOTES_ZL " to toggle the scope, hold it to aim quickly, and press " NOTES_ZR " to fire.\r\n"
    "Hold " NOTES_L " to open the ammo list, then use the native ammo selector.\r\n"
    "During the Valor heavy bowgun run, hold " NOTES_R " to sheathe when the action allows.\r\n"
    "Use the native stance controls for siege mode and " NOTES_ZR " to fire.",

    NOTES_HEADING("Unique Controls")
    NOTES_HEADING("Bow")
    "Press " NOTES_ZR " while sheathed for a melee draw attack. Release before charging.\r\n"
    "With the bow out, hold " NOTES_ZR " to draw and charge. Release it to fire.\r\n"
    "Hold " NOTES_ZL " to aim and release it to stop aiming.\r\n"
    "Hold " NOTES_L " and use " NOTES_X "/" NOTES_B " to select a coating. Press " NOTES_X "+" NOTES_A " to apply it.\r\n"
    "Press " NOTES_R " to sheathe. In Valor it uses the shortest Valor sheathe.\r\n"
    "Keep holding " NOTES_R " and move to sprint.",

    NOTES_HEADING("Unique Controls")
    NOTES_HEADING("Supported Weapons and Locked Settings")
    "Weapon changes cover all 14 hunter weapon types.\r\n"
    "Set the right-stick action to Type 1 to use the weapon changes.\r\n"
    "Mounting and fixed ballistae use the game's original controls.\r\n"
    "Type 5 locks shoulder swapping, left-stick action, bowgun aiming,\r\n"
    "bow controls, and bow aim cancel. Types 1–4 restore the saved values.",
};

#endif
