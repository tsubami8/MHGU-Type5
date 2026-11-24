#include "controls.h"
#include "notes_text.h"

#define EXPORT __attribute__((visibility("default")))

struct PluginInfo {
    uint16_t major, minor, patch;
    char name[64];
};

struct ForgeHook {
    void *target, *detour, *original, *context, **context_cell;
    bool has_context;
};

extern uint32_t forge_mem_getMainTextAddr(void);
extern struct ForgeHook forge_hook_create(void *target, void *detour, void **original);
extern void forge_log(int level, const char *message, ...);
extern bool igCollapsingHeader_TreeNodeFlags(const char *label, int flags);
extern void igTextUnformatted(const char *text, const char *end);
extern bool igCheckbox(const char *label, bool *value);
extern void wheel_cancel_gesture(void) __attribute__((weak, visibility("default")));

// Relative to ExeFS main, build FB08F1D20FD1204FE7070672777F869B.
enum {
    PAD_TRANSLATE = 0x3de4b0, PAD_GLOBAL_GOT = 0x183cbfc, PAD_GAMEPLAY_VIEW = 0x6b98,
    HUNT_ACTIVE = 0x66eeec, PLAYER_MANAGER = 0x1885b58, GAME_CONTROL = 0x188ca48,
    OPTIONS = 0x1897f78, OPT_LEFT_STICK_ACTION = 0x43fb, OPT_CONTROL_TYPE = 0x446c,
    OPT_SHOULDER_SWAP = 0x446e, OPT_STICK_SWAP = 0x446f,
    OPT_BOWGUN_TYPE = 0x67, OPT_BOW_TYPE = 0x6c, OPT_BOW_CANCEL = 0x6d,
    OPTIONS_ROW = 0x59, OPTIONS_PAGE = 0x5c, OPTIONS_STATE = 0x74,
    DISPLAY_OPTION_HELP = 0x65e5bc, TYPE5_HELP_KEY = 0x94, LOCKED_HELP_KEY = 0x212,
    OPTION_TITLE_RETURN = 0x5e6630, OPTION_VALUE_RETURN = 0x5e67b0, OPTIONS_ROWS = 8,
    MENU_PRESSED = 0x6bfc, MENU_CONFIRM = 0x2000,
    PLAYER_SLOTS = 0x1c, LOCAL_SLOT = 0x30, AREA_TYPE = 0x4c,
    EQUIPMENT = 0x13c4, WEAPON = 0x14c, DRAWN = 0x114c, HP = 0x13d6, ACTION_GROUP = 0x2506,
    ACTION_NUMBER = 0x250a, MOVE_TABLE = 0x24ac,
    GREAT_SWORD = 0, SWORD_AND_SHIELD = 1, HAMMER = 2, LANCE = 3, HEAVY_BOWGUN = 4,
    LIGHT_BOWGUN = 6, LONG_SWORD = 7, SWITCH_AXE = 8, GUNLANCE = 9, BOW = 10,
    DUAL_BLADES = 11, HUNTING_HORN = 12, INSECT_GLAIVE = 13, CHARGE_BLADE = 14,
};

typedef uint32_t (*PadTranslate)(uintptr_t, uint32_t);
typedef void (*HuntUpdate)(uintptr_t);

EXPORT volatile uint32_t type5_debug_enable;
static uintptr_t text;
static PadTranslate original_pad_translate;
static HuntUpdate original_hunt_update;
static Type5State inputs;
static unsigned hunt_updates, hunt_age;
static unsigned pad_updates;
static uintptr_t last_pad_view;
static bool shoulder_lock_ready;
static bool native_menu_ready;
static bool bow_options_ready;
static bool target_camera_ready;
static uintptr_t shortcut_wheel;

// Read by the ARM stubs. The original options object keeps the saved values.
volatile uint32_t type5_bow_options_active;
#define BOW_OPTION_SITE(index, address, reg, offset, compare) extern void type5_bow_option_##index(void);
#include "bow_option_sites.def"
#undef BOW_OPTION_SITE

static const struct {
    uintptr_t address;
    uint32_t load, compare;
    void (*detour)(void);
} bow_option_sites[] = {
#define BOW_OPTION_SITE(index, address, reg, offset, compare) \
    [index] = {address, 0xe5d00000u | ((reg) << 16) | ((reg) << 12) | (offset), \
               0xe3500000u | ((reg) << 16) | (compare), type5_bow_option_##index},
#include "bow_option_sites.def"
#undef BOW_OPTION_SITE
};
uintptr_t type5_bow_option_resume[sizeof(bow_option_sites) / sizeof(bow_option_sites[0])];

// These two branches read the native preset independently of weapon input.
volatile uint32_t type5_target_camera_active;
uintptr_t type5_target_camera_resume[2];
extern void type5_target_camera_input(void);
extern void type5_target_camera_hud(void);

static uint32_t word(uintptr_t address) { return *(const volatile uint32_t *)address; }
static unsigned half(uintptr_t address) { return *(const volatile uint16_t *)address; }
static unsigned byte(uintptr_t address) { return *(const volatile uint8_t *)address; }
static void set_byte(uintptr_t address, unsigned value) { *(volatile uint8_t *)address = value; }
static void set_half(uintptr_t address, unsigned value) { *(volatile uint16_t *)address = value; }

// Recomp's Forge can chain an existing LDR PC entry hook. The next two
// instructions still identify the underlying function when its entry is hooked.
static bool chainable_entry(uintptr_t address, uint32_t first, uint32_t second,
                            uint32_t third, uint32_t fourth)
{
    const uint32_t entry = word(address);
    const uint32_t next = word(address + 4);
    return word(address + 8) == third && word(address + 12) == fourth &&
           ((entry == first && next == second) ||
            (entry == 0xe51ff004u && next != 0 && (next & 3u) == 0));
}

static bool type5_selected(void)
{
    const uintptr_t options = word(text + OPTIONS);
    return shoulder_lock_ready && type5_debug_enable && options && !byte(options + OPT_CONTROL_TYPE);
}

static void refresh_effective_options(bool selected)
{
    type5_bow_options_active = bow_options_ready && selected;
    type5_target_camera_active = target_camera_ready && selected;
}

static uintptr_t shortcut_wheel_state(void)
{
    // CraftWheel 0.50's exported cancel routine identifies the wheel state via its GOT.
    const uintptr_t cancel = (uintptr_t)wheel_cancel_gesture;
    if (!cancel || word(cancel) != 0xe59f3058 || word(cancel + 4) != 0xe59f2058 ||
        word(cancel + 8) != 0xe08f3003 || word(cancel + 12) != 0xe7933002 ||
        word(cancel + 16) != 0xe5932004) return 0;
    const uintptr_t got = cancel + 0x10 + word(cancel + 0x60);
    return word(got + word(cancel + 0x64));
}

static uint32_t filter_shortcut_wheel(uint32_t native, bool selected)
{
    static uint32_t blocked;
    static bool was_open;
    const bool open = selected && shortcut_wheel &&
                      (word(shortcut_wheel) || word(shortcut_wheel + 0x38));
    blocked &= selected ? native : 0;
    if (open || (selected && was_open)) blocked |= native & (BTN_L3 | BTN_R3);
    was_open = open;
    // CraftWheel reads its own physical input. The hunt sees neither the click
    // nor a new press when the wheel closes with the stick still held.
    return native & ~blocked;
}

static unsigned (*original_left_stick_action)(void);

static unsigned left_stick_action(void)
{
    // The game's first left-stick action is map zoom. The others toggle sprint.
    return target_camera_ready && type5_selected() ? 0 : original_left_stick_action();
}

static bool install_target_camera(void)
{
    const uintptr_t left_stick = text + 0x65f8a0;
    const struct {
        uintptr_t address;
        uint32_t first, second, next;
        void (*detour)(void);
    } sites[] = {
        {0x220908, 0xe7d14002, 0xe5d61a8e, 0xe3510000, type5_target_camera_input},
        {0x8574a8, 0xe7d06001, 0xe5d40054, 0xe1560000, type5_target_camera_hud},
    };
    if (!chainable_entry(left_stick, 0xe59f0018, 0xe30413fb, 0xe79f0000, 0xe5900000)) return false;
    for (unsigned i = 0; i < sizeof(sites) / sizeof(sites[0]); ++i) {
        const uintptr_t address = text + sites[i].address - 0x200000;
        if (word(address) != sites[i].first || word(address + 4) != sites[i].second ||
            word(address + 8) != sites[i].next) return false;
        type5_target_camera_resume[i] = address + 8;
    }
    forge_hook_create((void *)left_stick, left_stick_action, (void **)&original_left_stick_action);
    if (!original_left_stick_action) return false;
    for (unsigned i = 0; i < sizeof(sites) / sizeof(sites[0]); ++i) {
        void *original = 0;
        forge_hook_create((void *)(type5_target_camera_resume[i] - 8), sites[i].detour, &original);
        if (!original) return false;
    }
    return true;
}

static bool install_bow_options(void)
{
    const unsigned count = sizeof(bow_option_sites) / sizeof(bow_option_sites[0]);
    for (unsigned i = 0; i < count; ++i) {
        const uintptr_t address = text + bow_option_sites[i].address - 0x200000;
        if (word(address) != bow_option_sites[i].load || word(address + 4) != bow_option_sites[i].compare) {
            forge_log(2, "[type5] unrecognized Bow option read %08x", (unsigned)address);
            return false;
        }
        type5_bow_option_resume[i] = address + 8;
    }
    for (unsigned i = 0; i < count; ++i) {
        const uintptr_t address = type5_bow_option_resume[i] - 8;
        void *original = 0;
        forge_hook_create((void *)address, (void *)bow_option_sites[i].detour, &original);
        if (!original) return false;
    }
    return true;
}

// The native roles use the unswapped values while the prototype is selected.
// The original option byte remains available to types 1–4 and to the save code.
#define SHOULDER_ROLE_LIST(X) \
    X(option,       0x7e8080, 0xe59f0034, 0) \
    X(swapped,      0x85f858, 0xe59f0018, 0) \
    X(field_l,      0x85f8c4, 0xe59f001c, 0x100) \
    X(field_l2,     0x85f8ec, 0xe59f001c, 0x400) \
    X(field_zl,     0x85f914, 0xe59f001c, 0x200) \
    X(field_r,      0x85f93c, 0xe59f001c, 0x40) \
    X(field_pair,   0x85f964, 0xe59f001c, 0x300) \
    X(menu_l,       0x85f98c, 0xe59f001c, 0x400) \
    X(menu_l2,      0x85f9b4, 0xe59f001c, 0x200) \
    X(menu_zl,      0x85f9dc, 0xe59f001c, 0x800) \
    X(field_zr,     0x85fa04, 0xe59f001c, 0x800) \
    X(menu_pair,    0x85fa2c, 0xe59f001c, 0xc00)

#define DEFINE_SHOULDER_ROLE(name, address, instruction, standard) \
    static void *original_##name; \
    static unsigned read_##name(void) \
    { \
        return type5_selected() ? standard : ((unsigned (*)(void))original_##name)(); \
    }
SHOULDER_ROLE_LIST(DEFINE_SHOULDER_ROLE)
#undef DEFINE_SHOULDER_ROLE

static void *original_set_shoulder;
static void *original_left_prompt, *original_right_prompt;

static void set_shoulder(unsigned value)
{
    if (!type5_selected()) ((void (*)(unsigned))original_set_shoulder)(value);
}

static unsigned left_prompt(unsigned prompt)
{
    return type5_selected() ? prompt : ((unsigned (*)(unsigned))original_left_prompt)(prompt);
}

static unsigned right_prompt(unsigned prompt)
{
    return type5_selected() ? prompt : ((unsigned (*)(unsigned))original_right_prompt)(prompt);
}

static bool install_shoulder_lock(void)
{
#define ROLE_HOOK(name, address, instruction, standard) {address, instruction, read_##name, &original_##name},
    const struct {
        uintptr_t address;
        uint32_t instruction;
        void *detour, **original;
    } hooks[] = {
        SHOULDER_ROLE_LIST(ROLE_HOOK)
        {0x7e84a0, 0xe59f1034, set_shoulder, &original_set_shoulder},
        {0x85faf4, 0xe59f104c, left_prompt, &original_left_prompt},
        {0x85fb4c, 0xe59f104c, right_prompt, &original_right_prompt},
    };
#undef ROLE_HOOK
    for (unsigned i = 0; i < sizeof(hooks) / sizeof(hooks[0]); ++i) {
        if (word(text + hooks[i].address - 0x200000) != hooks[i].instruction) return false;
    }
    for (unsigned i = 0; i < sizeof(hooks) / sizeof(hooks[0]); ++i) {
        forge_hook_create((void *)(text + hooks[i].address - 0x200000), hooks[i].detour,
                          hooks[i].original);
        if (!*hooks[i].original) return false;
    }
    return true;
}

static bool controls_row(unsigned page, unsigned row) { return page == 2 && row == 4; }

static bool locked_row(unsigned page, unsigned row)
{
    return (page == 2 && row == 6) || (page == 1 && (row == 0 || row == 5 || row == 6)) ||
           (target_camera_ready && page == 3 && row == 0);
}

static unsigned (*original_get_option)(uintptr_t, unsigned, unsigned);
static void (*original_set_option)(uintptr_t, unsigned, unsigned, uintptr_t, unsigned);
static void (*original_describe_option)(uintptr_t, uintptr_t, unsigned, unsigned);
static void (*original_options_update)(uintptr_t);
static void (*original_draw_options)(uintptr_t);
static void (*original_draw_option_text)(uintptr_t, unsigned, unsigned, const char *);
static void (*original_player_update)(uintptr_t);
static void (*original_options_help)(uintptr_t);
static const char *(*original_help_text)(uintptr_t, unsigned, unsigned, unsigned);
static enum { HELP_NATIVE, HELP_TYPE5, HELP_LOCKED } option_help;
static uintptr_t options_render_menu;
static unsigned locked_render_row = OPTIONS_ROWS;
static char locked_option_text[OPTIONS_ROWS][2][128];

enum {
    NOTES_SECTION = 0x50, NOTES_WIDGETS = 0x34, NOTES_LIST_WIDGET = 0x13, NOTES_BOOK_WIDGET = 0x1b,
    NOTES_TYPE_SELECTOR = 0x144, NOTES_PAGE_SELECTOR = 0x18c,
    NOTES_SELECTOR_ROW = 0x0d, NOTES_SELECTOR_PAGE_SIZE = 0x0e, NOTES_SELECTOR_PAGE = 0x10,
    NOTES_SELECTOR_PAGES = 0x12, NOTES_SELECTOR_COUNT = 0x14,
    NOTES_WIDGET_SELECTOR = 0x4c, NOTES_LIST_SIZE = 0x50, NOTES_BOOK_TYPE = 0x53,
    NOTES_ACTIVE_TYPE = 0x58, NOTES_TYPE5 = 4, NOTES_NATIVE_TYPES = 4,
    NOTES_LIST_TITLE = 0x25, NOTES_LIST_FIRST = 0x24, NOTES_LIST_CURRENT = 0x2a,
    NOTES_BOOK_TITLE = 0x27, NOTES_BOOK_BODY = 0x11, NOTES_MENU_CANCEL = 0x4000,
};
static unsigned (*original_notes_open)(uintptr_t);
static unsigned (*original_notes_update)(uintptr_t);
static void (*original_notes_book_setup)(uintptr_t, uintptr_t);
static void (*original_notes_book_render)(uintptr_t);
static uintptr_t notes_screen, notes_list, notes_book, notes_counter;
static unsigned notes_body_page;
static bool notes_chinese;

enum {
    PAGE_COUNTER_VTABLE = 0x1773efc, PAGE_COUNTER_TYPE = 0x183f23c,
};

static uintptr_t gui_node(uintptr_t gui, unsigned id)
{
    const unsigned fixed = word(gui + 0x298);
    if (id < fixed) return word(word(gui + 0xf8) + id * sizeof(uint32_t));
    return word(word(gui + 0x294) + (id - fixed) * sizeof(uint32_t));
}

static uintptr_t notes_page_counter(uintptr_t selector)
{
    uintptr_t counter = 0;
    for (uintptr_t child = word(notes_list + 0xc); child; child = word(child + 0x10)) {
        if (word(child) == text + PAGE_COUNTER_VTABLE && word(child + 0x7c) == selector) {
            counter = child;
            break;
        }
    }

    const uintptr_t gui = word(notes_list + 0x24);
    if (!gui) return 0;

    if (!counter) {
        typedef void (*CounterConfig)(uint32_t *);
        typedef uintptr_t (*AttachChild)(uintptr_t, uintptr_t, uint32_t *, unsigned);
        uint32_t config[0x70 / sizeof(uint32_t)] = {0};
        ((CounterConfig)(text + 0x60097c))(config);
        config[0x08 / 4] = gui;
        config[0x64 / 4] = 2;
        config[0x68 / 4] = word(notes_list + 0x28);
        // Slot zero belongs to another panel in menu.gui, with a hidden parent here.
        config[0x6c / 4] = 1;
        counter = ((AttachChild)(text + 0x56dd44))(notes_list, word(text + PAGE_COUNTER_TYPE), config, 0);
        ((CounterConfig)(text + 0x570710))(config);
        if (!counter) return 0;
        *(volatile uint32_t *)(counter + 0x7c) = selector;
    }

    // menu.gui already provides the native counter and both arrow instances.
    typedef void (*CounterStyle)(uintptr_t, unsigned);
    typedef void (*CounterVisibility)(uintptr_t);
    typedef void (*NodePosition)(uintptr_t, unsigned, float) __attribute__((pcs("aapcs-vfp")));
    ((CounterStyle)(text + 0x600de4))(counter, 0);
    const float x[] = {254.0f, 206.0f, 302.0f};
    for (unsigned i = 0; i < sizeof(x) / sizeof(x[0]); ++i) {
        const unsigned id = word(counter + 0x58 + i * sizeof(uint32_t));
        const uintptr_t node = gui_node(gui, id);
        if (!node || !word(node + 0xd8) || !word(node + 0xdc)) {
            set_byte(counter + 0x18, 0);
            return 0;
        }
        ((NodePosition)(text + 0x545a40))(gui, id, x[i]);
        ((NodePosition)(text + 0x545ac0))(gui, id, 22.0f);
    }
    ((CounterVisibility)(text + 0x600aa4))(counter);
    return counter;
}

static bool is_chinese(const char *label)
{
    const unsigned first = (unsigned char)label[0];
    return first >= 0xe4 && first <= 0xe9;
}

static void notes_update_list(void)
{
    typedef void (*Render)(uintptr_t);
    ((Render)word(word(notes_list) + 0x64))(notes_list);
    if (notes_counter) ((Render)(text + 0x600ec8))(notes_counter);
}

static void notes_book_setup(uintptr_t book, uintptr_t selector)
{
    if (book != notes_book || byte(book + NOTES_BOOK_TYPE) != NOTES_TYPE5) {
        original_notes_book_setup(book, selector);
        return;
    }
    // Reuse the first entry's booklet template. Its built-in tables contain four presets.
    set_byte(book + NOTES_BOOK_TYPE, 0);
    original_notes_book_setup(book, selector);
    set_byte(book + NOTES_BOOK_TYPE, NOTES_TYPE5);
    set_half(selector + NOTES_SELECTOR_PAGES, TYPE5_NOTES_PAGES);
    set_half(selector + NOTES_SELECTOR_COUNT, TYPE5_NOTES_PAGES);
}

static void notes_book_render(uintptr_t book)
{
    if (book != notes_book || byte(book + NOTES_BOOK_TYPE) != NOTES_TYPE5) {
        original_notes_book_render(book);
        return;
    }
    const uintptr_t selector = word(book + NOTES_WIDGET_SELECTOR);
    const unsigned page = half(selector + NOTES_SELECTOR_PAGE);
    notes_body_page = page < TYPE5_NOTES_PAGES ? page + 1 : 1;
    set_byte(book + NOTES_BOOK_TYPE, 0);
    set_half(selector + NOTES_SELECTOR_PAGE, 0);
    original_notes_book_render(book);
    set_half(selector + NOTES_SELECTOR_PAGE, page);
    set_byte(book + NOTES_BOOK_TYPE, NOTES_TYPE5);
    notes_body_page = 0;
}

static unsigned notes_open(uintptr_t menu)
{
    notes_screen = notes_list = notes_book = notes_counter = 0;
    if (native_menu_ready && word(menu + NOTES_SECTION) == 0) {
        const uintptr_t widgets = word(menu + NOTES_WIDGETS);
        if (widgets) {
            notes_list = word(widgets + NOTES_LIST_WIDGET * sizeof(uint32_t));
            notes_book = word(widgets + NOTES_BOOK_WIDGET * sizeof(uint32_t));
            if (notes_list && notes_book) notes_screen = menu;
        }
    }
    const unsigned result = original_notes_open(menu);
    if (!notes_screen) return result;
    const uintptr_t selector = menu + NOTES_TYPE_SELECTOR;
    set_byte(selector + NOTES_SELECTOR_PAGE_SIZE, NOTES_NATIVE_TYPES);
    set_half(selector + NOTES_SELECTOR_PAGE, 0);
    set_half(selector + NOTES_SELECTOR_PAGES, 2);
    set_half(selector + NOTES_SELECTOR_COUNT, NOTES_NATIVE_TYPES + 1);
    set_byte(notes_list + NOTES_LIST_SIZE, NOTES_NATIVE_TYPES + 1);
    notes_counter = notes_page_counter(selector);
    notes_update_list();
    return result;
}

static unsigned notes_update(uintptr_t menu)
{
    if (menu != notes_screen) return original_notes_update(menu);
    const uintptr_t selector = menu + NOTES_TYPE_SELECTOR;
    const unsigned page = half(selector + NOTES_SELECTOR_PAGE);
    const uintptr_t pad_global = word(text + PAD_GLOBAL_GOT);
    const uintptr_t pad = pad_global ? word(pad_global) : 0;
    const uint32_t pressed = pad ? word(pad + MENU_PRESSED) : 0;
    if ((pressed & (MENU_CONFIRM | NOTES_MENU_CANCEL)) == MENU_CONFIRM) {
        const unsigned choice = page * NOTES_NATIVE_TYPES + byte(selector + NOTES_SELECTOR_ROW);
        const uintptr_t options = word(text + OPTIONS);
        if (options && choice <= NOTES_TYPE5) {
            const unsigned current = type5_selected() ? NOTES_TYPE5 : byte(options + OPT_CONTROL_TYPE);
            if (choice != current) {
                const unsigned base = choice == NOTES_TYPE5 ? 0 : choice;
                type5_debug_enable = choice == NOTES_TYPE5;
                set_byte(options + OPT_CONTROL_TYPE, base);
                refresh_effective_options(type5_selected());
                set_byte(notes_list + NOTES_ACTIVE_TYPE, base);
                notes_update_list();
                typedef void (*MenuSound)(unsigned, unsigned);
                ((MenuSound)(text + 0x54f260))(0, 1);
            }
            return 0;
        }
    }
    const unsigned result = original_notes_update(menu);
    if (result) {
        typedef void (*CounterVisibility)(uintptr_t);
        if (notes_counter) ((CounterVisibility)(text + 0x600b70))(notes_counter);
        notes_screen = notes_list = notes_book = notes_counter = 0;
    } else if (page != half(selector + NOTES_SELECTOR_PAGE)) notes_update_list();
    return result;
}

static const char *notes_text(uintptr_t widget, unsigned node, unsigned field, const char *native)
{
    if (!native) return native;
    if (widget == notes_list) {
        const uintptr_t selector = word(widget + NOTES_WIDGET_SELECTOR);
        const bool second = selector && half(selector + NOTES_SELECTOR_PAGE) == 1;
        const bool chinese = is_chinese(native);
        if (node == NOTES_LIST_TITLE && field == 3 && !notes_counter)
            return chinese ? (second ? "操作类型 2/2 <ICON KEY_RL>" : "操作类型 1/2 <ICON KEY_RL>") :
                             (second ? "Control Type 2/2 <ICON KEY_RL>" : "Control Type 1/2 <ICON KEY_RL>");
        if ((node == NOTES_LIST_FIRST && field == 1 && second) ||
            (node == NOTES_LIST_CURRENT && field == 2 && type5_selected()))
            return chinese ? "类型5" : "Type 5";
    } else if (widget == notes_book && notes_body_page) {
        if (node == NOTES_BOOK_TITLE && field == 2) {
            notes_chinese = is_chinese(native);
            return notes_chinese ? "类型5" : "Type 5";
        }
        if (node == NOTES_BOOK_BODY && field == 1)
            return (notes_chinese ? type5_notes_chinese : type5_notes_english)[notes_body_page - 1];
    }
    return native;
}

static unsigned get_option(uintptr_t menu, unsigned page, unsigned row)
{
    if (native_menu_ready && type5_selected()) {
        if (controls_row(page, row)) return 4;
        if (locked_row(page, row)) return 0;
    }
    return original_get_option(menu, page, row);
}

static void set_option(uintptr_t menu, unsigned page, unsigned row, uintptr_t descriptor, unsigned value)
{
    if (native_menu_ready && controls_row(page, row) && value <= 4) {
        type5_debug_enable = value == 4;
        // Only the UI sees 4. Game code and saves keep a valid base preset.
        original_set_option(menu, page, row, descriptor, value == 4 ? 0 : value);
        refresh_effective_options(type5_selected());
        return;
    }
    if (native_menu_ready && type5_selected() && locked_row(page, row)) return;
    original_set_option(menu, page, row, descriptor, value);
}

static void describe_option(uintptr_t menu, uintptr_t descriptor, unsigned page, unsigned row)
{
    original_describe_option(menu, descriptor, page, row);
    if (!native_menu_ready) return;
    if (controls_row(page, row)) *(volatile uint16_t *)(descriptor + 0xc) = 4;
    const bool locked = type5_selected() && locked_row(page, row);
    if (menu == options_render_menu) locked_render_row = locked ? row : OPTIONS_ROWS;
    if (locked) {
        set_byte(descriptor + 8, 0);
        *(volatile uint16_t *)(descriptor + 0xc) = 0;
    }
}

static void draw_options(uintptr_t menu)
{
    options_render_menu = menu;
    original_draw_options(menu);
    options_render_menu = 0;
    locked_render_row = OPTIONS_ROWS;
}

static const char *grey_option_text(const char *name, char (*buffer)[128])
{
    static const char prefix[] = "<COL 808080FF>";
    static const char suffix[] = "</COL>";
    char *output = *buffer;
    const unsigned capacity = sizeof(*buffer) - (sizeof(prefix) - 1) - sizeof(suffix);
    unsigned length = 0;
    while (length <= capacity && name[length]) ++length;
    if (length > capacity) return name;
    for (unsigned i = 0; i < sizeof(prefix) - 1; ++i) *output++ = prefix[i];
    for (unsigned i = 0; i < length; ++i) *output++ = name[i];
    for (unsigned i = 0; i < sizeof(suffix); ++i) *output++ = suffix[i];
    return *buffer;
}

static void draw_option_text(uintptr_t menu, unsigned node, unsigned field, const char *name)
{
    const uintptr_t caller = (uintptr_t)__builtin_return_address(0) - text;
    if (menu && (menu == notes_list || menu == notes_book)) name = notes_text(menu, node, field, name);
    if (name && menu == options_render_menu && locked_render_row < OPTIONS_ROWS &&
        (caller == OPTION_TITLE_RETURN || caller == OPTION_VALUE_RETURN)) {
        const unsigned value = caller == OPTION_VALUE_RETURN;
        name = grey_option_text(name, &locked_option_text[locked_render_row][value]);
    }
    original_draw_option_text(menu, node, field, name);
}

static void update_options(uintptr_t menu)
{
    if (native_menu_ready && type5_selected() && word(menu + OPTIONS_STATE) == 1 &&
        locked_row(half(menu + OPTIONS_PAGE), byte(menu + OPTIONS_ROW))) {
        const uintptr_t pad_global = word(text + PAD_GLOBAL_GOT);
        const uintptr_t pad = pad_global ? word(pad_global) : 0;
        if (pad) {
            volatile uint32_t *pressed = (volatile uint32_t *)(pad + MENU_PRESSED);
            const uint32_t saved = *pressed;
            *pressed = saved & ~MENU_CONFIRM;
            original_options_update(menu);
            *pressed = saved;
            return;
        }
    }
    original_options_update(menu);
}

static void show_options_help(uintptr_t menu)
{
    original_options_help(menu);
    if (native_menu_ready && type5_selected()) {
        const unsigned page = half(menu + OPTIONS_PAGE);
        const unsigned row = byte(menu + OPTIONS_ROW);
        if (controls_row(page, row)) option_help = HELP_TYPE5;
        if (locked_row(page, row)) option_help = HELP_LOCKED;
    }
    if (option_help != HELP_NATIVE) {
        typedef void (*DisplayHelp)(unsigned, unsigned, unsigned, unsigned, unsigned, unsigned);
        const unsigned key = option_help == HELP_TYPE5 ? TYPE5_HELP_KEY : LOCKED_HELP_KEY;
        ((DisplayHelp)(text + DISPLAY_OPTION_HELP))(4, 0, 9, key, 0, 0);
    }
    option_help = HELP_NATIVE;
}

static const char *help_text(uintptr_t table, unsigned mode, unsigned group, unsigned id)
{
    const char *native = original_help_text(table, mode, group, id);
    if (!native || group != 9 ||
        !((option_help == HELP_TYPE5 && id == TYPE5_HELP_KEY) ||
          (option_help == HELP_LOCKED && id == LOCKED_HELP_KEY))) return native;
    const bool chinese = is_chinese(native);
    if (option_help == HELP_LOCKED) {
        return chinese ? "操作类型 5 已锁定此选项。" : "This setting is locked by Control Type 5.";
    }
    return chinese ?
        "最大程度还原从怪物猎人：世界起的作品的手柄操作，\r\n"
        "分离肩键及扳机键的功能，\r\n"
        "在猎人笔记-操作说明中查看详细说明。" :
        "Controller layout inspired by Monster Hunter: World.\r\n"
        "Bumpers and triggers have separate functions.\r\n"
        "See the controls in Hunter's Notes for details.";
}

static void update_player(uintptr_t player)
{
    const uintptr_t options = word(text + OPTIONS);
    if (native_menu_ready && type5_selected()) {
        const unsigned shoulders = byte(options + OPT_SHOULDER_SWAP);
        const unsigned bowgun = byte(options + OPT_BOWGUN_TYPE);
        const unsigned bow = byte(options + OPT_BOW_TYPE);
        const unsigned cancel = byte(options + OPT_BOW_CANCEL);
        const unsigned left_stick = byte(options + OPT_LEFT_STICK_ACTION);
        // Recomp's combo window reads this byte directly, outside the role getters.
        set_byte(options + OPT_SHOULDER_SWAP, 0);
        set_byte(options + OPT_BOWGUN_TYPE, 0);
        set_byte(options + OPT_BOW_TYPE, 0);
        set_byte(options + OPT_BOW_CANCEL, 0);
        if (target_camera_ready) set_byte(options + OPT_LEFT_STICK_ACTION, 0);
        original_player_update(player);
        set_byte(options + OPT_SHOULDER_SWAP, shoulders);
        set_byte(options + OPT_BOWGUN_TYPE, bowgun);
        set_byte(options + OPT_BOW_TYPE, bow);
        set_byte(options + OPT_BOW_CANCEL, cancel);
        if (target_camera_ready) set_byte(options + OPT_LEFT_STICK_ACTION, left_stick);
        return;
    }
    original_player_update(player);
}

static bool install_native_menu(void)
{
    const uintptr_t help = text + 0x55bac0;
    const uintptr_t player = text + 0x2a79cc;
    if (!chainable_entry(help, 0xe92d4070u, 0xe1a04003u, 0xe1a06002u, 0xe1a05000u) ||
        word(text + DISPLAY_OPTION_HELP) != 0xe92d41f0u ||
        !chainable_entry(player, 0xe92d4ff0u, 0xe24dd014u, 0xe1a04000u, 0xe59f05f0u)) {
        forge_log(2, "[type5] unrecognized help or player-update entry");
        return false;
    }
    const struct {
        uintptr_t address;
        uint32_t instruction;
        void *detour, **original;
    } hooks[] = {
        {0x7e7d34, 0xe3a00000, get_option, (void **)&original_get_option},
        {0x7e8130, 0xe3510003, set_option, (void **)&original_set_option},
        {0x7e7558, 0xe92d4830, describe_option, (void **)&original_describe_option},
        {0x7e5840, 0xe92d47f0, update_options, (void **)&original_options_update},
        {0x7e635c, 0xe92d4ff0, draw_options, (void **)&original_draw_options},
        {0x76e808, 0xe5900024, draw_option_text, (void **)&original_draw_option_text},
        {0x7e87d0, 0xe92d4010, show_options_help, (void **)&original_options_help},
        {0x75bac0, 0, help_text, (void **)&original_help_text},
        {0x87655c, 0xe92d48f0, notes_open, (void **)&original_notes_open},
        {0x876940, 0xe92d41f0, notes_update, (void **)&original_notes_update},
        {0x846a3c, 0xe92d4010, notes_book_setup, (void **)&original_notes_book_setup},
        {0x846ad0, 0xe92d4ff0, notes_book_render, (void **)&original_notes_book_render},
        {0x4a79cc, 0, update_player, (void **)&original_player_update},
    };
    for (unsigned i = 0; i < sizeof(hooks) / sizeof(hooks[0]); ++i) {
        if (hooks[i].instruction && word(text + hooks[i].address - 0x200000) != hooks[i].instruction) {
            forge_log(2, "[type5] unrecognized options entry %08x", (unsigned)hooks[i].address);
            return false;
        }
    }
    for (unsigned i = 0; i < sizeof(hooks) / sizeof(hooks[0]); ++i) {
        forge_hook_create((void *)(text + hooks[i].address - 0x200000), hooks[i].detour,
                          hooks[i].original);
        if (!*hooks[i].original) return false;
    }
    return true;
}

static unsigned hunter_weapon(uintptr_t player)
{
    const uintptr_t equipment = player ? word(player + EQUIPMENT) : 0;
    return equipment ? word(equipment + WEAPON) : 0xff;
}

static uintptr_t local_hunter(void)
{
    const uintptr_t control = word(text + GAME_CONTROL);
    const uintptr_t manager = word(text + PLAYER_MANAGER);
    if (!control || !manager || byte(control + AREA_TYPE) != 0xff) return 0;
    const unsigned slot = word(control + LOCAL_SLOT);
    if (slot >= 4) return 0;
    const uintptr_t hunter = word(manager + PLAYER_SLOTS + slot * sizeof(uint32_t));
    return hunter && half(hunter + HP) ? hunter : 0;
}

static void observe_hunt_update(uintptr_t manager)
{
    ++hunt_updates;
    original_hunt_update(manager);
}

static uint32_t translate_gameplay_pad(uintptr_t view, uint32_t native)
{
    ++pad_updates;
    last_pad_view = view;
    const uintptr_t pad_global = word(text + PAD_GLOBAL_GOT);
    const uintptr_t pad = pad_global ? word(pad_global) : 0;
    if (!pad || view != pad + PAD_GAMEPLAY_VIEW) return original_pad_translate(view, native);

    const bool selected = type5_selected();
    refresh_effective_options(selected);

    static unsigned previous_hunt_updates;
    hunt_age = previous_hunt_updates != hunt_updates ? 0 : hunt_age + (hunt_age < 4);
    previous_hunt_updates = hunt_updates;

    const uintptr_t player = local_hunter();
    const uintptr_t options = word(text + OPTIONS);
    const unsigned preset = options ? byte(options + OPT_CONTROL_TYPE) + 1 : 0;
    const unsigned weapon = hunter_weapon(player);
    const unsigned group = player ? half(player + ACTION_GROUP) : 0;
    const unsigned act = player ? half(player + ACTION_NUMBER) : 0;
    const bool drawn = player && byte(player + DRAWN);
    static unsigned observed_preset, observed_weapon = 0xff;
    static unsigned observed_group, observed_action;
    if (preset != observed_preset || weapon != observed_weapon ||
        (type5_debug_enable && (group != observed_group || act != observed_action))) {
        forge_log(1, "[type5] native preset=%u weapon=%u action=%x:%u drawn=%u table=%u",
                  preset, weapon, group, act, (unsigned)drawn,
                  player ? word(player + MOVE_TABLE) : 0);
        observed_preset = preset;
        observed_weapon = weapon;
        observed_group = group;
        observed_action = act;
    }

    enum Type5Context context = CTX_NATIVE;
    if (selected && hunt_age <= 2 && !byte(options + OPT_STICK_SWAP) &&
        (group == 1 || group == 2 || group == 0x10 || group == 0x20 || group == 0x8010)) {
        switch (weapon) {
        case GREAT_SWORD:
        case SWORD_AND_SHIELD:
        case HAMMER:
        case LANCE:
        case LONG_SWORD:
        case SWITCH_AXE:
        case GUNLANCE:
        case DUAL_BLADES:
        case HUNTING_HORN:
        case INSECT_GLAIVE:
        case CHARGE_BLADE:
            context = drawn ? CTX_MELEE_DRAWN : CTX_MELEE_SHEATHED;
            break;
        case HEAVY_BOWGUN:
        case LIGHT_BOWGUN:
            context = drawn ? CTX_BOWGUN_DRAWN : CTX_BOWGUN_SHEATHED;
            break;
        case BOW:
            if (bow_options_ready)
                context = drawn ? CTX_BOW_DRAWN : CTX_BOW_SHEATHED;
            break;
        }
    }

    // Native Y can also sheathe the bow from these two drawn weapon states.
    const bool bow_sheathe = context == CTX_BOW_DRAWN && group == 0x10 && (act == 2 || act == 12);
    const bool accepts_sheathe = (group == 1 && act == 0) || (group == 2 && act <= 1) || bow_sheathe;
    const uint32_t field_buttons = filter_shortcut_wheel(native, target_camera_ready && selected && player);
    const uint32_t mapped = type5_map(&inputs, context, field_buttons, accepts_sheathe);
    static uint32_t last_in, last_out;
    if ((mapped != native || last_in != last_out) && (last_in != native || last_out != mapped)) {
        forge_log(1, "[type5] input %08x -> %08x state=%u", native, mapped, (unsigned)context);
        last_in = native;
        last_out = mapped;
    }
    return original_pad_translate(view, mapped);
}

EXPORT void forge_onUpdate(float delta)
{
    (void)delta;
    if (!original_pad_translate) return;
    static bool logged;
    const uintptr_t player = local_hunter();
    if (player && !logged) {
        const uintptr_t pad_global = word(text + PAD_GLOBAL_GOT);
        const uintptr_t pad = pad_global ? word(pad_global) : 0;
        forge_log(1, "[type5] hunt hook=%u pad hook=%u view=%08x pad=%08x expected=%08x weapon=%u",
                  hunt_updates, pad_updates, (unsigned)last_pad_view, (unsigned)pad,
                  (unsigned)(pad + PAD_GAMEPLAY_VIEW), hunter_weapon(player));
        logged = true;
    }
}

EXPORT void forge_onImGuiRender(void)
{
    if (!igCollapsingHeader_TreeNodeFlags("类型 5 操作###type5_prototype", 0)) return;
    if (!original_pad_translate || !shoulder_lock_ready) {
        igTextUnformatted("初始化未完成。请查看启动日志中的 [type5]。", 0);
        return;
    }
    igTextUnformatted("适用于全部 14 种猎人武器。", 0);
    igTextUnformatted(native_menu_ready ? "也可从原生选项的「操作类型」选择类型 5。" :
                      "使用操作类型 1、弓操作类型 1 和标准摇杆设置。", 0);
    igTextUnformatted("使用标准摇杆设置。启用时锁定肩键交换。", 0);
    if (target_camera_ready) igTextUnformatted("L3 缩放地图，R3 选择大型怪物，L 操作目标视角。", 0);
    bool enabled = type5_debug_enable != 0;
    if (igCheckbox("启用类型 5###type5_enable", &enabled)) {
        type5_debug_enable = enabled;
        refresh_effective_options(type5_selected());
    }
}

EXPORT void forge_onLoad(struct PluginInfo *info)
{
    const char name[] = "Type 5 Controls (MHGU 1.4.0)";
    info->major = 1;
    info->minor = 0;
    info->patch = 0;
    for (unsigned i = 0; i < sizeof(name); ++i) info->name[i] = name[i];
}

EXPORT void forge_onInit(void)
{
    text = forge_mem_getMainTextAddr();
    if (!text || !chainable_entry(text + PAD_TRANSLATE, 0xe92d4800u, 0xe59f0034u,
                                  0xe3a02000u, 0xe79fe000u) ||
        word(text + HUNT_ACTIVE) != 0xe92d4070u ||
        word(text + HUNT_ACTIVE + 4) != 0xe1a05000u ||
        word(text + 0x27ca28) != 0xe92d4bf0u || word(text + 0x5e8060) != 0xe59f005cu) {
        forge_log(2, "[type5] unrecognized startup instructions; no hooks installed");
        if (text) {
            forge_log(2, "[type5] startup words pad=%08x,%08x hunt=%08x,%08x input=%08x option=%08x",
                      word(text + PAD_TRANSLATE), word(text + PAD_TRANSLATE + 4),
                      word(text + HUNT_ACTIVE), word(text + HUNT_ACTIVE + 4),
                      word(text + 0x27ca28), word(text + 0x5e8060));
        }
        return;
    }

    const bool chained_pad = word(text + PAD_TRANSLATE) == 0xe51ff004u;
    void *pad_original = 0;
    void *hunt_original = 0;
    forge_hook_create((void *)(text + HUNT_ACTIVE), (void *)observe_hunt_update, &hunt_original);
    original_hunt_update = (HuntUpdate)hunt_original;
    if (original_hunt_update) {
        forge_hook_create((void *)(text + PAD_TRANSLATE), (void *)translate_gameplay_pad, &pad_original);
        original_pad_translate = (PadTranslate)pad_original;
    }
    if (!original_pad_translate) {
        forge_log(2, "[type5] could not install the gameplay pad hook");
        return;
    }
    if (chained_pad) forge_log(1, "[type5] chained the existing gameplay input hook");
    shoulder_lock_ready = install_shoulder_lock();
    if (!shoulder_lock_ready) {
        forge_log(2, "[type5] could not install the shoulder option lock");
        return;
    }
    native_menu_ready = install_native_menu();
    bow_options_ready = native_menu_ready && install_bow_options();
    target_camera_ready = native_menu_ready && install_target_camera();
    if (target_camera_ready) {
        shortcut_wheel = shortcut_wheel_state();
        if (shortcut_wheel) forge_log(1, "[type5] CraftWheel stick click priority ready");
    }
    forge_log(bow_options_ready ? 1 : 2, "[type5] persistent Bow option reads %s",
              bow_options_ready ? "ready" : "unavailable");
    forge_log(native_menu_ready ? 1 : 2, "[type5] native Type 5 menu %s",
              native_menu_ready ? "ready" : "unavailable");
    forge_log(target_camera_ready ? 1 : 2, "[type5] native R3 target camera and HUD %s",
              target_camera_ready ? "ready" : "unavailable");
    forge_log(1, "[type5] GU 1.4.0 hooks ready; experimental input flag=%08x (off)",
              (unsigned)(uintptr_t)&type5_debug_enable);
}
