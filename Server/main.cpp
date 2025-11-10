/*
 * SERVER C++ DÙNG POCO
 * -------------------
 * Sử dụng thư viện POCO (một framework hoàn chỉnh)
 *
 * Chức năng:
 * 1. Mở port 9001.
 * 2. Quản lý danh sách client (thêm/xóa an toàn).
 * 3. Cứ 1 giây, gửi dữ liệu (CPU/RAM giả mạo) cho tất cả client.
 *
 * Biên dịch (trên MSYS2 MINGW64):
 * g++ main.cpp -o server.exe -std=c++14 -lPocoNet -lPocoUtil -lPocoFoundation -lpthread -lws2_32
 */

#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/WebSocket.h>
#include <Poco/Net/NetException.h>
#include <Poco/Util/ServerApplication.h>
#include <Poco/Mutex.h>
#include <iostream>
#include <string>
#include <set>
#include <thread>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip> // Để dùng std::setprecision

using Poco::Net::HTTPServer;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPRequestHandlerFactory;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::WebSocket;
using Poco::Net::WebSocketException;
using Poco::Util::ServerApplication;

// -----------------------------------------------------------------------------
// PHẦN QUẢN LÝ TRẠNG THÁI (GLOBAL STATE)
// -----------------------------------------------------------------------------

// Dùng mutex của POCO để bảo vệ danh sách
Poco::FastMutex g_mutex;
// Danh sách chứa tất cả các WebSocket đang hoạt động
std::set<WebSocket*> g_connections;

/**
 * @brief Thread này chạy mãi mãi, cứ 1 giây gửi
 * dữ liệu cho TẤT CẢ các client đang kết nối.
 */
void broadcast_loop() {
    // Thiết lập bộ tạo số ngẫu nhiên
    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<> cpu_dist(10.0, 70.0);
    std::uniform_int_distribution<> ram_dist(2048, 4096);

    while (true) {
        // Chờ 1 giây
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // 1. Tạo dữ liệu JSON giả
        double cpu = cpu_dist(gen);
        int ram = ram_dist(gen);

        std::stringstream ss;
        ss << "{\"cpu\": " << std::fixed << std::setprecision(1) << cpu 
           << ", \"ram\": " << ram << "}";
        std::string json_message = ss.str();

        // 2. Khóa mutex và gửi tin nhắn
        Poco::FastMutex::ScopedLock lock(g_mutex);

        // Dùng iterator để có thể xóa các client đã chết
        for (auto it = g_connections.begin(); it != g_connections.end(); /* không tăng ở đây */) {
            WebSocket* ws = *it;
            try {
                // Gửi dữ liệu dưới dạng TEXT
                ws->sendFrame(json_message.data(), json_message.length(), WebSocket::FRAME_TEXT);
                ++it; // Chỉ tăng iterator nếu gửi thành công
            } catch (WebSocketException &e) {
                // Bắt lỗi (ví dụ: client đã ngắt kết nối đột ngột)
                std::cerr << "[Broadcast] Lỗi khi gửi: " << e.displayText() << ". Đang xóa client..." << std::endl;
                // Xóa client "chết" khỏi danh sách
                it = g_connections.erase(it); 
            }
        }
    }
}

// -----------------------------------------------------------------------------
// PHẦN XỬ LÝ KẾT NỐI (SERVER LOGIC)
// -----------------------------------------------------------------------------

/**
 * @brief Class này xử lý MỘT kết nối WebSocket
 */
class WebSocketRequestHandler : public HTTPRequestHandler {
private:
    WebSocket* m_ws = nullptr;

public:
    void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) override {
        try {
            // Nâng cấp kết nối HTTP lên WebSocket
            m_ws = new WebSocket(request, response);
            std::cout << "[Server] Client connected: " << request.clientAddress().toString() << std::endl;

            // Thêm vào danh sách global
            {
                Poco::FastMutex::ScopedLock lock(g_mutex);
                g_connections.insert(m_ws);
            }

            char buffer[1024];
            int flags;
            int n;

            // Vòng lặp nhận tin nhắn (để phát hiện khi client đóng)
            do {
                n = m_ws->receiveFrame(buffer, sizeof(buffer), flags);
                // (Chúng ta không làm gì với tin nhắn, chỉ để phát hiện ngắt kết nối)
            } while (n > 0 && (flags & WebSocket::FRAME_OP_BITMASK) != WebSocket::FRAME_OP_CLOSE);

            std::cout << "[Server] Client disconnected: " << request.clientAddress().toString() << std::endl;

        } catch (WebSocketException &e) {
            // Lỗi xảy ra (ví dụ client đóng trình duyệt)
// Chỉ cần in ra lỗi, bất kể mã lỗi là gì
std::cerr << "[Handler] Lỗi: " << e.displayText() << std::endl;
        }

        // --- Dọn dẹp ---
        // Xóa khỏi danh sách global
        if (m_ws) {
            Poco::FastMutex::ScopedLock lock(g_mutex);
            g_connections.erase(m_ws);
        }
        
        // Hủy đối tượng WebSocket
        delete m_ws;
    }
};

/**
 * @brief Class này "tạo" ra các RequestHandler mới
 * mỗi khi có client kết nối
 */
class RequestHandlerFactory : public HTTPRequestHandlerFactory {
public:
    HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request) override {
        // Chỉ chấp nhận WebSocket
        if (request.find("Upgrade") != request.end() && Poco::icompare(request["Upgrade"], "websocket") == 0) {
            return new WebSocketRequestHandler;
        }
        // Từ chối các kết nối HTTP thông thường
        return nullptr; 
    }
};

/**
 * @brief Lớp Server chính (dùng để chạy)
 */
class WebSocketServer : public ServerApplication {
protected:
    int main(const std::vector<std::string>& args) override {
        // 1. Khởi động thread broadcast
        std::thread broadcaster_thread(broadcast_loop);

        // 2. Cấu hình và chạy server POCO
        unsigned short port = 9001;
        HTTPServer srv(new RequestHandlerFactory, port);
        srv.start(); // Bắt đầu server
        
        std::cout << "Starting WebSocket Server on port " << port << "..." << std::endl;

        // Chờ lệnh dừng (ví dụ: Ctrl+C)
        waitForTerminationRequest();
        
        // Dừng server
        srv.stop();

        broadcaster_thread.join();
        return Application::EXIT_OK;
    }
};

// -----------------------------------------------------------------------------
// HÀM MAIN
// -----------------------------------------------------------------------------
int main(int argc, char** argv) {
    WebSocketServer app;
    return app.run(argc, argv);
}