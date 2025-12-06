#include "Headers/apps.h"
#include <Windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <propvarutil.h> 
#include <vector>
#include <algorithm>
#include <string>
#include <iostream>
#include <filesystem> 

#include <nlohmann/json.hpp>
using json = nlohmann::json;

// =============================================================
// HELPER FUNCTIONS
// =============================================================

std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::wstring ToLowerW(const std::wstring& str) {
    std::wstring result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::towlower);
    return result;
}

// =============================================================
// CÁC HÀM QUẢN LÝ PROCESS 
// =============================================================

// Cấu trúc dữ liệu để truyền vào callback tìm kiếm cửa sổ
struct FindWindowData { 
    std::wstring keywordW; 
    int pid; 
};

// Callback để liệt kê danh sách cửa sổ đang mở
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    json* jsonArray = (json*)lParam;
    wchar_t title[512];
    if (IsWindowVisible(hwnd)) {
        GetWindowTextW(hwnd, title, 512);
        std::wstring wTitle(title);
        if (wTitle.length() > 0) {
            DWORD pid;
            GetWindowThreadProcessId(hwnd, &pid);
            std::string utf8Title = WStringToString(wTitle);
            // Thêm vào danh sách JSON
            jsonArray->push_back({ {"PID", pid}, {"Window Name", utf8Title} });
        }
    }
    return TRUE;
}

// Lấy danh sách ứng dụng đang chạy 
json listApps() {
    json appList = json::array();
    EnumWindows(EnumWindowsProc, (LPARAM)&appList);
    return appList;
}

// Callback để tìm PID theo tên
BOOL CALLBACK FindAppProc(HWND hwnd, LPARAM lParam) {
    FindWindowData* data = (FindWindowData*)lParam;
    wchar_t title[512];
    if (IsWindowVisible(hwnd)) {
        GetWindowTextW(hwnd, title, 512);
        std::wstring strTitle(title);
        if (strTitle.length() > 0) {
            std::wstring strLower = ToLowerW(strTitle);
            // Tìm tên chứa từ khóa
            if (strLower.find(data->keywordW) != std::wstring::npos) {
                DWORD pid;
                GetWindowThreadProcessId(hwnd, &pid);
                data->pid = (int)pid;
                return FALSE; // Tìm thấy thì dừng
            }
        }
    }
    return TRUE;
}

// Lấy PID theo tên 
int getAppPIDByName(const std::string& appName) {
    std::wstring wKeyword = StringToWString(appName);
    FindWindowData data;
    data.keywordW = ToLowerW(wKeyword); 
    data.pid = 0;
    EnumWindows(FindAppProc, (LPARAM)&data);
    return data.pid;
}

// Tắt ứng dụng theo ID 
bool killAppByID(int pid) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess == NULL) return false;
    BOOL result = TerminateProcess(hProcess, 1);
    CloseHandle(hProcess);
    return result != 0;
}

// =============================================================
// WINDOWS SEARCH
// =============================================================

// Hàm tìm kiếm trong thư mục AppsFolder ảo
bool RunAppFromAppsFolder(const std::wstring& keyword) {
    HRESULT hr = CoInitialize(NULL);
    
    IShellItem* pAppsFolder = NULL;
    hr = SHGetKnownFolderItem(FOLDERID_AppsFolder, KF_FLAG_DEFAULT, NULL, IID_PPV_ARGS(&pAppsFolder));

    bool found = false;

    if (SUCCEEDED(hr)) {
        IEnumShellItems* pEnum = NULL;
        hr = pAppsFolder->BindToHandler(NULL, BHID_EnumItems, IID_PPV_ARGS(&pEnum));

        if (SUCCEEDED(hr)) {
            IShellItem* pItem = NULL;
            ULONG fetched;
            std::wstring keywordLower = ToLowerW(keyword);

            while (pEnum->Next(1, &pItem, &fetched) == S_OK && !found) {
                LPWSTR pszName = NULL;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_NORMALDISPLAY, &pszName))) {
                    std::wstring appName(pszName);
                    std::wstring appNameLower = ToLowerW(appName);
                    CoTaskMemFree(pszName); 

                    if (appNameLower.find(keywordLower) != std::wstring::npos) {
                        std::wcout << L"[Windows Search] Found: " << appName << std::endl;
                        LPWSTR pszParseName = NULL;
                        if (SUCCEEDED(pItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pszParseName))) {
                            ShellExecuteW(NULL, L"open", pszParseName, NULL, NULL, SW_SHOWNORMAL);
                            CoTaskMemFree(pszParseName);
                            found = true;
                        }
                    }
                }
                pItem->Release();
            }
            pEnum->Release();
        }
        pAppsFolder->Release();
    }
    CoUninitialize(); 
    return found;
}

// Mở ứng dụng 
bool startApp(const std::string& appName) {
    std::wstring wAppName = StringToWString(appName);
    
    // Ưu tiên chạy lệnh trực tiếp 
    HINSTANCE result = ShellExecuteW(NULL, L"open", wAppName.c_str(), NULL, NULL, SW_SHOWNORMAL);
    if ((intptr_t)result > 32) return true;

    // Fallback: Quét hệ thống 
    std::cout << "Dang quet he thong (AppsFolder) tim: " << appName << "..." << std::endl;
    if (RunAppFromAppsFolder(wAppName)) {
        return true;
    }

    std::cout << "Khong tim thay ung dung nao ten la: " << appName << std::endl;
    return false;
}


