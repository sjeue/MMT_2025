#include "Headers/processes.h"
#include <Windows.h>
#include <algorithm>
#include <TlHelp32.h>

using json = nlohmann::json;

json listProcesses() {
    json procList = json::array();
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return json::array(); // Trả về mảng rỗng nếu lỗi
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        return json::array();
    }

    do {
        // Thêm Object vào mảng JSON
        procList.push_back({
            {"PID", (int)pe32.th32ProcessID},
            {"Process Name", std::string(pe32.szExeFile)}
        });
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return procList;
}

bool killProcessByID(int pid) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess == NULL) return false;
    BOOL result = TerminateProcess(hProcess, 1);
    CloseHandle(hProcess);
    return result != 0;
}

int getProcessPIDByName(const std::string& procName) {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    int foundPID = 0;

    // Chuyển input về chữ thường
    std::string keyword = procName;
    std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::tolower);

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) return 0;

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hProcessSnap, &pe32)) {
        do {
            std::string currentProcName = pe32.szExeFile;
            std::string currentLower = currentProcName;
            std::transform(currentLower.begin(), currentLower.end(), currentLower.begin(), ::tolower);

            // So sánh xem tên process có CHỨA từ khóa không (vd: "chrome" khớp "chrome.exe")
            if (currentLower.find(keyword) != std::string::npos) {
                foundPID = (int)pe32.th32ProcessID;
                break; // Tìm thấy thì thoát vòng lặp
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }

    CloseHandle(hProcessSnap);
    return foundPID;
}