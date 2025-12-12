#include "Headers/apps.h"
#include <Windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <filesystem>
#include <propsys.h> // Để dùng IPropertyStore
#include <propkey.h> // Để dùng PKEY_AppUserModel_ID
#pragma comment(lib, "propsys.lib") // Link thư viện
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
// =====================================================================
// UWP WORKER - QUÉT APP TRÊN LUỒNG RIÊNG (TRÁNH LỖI COM/NETWORK)
// =====================================================================
#include <thread>
#include <mutex>
#include <atomic>

// Hàm nội bộ: Thực hiện quét trong môi trường an toàn
void FindUwpAppWorker(std::wstring keyword, std::wstring* outAumid) {
    HRESULT hrInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    IShellItem* pAppsFolder = nullptr;
    // Lấy folder chứa Apps
    HRESULT hr = SHGetKnownFolderItem(FOLDERID_AppsFolder, KF_FLAG_DEFAULT, NULL, IID_PPV_ARGS(&pAppsFolder));

    if (SUCCEEDED(hr)) {
        IEnumShellItems* pEnum = nullptr;
        hr = pAppsFolder->BindToHandler(NULL, BHID_EnumItems, IID_PPV_ARGS(&pEnum));

        if (SUCCEEDED(hr)) {
            IShellItem* pItem = nullptr;
            ULONG fetched = 0;
            std::wstring keyNorm = NormalizeKey(ToLowerW(keyword));

            while (pEnum->Next(1, &pItem, &fetched) == S_OK) {
                // 1. Lấy Tên hiển thị (Display Name) để so sánh
                LPWSTR pszName = nullptr;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_NORMALDISPLAY, &pszName))) {
                    std::wstring displayName = pszName;
                    std::wstring displayNorm = NormalizeKey(ToLowerW(displayName));
                    CoTaskMemFree(pszName);

                    // 2. Nếu tên khớp với từ khóa
                    if (displayNorm.find(keyNorm) != std::wstring::npos) {
                        
                        // --- SỬA LỖI TẠI ĐÂY: Dùng Property Store để lấy ID chuẩn ---
                        IPropertyStore* pStore = nullptr;
                        // Hỏi xin Property Store của item này
                        if (SUCCEEDED(pItem->BindToHandler(NULL, BHID_PropertyStore, IID_PPV_ARGS(&pStore)))) {
                            PROPVARIANT pv;
                            PropVariantInit(&pv);

                            // Lấy giá trị PKEY_AppUserModel_ID (Đây là ID chuẩn để mở app)
                            if (SUCCEEDED(pStore->GetValue(PKEY_AppUserModel_ID, &pv))) {
                                if (pv.vt == VT_LPWSTR && pv.pwszVal != nullptr) {
                                    *outAumid = pv.pwszVal; // Gán ID tìm được
                                }
                                PropVariantClear(&pv);
                            }
                            pStore->Release();
                        }
                        // -----------------------------------------------------------

                        // Nếu đã tìm thấy ID thì thoát vòng lặp ngay
                        if (!outAumid->empty()) {
                            pItem->Release();
                            break; 
                        }
                    }
                }
                pItem->Release();
            }
            pEnum->Release();
        }
        pAppsFolder->Release();
    }

    if (SUCCEEDED(hrInit)) CoUninitialize();
}

bool StartUwpByName(const std::wstring& keywordW) {
    std::wstring foundAumid = L"";
    
    // --- TẠO LUỒNG RIÊNG ĐỂ QUÉT ---
    // Việc này đảm bảo CoInitialize không bị lỗi do luồng mạng gây ra
    std::thread t(FindUwpAppWorker, keywordW, &foundAumid);
    
    if (t.joinable()) t.join(); // Chờ luồng quét xong

    // Nếu tìm thấy ID
    if (!foundAumid.empty()) {
        std::wstring appUri = L"shell:AppsFolder\\" + foundAumid;
        
        // Debug
        // std::wcout << L"[UWP OPEN] " << appUri << std::endl;

        HINSTANCE h = ShellExecuteW(NULL, L"open", appUri.c_str(), NULL, NULL, SW_SHOWNORMAL);
        return ((intptr_t)h > 32);
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

    // 2. .lnk (Shortcut)
    std::wstring shortcut = FindShortcutPath(wApp);
    if (!shortcut.empty()) {
        HINSTANCE r2 = ShellExecuteW(NULL, L"open", shortcut.c_str(), NULL, NULL, SW_SHOWNORMAL);
        if ((intptr_t)r2 > 32) return true;
    }

    // 3. UWP (Universal App) - Giờ đã hoạt động ổn định
    if (StartUwpByName(wApp)) return true;

    std::cout << "[ERROR] Không tìm thấy ứng dụng: " << appName << std::endl;
    return false;
}
