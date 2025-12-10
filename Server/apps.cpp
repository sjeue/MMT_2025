#include "Headers/apps.h"
#include <Windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

// =====================================================================
// UTF-8 <-> UTF-16
// =====================================================================
std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return {};

    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wstr.size(), NULL, 0, NULL, NULL);
    std::string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wstr.size(), result.data(), size, NULL, NULL);
    return result;
}

std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return {};

    int size = MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), NULL, 0);
    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), str.size(), result.data(), size);
    return result;
}

std::wstring ToLowerW(const std::wstring& s) {
    std::wstring r = s;
    std::transform(r.begin(), r.end(), r.begin(), ::towlower);
    return r;
}
std::wstring NormalizeKey(const std::wstring& s) {
    std::wstring r;
    r.reserve(s.size());

    for (wchar_t c : s) {
        switch (c) {
        case L'á': case L'à': case L'ả': case L'ã': case L'ạ':
        case L'ă': case L'ắ': case L'ằ': case L'ẳ': case L'ẵ': case L'ặ':
        case L'â': case L'ấ': case L'ầ': case L'ẩ': case L'ẫ': case L'ậ':
            r += L'a'; break;

        case L'é': case L'è': case L'ẻ': case L'ẽ': case L'ẹ':
        case L'ê': case L'ế': case L'ề': case L'ể': case L'ễ': case L'ệ':
            r += L'e'; break;

        case L'í': case L'ì': case L'ỉ': case L'ĩ': case L'ị':
            r += L'i'; break;

        case L'ó': case L'ò': case L'ỏ': case L'õ': case L'ọ':
        case L'ô': case L'ố': case L'ồ': case L'ổ': case L'ỗ': case L'ộ':
        case L'ơ': case L'ớ': case L'ờ': case L'ở': case L'ỡ': case L'ợ':
            r += L'o'; break;

        case L'ú': case L'ù': case L'ủ': case L'ũ': case L'ụ':
        case L'ư': case L'ứ': case L'ừ': case L'ử': case L'ữ': case L'ự':
            r += L'u'; break;

        case L'ý': case L'ỳ': case L'ỷ': case L'ỹ': case L'ỵ':
            r += L'y'; break;

        case L'đ':
            r += L'd'; break;

        default:
            r += c;
        }
    }

    return r;
}

// =====================================================================
// ENUM WINDOWS — for PID & listApps()
// =====================================================================
struct FindWindowData { std::wstring keyword; int pid; };

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    json* arr = (json*)lParam;
    if (!IsWindowVisible(hwnd)) return TRUE;

    wchar_t title[512];
    GetWindowTextW(hwnd, title, 512);
    if (wcslen(title) == 0) return TRUE;

    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);

    arr->push_back({
        {"PID", (int)pid},
        {"Window Name", WStringToString(title)}
    });

    return TRUE;
}

json listApps() {
    json arr = json::array();
    EnumWindows(EnumWindowsProc, (LPARAM)&arr);
    return arr;
}

BOOL CALLBACK FindAppProc(HWND hwnd, LPARAM lParam) {
    FindWindowData* data = (FindWindowData*)lParam;

    if (!IsWindowVisible(hwnd)) return TRUE;

    wchar_t title[512];
    GetWindowTextW(hwnd, title, 512);

    std::wstring low = ToLowerW(title);
    if (low.find(data->keyword) != std::wstring::npos) {
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);

        data->pid = pid;
        return FALSE;
    }

    return TRUE;
}

int getAppPIDByName(const std::string& name) {
    FindWindowData data;
    data.keyword = ToLowerW(StringToWString(name));
    data.pid = 0;

    EnumWindows(FindAppProc, (LPARAM)&data);
    return data.pid;
}

bool killAppByID(int pid) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProcess) return false;

    BOOL ok = TerminateProcess(hProcess, 1);
    CloseHandle(hProcess);
    return ok;
}

// =====================================================================
// SMART SEARCH — Find .lnk in Start Menu & Desktop
// =====================================================================
std::vector<std::wstring> GetSearchPaths() {
    std::vector<std::wstring> v;
    wchar_t p[MAX_PATH];

    // Start Menu Current User
    SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, p);
    v.push_back(p);

    // Start Menu All Users
    SHGetFolderPathW(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, p);
    v.push_back(p);

    // Desktop
    SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, p);
    v.push_back(p);

    return v;
}

std::wstring FindShortcutPath(const std::wstring& keyword) {
    std::wstring keyLower = ToLowerW(keyword);

    for (auto folder : GetSearchPaths()) {
        if (!fs::exists(folder)) continue;
        for (auto& entry : fs::recursive_directory_iterator(folder)) {
            if (!entry.is_regular_file()) continue;

            std::wstring fname = ToLowerW(entry.path().filename().wstring());

            if (fname.find(keyLower) != std::wstring::npos) {
                return entry.path().wstring();
            }
        }
    }
    return L"";
}
struct UwpItem {
    std::wstring displayName;
    std::wstring appUserModelID;
};
std::vector<UwpItem> GetAllUwpApps() {
    std::vector<UwpItem> list;

    IShellItem* pAppsFolder = nullptr;
    HRESULT hr = SHGetKnownFolderItem(FOLDERID_AppsFolder, KF_FLAG_DEFAULT, NULL, IID_PPV_ARGS(&pAppsFolder));
    if (FAILED(hr)) return list;

    IEnumShellItems* pEnum = nullptr;
    hr = pAppsFolder->BindToHandler(NULL, BHID_EnumItems, IID_PPV_ARGS(&pEnum));
    if (FAILED(hr)) {
        pAppsFolder->Release();
        return list;
    }

    IShellItem* pItem = nullptr;
    ULONG fetched = 0;

    while (pEnum->Next(1, &pItem, &fetched) == S_OK) {

        UwpItem app;

        // Display Name
        LPWSTR pszName = nullptr;
        if (SUCCEEDED(pItem->GetDisplayName(SIGDN_NORMALDISPLAY, &pszName))) {
            app.displayName = pszName;
            CoTaskMemFree(pszName);
        }

        // AppUserModelID (parsing name)
        LPWSTR pszParse = nullptr;
        if (SUCCEEDED(pItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pszParse))) {
            std::wstring fullPath = pszParse;
            CoTaskMemFree(pszParse);

            size_t pos = fullPath.find(L"shell:AppsFolder\\");
            if (pos != std::wstring::npos) {
                app.appUserModelID = fullPath.substr(pos + 17);
                list.push_back(app);
                std::wcout << L"[UWP] " << app.displayName << L"  ->  " << app.appUserModelID << std::endl;

            }
        }
        pItem->Release();
    }

    pEnum->Release();
    pAppsFolder->Release();
    return list;
}
bool StartUwpByName(const std::wstring& keywordW) {
    auto apps = GetAllUwpApps();
    std::wstring keyLower = ToLowerW(keywordW);

    for (auto& app : apps) {
        std::wstring appNameLower = ToLowerW(app.displayName);

        // Tìm tên hiển thị
        if (
    NormalizeKey(appNameLower).find(NormalizeKey(keyLower)) != std::wstring::npos ||
    ToLowerW(app.appUserModelID).find(keyLower) != std::wstring::npos
) {


            // CHỈNH Ở ĐÂY — gọi trực tiếp App URI
            std::wstring appUri = L"shell:AppsFolder\\" + app.appUserModelID;

            HINSTANCE h = ShellExecuteW(
                NULL,
                L"open",
                appUri.c_str(),   // target = chính appUri
                NULL,
                NULL,
                SW_SHOWNORMAL
            );

            return ((intptr_t)h > 32);
        }
    }
    return false;
}




// =====================================================================
// START APP — CÁCH 1 (STABLE)
// =====================================================================
bool startApp(const std::string& appName) {
    std::wstring wApp = StringToWString(appName);

    // 1. Direct EXE
    HINSTANCE r = ShellExecuteW(NULL, L"open", wApp.c_str(), NULL, NULL, SW_SHOWNORMAL);
    if ((intptr_t)r > 32) return true;

    // 2. .lnk
    std::wstring shortcut = FindShortcutPath(wApp);
    if (!shortcut.empty()) {
        HINSTANCE r2 = ShellExecuteW(NULL, L"open", shortcut.c_str(), NULL, NULL, SW_SHOWNORMAL);
        if ((intptr_t)r2 > 32) return true;
    }

    // 3. UWP
    if (StartUwpByName(wApp)) return true;

    std::cout << "[ERROR] Không tìm thấy ứng dụng: " << appName << std::endl;
    return false;
}
