#include "kbdacc.h"
#pragma comment(lib, "winmm.lib")

struct KbdAccConfig {
    int delay  = 200;   // Repeat delay in milliseconds
    int repeat = 10;    // Repeat rate in milliseconds
};

static int getEnvironmentVariableAsInt(const char* name, int defaultValue) {
    char buf[256] = { 0 };
    const DWORD len = GetEnvironmentVariableA(name, buf, (DWORD) sizeof(buf));
    if(len == 0) {
        return defaultValue;
    }
    static const auto my_atoi = [](const char* p) -> int {
        int x = 0;
        for(;;) {
            const int c = (*p++) - '0';
            if(c < 0 || c >= 10) {
                break;
            }
            x = (x * 10) + c;
        }
        return x;
    };
    return my_atoi(buf);
}


struct CriticalSection {
    CriticalSection() { InitializeCriticalSection(&cs); }
    ~CriticalSection() { DeleteCriticalSection(&cs); }
    void enter() { EnterCriticalSection(&cs); }
    void leave() { LeaveCriticalSection(&cs); }
protected:
    CRITICAL_SECTION cs;
};


struct CriticalSectionBlock {
    CriticalSectionBlock(CriticalSection& cs) : cs(cs) { cs.enter(); }
    ~CriticalSectionBlock() { cs.leave(); }
protected:
    CriticalSection& cs;
};


struct Shared {
    struct GetMessageLlHook {
        LONG    globalCounter;
        HHOOK   hook;
    } getMessageLlHook;
};

#pragma data_seg(".shared") // FIXME
volatile Shared shared_ = {
    { -1, nullptr },
};
#pragma data_seg()
#pragma comment(linker, "/SECTION:.shared,RWS")

static volatile Shared* shared = &shared_;


struct Entry {
    void init() {
        uTimerId = TIMER_ID_UNUSED;
    }

    void set(const MSG* pMsg, UINT wDelay, UINT wResolution, LPTIMECALLBACK lptc, DWORD dwUser, UINT fuCallbackType) {
        if(isUsed()) {
            return;
        }
        msg = *pMsg;
        msg.lParam |= 0x40000000;
        threadId = GetCurrentThreadId();
        uTimerId = timeSetEvent(wDelay, wResolution, lptc, dwUser, fuCallbackType);
    }

    void unset() {
        if(isUsed()) {
            timeKillEvent(uTimerId);
            uTimerId = TIMER_ID_UNUSED;
        }
    }

    void forceUnset() {
        uTimerId = TIMER_ID_UNUSED;
    }

    BOOL postMessage() const {
        return PostMessage(msg.hwnd, msg.message, msg.wParam, msg.lParam);
    }

    bool isUsed() const {
        return TIMER_ID_UNUSED != uTimerId;
    }

    bool checkThread(DWORD currentThreadId) const {
        return currentThreadId == threadId;
    }

    DWORD getVirtualKey() const {
        return static_cast<DWORD>(msg.wParam);
    }

protected:
    MSG         msg = { 0 };
    DWORD       threadId = 0;
    MMRESULT    uTimerId = TIMER_ID_UNUSED;

    const DWORD TIMER_ID_UNUSED = 0;
};


class EntryPool {
public:
    EntryPool() {
        init();
    }

    ~EntryPool() {
        cleanup();
    }

    void init() {
        const CriticalSectionBlock csb(cs);
        for(Entry& e : entryArray) {
            e.init();
        }
    }

    void cleanup() {
        const CriticalSectionBlock csb(cs);
        for(Entry& e : entryArray) {
            e.unset();
        }
    }

    Entry* setEntry(const MSG* pMsg, UINT wDelay, LPTIMECALLBACK lptc) {
        if(wDelay) {
            const CriticalSectionBlock csb(cs);
            for(size_t i = 0; i < numEntryArray; ++i) {
                Entry* p = &entryArray[i];
                if(!p->isUsed()) {
                    p->set(pMsg, wDelay, wDelay, lptc, static_cast<DWORD>(i), TIME_ONESHOT);
                    return p;
                }
            }
        }
        return nullptr;
    }

    Entry* getEntryByIndex(int index) {
        if((index < 0) || (index >= numEntryArray)) {
            return nullptr;
        }
        return &entryArray[index];
    }

protected:
    static const int numEntryArray = 32;
    Entry entryArray[numEntryArray];
    CriticalSection cs;
};


struct KbdAcc {
    static void attach(HINSTANCE hInstDll) {
        if(! instance) {
            instance = new KbdAcc;
            instance->init(hInstDll);
        }
    }

    static void detach() {
        if(instance) {
            KbdAcc* p = instance;
            instance = nullptr;
            p->cleanup();
            delete p;
        }
    }

protected:
    void init(HINSTANCE hInstDll) {
        entryPool.init();

        cfg.delay  = getEnvironmentVariableAsInt("KBDACC_DELAY", 200);
        cfg.repeat = getEnvironmentVariableAsInt("KBDACC_REPEAT", 10);

        auto* s = &shared->getMessageLlHook;
        const LONG k = InterlockedIncrement(&s->globalCounter);
        if(0 == k && nullptr == s->hook) {
            s->hook = SetWindowsHookEx(WH_GETMESSAGE, static_GetMsgProc, hInstDll, 0);
            if(! s->hook) {
                const DWORD lastError = GetLastError();
                outputDebugString(L"" DLL_NAME L": SetWindowsHookEx() Install Error (lastError=0x%08x)", lastError);
            }
        }
    }

    void cleanup() {
        entryPool.cleanup();

        auto* s = &shared->getMessageLlHook;
        const LONG k = InterlockedDecrement(&s->globalCounter);
        if(0 > k && s->hook) {
            UnhookWindowsHookEx(s->hook);
            s->hook = nullptr;
        }
        PostMessage(HWND_BROADCAST, 0, 0, 0);
    }

    void timerCallback(UINT, UINT, DWORD_PTR dwUser, DWORD_PTR, DWORD_PTR) {
        const auto index = static_cast<int>(dwUser);
        if(Entry* p = entryPool.getEntryByIndex(index)) {
            p->forceUnset();
            const CriticalSectionBlock tcs(timerCs);
            if(const HWND fw = GetForegroundWindow()) {
                if(p->checkThread(GetWindowThreadProcessId(fw, 0))) {
                    const short ks = GetAsyncKeyState(p->getVirtualKey());
                    if(ks < 0) {
                        if(0 == GetQueueStatus(QS_KEY)) {
                            p->postMessage();
                        }
                    }
                }
            }
        }
    }

    static void CALLBACK lptc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2) {
        if(instance) {
            instance->timerCallback(uTimerID, uMsg, dwUser, dw1, dw2);
        }
    }

    void onKeyDown(const MSG* msg) {
        entryPool.cleanup();

        switch(msg->wParam) {
        case VK_SHIFT:      // 0x10
        case VK_CONTROL:    // 0x11
        case VK_MENU:       // 0x12
        case VK_OEM_COPY:   // 0xf2 Japanese keyboard "KATAKANA, HIRAGANA, ROMAJI" key
            break;

        default:
            if(0 == (msg->lParam & 0x40000000)) {
                //  first stroke
                entryPool.setEntry(msg, cfg.delay, lptc);
            } else {
                //  repeat
                entryPool.setEntry(msg, cfg.repeat, lptc);
            }
            break;
        }
    }

    static LRESULT CALLBACK static_GetMsgProc(int code, WPARAM wParam, LPARAM lParam) {
        if(HC_ACTION == code && PM_REMOVE == wParam && instance) {
            instance->getMsgProc(code, wParam, lParam);
        }

        // note: The first argument of CallNextHookEx() is ignored.
        // http://msdn.microsoft.com/en-us/library/ms644974.aspx
        LRESULT r = CallNextHookEx(0, code, wParam, lParam);
        if(code < 0) {
            r = 0;
        }
        return r;
    }

    LRESULT getMsgProc(int, WPARAM, LPARAM lParam) {
        const auto* msg = reinterpret_cast<const MSG*>(lParam);
        switch(msg->message) {
        default:
            break;

        case WM_KEYUP:
        case WM_SYSKEYUP:
            entryPool.cleanup();
            break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            onKeyDown(msg);
            break;
        }
        return 0;
    }

    EntryPool       entryPool;
    CriticalSection timerCs;
    KbdAccConfig    cfg;
    inline static KbdAcc* instance;
};


BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID) {
    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        KbdAcc::attach(hInstDll);
        break;

    case DLL_PROCESS_DETACH:
        KbdAcc::detach();
        break;

    default:
        break;
    }
    return TRUE;
}
