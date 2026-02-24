#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <random>

static HHOOK g_hook = nullptr;
static std::mt19937 g_rng{ std::random_device{}() };

static bool isLetter(DWORD vk) { return vk >= 'A' && vk <= 'Z'; }
static bool isDigit(DWORD vk) { return vk >= '0' && vk <= '9'; }

static WORD randomLetter() {
    std::uniform_int_distribution<int> dist('A', 'Z');
    return static_cast<WORD>(dist(g_rng));
}

static WORD randomDigit() {
    std::uniform_int_distribution<int> dist('0', '9');
    return static_cast<WORD>(dist(g_rng));
}

static WORD randomAlphaNum() {
    std::uniform_int_distribution<int> pick(0, 1);
    return (pick(g_rng) == 0) ? randomLetter() : randomDigit();
}

static void sendKeyTap(WORD vk)
{
    INPUT inputs[2]{};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vk;

    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = vk;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(2, inputs, sizeof(INPUT));
}

static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        const KBDLLHOOKSTRUCT* k = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

        if (k->flags & LLKHF_INJECTED)
            return CallNextHookEx(g_hook, nCode, wParam, lParam);

        const DWORD vk = k->vkCode;
        const bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        const bool isUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);

        if (isDown && vk == VK_F11) {
            PostQuitMessage(0);
            return 1;
        }

        if (isDown)
        {
            if (isLetter(vk)) { sendKeyTap(randomLetter()); return 1; }
            if (isDigit(vk)) { sendKeyTap(randomDigit());  return 1; }

            sendKeyTap(randomAlphaNum());
            return 1;
        }

        if (isUp) return 1;
    }

    return CallNextHookEx(g_hook, nCode, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    g_hook = SetWindowsHookExW(
        WH_KEYBOARD_LL,
        LowLevelKeyboardProc,
        GetModuleHandleW(nullptr),
        0
    );

    if (!g_hook) return 1;

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    UnhookWindowsHookEx(g_hook);
    return 0;
}