#include <windows.h>
#include <stdbool.h>

static HHOOK g_hook;
static DWORD g_mainTid;

// Windows key codes
#define winlen 67

typedef struct {
    int  code;          // Windows virtual key code
    char normal;        // Character when unshifted
    char shifted;       // Character when shifted
    bool use_caps;      // true if CAPS lock acts like shift for this key
    bool use_numpad;    // true if this key is on the numeric keypad
} winkey_t;

const winkey_t winkeys[winlen] = {
    {   8, '\b', '\b', false, false },  // VK_BACK
    {   9, '\t', '\t', false, false },  // VK_TAB
    {  13, '\n', '\n', false, false },  // VK_RETURN
    {  32, ' ',  ' ',  false, false },  // VK_SPACE
    {  48, '0',  ')',  false, false },  // 0 key
    {  49, '1',  '!',  false, false },  // 1 key
    {  50, '2',  '@',  false, false },  // 2 key
    {  51, '3',  '#',  false, false },  // 3 key
    {  52, '4',  '$',  false, false },  // 4 key
    {  53, '5',  '%',  false, false },  // 5 key
    {  54, '6',  '^',  false, false },  // 6 key
    {  55, '7',  '&',  false, false },  // 7 key
    {  56, '8',  '*',  false, false },  // 8 key
    {  57, '9',  '(',  false, false },  // 9 key
    {  65, 'a',  'A',  true,  false },  // A key
    {  66, 'b',  'B',  true,  false },  // B key
    {  67, 'c',  'C',  true,  false },  // C key
    {  68, 'd',  'D',  true,  false },  // D key
    {  69, 'e',  'E',  true,  false },  // E key
    {  70, 'f',  'F',  true,  false },  // F key
    {  71, 'g',  'G',  true,  false },  // G key
    {  72, 'h',  'H',  true,  false },  // H key
    {  73, 'i',  'I',  true,  false },  // I key
    {  74, 'j',  'J',  true,  false },  // J key
    {  75, 'k',  'K',  true,  false },  // K key
    {  76, 'l',  'L',  true,  false },  // L key
    {  77, 'm',  'M',  true,  false },  // M key
    {  78, 'n',  'N',  true,  false },  // N key
    {  79, 'o',  'O',  true,  false },  // O key
    {  80, 'p',  'P',  true,  false },  // P key
    {  81, 'q',  'Q',  true,  false },  // Q key
    {  82, 'r',  'R',  true,  false },  // R key
    {  83, 's',  'S',  true,  false },  // S key
    {  84, 't',  'T',  true,  false },  // T key
    {  85, 'u',  'U',  true,  false },  // U key
    {  86, 'v',  'V',  true,  false },  // V key
    {  87, 'w',  'W',  true,  false },  // W key
    {  88, 'x',  'X',  true,  false },  // X key
    {  89, 'y',  'Y',  true,  false },  // Y key
    {  90, 'z',  'Z',  true,  false },  // Z key
    { 186, ';',  ':',  false, false },  // VK_OEM_1
    { 187, '=',  '+',  false, false },  // VK_OEM_PLUS
    { 188, ',',  '<',  false, false },  // VK_OEM_COMMA
    { 189, '-',  '_',  false, false },  // VK_OEM_MINUS
    { 190, '.',  '>',  false, false },  // VK_OEM_PERIOD
    { 191, '/',  '?',  false, false },  // VK_OEM_2
    { 192, '`',  '~',  false, false },  // VK_OEM_3
    { 219, '[',  '{',  false, false },  // VK_OEM_4
    { 220, '\\', '|',  false, false },  // VK_OEM_5
    { 221, ']',  '}',  false, false },  // VK_OEM_6
    { 222, '\'', '"',  false, false },  // VK_OEM_7
    {  96, '0',  '0',  false, true  },  // VK_NUMPAD0
    {  97, '1',  '1',  false, true  },  // VK_NUMPAD1
    {  98, '2',  '2',  false, true  },  // VK_NUMPAD2
    {  99, '3',  '3',  false, true  },  // VK_NUMPAD3
    { 100, '4',  '4',  false, true  },  // VK_NUMPAD4
    { 101, '5',  '5',  false, true  },  // VK_NUMPAD5
    { 102, '6',  '6',  false, true  },  // VK_NUMPAD6
    { 103, '7',  '7',  false, true  },  // VK_NUMPAD7
    { 104, '8',  '8',  false, true  },  // VK_NUMPAD8
    { 105, '9',  '9',  false, true  },  // VK_NUMPAD9
    { 106, '*',  '*',  false, true  },  // VK_MULTIPLY
    { 107, '+',  '+',  false, true  },  // VK_ADD
    { 109, '-',  '-',  false, true  },  // VK_SUBTRACT
    { 110, '.',  '.',  false, true  },  // VK_DECIMAL
    { 111, '/',  '/',  false, true  },  // VK_DIVIDE
    { 226, '\\', '|',  false, false },  // VK_OEM_102
};

char winkey_to_char(const winkey_t *k, bool shift, bool caps) {
    if (shift ^ (caps && k->use_caps)) {
        return k->shifted;
    }
    return k->normal;
}

const winkey_t *code_to_winkey(int code) {
    for (int i = 0; i < winlen; i++)
        if (winkeys[i].code == code)
            return &winkeys[i];
    return NULL;
}

#define WM_KEYEVENT (WM_APP + 1)

LRESULT CALLBACK KbProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION) {
        KBDLLHOOKSTRUCT *k = (KBDLLHOOKSTRUCT *)lParam;
        bool down = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        PostThreadMessage(g_mainTid, WM_KEYEVENT, k->vkCode, down);
    }
    return CallNextHookEx(g_hook, code, wParam, lParam);
}

static void put_char(char c) {
    DWORD written;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), &c, 1, &written, NULL);
}

void _start(void) {
    g_mainTid = GetCurrentThreadId();
    g_hook = SetWindowsHookExA(WH_KEYBOARD_LL, KbProc, GetModuleHandle(NULL), 0);
    if (!g_hook) {
        ExitProcess(1);
    }
    MSG msg;
    bool shift = false;
    bool caps_lock = false;
    bool num_lock = false;

    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.hwnd == NULL && msg.message == WM_KEYEVENT) {
            // Set shift
            if (msg.wParam == VK_LSHIFT || msg.wParam == VK_RSHIFT || msg.wParam == VK_SHIFT) {
                shift = msg.lParam;
            }
            // Set caps lock (trigger only on press)
            else if (msg.wParam == VK_CAPITAL) {
                if (msg.lParam == 1) caps_lock = !caps_lock;
            }
            // Set num lock (trigger only on press)
            else if (msg.wParam == VK_NUMLOCK) {
                if (msg.lParam == 1) num_lock = !num_lock;
            }
            // Handle all other keycodes
            else {
                const winkey_t *key = code_to_winkey(msg.wParam);
                if (key && msg.lParam == 1) {
                    if (key->use_numpad && !num_lock) continue;
                    put_char(winkey_to_char(key, shift, caps_lock));
                }
            }
        }
    }
    UnhookWindowsHookEx(g_hook);
    ExitProcess(0);
}