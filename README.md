# GIÁM SÁT VÀ QUẢN LÍ MÁY TÍNH TỪ XA
Đồ án môn học Mạng máy tính - Lớp 24TNT1 - Nhóm 2.

> **Mô tả:** 
- Hệ thống cho phép quản trị viên giám sát và điều khiển máy tính từ xa thông qua mô hình Client - Server
- Client chạy trên máy bị giám sát, Server đóng vai trò trung tâm điều khiển và thu thập dữ liệu qua mạng LAN/WAN.

Hệ thống được xây dựng theo hướng **đa luồng**, **xử lý bất đồng bộ**, đảm bảo hiệu năng và khả năng mở rộng.
> **Tính năng chính:**
### 📸 Webcam & Screenshot
- Chụp ảnh màn hình từ xa theo yêu cầu.
- Chụp ảnh webcam của máy Client.
- Truyền dữ liệu hình ảnh về Server theo thời gian thực.
- Sử dụng **OpenCV** để xử lý hình ảnh và camera.

---

### 🎥 Quay video màn hình & webcam
- Ghi lại video màn hình máy Client.
- Ghi hình webcam trong khoảng thời gian xác định.
- Video được lưu tạm tại Client và gửi về Server.
- Có thể cấu hình:
  - Độ phân giải
  - FPS
  - Thời lượng quay
- Sử dụng **OpenCV VideoWriter** để mã hóa video.

---

### ⌨️ Keylogger
- Ghi lại các phím bấm trên bàn phím của máy Client.
- Hoạt động nền, không ảnh hưởng trải nghiệm người dùng.
- Dữ liệu được:
  - Lưu cục bộ
  - Gửi về Server theo chu kỳ
- Phục vụ mục đích giám sát và phân tích hành vi.

---

### ⚙️ System Control – Quản lý hệ thống

#### 🧩 Processes
- Lấy danh sách tiến trình đang chạy.
- Xem:
  - PID
  - Tên tiến trình
  - Mức sử dụng tài nguyên
- Cho phép kết thúc (kill) tiến trình từ xa.

#### 📦 Applications
- Liệt kê các ứng dụng đang chạy và ứng dụng cài đặt.
- Mở ứng dụng từ xa (bao gồm Win32 và UWP).
- Đóng ứng dụng theo yêu cầu quản trị viên.

---

### 🔌 Power Control – Điều khiển nguồn
- **Shutdown**: Tắt máy Client từ xa.
- **Restart**: Khởi động lại máy Client.
- **Log out**: Đăng xuất người dùng hiện tại.
- Có cơ chế xác nhận lệnh từ Server để tránh thao tác nhầm.
- Sử dụng API hệ điều hành Windows (WinAPI).

---

### 💬 Message & Notification
- Gửi tin nhắn từ Server tới Client.
- Hiển thị thông báo dạng:
  - MessageBox
  - Notification (toast)
- Nội dung thông báo có thể tùy chỉnh:
  - Tiêu đề
  - Nội dung
  - Loại cảnh báo (Info / Warning / Error)
- Phù hợp cho việc:
  - Nhắc nhở
  - Cảnh báo hệ thống
  - Thông báo khẩn

---

## 1. Giới thiệu - Thành viên nhóm 👥
| STT | Họ và tên | MSSV |
| :---: | :--- | :---: | 
| 1 | Nguyễn Hoài Phước | 24122022 |
| 2 | Phan Thế Phong | 24122009 |
| 3 | Lê Thái Ngọc | 23122012 |


## 2. Cấu trúc thư mục 📁
Dưới đây là cây thư mục của dự án:

```text
MMT_2025/
├── Client/
│   ├── app.js
│   ├── index.html
│   └── style.css
├── Server/
│   ├── captures/
│   │   ├── anh_chup.jpg
│   │   ├── screenshot.jpg
│   │   └── video_recording.mp4
│   ├── Headers/
│   │   ├── apps.h
│   │   ├── control.h
│   │   ├── keylogger.h
│   │   ├── processes.h
│   │   ├── screenshot.h
│   │   └── webcam.h
│   ├── apps.cpp
│   ├── CMakeLists.txt
│   ├── control.cpp
│   ├── keylogger.cpp
│   ├── main.cpp
│   ├── PID
│   ├── processes.cpp
│   ├── screenshot.cpp
│   ├── Starting
│   ├── webcam.cpp
└── .gitignore
```

## 3. Cách Compile (Biên dịch) 🛠️

Dự án được phát triển và biên dịch trên môi trường **Windows** sử dụng **MSYS2 (UCRT64)**.

### Yêu cầu hệ thống (Prerequisites)
* **Môi trường:** MSYS2 UCRT64 Shell.
* **Công cụ build:** CMake (3.10+), Ninja, GCC (hỗ trợ C++17).
* **Thư viện (Dependencies):**
    * `OpenCV`: Xử lý hình ảnh (Webcam, Screenshot).
    * `Boost` (System, Thread): Hỗ trợ đa luồng và mạng.
    * `nlohmann_json`: Xử lý dữ liệu JSON.
    * `Windows SDK`: (ws2_32, gdi32, user32) - có sẵn trong toolchain.

Để cài đặt đầy đủ công cụ và thư viện trên MSYS2, chạy lệnh sau:
```bash
pacman -S mingw-w64-ucrt-x86_64-toolchain \
          mingw-w64-ucrt-x86_64-cmake \
          mingw-w64-ucrt-x86_64-ninja \
          mingw-w64-ucrt-x86_64-opencv \
          mingw-w64-ucrt-x86_64-boost \
          mingw-w64-ucrt-x86_64-nlohmann-json
```
Các bước biên dịch (Build Steps)

Mở terminal MSYS2 UCRT64, di chuyển đến thư mục dự án và chạy lần lượt các lệnh sau:

```bash
# 1. Clean build cũ (Xóa thư mục build để tránh lỗi cache)
rm -rf build

# 2. Cấu hình CMake (Sử dụng Ninja Generator, chế độ Release)
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release

# 3. Tiến hành Build
cmake --build build --config Release

# 4. Chạy chương trình
./build/server.exe
```
## ⚠️ Quan trọng: Copy thư viện DLL (Deployment)

Vì chương trình sử dụng **thư viện liên kết động**, để file `server.exe` chạy được và **tránh lỗi**: `System Error: ...dll was not found` bạn cần **copy các file `.dll` từ MSYS2** vào **thư mục chứa file thực thi** (thư mục `build/`).

### 🔧 Tự động copy DLL bằng Terminal (MSYS2)

Chạy các lệnh sau trong **MSYS2 UCRT64 terminal**:

```bash
# Copy các DLL của OpenCV, Boost và trình biên dịch vào thư mục build
cp /ucrt64/bin/libopencv_core*.dll build/
cp /ucrt64/bin/libopencv_video*.dll build/
cp /ucrt64/bin/libopencv_highgui*.dll build/
cp /ucrt64/bin/libopencv_imgcodecs*.dll build/
cp /ucrt64/bin/libopencv_imgproc*.dll build/

cp /ucrt64/bin/libboost_system*.dll build/
cp /ucrt64/bin/libboost_thread*.dll build/

cp /ucrt64/bin/libstdc++-6.dll build/
cp /ucrt64/bin/libgcc_s_seh-1.dll build/
cp /ucrt64/bin/libwinpthread-1.dll build/

# Kiểm tra các file DLL đã được copy hay chưa
ls build/*.dll
```




## 4. Kết nối & Chạy chương trình (Connection) 🚀

Hệ thống hoạt động theo mô hình **Client - Server** qua giao thức TCP/IP.

### Bước 1: Khởi chạy Server (Máy bị điều khiển/Target)
Server cần được chạy trước để mở cổng lắng nghe kết nối.

```bash
# Tại terminal MSYS2 (Máy Server):
./build/server.exe
```
Màn hình Server sẽ hiện thông báo:
```bash
Starting Single-Client WebSocket/HTTP Server...
Address: 0.0.0.0
Port: 9001
Threads: 8
```
### Bước 2: Khởi chạy Client (Frontend)

Vì Client là ứng dụng Web, bạn không cần cài đặt gì thêm.

1. Truy cập vào thư mục `Client/`.

2. Mở file `index.html` bằng trình duyệt web bất kỳ (Chrome, Edge, Firefox...).
    * **Cách nhanh:** Nhấp đúp chuột vào file `index.html`.
    * **Khuyên dùng:** Nếu code có sử dụng module, hãy dùng **Live Server** (Extension của VS Code) để chạy nhằm tránh lỗi CORS.

3. Trên giao diện Web, nhập **IP của máy Server**,   **Port** và nhấn nút **Kết nối**.
