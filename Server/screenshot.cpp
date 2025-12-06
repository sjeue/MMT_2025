#include "Headers/screenshot.h"
#include <Windows.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <iostream>

using namespace cv;

std::string captureScreen() {
    SetProcessDPIAware(); 

    // Lấy kích thước bao trùm tất cả màn hình
    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    // Lấy Device Context (DC) của màn hình
    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    // Tạo Bitmap để chứa ảnh
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    SelectObject(hMemoryDC, hBitmap);

    // Copy dữ liệu từ màn hình sang Bitmap
    BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, x, y, SRCCOPY);

    // Chuyển đổi sang định dạng OpenCV
    BITMAPINFOHEADER bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height;  // Âm để ảnh không bị lộn ngược 
    bi.biPlanes = 1;
    bi.biBitCount = 24;     // 3 byte (BGR)
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    Mat mat;
    mat.create(height, width, CV_8UC3); // Tạo Matrix 3 kênh màu

    // Lấy dữ liệu pixel từ Bitmap nạp vào Mat
    GetDIBits(hMemoryDC, hBitmap, 0, height, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    // Lưu file
    std::string filename = "captures/screenshot.jpg";
    
    CreateDirectoryA("captures", NULL); 
    
    bool success = imwrite(filename, mat);

    // Dọn dẹp bộ nhớ GDI
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);

    if (success) return "/captures/screenshot.jpg";
    return "";
}


