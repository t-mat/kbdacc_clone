#include "kbdacc.h"
static const wchar_t appName[] = L"" APPNAME;
static const wchar_t syncMutexName[] = L"" APPNAME L"__synchronize_mutex__";
static const wchar_t globalMutexName[] = L"Global\\" APPNAME;


class DllLoader {
public:
    DllLoader(const wchar_t* filename) {
        hInstance = LoadLibraryW(filename);
    }

    ~DllLoader() {
        if(good()) {
            FreeLibrary(hInstance);
        }
    }

    bool good() const {
        return nullptr != hInstance;
    }

protected:
    HINSTANCE hInstance = nullptr;
};


static void errorDialog(const wchar_t* title, DWORD err, UINT style = MB_OK | MB_ICONINFORMATION) {
    wchar_t* buf = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD langId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
    FormatMessageW(flags, 0, err, langId, reinterpret_cast<LPWSTR>(&buf), 0, 0);
    MessageBoxW(nullptr, buf, title, style);
    LocalFree(buf);
}


#if defined(_WIN64)
class Mutex {
public:
    Mutex(const wchar_t* mutexName, BOOL bInitialOwner) {
        handle = CreateMutexW(0, bInitialOwner, mutexName);
    }

    ~Mutex() {
        if(good()) {
            ReleaseMutex(handle);
        }
    }

    bool good() const {
        return nullptr != handle;
    }

protected:
    HANDLE handle = nullptr;
};


struct NotifyIcon {
    NotifyIcon(HWND hWnd, HICON hIcon, const wchar_t* tipText, UINT uCallbackMessage)
        : hWnd(hWnd)
    {
        notifyIcon(hWnd, hIcon, tipText, true, uCallbackMessage);
    }

    ~NotifyIcon() {
        notifyIcon(hWnd, 0, 0, false, 0);
    }

protected:
    static void notifyIcon(HWND hWnd, HICON icon, const wchar_t* tipText, bool add, UINT uCallbackMessage) {
        NOTIFYICONDATAW nid;
        nid.cbSize             = sizeof(nid);
        nid.hWnd               = hWnd;
        nid.uID                = 1;
        nid.uFlags             = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        nid.uCallbackMessage   = uCallbackMessage;
        nid.hIcon              = icon;
        if(tipText) {
            StringCchCopy(nid.szTip, sizeof(nid.szTip)/sizeof(nid.szTip[0]), tipText);
        }
        if(add) {
            Shell_NotifyIconW(NIM_ADD, &nid);
        } else {
            Shell_NotifyIconW(NIM_DELETE, &nid);
        }
    }

    HWND hWnd = nullptr;
};


static constexpr UINT WM_MY_TASKTRAY    = WM_USER + 1;


static LRESULT wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static constexpr UINT CM_MY_EXIT = 1;

    switch(uMsg) {
    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_COMMAND:
        if(LOWORD(wParam) == CM_MY_EXIT) {
            PostQuitMessage(0);
        }
        return 0;

    case WM_MY_TASKTRAY:
        if(lParam == WM_LBUTTONUP || lParam == WM_RBUTTONUP) {
            SetForegroundWindow(hWnd);
            SetFocus(hWnd);
            POINT p;
            GetCursorPos(&p);

            class PopupMenu {
            public:
                PopupMenu() : hMenu(CreatePopupMenu()) {}
                ~PopupMenu() { DestroyMenu(hMenu); }
                operator HMENU() const { return hMenu; }
            protected:
                HMENU hMenu = nullptr;
            };

            PopupMenu menu;
            AppendMenu(menu, MF_STRING, CM_MY_EXIT, L"Quit");
            return TrackPopupMenu(menu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN, p.x, p.y, 0, hWnd, nullptr);
        }
        return 0;
    }
}


static void winMain64() {
    // Global mutex for keeping single instance.
    Mutex mutex(globalMutexName, FALSE);
    if(! mutex.good()) {
        const DWORD lastError = GetLastError();
        outputDebugString(L"Failed to create global mutex %s\n", globalMutexName);
        errorDialog(appName, lastError);
        return;
    }

    HWND hWnd;
    {
        WNDCLASSW wc = {
            .lpfnWndProc    = wndProc,
            .hInstance      = GetModuleHandle(0),
            .lpszClassName  = appName,
        };
        RegisterClass(&wc);
        hWnd = CreateWindowExW(0, wc.lpszClassName, appName, 0, 0, 0, 0, 0, HWND_MESSAGE, 0, wc.hInstance, 0);
    }

    const wchar_t* myDllName = L".\\" DLL_NAME;
    const DllLoader myDll(myDllName);
    if(! myDll.good()) {
        const DWORD lastError = GetLastError();
        outputDebugString(L"Failed to load %s\n", myDllName);
        errorDialog(appName, lastError);
        return;
    }

    HICON icon = (HICON) LoadImageW(0, L"kbdacc.ico", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
    if(!icon) {
        icon = LoadIcon(0, IDI_APPLICATION);
    }
    const NotifyIcon ni(hWnd, icon, appName, WM_MY_TASKTRAY);

    // Inter process mutex
    //  32-bit version of kbdacc will be waiting for signal of this mutex.
    //  Mutex object is signaled when it is not owned.  In other words, it
    //  will be signaled when 64-bit version of kbdacc process is terminated,
    //  32-bit kbdacc uses this property to wait the termination
    //  of 64-bit app without polling.
    Mutex procMutex(syncMutexName, TRUE);
    if(!procMutex.good()) {
        const DWORD lastError = GetLastError();
        outputDebugString(L"Failed to create syncMutex %s\n", syncMutexName);
        errorDialog(appName, lastError);
        return;
    }

    {   // Invoke 32-bit executable.
        wchar_t imageName[MAX_PATH];
        const DWORD nn = GetModuleFileNameW(nullptr, &imageName[0], MAX_PATH);
        StringCchCopyW(&imageName[nn], MAX_PATH-nn, L"32"); // L"kbdacc.exe32"

        STARTUPINFOW si = { .cb = sizeof(si) };
        PROCESS_INFORMATION pi = { 0 };
        wchar_t* cmd = GetCommandLineW();
        if(! CreateProcessW(imageName, cmd, 0, 0, FALSE, 0, 0, 0, &si, &pi)) {
            const DWORD lastError = GetLastError();
            outputDebugString(L"Failed to invoke %s\n", imageName);
            errorDialog(appName, lastError);
            return;
        }
    }

    SetProcessWorkingSetSize(GetCurrentProcess(), static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1));
    MSG msg;
    while(GetMessage(&msg, 0, 0, 0)) {
        DispatchMessage(&msg);
    }
}


#else // defined(_WIN64)


static void winMain32() {
    const HANDLE h = OpenMutex(SYNCHRONIZE, FALSE, syncMutexName);
    if(!h) {
        const DWORD lastError = GetLastError();
        outputDebugString(L"%s: Failed to open Mutex(%s)\n", L"" APPNAME, syncMutexName);
        errorDialog(appName, lastError);
        return;
    }

    const wchar_t* myDllName = L".\\" DLL_NAME;
    const DllLoader myDll(myDllName);
    if(! myDll.good()) {
        const DWORD lastError = GetLastError();
        outputDebugString(L"Failed to load %s\n", myDllName);
        errorDialog(appName, lastError);
        return;
    }

    SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
    WaitForSingleObject(h, INFINITE);
    ReleaseMutex(h);
}


#endif // defined(_WIN64)


int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
#if defined(_WIN64)
    winMain64();
#else
    winMain32();
#endif
    return 0;
}
