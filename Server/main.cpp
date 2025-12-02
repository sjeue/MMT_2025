#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <random>

//------------------------------------------------------------------------------
// Namespaces cho gọn
namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

//------------------------------------------------------------------------------

/**
 * @brief Lớp giả mạo việc lấy thông tin hệ thống. (Không đổi)
 */
class SystemMonitor
{
    std::mt19937 gen_;
    std::uniform_real_distribution<> cpu_dist_;
    std::uniform_int_distribution<> ram_dist_;

public:
    SystemMonitor()
        : gen_(std::random_device{}()),
        cpu_dist_(10.0, 70.0),
        ram_dist_(2048, 4096)
    {
    }

    std::string getStatsJson()
    {
        double cpu = cpu_dist_(gen_);
        int ram = ram_dist_(gen_);

        std::string json = "{";
        json += "\"cpu\": " + std::to_string(cpu) + ",";
        json += "\"ram\": " + std::to_string(ram);
        json += "}";
        return json;
    }
};

/**
 * @brief Đại diện cho MỘT kết nối WebSocket của client
 * Tích hợp Timer và logic gửi tin nhắn.
 */
class WebsocketSession : public std::enable_shared_from_this<WebsocketSession>
{
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;

    // Tích hợp Monitor và Timer trực tiếp vào Session
    SystemMonitor monitor_;
    net::steady_timer timer_;

    // SỬA LỖI C2039: Khai báo strand sử dụng Executor type của io_context
    net::strand<net::io_context::executor_type> strand_;

    // Hàng đợi tin nhắn đơn giản cho việc ghi tuần tự
    std::vector<std::shared_ptr<std::string const>> queue_;

public:
    // Hàm tạo: Không cần SharedState nữa
    WebsocketSession(tcp::socket&& socket)
        : ws_(std::move(socket)),
        // Khởi tạo timer với cùng io_context của socket
        timer_(ws_.get_executor()),
        // SỬA LỖI: Lấy Executor từ io_context gốc của socket
        strand_(static_cast<net::io_context&>(ws_.get_executor().context()).get_executor())
    {
    }

    void run()
    {
        ws_.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::server));

        // Bắt đầu bắt tay (handshake)
        ws_.async_accept(
            net::bind_executor(
                strand_,
                std::bind(
                    &WebsocketSession::on_accept,
                    shared_from_this(),
                    std::placeholders::_1)));
    }

    // Hàm gửi tin nhắn (Bây giờ chỉ được gọi nội bộ)
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
        // ws_ đã là thành viên của lớp này, nên có thể truy cập private
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

        // Bắt đầu chu kỳ gửi dữ liệu ngay sau khi chấp nhận
        scheduleTimer();

        // Bắt đầu vòng lặp đọc (vẫn cần để phát hiện ngắt kết nối)
        do_read();
    }

    // --- TIMER LOGIC (Thay thế SharedState::onTimer) ---
    void scheduleTimer()
    {
        // Kiểm tra xem socket có còn mở không trước khi hẹn giờ
        if (!ws_.is_open()) return;

        timer_.expires_after(std::chrono::seconds(1));
        timer_.async_wait(
            net::bind_executor(
                strand_, // Quan trọng: Chạy trên strand để đồng bộ hóa với ghi/đọc
                std::bind(
                    &WebsocketSession::onTimer,
                    shared_from_this(),
                    std::placeholders::_1)));
    }

    void onTimer(beast::error_code ec)
    {
        if (ec == net::error::operation_aborted) return; // Bị hủy do đóng socket
        if (ec) {
            std::cerr << "[Timer] Error: " << ec.message() << std::endl;
            return;
        }

        // Lấy dữ liệu và gửi đi
        std::string statsJson = monitor_.getStatsJson();
        auto const ss = std::make_shared<std::string const>(std::move(statsJson));

        // Gửi tin nhắn nội bộ
        send(ss);

        // Lặp lại
        scheduleTimer();
    }
    // ----------------------------------------------------

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
            // Dừng timer khi ngắt kết nối
            timer_.cancel();
            return;
        }

        if (ec) {
            std::cerr << "[Session] Read error: " << ec.message() << std::endl;
            timer_.cancel();
            return;
        }

        buffer_.consume(buffer_.size());
        do_read();
    }

    void on_send(std::shared_ptr<std::string const> const& ss)
    {
        // Logic Write Queue giữ nguyên
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
            timer_.cancel();
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
 * Chấp nhận kết nối, tạo WebsocketSession và NGỪNG lắng nghe.
 */
class Listener : public std::enable_shared_from_this<Listener>
{
    net::io_context& ioc_;
    tcp::acceptor acceptor_;

    // Lưu trữ session duy nhất để quản lý
    std::shared_ptr<WebsocketSession> active_session_;

public:
    Listener(
        net::io_context& ioc,
        tcp::endpoint endpoint)
        : ioc_(ioc),
        acceptor_(ioc)
    {
        beast::error_code ec;

        acceptor_.open(endpoint.protocol(), ec);
        if (ec) { /* Xử lý lỗi */ return; }
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if (ec) { /* Xử lý lỗi */ return; }
        acceptor_.bind(endpoint, ec);
        if (ec) { /* Xử lý lỗi */ return; }
        acceptor_.listen(net::socket_base::max_listen_connections, ec);
        if (ec) { /* Xử lý lỗi */ return; }
    }

    void run()
    {
        do_accept();
    }

private:
    void do_accept()
    {
        // Khi chấp nhận, ta sử dụng ioc_ trực tiếp
        acceptor_.async_accept(
            beast::bind_front_handler(
                &Listener::on_accept,
                shared_from_this()));
    }

    void on_accept(beast::error_code ec, tcp::socket socket)
    {
        if (ec) {
            std::cerr << "[Listener] Accept error: " << ec.message() << std::endl;
        }
        else {
            if (active_session_ && active_session_->is_open()) {
                // Tùy chọn: Từ chối kết nối thứ hai
                std::cerr << "[Listener] Connection refused: Server already has an active client." << std::endl;
                // Có thể đóng socket ngay lập tức ở đây
            }
            else {
                // Tạo và lưu session duy nhất
                active_session_ = std::make_shared<WebsocketSession>(std::move(socket));
                active_session_->run();
            }
        }

        // Tiếp tục lắng nghe để chấp nhận kết nối lại sau khi client ngắt kết nối
        do_accept();
    }
};

//------------------------------------------------------------------------------
// HÀM MAIN (Không đổi)
//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // Cấu hình mặc định
    auto const address = net::ip::make_address("0.0.0.0");
    auto const port = static_cast<unsigned short>(9001);
    auto const threads = std::max<int>(1, std::thread::hardware_concurrency());

    std::cout << "Starting Single-Client WebSocket Server..." << std::endl;
    std::cout << "Address: " << address.to_string() << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "Threads: " << threads << std::endl;

    net::io_context ioc{ threads };

    // Chỉ tạo Listener (Không cần SharedState)
    std::make_shared<Listener>(
        ioc,
        tcp::endpoint{ address, port })
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

    return EXIT_SUCCESS;
}