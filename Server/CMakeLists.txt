cmake_minimum_required(VERSION 3.10)
project(MMT_Server)

# Cấu hình chuẩn C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- PHẦN 1: Cấu hình thư viện ---

# 1. Định nghĩa phiên bản Windows (Bắt buộc cho Boost.Asio)
add_compile_definitions(_WIN32_WINNT=0x0601)

# 2. Tìm OpenCV (Đã cài qua pacman)
find_package(OpenCV REQUIRED)
message(STATUS "OpenCV library status: FOUND")

# 3. Tìm Boost (Cụ thể là component system và thread)
# Lưu ý: Trên MSYS2, Boost thường là dynamic link
set(Boost_USE_STATIC_LIBS OFF) 
find_package(Boost REQUIRED COMPONENTS system thread)
message(STATUS "Boost library status: FOUND")

# 4. Tìm nlohmann_json (Header-only)
find_package(nlohmann_json REQUIRED)

# --- PHẦN 2: Khai báo File nguồn ---

# Liệt kê các file .cpp của bạn
set(SOURCE_FILES
    main.cpp
    keylogger.cpp
    webcam.cpp
    shutdown.cpp
    screenshot.cpp
    processes.cpp
    apps.cpp
)

# --- PHẦN 3: Tạo file thực thi ---

# Đặt tên file đầu ra là "server" (sẽ thành server.exe trên Windows)
add_executable(server ${SOURCE_FILES})

# Thêm đường dẫn file Header (thư mục hiện tại)
target_include_directories(server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# --- PHẦN 4: Liên kết thư viện (Linking) ---

target_link_libraries(server PRIVATE
    # Thư viện bên thứ 3
    ${OpenCV_LIBS}                  # Tất cả các module OpenCV
    Boost::system                   # Boost System
    Boost::thread                   # Boost Thread
    nlohmann_json::nlohmann_json    # JSON library

    # Thư viện hệ thống Windows (Bắt buộc)
    ws2_32      # Winsock (Mạng)
    wsock32     # Winsock mở rộng
    gdi32       # Đồ họa (Để chụp màn hình/webcam)
    user32      # User API (Để bắt phím/Keylogger)
)