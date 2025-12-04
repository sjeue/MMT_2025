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

// Thư viện JSON (đảm bảo bạn đã cài nlohmann/json)
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// =============================================================
// PHẦN 1: CÁC HÀM TIỆN ÍCH (HELPER FUNCTIONS)
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
// PHẦN 2: CÁC HÀM QUẢN LÝ PROCESS (KHÔI PHỤC LẠI)
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

// Hàm 1: Lấy danh sách ứng dụng đang chạy (listApps)
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

// Hàm 2: Lấy PID theo tên (getAppPIDByName)
int getAppPIDByName(const std::string& appName) {
    std::wstring wKeyword = StringToWString(appName);
    FindWindowData data;
    data.keywordW = ToLowerW(wKeyword); 
    data.pid = 0;
    EnumWindows(FindAppProc, (LPARAM)&data);
    return data.pid;
}

// Hàm 3: Tắt ứng dụng theo ID (killAppByID)
bool killAppByID(int pid) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess == NULL) return false;
    BOOL result = TerminateProcess(hProcess, 1);
    CloseHandle(hProcess);
    return result != 0;
}

// =============================================================
// PHẦN 3: TÍNH NĂNG START APP NÂNG CAO (WINDOWS SEARCH)
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

// Hàm 4: Mở ứng dụng (startApp)
bool startApp(const std::string& appName) {
    std::wstring wAppName = StringToWString(appName);
    
    // Ưu tiên chạy lệnh trực tiếp (Nhanh)
    HINSTANCE result = ShellExecuteW(NULL, L"open", wAppName.c_str(), NULL, NULL, SW_SHOWNORMAL);
    if ((intptr_t)result > 32) return true;

    // Fallback: Quét hệ thống (Chậm hơn nhưng tìm được Paint, Zalo...)
    std::cout << "Dang quet he thong (AppsFolder) tim: " << appName << "..." << std::endl;
    if (RunAppFromAppsFolder(wAppName)) {
        return true;
    }

    std::cout << "Khong tim thay ung dung nao ten la: " << appName << std::endl;
    return false;
}


//Cũ chạy được nhưng kém hiệu quả hơn
// #include "Headers/apps.h"
// #include <Windows.h>
// #include <shellapi.h>
// #include <shlobj.h> // Để lấy đường dẫn Start Menu
// #include <vector>
// #include <algorithm>
// #include <string>
// #include <filesystem> // C++17: Duyệt file hệ thống
// #include <iostream>

// namespace fs = std::filesystem;
// using json = nlohmann::json;

// // --- CÁC HÀM CHUYỂN ĐỔI UNICODE (GIỮ NGUYÊN) ---
// std::string WStringToString(const std::wstring& wstr) {
//     if (wstr.empty()) return std::string();
//     int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
//     std::string strTo(size_needed, 0);
//     WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
//     return strTo;
// }

// std::wstring StringToWString(const std::string& str) {
//     if (str.empty()) return std::wstring();
//     int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
//     std::wstring wstrTo(size_needed, 0);
//     MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
//     return wstrTo;
// }

// std::wstring ToLowerW(const std::wstring& str) {
//     std::wstring result = str;
//     std::transform(result.begin(), result.end(), result.begin(), ::tolower);
//     return result;
// }

// // -----------------------------------------------------------------
// // LOGIC TÌM KIẾM SHORTCUT TRONG START MENU (MỚI)
// // -----------------------------------------------------------------

// // Lấy danh sách các thư mục chứa Shortcut (Start Menu của User và của All Users)
// std::vector<std::wstring> GetStartMenuPaths() {
//     std::vector<std::wstring> paths;
//     wchar_t path[MAX_PATH];

//     // 1. Start Menu của User hiện tại
//     if (SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, path) == S_OK) {
//         paths.push_back(std::wstring(path));
//     }
    
//     // 2. Start Menu chung (All Users)
//     if (SHGetFolderPathW(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, path) == S_OK) {
//         paths.push_back(std::wstring(path));
//     }
    
//     // 3. Desktop (Thường user hay để icon ở đây)
//     if (SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, path) == S_OK) {
//         paths.push_back(std::wstring(path));
//     }

//     return paths;
// }

// // Hàm đệ quy tìm file .lnk khớp tên
// std::wstring FindShortcutPath(const std::wstring& keyword) {
//     auto searchPaths = GetStartMenuPaths();
//     std::wstring keywordLower = ToLowerW(keyword);

//     for (const auto& rootPath : searchPaths) {
//         try {
//             if (!fs::exists(rootPath)) continue;

//             // Duyệt đệ quy qua các thư mục con
//             for (const auto& entry : fs::recursive_directory_iterator(rootPath)) {
//                 if (entry.is_regular_file()) {
//                     // Lấy tên file (vd: "Cốc Cốc.lnk")
//                     std::wstring filename = entry.path().filename().wstring();
//                     std::wstring filenameLower = ToLowerW(filename);

//                     // Kiểm tra xem tên file có chứa từ khóa không
//                     // Ví dụ: "cốc cốc.lnk" chứa "cốc" -> OK
//                     if (filenameLower.find(keywordLower) != std::wstring::npos) {
//                         // Trả về đường dẫn tuyệt đối tới file shortcut
//                         return entry.path().wstring();
//                     }
//                 }
//             }
//         } catch (...) {
//             // Bỏ qua lỗi truy cập folder (nếu có)
//             continue;
//         }
//     }
//     return L""; // Không tìm thấy
// }

// // -----------------------------------------------------------------
// // CÁC HÀM CŨ (GIỮ NGUYÊN)
// // -----------------------------------------------------------------

// struct FindWindowData { std::wstring keywordW; int pid; };

// BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
//     json* jsonArray = (json*)lParam;
//     wchar_t title[512];
//     if (IsWindowVisible(hwnd)) {
//         GetWindowTextW(hwnd, title, 512);
//         std::wstring wTitle(title);
//         if (wTitle.length() > 0) {
//             DWORD pid;
//             GetWindowThreadProcessId(hwnd, &pid);
//             std::string utf8Title = WStringToString(wTitle);
//             jsonArray->push_back({ {"PID", pid}, {"Window Name", utf8Title} });
//         }
//     }
//     return TRUE;
// }

// json listApps() {
//     json appList = json::array();
//     EnumWindows(EnumWindowsProc, (LPARAM)&appList);
//     return appList;
// }

// BOOL CALLBACK FindAppProc(HWND hwnd, LPARAM lParam) {
//     FindWindowData* data = (FindWindowData*)lParam;
//     wchar_t title[512];
//     if (IsWindowVisible(hwnd)) {
//         GetWindowTextW(hwnd, title, 512);
//         std::wstring strTitle(title);
//         if (strTitle.length() > 0) {
//             std::wstring strLower = ToLowerW(strTitle);
//             if (strLower.find(data->keywordW) != std::wstring::npos) {
//                 DWORD pid;
//                 GetWindowThreadProcessId(hwnd, &pid);
//                 data->pid = (int)pid;
//                 return FALSE;
//             }
//         }
//     }
//     return TRUE;
// }

// int getAppPIDByName(const std::string& appName) {
//     std::wstring wKeyword = StringToWString(appName);
//     FindWindowData data;
//     data.keywordW = ToLowerW(wKeyword); 
//     data.pid = 0;
//     EnumWindows(FindAppProc, (LPARAM)&data);
//     return data.pid;
// }
// bool killAppByID(int pid) {
//     HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
//     if (hProcess == NULL) return false;
//     BOOL result = TerminateProcess(hProcess, 1);
//     CloseHandle(hProcess);
//     return result != 0;
// }

// // -----------------------------------------------------------------
// // HÀM START APP (ĐƯỢC NÂNG CẤP)
// // -----------------------------------------------------------------

// bool startApp(const std::string& appName) {
//     std::wstring wAppName = StringToWString(appName);
    
//     // CÁCH 1: Thử chạy trực tiếp (Nếu nhập "notepad", "calc")
//     HINSTANCE result = ShellExecuteW(NULL, L"open", wAppName.c_str(), NULL, NULL, SW_SHOWNORMAL);
//     if ((intptr_t)result > 32) return true;

//     // CÁCH 2: Nếu thất bại -> Tìm trong Start Menu (Smart Search)
//     std::cout << "[Smart Start] Searching for shortcut: " << appName << "..." << std::endl;
    
//     std::wstring shortcutPath = FindShortcutPath(wAppName);
    
//     if (!shortcutPath.empty()) {
//         std::cout << "[Smart Start] Found: " << WStringToString(shortcutPath) << std::endl;
        
//         // Chạy file Shortcut (.lnk) tìm được
//         result = ShellExecuteW(NULL, L"open", shortcutPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
//         return ((intptr_t)result > 32);
//     }

//     return false;
// }