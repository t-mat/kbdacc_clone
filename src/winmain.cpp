#include "kbdacc.h"
static const wchar_t appName[] = APPNAME_L;
static const wchar_t syncMutexName[] = APPNAME_L L"__synchronize_mutex__";


class DllLoader {
public:
	DllLoader(const wchar_t* filename) { load(filename); }

	~DllLoader() { unload(); }

	bool good() const { return nullptr != hInstance; }

	void* getProcAddress(const char* procName) const {
		if(!good()) {
			return nullptr;
		}
		return GetProcAddress(hInstance, procName);
	}

protected:
	bool load(const wchar_t* filename) {
		hInstance = LoadLibraryW(filename);
		return good();
	}

	void unload() {
		if(good()) {
			FreeLibrary(hInstance);
			hInstance = nullptr;
		}
	}

	HINSTANCE hInstance = nullptr;
};


inline std::unique_ptr<wchar_t, std::function<void(wchar_t*)>> makeErrorString(DWORD dwError) {
	wchar_t* buf = nullptr;
	FormatMessageW(
		  FORMAT_MESSAGE_ALLOCATE_BUFFER
		| FORMAT_MESSAGE_FROM_SYSTEM
		| FORMAT_MESSAGE_IGNORE_INSERTS
		, 0
		, dwError
		, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)
		, reinterpret_cast<LPWSTR>(&buf), 0, 0
	);
	return { buf, [](auto* p) { LocalFree(p); } };
}

inline void errorDialog(const wchar_t* title, const wchar_t* text, UINT style = MB_OK | MB_ICONINFORMATION) {
	MessageBoxW(nullptr, text, title, style);
}

inline void errorDialog(const wchar_t* title, DWORD err, UINT style = MB_OK | MB_ICONINFORMATION) {
	auto e = makeErrorString(err);
	errorDialog(title, e.get(), style);
}


#if defined(_WIN64)
class Mutex {
public:
	Mutex(const wchar_t* mutexName, BOOL fInitialOwner = FALSE) {
	//	create(mutexName, fInitialOwner);
//		if(!good()) {
			handle = CreateMutexW(0, fInitialOwner, mutexName);
//		}
//		return ERROR_ALREADY_EXISTS != GetLastError();
	}

	~Mutex() {
		if(good()) {
			ReleaseMutex(handle);
			handle = nullptr;
		}
	}

//	bool create(const wchar_t* mutexName, BOOL fInitialOwner = FALSE) {
//		if(!good()) {
//			handle = CreateMutexW(0, fInitialOwner, mutexName);
//		}
//		return ERROR_ALREADY_EXISTS != GetLastError();
//	}

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

	static void
	notifyIcon(HWND hwnd, HICON icon, const wchar_t* tipText, bool add, UINT uCallbackMessage) {
		NOTIFYICONDATAW nid {
			.cbSize				= sizeof(nid),
			.hWnd				= hwnd,
			.uID				= 1,
			.uFlags				= NIF_MESSAGE | NIF_ICON | NIF_TIP,
			.uCallbackMessage	= uCallbackMessage,
			.hIcon				= icon,
		};
		if(tipText) {
			StringCchCopy(nid.szTip, std::size(nid.szTip), tipText);
		}
		if(add) {
			Shell_NotifyIconW(NIM_ADD, &nid);
		} else {
			Shell_NotifyIconW(NIM_DELETE, &nid);
		}
	}

protected:
	HWND hWnd = nullptr;
};


static constexpr UINT WM_MY_TASKTRAY	= WM_USER + 1;
static constexpr UINT CM_MY_EXIT		= 1;


static LRESULT wndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);

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
			SetForegroundWindow(hwnd);
			SetFocus(hwnd);
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
			return TrackPopupMenu(menu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN, p.x, p.y, 0, hwnd, nullptr);
		}
		return 0;
	}
}


int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	static const wchar_t mutexName[] = L"Global\\" APPNAME;
	Mutex mutex(mutexName);
	if(! mutex.good()) { return 0; }

	const HWND hwnd = [&]() {
		WNDCLASSW wc = { 0 };
		wc.lpfnWndProc		= wndProc;
		wc.hInstance		= GetModuleHandle(0);
		wc.lpszClassName	= appName;
		RegisterClass(&wc);
		return CreateWindowExW(0, wc.lpszClassName, appName, 0, 0, 0, 0, 0
			, HWND_MESSAGE, 0, wc.hInstance, 0);
	}();

	const wchar_t* myDllName = L".\\" DLL_NAME;
	const DllLoader myDll(myDllName);
	if(! myDll.good()) {
		const DWORD lastError = GetLastError();
		outputDebugString(L"Failed to load %s\n", myDllName);
		errorDialog(appName, lastError);
		return 0;
	}

	const NotifyIcon ni(hwnd, LoadIcon(0, IDI_APPLICATION), appName, WM_MY_TASKTRAY);

	SetProcessWorkingSetSize(GetCurrentProcess(), static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1));

	Mutex procMutex(syncMutexName);
	if(!procMutex.good()) { return 0; }

	{
		wchar_t imageName[MAX_PATH] = { 0 };
		const DWORD nn = GetModuleFileNameW(nullptr, &imageName[0], static_cast<DWORD>(std::size(imageName)));
		wcscpy_s(&imageName[nn], std::size(imageName)-nn, L"32");		// L"kbdacc.exe32"

		STARTUPINFOW si = { .cb = sizeof(si) };
		PROCESS_INFORMATION pi = { 0 };
		wchar_t cmd[1] = { 0 };
		if(! CreateProcessW(imageName, cmd, 0, 0, FALSE, 0, 0, 0, &si, &pi)) {
			outputDebugString(L"Failed to invoke %s\n", imageName);
			return 0;
		}
	}

	MSG msg = { 0 };
	while(GetMessage(&msg, 0, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}


#else // defined(_WIN64)


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	const HANDLE h = OpenMutex(SYNCHRONIZE, FALSE, syncMutexName);
	if(!h) {
		const DWORD lastError = GetLastError();
		outputDebugString(L"%s: Failed to open Mutex(%s)\n", APPNAME_L, syncMutexName);
		errorDialog(appName, lastError);
		return 0;
	}

	const wchar_t* myDllName = L".\\" DLL_NAME;
	const DllLoader myDll(myDllName);
	if(! myDll.good()) {
		const DWORD lastError = GetLastError();
		outputDebugString(L"Failed to load %s\n", myDllName);
		errorDialog(appName, lastError);
		return 0;
	}

	SetProcessWorkingSetSize(GetCurrentProcess(), (SIZE_T)-1, (SIZE_T)-1);
	WaitForSingleObject(h, INFINITE);
	ReleaseMutex(h);
	return 0;
}


#endif // defined(_WIN64)
