#include "Headers/screenshot.h"
#include <Windows.h>
#include <vector>
#include <iostream>

// Thêm thư viện OpenCV
#include <opencv2/opencv.hpp>

// Link thư viện GDI (thường các IDE đã tự link, nhưng nếu lỗi thì thêm vào settings)
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")

using namespace cv;

std::string captureScreen() {
    // 1. CẤU HÌNH DPI (QUAN TRỌNG: Dùng chuẩn mới V2 để lấy đúng độ phân giải thật)
    // Nếu hàm này báo lỗi trên Windows cũ, hãy quay lại dùng SetProcessDPIAware();
    if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
        SetProcessDPIAware(); // Fallback cho Windows cũ
    }

    // 2. TÍNH TOÁN KÍCH THƯỚC MÀN HÌNH (VIRTUAL SCREEN)
    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    // 3. KHỞI TẠO GDI
    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    HGDIOBJ oldBitmap = SelectObject(hMemoryDC, hBitmap);

    // 4. CHỤP MÀN HÌNH (BitBlt)
    BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, x, y, SRCCOPY);

    // 5. VẼ CON TRỎ CHUỘT (FEATURE MỚI)
    // Code cũ không chụp được chuột, thêm đoạn này để thấy chuột
    CURSORINFO cursor = { sizeof(cursor) };
    if (GetCursorInfo(&cursor) && cursor.flags == CURSOR_SHOWING) {
        ICONINFO info = { sizeof(info) };
        if (GetIconInfo(cursor.hCursor, &info)) {
            // Tính vị trí chuột so với góc màn hình ảo
            int cursorX = cursor.ptScreenPos.x - x - info.xHotspot;
            int cursorY = cursor.ptScreenPos.y - y - info.yHotspot;
            DrawIcon(hMemoryDC, cursorX, cursorY, cursor.hCursor);
            DeleteObject(info.hbmColor);
            DeleteObject(info.hbmMask);
        }
    }

    // 6. CHUYỂN ĐỔI SANG OPENCV (FIX LỖI PADDING)
    // Mẹo: Dùng 32-bit (CV_8UC4) thay vì 24-bit. 
    // Lý do: 32 bit luôn chia hết cho 4 => Không bao giờ bị lệch dòng (skewing)
    BITMAPINFOHEADER bi = { 0 };
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height; // Âm để ảnh Top-Down (không bị lộn ngược)
    bi.biPlanes = 1;
    bi.biBitCount = 32;    // Dùng 32 bit (Blue, Green, Red, Alpha)
    bi.biCompression = BI_RGB;

    Mat mat;
    mat.create(height, width, CV_8UC4); // Tạo ma trận 4 kênh

    GetDIBits(hMemoryDC, hBitmap, 0, height, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    // 7. DỌN DẸP GDI
    SelectObject(hMemoryDC, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);

    // 8. XỬ LÝ ẢNH CUỐI CÙNG
    // Ảnh GDI 32-bit có kênh Alpha (trong suốt) thường bị Windows để là 0 hoặc 255 lung tung.
    // Chúng ta bỏ kênh Alpha đi để về dạng JPG chuẩn (3 kênh)
    Mat finalImage;
    cvtColor(mat, finalImage, COLOR_BGRA2BGR); 

    // Resize nếu ảnh quá to (Tùy chọn: giúp gửi qua mạng nhanh hơn)
    // if (width > 1920) resize(finalImage, finalImage, Size(1920, 1080 * 1920 / width));

    // 9. LƯU FILE
    std::string filename = "captures/screenshot.jpg";
    CreateDirectoryA("captures", NULL);
    
    // Nén JPG chất lượng 80 để nhẹ (mặc định là 95)
    std::vector<int> compression_params;
    compression_params.push_back(IMWRITE_JPEG_QUALITY);
    compression_params.push_back(80);

    bool success = imwrite(filename, finalImage, compression_params);

    if (success) return "/captures/screenshot.jpg";
    return "";
}