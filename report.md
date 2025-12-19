<p align="center">
<b>TRƯỜNG ĐẠI HỌC KHOA HỌC TỰ NHIÊN</b><br>
<b>KHOA CÔNG NGHỆ THÔNG TIN</b><br>
<i>*</i>
</p>

<p align="center">
<img src="img/hcmus-logo.png" alt="Logo HCMUS" width="180">
</p>

<p align="center">
<b>Ngành:</b> Trí Tuệ Nhân Tạo<br>
<b>Môn học:</b> Mạng Máy Tính
</p>

<p align="center">
<b>ĐỒ ÁN: GIÁM SÁT VÀ QUẢN LÍ MÁY TÍNH TỪ XA</b>
</p>

<p align="center">
<b>Tên nhóm:</b> Nhóm 2<br>
<b>Giáo viên hướng dẫn:</b> ThS Đỗ Hoàng Cường<br>
</p>

<p align="center">
<b>SV thực hiện:</b>
</p>

<table align="center">
<tr><th>Họ và Tên</th><th>Mã số sinh viên</th></tr>
<tr><td>Nguyễn Hoài Phước</td><td>24122022</td></tr>
<tr><td>Phan Thế Phong</td><td>24122009</td></tr>
<tr><td>Lê Thái Ngọc</td><td>23122012</td></tr>
</table>

<p align="center">
<b>ĐỒ ÁN MÔN HỌC</b><br>
<b>CHƯƠNG TRÌNH CHÍNH QUY</b><br><br>
TP. Hồ Chí Minh, Tháng 12/2025
</p>
</p>

<div style="page-break-after: always;"></div>

# **MỤC LỤC**
- [**MỤC LỤC**](#mục-lục)
- [PHẦN 1: KIẾN TRÚC HỆ THỐNG SERVER](#phần-1-kiến-trúc-hệ-thống-server)
  - [1.1. Mô hình Asynchronous Event-Loop](#11-mô-hình-asynchronous-event-loop)
    - [1.2. Phân tích cơ chế hoạt động](#12-phân-tích-cơ-chế-hoạt-động)
      - [1.2.1. Tổng quan Workflow của Server](#121-tổng-quan-workflow-của-server)
      - [1.2.2. Quản lý vòng đời kết nối](#122-quản-lý-vòng-đời-kết-nối)
      - [1.2.3. Phân Hệ WebSocket](#123-phân-hệ-websocket)
      - [1.2.4. Handle HTTP Request](#124-handle-http-request)
        - [a) Quy trình kiểm tra hợp lệ \& định tuyến](#a-quy-trình-kiểm-tra-hợp-lệ--định-tuyến)
        - [b) Cơ chế xác định định dạng (MIME Type Detection)](#b-cơ-chế-xác-định-định-dạng-mime-type-detection)
        - [c) Kiểm tra tồn tại \& xử lý lỗi 404 (Existence Check)](#c-kiểm-tra-tồn-tại--xử-lý-lỗi-404-existence-check)
        - [d) Kỹ thuật "Zero-Copy Streaming"](#d-kỹ-thuật-zero-copy-streaming)
- [PHẦN 2: GIAO THỨC GIAO TIẾP (PROTOCOL)](#phần-2-giao-thức-giao-tiếp-protocol)
- [PHẦN 3: PHÂN TÍCH CHI TIẾT MODULES CHỨC NĂNG](#phần-3-phân-tích-chi-tiết-modules-chức-năng)
  - [3.1. Module điều khiển nguồn (`control.cpp`)](#31-module-điều-khiển-nguồn-controlcpp)
  - [3.2. Module Keylogger (`keylogger.cpp`)](#32-module-keylogger-keyloggercpp)
  - [3.3. Module Webcam (`webcam.cpp`)](#33-module-webcam-webcamcpp)
  - [3.4. Module Apps (`apps.cpp`)](#34-module-apps-appscpp)
    - [3.4.1. Chuyển đổi mã hóa UTF-8 ⇆ UTF-16](#341-chuyển-đổi-mã-hóa-utf-8--utf-16)
    - [3.4.2. Liệt kê ứng dụng đang chạy — `listApps()`](#342-liệt-kê-ứng-dụng-đang-chạy--listapps)
    - [3.4.3. Tìm PID theo tên ứng dụng — `getAppPIDByName()`](#343-tìm-pid-theo-tên-ứng-dụng--getapppidbyname)
    - [3.4.4. Tắt ứng dụng theo PID — `killAppByID()`](#344-tắt-ứng-dụng-theo-pid--killappbyid)
    - [3.4.5. Smart Search — Tìm Shortcut (.lnk)](#345-smart-search--tìm-shortcut-lnk)
    - [3.4.6. Khởi chạy ứng dụng UWP](#346-khởi-chạy-ứng-dụng-uwp)
      - [a) Tại sao cần xử lý riêng?](#a-tại-sao-cần-xử-lý-riêng)
      - [b) Quy trình mở ứng dụng UWP](#b-quy-trình-mở-ứng-dụng-uwp)
      - [c) Hàm Worker](#c-hàm-worker)
      - [d) Hàm mở UWP](#d-hàm-mở-uwp)
    - [3.4.7. Cơ chế mở ứng dụng tổng hợp — `startApp()`](#347-cơ-chế-mở-ứng-dụng-tổng-hợp--startapp)
    - [3.4.8. Ưu điểm của Module Apps](#348-ưu-điểm-của-module-apps)
  - [3.5. Module Processes (`processes.cpp`)](#35-module-processes-processescpp)
    - [3.5.1. Liệt kê tiến trình – `listProcesses()`](#351-liệt-kê-tiến-trình--listprocesses)
    - [3.5.2. Kết thúc tiến trình – `killProcessByID(int pid)`](#352-kết-thúc-tiến-trình--killprocessbyidint-pid)
    - [3.5.3. Tìm PID theo tên tiến trình – `getProcessPIDByName()`](#353-tìm-pid-theo-tên-tiến-trình--getprocesspidbyname)
    - [3.5.4. Đánh giá hiệu năng \& ưu điểm](#354-đánh-giá-hiệu-năng--ưu-điểm)
    - [3.5.5. Kết luận](#355-kết-luận)
  - [3.6. Module Screenshot (`screenshot.cpp`)](#36-module-screenshot-screenshotcpp)
    - [3.6.1. Mục tiêu](#361-mục-tiêu)
    - [3.6.2. Quy trình chụp màn hình](#362-quy-trình-chụp-màn-hình)
    - [3.6.3. Ưu điểm thiết kế](#363-ưu-điểm-thiết-kế)
    - [3.6.4. Ứng dụng trong hệ thống](#364-ứng-dụng-trong-hệ-thống)
    - [3.6.5. Kết luận](#365-kết-luận)
- [PHẦN 4: CLIENT: KIẾN TRÚC \& TRIỂN KHAI](#phần-4-client-kiến-trúc--triển-khai)
  - [4.1. Kiến trúc tổng quan](#41-kiến-trúc-tổng-quan)
  - [4.2. Frontend](#42-frontend)
    - [a) View Quản lý Kết nối (Login View)](#a-view-quản-lý-kết-nối-login-view)
    - [b) View Điều khiển (Control Panel)](#b-view-điều-khiển-control-panel)
    - [c) Hệ thống Phản hồi (Feedback System)](#c-hệ-thống-phản-hồi-feedback-system)
  - [4.3. Backend](#43-backend)
    - [4.3.1. Cơ chế Socket \& Giao thức Kết nối](#431-cơ-chế-socket--giao-thức-kết-nối)
    - [4.3.2. Xử lý Giao thức Ứng dụng (Application Protocol)](#432-xử-lý-giao-thức-ứng-dụng-application-protocol)
    - [4.3.3. Tải tài nguyên (Hybrid Data Handling)](#433-tải-tài-nguyên-hybrid-data-handling)
  - [4.4. Triển khai](#44-triển-khai)
- [Phần 5: QUẢN LÝ LỖI \& TỐI ƯU HÓA](#phần-5-quản-lý-lỗi--tối-ưu-hóa)
  - [5.1. **Chiến lược:**](#51-chiến-lược)
    - [5.1.1. Tầng Mạng \& Giao thức (Network Layer)](#511-tầng-mạng--giao-thức-network-layer)
    - [5.1.2. Tầng Ứng dụng \& Dữ liệu (Application Layer)](#512-tầng-ứng-dụng--dữ-liệu-application-layer)
    - [5.1.3. Xử lý lỗi File (File System)](#513-xử-lý-lỗi-file-file-system)
  - [5.2. Các kỹ thuật tối ưu hóa](#52-các-kỹ-thuật-tối-ưu-hóa)
    - [5.2.1. Truyền tải File hiệu năng cao](#521-truyền-tải-file-hiệu-năng-cao)
    - [5.2.2. Cơ chế "Strand"](#522-cơ-chế-strand)
    - [5.2.3. Hàng đợi Gửi](#523-hàng-đợi-gửi)
- [PHẦN 6: HẠN CHẾ VÀ NÂNG CẤP](#phần-6-hạn-chế-và-nâng-cấp)
  - [6.1. **Quản lý đa Client**](#61-quản-lý-đa-client)
  - [6.2 Bảo mật](#62-bảo-mật)
  - [6.3 Xử lý lỗi JSON](#63-xử-lý-lỗi-json)

<div style="page-break-after: always;"></div>

# PHẦN 1: KIẾN TRÚC HỆ THỐNG SERVER

## 1.1. Mô hình Asynchronous Event-Loop

Hệ thống Server được xây dựng theo mô hình **Asynchronous I/O (Bất đồng bộ)** sử dụng thư viện `Boost.Asio`, cho phép xử lý đồng thời nhiều kết nối mạng mà không bị tắc nghẽn (non-blocking). Điểm đặc biệt của Server này là khả năng **Hybrid Protocol Handling**: nó có thể xử lý cả giao thức **HTTP** (để tải file) và **WebSocket** (để điều khiển thời gian thực) trên cùng một cổng duy nhất (Port `9001`).

Server hoạt động đa luồng (Multi-threading) dựa trên số lõi CPU của phần cứng, đảm bảo hiệu năng cao khi chịu tải.
### 1.2. Phân tích cơ chế hoạt động

#### 1.2.1. Tổng quan Workflow của Server
Để hình dung trực quan, hãy xem xét quy trình xử lý từ lúc Server khởi động đến khi tiếp nhận yêu cầu:

**Giai đoạn 1: Khởi động (Initialization)**
1. Hàm `main` tạo `io_context` và khởi chạy `Listener` tại port **9001**.
2. Server rơi vào trạng thái Idle, không tiêu tốn CPU nhờ cơ chế bất đồng bộ.

**Giai đoạn 2: Tiếp nhận và Phân loại (The Fork)**

Khi có một Client kết nối đến:
1. `Listener` chấp nhận kết nối TCP.
2. `Listener` đọc Header của gói tin đầu tiên.

**Kịch bản A: Client muốn xem ảnh/video (HTTP Mode)**
* **Listener:** Nhận thấy đây là phương thức `GET`.
* **Action:** Gọi hàm `handle_http_file_request`.
* **Server:** Tìm file trong thư mục `/captures/`, đọc binary, đóng gói HTTP Response (kèm Content-Type phù hợp: image/png, video/mp4...).
* **Kết thúc:** Gửi dữ liệu và đóng kết nối ngay lập tức. Socket được giải phóng.

**Kịch bản B: Client muốn điều khiển (WebSocket Mode)**

* **Listener:** Nhận thấy Header có `Upgrade: websocket`.

* **Check:** Kiểm tra biến `active_ws_session_`.

    * *Nếu đang bận:* Trả về `HTTP 503` "Server Busy".

    * *Nếu rảnh:* Tạo mới `WebsocketSession`.

* **WebsocketSession:**

    * Hoàn tất **Handshake**.

    * Mở kênh giao tiếp 2 chiều.

    * Chờ lệnh JSON từ Client (ví dụ: `{"command": "screenshot"}`).

    * Gửi kết quả trả về Client thông qua hàng đợi gửi (`queue_`).

    * Kết nối được duy trì liên tục cho đến khi một bên chủ động ngắt hoặc timeout.

#### 1.2.2. Quản lý vòng đời kết nối
Class `Listener` đóng vai trò là "người gác cổng". Đây là điểm tiếp nhận đầu tiên của mọi kết nối mạng. Nhiệm vụ của nó không chỉ là chấp nhận kết nối mà còn phải phân loại giao thức. Quy trình tiếp nhận kết nối diễn ra như sau:

1. **Khởi tạo Socket:** **Server** thiết lập kết nối với **Socket** qua 2 hàm sau trong class `Listener`.

* `run()` & `do_accept()`:

    * Server bind vào địa chỉ `0.0.0.0` (lắng nghe mọi interface mạng), khởi tạo vòng lặp bất đồng bộ để lắng nghe cổng 9001.
    * Ngay khi có một Client kết nối, nó chuyển quyền xử lý sang `on_accept()`.

2. **Thread Pool:** `io_context` được chạy trên số lượng luồng tương ứng với số nhân CPU (`std::thread::hardware_concurrency`). Điều này đảm bảo tận dụng tối đa phần cứng.

3. **Phát hiện giao thức:** Đây là phần quan trọng nhất trong việc thiết lập kết nối với máy **Client**, được thực hiện thông qua hàm `on_accept()`.

* `on_accept()`

    * Không giống server thông thường, hàm này đọc trước (peek) header của request HTTP đầu tiên.

    * **Cơ chế Phân Luồng (Routing Logic):**

        * **Trường hợp 1 (WebSocket Upgrade):** Nếu phát hiện header yêu cầu nâng cấp lên WebSocket (`websocket::is_upgrade(req)`), nó sẽ khởi tạo `WebsocketSession`.

            * *Lưu ý*: Server áp dụng chính sách Single-Client. Nếu đã có một session WebSocket đang chạy, server sẽ từ chối kết nối mới (trả về lỗi 503) để đảm bảo tính độc quyền điều khiển.

        * **Trường hợp 2 (HTTP Request):** Nếu là yêu cầu HTTP thông thường (`GET` file ảnh, video...), nó gọi hàm `handle_http_file_request` để trả file và đóng kết nối ngay lập tức (Stateless).


#### 1.2.3. Phân Hệ WebSocket
Đây là "trái tim" của hệ thống, nơi chịu trách nhiệm duy trì kết nối bền vững (persistent connection) để điều khiển và nhận lệnh từ Client.

**Nhóm hàm khởi tạo kết nối, thiết lập môi trường và dọn dẹp:**
* `WebsocketSession(tcp::socket&& socket)` **(Constructor)**

    * Nhận một socket TCP đã được kết nối từ `Listener` và khởi tạo 1 đối tượng.

    * Quan trọng nhất là việc khởi tạo `strand_`. Trong lập trình bất đồng bộ (Async), `strand` đảm bảo các lệnh thực thi trên socket này sẽ chạy tuần tự, không bao giờ chạy song song đè lên nhau, tránh lỗi bộ nhớ (Race Condition).

* `~WebsocketSession()` **(Destructor)**

    * Hàm hủy, chạy khi kết nối bị ngắt hoặc đối tượng bị xóa.

    * Nhiệm vụ quan trọng nhất của nó trong code này là dọn dẹp **Keylogger** (sẽ được đề cập ở phần sau). Nó set cờ `is_logging_ = false` và gọi `keylogger_thread_.join()` để đảm bảo luồng (thread) chạy ngầm của **Keylogger dừng hẳn**, tránh rò rỉ tài nguyên hệ thống (Resource Leak).

* `run(http::request<...> req)`

    * Kích hoạt phiên làm việc. Đây là điểm bắt đầu.


    * Đặt thời gian timeout (đề phòng kết nối treo).

    * Thực hiện bắt tay WebSocket (`async_accept`) sử dụng `req` (request HTTP) đã được `Listener` đọc trước đó. Điều này giúp nâng cấp kết nối từ HTTP thường lên WebSocket mượt mà.

* `on_accept(beast::error_code ec)`

    * Callback được gọi sau khi bắt tay (Handshake) thành công.

    * Tắt timeout (`expires_never`) vì WebSocket là kết nối dài hạn (Long-lived connection), không phải kiểu "gửi xong ngắt" như HTTP.

    * Gọi hàm `do_read()` để bắt đầu lắng nghe tin nhắn từ Client.

**Nhóm hàm xử lý đọc dữ liệu:** Chịu trách nhiệm lắng nghe "lệnh" từ **Client** gửi tới.

* `do_read()`

    * Gọi hàm `ws_.async_read`. Đây là hàm bất đồng bộ, nó không chặn chương trình lại mà chỉ đăng ký một yêu cầu: "Khi nào có dữ liệu, hãy gọi hàm `on_read`".

* `on_read(error_code, bytes_transferred)`

    * **Xử lý dữ liệu vừa nhận được theo cơ chế:**

        * Nếu Client ngắt kết nối (eof hoặc closed), nó sẽ dừng phiên làm việc.

        * Chuyển đổi buffer thành chuỗi (`beast::buffers_to_string`), sau đó dùng `nlohmann::json` để parse chuỗi đó thành object JSON. Tại đây, server sẽ phân tích xem Client muốn làm gì (dựa vào key `"command"`).

        * Cuối hàm, nó gọi lại `do_read()` để tiếp tục chờ tin nhắn tiếp theo. Nếu không gọi lại, Server chỉ nhận đúng 1 tin rồi im lặng mãi mãi.

**Nhóm hàm xử lý gửi dữ liệu:** Đây là phần phức tạp nhất vì Server phải đảm bảo cơ chế **Thread-safe Queue** (Hàng đợi an toàn luồng).

* `send(std::shared_ptr<...> ss)`

    * Hàm công khai (Public API) để các phần khác của chương trình gọi khi muốn gửi tin nhắn cho Client.

    * Nó không gửi ngay lập tức. Nó dùng `net::post` để ủy quyền việc gửi cho `strand_`. Điều này đảm bảo dù có 10 luồng khác nhau cùng gọi `send`, dữ liệu vẫn được xếp hàng ngăn nắp.

* `on_send(std::shared_ptr<...> ss)`

    * Đẩy tin nhắn vào vector `queue_`.

    * Kiểm tra xem `queue_.size()`. Nếu kích thước > 1, nghĩa là đang có một tin nhắn khác đang được gửi đi (chưa xong). Nó sẽ return ngay (không làm gì cả). Nếu kích thước là 1 (chỉ có tin nhắn vừa thêm), nó sẽ kích hoạt việc gửi (`ws_.async_write`).

* `on_write(error_code, bytes_transferred)`

    * Được gọi khi một tin nhắn đã gửi xong hoàn toàn.

    * Xóa tin nhắn vừa gửi xong khỏi hàng đợi (`queue_.erase`).

    * Nếu hàng đợi vẫn còn tin nhắn (do các lệnh `send` khác thêm vào trong lúc đang gửi), nó lấy tin tiếp theo ra và gọi `ws_.async_write`.

    * Cơ chế này tạo thành một dây chuyền liên tục cho đến khi hàng đợi rỗng.

#### 1.2.4. Handle HTTP Request
Ở đây ta xử lý yêu cầu HTTP qua hàm `handle_http_file_request`. Hàm hoạt động như một Web Server tĩnh.

##### a) Quy trình kiểm tra hợp lệ & định tuyến
Đảm bảo Server chỉ trả lời những yêu cầu hợp lệ.
```c++
std::string target_str = std::string(req.target()); 
bool starts_with_captures = target_str.find("/captures/") == 0;

if (req.method() == http::verb::get && starts_with_captures) {
    file_path_relative = target_str.substr(1); // Bỏ dấu '/' đầu tiên
}
```
* **Bộ lọc Method:** Chỉ chấp nhận `http::verb::get`. Các lệnh `POST`, `PUT`, `DELETE` sẽ bị từ chối ngay lập tức. Điều này ngăn chặn hacker lợi dụng cổng này để upload file độc hại lên server.

* **Bộ lọc Đường dẫn:** Chỉ chấp nhận các request bắt đầu bằng `/captures/`.
    * **Ý nghĩa:** Server cô lập quyền truy cập. Người dùng không thể truy cập các file hệ thống khác nếu không có tiền tố này.

* **Xử lý Đường dẫn:** `target_str.substr(1)` biến đường dẫn URL (ví dụ: `/captures/img.jpg`) thành đường dẫn tệp tương đối (relative file path: `captures/img.jpg`) để hệ điều hành có thể hiểu và mở được.

##### b) Cơ chế xác định định dạng (MIME Type Detection)
Trình duyệt cần biết file gửi về là ảnh hay video để chọn cách hiển thị phù hợp.
```c++
if (string_ends_with(file_path_relative, ".jpg") || ...) {
    content_type = "image/jpeg";
} else if (string_ends_with(file_path_relative, ".mp4")) {
    content_type = "video/mp4";
}
```
* **Hard-coded Mapping:** Code kiểm tra thủ công đuôi file (`.jpg`, `.png`, `.mp4`).

* **Xử lý lỗi định dạng:** Nếu đuôi file không nằm trong danh sách hỗ trợ, server trả về lỗi `400 Bad Request`. Đây là tính năng bảo mật gián tiếp, ngăn người dùng tải về các file nhạy cảm như `.exe`, `.dll`, hay source code `.cpp` dù chúng có nằm trong thư mục `/captures/`.

##### c) Kiểm tra tồn tại & xử lý lỗi 404 (Existence Check)
```c++
std::ifstream file_check(file_path_relative, std::ios::binary | std::ios::ate);
if (!file_check.is_open()) {
    // Trả về 404 Not Found
    return;
}
file_check.close();
```
* **Ý nghĩa:** Mặc dù hàm mở file chính thức ở bên dưới cũng có thể báo lỗi, nhưng việc dùng `std::ifstream` kiểm tra trước giúp ta trả về một thông điệp lỗi 404 Not Found rõ ràng và chuẩn mực ("User Friendly").

* **Lưu ý hiệu năng:** Việc mở file chỉ để kiểm tra rồi đóng lại (`file_check.close()`) ngay lập tức tốn một lượng nhỏ tài nguyên I/O đĩa. Tuy nhiên với quy mô ứng dụng nhỏ, sự an toàn và rõ ràng được ưu tiên hơn.

##### d) Kỹ thuật "Zero-Copy Streaming"
```c++
// 1. Tạo response với body là file
http::response<http::file_body> res{http::status::ok, req.version()};

// 2. Mở file bằng file_mode::read
res.body().open(file_path_relative.c_str(), beast::file_mode::read, ec);

// 3. Tính toán độ dài content-length
res.content_length(res.body().size()); 

// 4. Gửi đi
http::write(socket, res, ec);
```
* `http::file_body`: Đây là cấu trúc dữ liệu đặc biệt. Thay vì đọc toàn bộ nội dung file vào biến `std::string` (gây tràn RAM nếu file nặng vài GB), nó chỉ giữ một cái handle đến file đó trên đĩa cứng.

* **Cơ chế gửi:** Khi gọi `http::write`, `Boost.Beast` sẽ đọc từng (chunk) từ ổ cứng và gửi thẳng ra card mạng (Network Interface Card).

* **Hiệu quả:**

    * **RAM Usage:** Cực thấp và ổn định, bất kể file video nặng 10MB hay 10GB.

    * **Tốc độ:** Tận dụng tối đa băng thông ổ cứng và mạng.

# PHẦN 2: GIAO THỨC GIAO TIẾP (PROTOCOL)
Giao tiếp hoàn toàn dựa trên **JSON**. Cấu trúc giao tiếp được sử dụng là: 
* `"command"`: phân loại request của lệnh gì
* `"payload"`: thông tin thêm của lệnh, thay đổi tùy vào lệnh (`NULL` by default)

**Ví dụ:**

* **Cấu trúc Request (Client -> Server):**
```json
{ "command": "webcam_record", "payload": { "duration": 5.0 } }
```
* **Cấu trúc Request (Server -> Client):**
```json
{ "command": "webcam_record", "payload": "/captures/video_recording.mp4" }
```
**Các lệnh chính (`"command"`):**
* `shutdown`, `restart`: Điều khiển nguồn.
* `keylogger_start`, `keylogger_stop`: Ghi phím.
* `webcam_capture`, `screen_capture`: Chụp ảnh.
* `list_processes`, `stop_process`: Quản lý tác vụ.

# PHẦN 3: PHÂN TÍCH CHI TIẾT MODULES CHỨC NĂNG

## 3.1. Module điều khiển nguồn (`control.cpp`)

**Chức năng:** Dùng để **Shutdown**, hay **Restart** máy chủ nguồn (**Server**).

**Kĩ thuật:** Với kiến trúc máy **Server** giả định là hệ điều hành Windows.

* **Cơ chế:** Sử dụng `system()` để sử dụng các lệnh của hệ điều hành để điều khiển nguồn. Giữ delay 2s trước khi **Shutdown** hay **Restart** máy, để gửi phản hồi (response) cho **Client** trước khi bị ngắt kết nối.

**Kết quả:** Trả về `success` nếu thành công, hay `error` nếu thất bại và gửi `error_code` cho **Client**.

## 3.2. Module Keylogger (`keylogger.cpp`)

**Chức năng:** Dùng để ghi lại các phím bấm (key press), kể cả chuột, của máy phía **Server** cho đến khi nhận được yêu cầu dừng của bên phía **Client**.

**Kĩ thuật:** Đây là module phức tạp nhất về quản lý luồng (concurrency).

* **Vấn đề:** Việc quét phím yêu cầu vòng lặp vô hạn `while(true)`. Nếu chạy trên luồng mạng chính, **Server** sẽ bị treo.
* **Giải pháp:**
    *  **Đa luồng:** Keylogger chạy trên một std::thread hoàn toàn riêng biệt (`keylogger_thread_`) để vòng lặp `while(true)` của nó không chặn luồng mạng chính. riêng biệt.
    *  **Polling:** Sử dụng `GetAsyncKeyState` để bắt phím ở mức hệ thống (ngay cả khi ứng dụng không focus).
    *  **Đồng bộ hóa:**
        * `std::atomic<bool> is_logging_`: Cờ hiệu để dừng vòng lặp an toàn.
        * `std::mutex`: Khóa bảo vệ dữ liệu log khi **Client** yêu cầu đọc, tránh xung đột dữ liệu (Race Condition).
* **Cơ chế lấy phím:** 

    * Dùng API Windows `GetAsyncKeyState` để quét trạng thái 254 phím.

    * Dùng `LogKey()` để xác định phím nào được bấm, sau đó `AnsiToUtf8()` chuyển phím được bấm thành một string **UTF-8** để truyền vào log.

    * `key_status` để đảm bảo một kí tự sẽ không xuất hiện trong log nhiều lần nếu nó bị giữ.

**Kết quả:** Trả về cho **Client** một chuỗi kí tự (string), biểu thị cho các phím bấm được ghi lại.

## 3.3. Module Webcam (`webcam.cpp`)

**Chức năng:** Sử dụng webcam để chụp hoặc quay trong 1 khoảng thời gian (duration) nhất định rồi trả về cho **Client** kết quả tương ứng.

**Kĩ thuật:** Sử dụng OpenCV để tương tác phần cứng.

* **Khởi tạo:** `VideoCapture(0, CAP_DSHOW)` giúp tăng tốc khởi động camera trên Windows.
* **Nén Video:** Sử dụng codec **H.264** (`VideoWriter::fourcc('H', '2', '6', '4')`). Việc nén này cực kỳ quan trọng để giảm dung lượng video truyền qua mạng (từ hàng GB xuống vài MB).
* **Timing:** Sử dụng `cv::getTickCount()` để đảm bảo thời gian quay video chính xác đến mili-giây.
* **Write**: Sử dụng `write()` (hay `imwrite()` cho `capture`) để ghi từng khung hình vào trong file.

**Kết quả:** Trả về cho **Client** đường dẫn tương đối với file `.exe` của **Server** (`/captures/anh_chup.jpg` cho ảnh, và `/captures/video_recording.mp4` cho video)

## 3.4. Module Apps (`apps.cpp`)

Module Apps đóng vai trò cung cấp toàn bộ khả năng tương tác với ứng dụng trên Windows, bao gồm:

* Liệt kê ứng dụng đang mở.
* Tìm PID dựa theo tên ứng dụng.
* Tắt ứng dụng theo PID.
* Tìm file shortcut `.lnk` trong Start Menu / Desktop.
* Nhận diện và khởi chạy ứng dụng UWP (Microsoft Store).
* Khởi chạy ứng dụng theo ba phương thức: **EXE → LNK → UWP**.

Dưới đây là mô tả chi tiết.

### 3.4.1. Chuyển đổi mã hóa UTF-8 ⇆ UTF-16
Windows API sử dụng WCHAR (UTF-16). Do đó module phải chuyển đổi dữ liệu từ Client (UTF-8) sang UTF-16.

**Các hàm chính:**

| Hàm | Chức năng |
| :--- | :--- |
| `StringToWString()` | UTF-8 → UTF-16 |
| `WStringToString()` | UTF-16 → UTF-8 |
| `ToLowerW()` | Chuyển chuỗi wide thành chữ thường |
| `NormalizeKey()` | Chuẩn hóa dấu tiếng Việt (loại bỏ dấu) |

Nhờ đó, hệ thống có thể tìm ứng dụng với input như:
* `chrome`
* `ChRômE`
* `trình duyệt cờ rôm`
* `word` / `wórd`

### 3.4.2. Liệt kê ứng dụng đang chạy — `listApps()`
Hệ thống sử dụng các API:
```cpp
EnumWindows()
GetWindowTextW()
GetWindowThreadProcessId()
```
Chỉ các cửa sổ *visible* mới được liệt kê.

**Kết quả trả về dạng JSON:**
```json
{
  "PID": 1234,
  "Window Name": "Google Chrome"
}
```

**Ưu điểm:**
* Không cần quyền admin.
* Chỉ liệt kê ứng dụng có giao diện.
* Thực hiện rất nhanh.

### 3.4.3. Tìm PID theo tên ứng dụng — `getAppPIDByName()`
Module dùng cơ chế tương tự `EnumWindows`, nhưng thêm bước:

1. Chuẩn hóa tên ứng dụng người dùng nhập (lowercase + bỏ dấu).
2. Với mỗi cửa sổ:
    * Lấy tiêu đề.
    * Hạ chữ.
    * Chuẩn hóa dấu.
    * So sánh theo substring.

**Ví dụ:**
* "chrome" → tìm được "Google Chrome"
* "máy tính" → tìm được "Calculator" (UWP hoặc Win32)
* "visual" → tìm được "Visual Studio 2022"

**Kết quả:** Trả về PID đầu tiên khớp với từ khóa.

### 3.4.4. Tắt ứng dụng theo PID — `killAppByID()`
Sử dụng API:
```cpp
OpenProcess(PROCESS_TERMINATE);
TerminateProcess();
```
Trả về `true` / `false` giúp Client biết tiến trình có bị đóng thành công hay không.

### 3.4.5. Smart Search — Tìm Shortcut (.lnk)
Khi người dùng muốn mở ứng dụng nhưng không biết đường dẫn EXE, module hỗ trợ tìm kiếm file `.lnk` trong:

| Vị trí | Ý nghĩa |
| :--- | :--- |
| `CSIDL_PROGRAMS` | Start Menu người dùng |
| `CSIDL_COMMON_PROGRAMS` | Start Menu toàn hệ thống |
| `CSIDL_DESKTOPDIRECTORY` | Desktop |

**Cách hoạt động:**
1. Duyệt toàn bộ thư mục trong 3 vị trí trên.
2. Chuyển tất cả tên file về lowercase.
3. So khớp từ khóa người dùng nhập.
4. Trả về đường dẫn đầy đủ của shortcut.

**Ví dụ:**
* Nhập: `spotify` → tìm `Spotify.lnk`
* Nhập: `vs` → tìm `Visual Studio 2022.lnk`

### 3.4.6. Khởi chạy ứng dụng UWP

#### a) Tại sao cần xử lý riêng?
Ứng dụng UWP không có file `.exe`. Chúng chỉ có **AppUserModelID**, được Windows lưu trong thư mục đặc biệt:
`FOLDERID_AppsFolder`

#### b) Quy trình mở ứng dụng UWP
1. Tạo luồng riêng (tránh lỗi COM).
2. Dùng Shell API để duyệt toàn bộ UWP apps.
3. Lấy `DisplayName` để so sánh.
4. Lấy `AppUserModelID` từ property: `PKEY_AppUserModel_ID`.
5. Mở app bằng lệnh:
    ```cpp
    shell:AppsFolder\<AppUserModelID>
    ```

#### c) Hàm Worker
`FindUwpAppWorker()` chịu trách nhiệm:
* Khởi tạo COM (`CoInitializeEx`).
* Duyệt `AppsFolder`.
* So khớp keyword.
* Lấy ra `AppUserModelID`.

#### d) Hàm mở UWP
`StartUwpByName()`:
* Gọi thread worker.
* Nếu tìm thấy ID thì mở bằng `ShellExecuteW`.
* Trả về `true` hoặc `false`.

### 3.4.7. Cơ chế mở ứng dụng tổng hợp — `startApp()`
Đây là hàm trung tâm với thứ tự ưu tiên:

**Bước 1: Mở trực tiếp EXE**
```cpp
ShellExecuteW(open, appName)
```
Nếu tên ứng dụng trùng với file exe trong PATH → mở ngay.

**Bước 2: Mở bằng shortcut (.lnk)**
Dùng `FindShortcutPath()`. Nếu tìm thấy → `ShellExecuteW(open, shortcutPath)`.

**Bước 3: Ứng dụng UWP**
* Tìm `AppUserModelID`.
 * Mở bằng `shell:AppsFolder\<ID>`.

**Kết luận:**
Hàm trả về:
* `true` → nếu mở app thành công theo bất kỳ phương pháp nào.
* `false` → nếu cả EXE, LNK và UWP đều không tìm thấy.

### 3.4.8. Ưu điểm của Module Apps

| Tính năng | Mô tả |
| :--- | :--- |
| **Tìm kiếm thông minh** | Hỗ trợ tiếng Việt có/không dấu, lowercase |
| **Hỗ trợ 3 kiểu ứng dụng** | Win32 EXE, Shortcut LNK, UWP |
| **Không yêu cầu quyền admin** | Tất cả tính năng chạy bằng quyền User |
| **Native Windows API** | Tương thích mọi phiên bản Windows |
| **Đa dạng đầu vào** | Người dùng có thể nhập tên bất kỳ để tìm |

## 3.5. Module Processes (`processes.cpp`)

Module `processes.cpp` chịu trách nhiệm quản lý tiến trình hệ thống Windows, bao gồm:

* Liệt kê tất cả tiến trình đang chạy.
* Tìm PID của tiến trình theo tên.
* Kết thúc một tiến trình theo PID.

Module sử dụng **Windows API (ToolHelp32Snapshot)** để tương tác trực tiếp với hệ thống.

### 3.5.1. Liệt kê tiến trình – `listProcesses()`

**Mục tiêu:**
Trả về danh sách tất cả tiến trình hiện đang chạy trên máy, dưới dạng JSON Array.

**Cơ chế hoạt động:**
1. Sử dụng `CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS)` để tạo snapshot của toàn bộ process.
2. Duyệt từng process bằng:
    * `Process32First()`
    * `Process32Next()`
3. Mỗi tiến trình được đưa vào một JSON object gồm:
    * `PID`: mã định danh tiến trình.
    * `Process Name`: tên file executable (ví dụ: `chrome.exe`, `explorer.exe`).
4. Trả về JSON Array của tất cả tiến trình.

**Đặc điểm quan trọng:**
* Hàm trả về mảng rỗng nếu snapshot lỗi.
* Không truy cập Memory thông qua HANDLE của tiến trình → tránh crash.

### 3.5.2. Kết thúc tiến trình – `killProcessByID(int pid)`

**Mục tiêu:**
Dừng một tiến trình đang chạy dựa vào PID.

**Cơ chế hoạt động:**
1. Mở tiến trình bằng Windows API:
    ```cpp
    OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    ```
2. Gọi `TerminateProcess()`.
3. Đóng handle.

**Điểm cần lưu ý:**
* Hàm trả về `false` nếu không thể mở tiến trình (thường do không đủ quyền).
* Một số tiến trình hệ thống sẽ không thể bị terminate.

### 3.5.3. Tìm PID theo tên tiến trình – `getProcessPIDByName()`

**Mục tiêu:**
Tìm một tiến trình bất kỳ có tên khớp với chuỗi đầu vào.

**Phương pháp:**
1. Chuyển input và tên tiến trình trong hệ thống về dạng chữ thường.
2. So sánh bằng `string::find()`:
    * Cho phép tìm theo từ khóa.
    * Ví dụ: nhập "chrome" có thể bắt được "chrome.exe".

**Ứng dụng:**
Cho phép Client gửi lệnh dạng:
```json
{ 
  "command": "stop_process", 
  "payload": { "name": "chrome" } 
}
```
Server tự động tìm PID và kill process.

### 3.5.4. Đánh giá hiệu năng & ưu điểm

| Tính năng | Ưu điểm | Ghi chú |
| :--- | :--- | :--- |
| **Liệt kê tiến trình** | Nhanh, không block, độ ổn định cao | Windows API native |
| **Kill tiến trình** | Đơn giản, hiệu quả | Yêu cầu quyền Administrator với một số PID |
| **Tìm PID theo tên** | Hỗ trợ tìm gần đúng (substring) | Dừng khi gặp tiến trình đầu tiên |

### 3.5.5. Kết luận
Module `processes.cpp` cung cấp đầy đủ các chức năng cần thiết cho quản lý tiến trình:
1. Liệt kê toàn bộ process.
2. Tìm tiến trình theo tên.
3. Kết thúc tiến trình theo PID.

Thiết kế đơn giản, trực tiếp, và phù hợp với yêu cầu vận hành trên Windows cho hệ thống điều khiển từ xa (remote management).

## 3.6. Module Screenshot (`screenshot.cpp`)

Module `screenshot.cpp` đảm nhiệm chức năng chụp toàn bộ màn hình máy tính và lưu lại dưới dạng file ảnh JPEG. Chức năng này được sử dụng trong hệ thống điều khiển từ xa để cho phép Client theo dõi trạng thái màn hình của máy chủ.

### 3.6.1. Mục tiêu
* Chụp ảnh màn hình đa màn hình (multi-monitor) hoặc màn hình ảo (virtual screen).
* Xuất ảnh sang định dạng `.jpg`.
* Trả về đường dẫn ảnh để Server gửi lại cho Client.

### 3.6.2. Quy trình chụp màn hình
Hàm `captureScreen()` thực hiện các bước sau:

**Bước 1 — Xử lý DPI**
```cpp
SetProcessDPIAware();
```
Giúp chương trình lấy đúng độ phân giải thật của màn hình, tránh hiện tượng scale 125%, 150% làm ảnh bị lệch.

**Bước 2 — Lấy thông tin Virtual Screen**
Sử dụng Windows API:
* `SM_XVIRTUALSCREEN` – tọa độ X của màn hình ảo
* `SM_YVIRTUALSCREEN` – tọa độ Y
* `SM_CXVIRTUALSCREEN` – tổng chiều rộng
* `SM_CYVIRTUALSCREEN` – tổng chiều cao

Điều này giúp module hỗ trợ đa màn hình.

**Bước 3 — Tạo Bitmap qua GDI**
* Lấy Device Context (DC) của toàn màn hình.
* Tạo bộ nhớ đệm (MemoryDC) để chứa ảnh.
* Dùng `BitBlt()` để sao chép toàn bộ pixel từ màn hình sang bitmap.

*Ưu điểm:*
* Nhanh, ổn định.
* Không yêu cầu quyền cao.
* Hoạt động tốt trên mọi bản Windows.

**Bước 4 — Chuyển đổi sang OpenCV Mat**
Bitmap thu được cần convert sang định dạng OpenCV để có thể lưu thành file:
* Khai báo `BITMAPINFOHEADER` với định dạng 24-bit BGR.
* Dùng `GetDIBits()` để lấy toàn bộ pixel vào `cv::Mat`.

> **Lưu ý:** `biHeight = -height` giúp ảnh không bị đảo ngược theo trục Y.

**Bước 5 — Lưu file ảnh**
```cpp
imwrite("captures/screenshot.jpg", mat);
```
* Tự động tạo thư mục `captures` nếu chưa tồn tại.
* Lưu file ở dạng `.jpg`.

*Kết quả trả về:*
* `"/captures/screenshot.jpg"` nếu lưu thành công.
* `""` nếu thất bại.

### 3.6.3. Ưu điểm thiết kế

| Đặc điểm | Mô tả |
| :--- | :--- |
| **Hỗ trợ đa màn hình** | Chụp toàn bộ vùng màn hình ảo |
| **DPI aware** | Ảnh không bị scaling sai khi hệ thống phóng to |
| **Kết hợp GDI + OpenCV** | Vừa hiệu suất (GDI), vừa hiện đại (OpenCV) |
| **File đầu ra chuẩn** | JPEG nhẹ, dễ truyền qua WebSocket |

### 3.6.4. Ứng dụng trong hệ thống
Module screenshot được Server sử dụng khi nhận các lệnh:

```json
{
  "command": "screen_capture"
}
```
Server trả về đường dẫn file ảnh để Client hiển thị hoặc tải xuống.

### 3.6.5. Kết luận
Module screenshot được xây dựng tối ưu cho môi trường Windows, kết hợp cả GDI để trích xuất pixel và OpenCV để xử lý ảnh. Kết quả là một cơ chế chụp màn hình:
* Nhanh,
* Chính xác độ phân giải,
* Hỗ trợ đa màn hình,
* Dễ dàng tích hợp với kiến trúc WebSocket của Server.

# PHẦN 4: CLIENT: KIẾN TRÚC & TRIỂN KHAI

## 4.1. Kiến trúc tổng quan 

Phần Client của hệ thống được xây dựng dưới dạng một **Single Page Application (SPA)**, đóng vai trò là trạm điều khiển trung tâm (Dashboard). Client không chịu trách nhiệm xử lý logic nghiệp vụ nặng (như chụp ảnh, quét process) mà tập trung vào hai nhiệm vụ chính:

1.  **Gửi lệnh (Command Dispatcher):** Đóng gói yêu cầu người dùng thành định dạng JSON và gửi qua mạng.
2.  **Trực quan hóa (Data Visualization):** Nhận dữ liệu thô từ Server (danh sách tiến trình, chuỗi keylog, hình ảnh) và hiển thị lên giao diện đồ họa.

**Công nghệ sử dụng:**

  * **HTML5/CSS3:** Xây dựng khung xương và giao diện người dùng. Sử dụng các biến CSS (`:root`) để quản lý theme và `Flexbox/Grid` cho bố cục phản hồi (Responsive Design).
  * **JavaScript (Vanilla ES6+):** Xử lý logic kết nối, không phụ thuộc vào bất kỳ Framework bên thứ ba nào (như React hay Vue) để đảm bảo tính nhẹ nhàng (lightweight) và dễ dàng nhúng vào bất kỳ trình duyệt nào mà không cần quy trình build phức tạp.

Mô hình tương tác hoạt động theo cơ chế **Event-Driven**: Client luôn ở trạng thái lắng nghe sự kiện từ người dùng (click chuột, nhập liệu) để gửi lệnh, và lắng nghe sự kiện từ Socket để cập nhật giao diện thời gian thực (Real-time).

## 4.2. Frontend

Giao diện được thiết kế theo phong cách **Dashboard (Bảng điều khiển)** hiện đại, với chủ đề, tập trung vào trải nghiệm người dùng (UX) với các thành phần chính:

### a) View Quản lý Kết nối (Login View)

  * Đây là màn hình đầu tiên khi khởi động ứng dụng.
  * Cung cấp các trường nhập liệu cho **IP Address** và **Port**. Điều này cho phép Client linh hoạt kết nối tới bất kỳ Server nào trong mạng LAN hoặc Internet mà không cần hard-code địa chỉ.
  * Hiển thị trạng thái kết nối trực quan thông qua các đèn tín hiệu (Status Light).

### b) View Điều khiển (Control Panel)

Được chia thành các thẻ (Card) chức năng riêng biệt, tương ứng với các module phía Server:

  * **Apps & Processes:** Bảng quản lý cho phép xem danh sách, lọc, và gửi lệnh Start/Kill/Stop thông qua PID hoặc tên ứng dụng.
  * **Giám sát (Monitoring):** Các nút bấm lớn để kích hoạt chụp màn hình và Webcam.
  * **Keylogger & System:** Khu vực điều khiển ghi phím và các lệnh hệ thống (Shutdown/Restart).

### c) Hệ thống Phản hồi (Feedback System)

Để đảm bảo người dùng biết lệnh đã được thực thi hay chưa, Frontend cài đặt hai cơ chế:

  * **Toast Notification:** Các thông báo nhỏ trượt ra từ góc màn hình (Success, Error, Warning) báo hiệu trạng thái gửi lệnh.
  * **Modal (Hộp thoại):** Sử dụng để hiển thị dữ liệu lớn trả về từ Server như danh sách tiến trình (dạng bảng), hình ảnh (Image Preview), hoặc Video player. Modal hỗ trợ tính năng tải xuống (Download) dữ liệu về máy Client.

## 4.3. Backend

Mặc dù chạy trên trình duyệt, phần logic JavaScript (`app.js`) đóng vai trò là Backend của phía Client, chịu trách nhiệm thiết lập đường truyền tin cậy tới Server C++.

### 4.3.1. Cơ chế Socket & Giao thức Kết nối

Client sử dụng **WebSocket API** (`window.WebSocket`) để thiết lập kết nối **TCP** bền vững (Persistent Connection) tới Server.

  * **Khởi tạo:** `socket = new WebSocket("ws://IP:PORT")`.
  * **IP & Port:** Client yêu cầu người dùng nhập chính xác địa chỉ IPv4 của máy Server và Port (mặc định 9001) để thực hiện bắt tay (Handshake).
  * **State Machine:** Logic Client quản lý vòng đời kết nối qua các sự kiện chuẩn của giao thức mạng:
      * `onopen`: Chuyển giao diện sang màn hình điều khiển, thông báo kết nối thành công.
      * `onclose`: Tự động phát hiện khi Server ngắt kết nối (hoặc sập nguồn), đưa giao diện về màn hình đăng nhập.
      * `onerror`: Bắt các lỗi mạng (như sai IP, Firewall chặn) và thông báo cho người dùng.

### 4.3.2. Xử lý Giao thức Ứng dụng (Application Protocol)

Tương tự như Server, Client giao tiếp hoàn toàn qua **JSON**.

**Quy trình Gửi (Request):**
Hàm `sendSocketMessage(command, payload)` sẽ đóng gói dữ liệu thành chuỗi JSON string trước khi đẩy vào đường truyền:

```javascript
// Ví dụ gửi lệnh chặn process
socket.send(JSON.stringify({ 
    command: "stop_process", 
    payload: { id: "1234" } 
}));
```

**Quy trình Nhận (Response Dispatcher):**
Tại sự kiện `onmessage`, Client thực hiện phân tích cú pháp (Parsing):

1.  **Parse JSON:** Chuyển chuỗi nhận được thành Object.
2.  **Routing (Định tuyến):** Dựa vào trường `"command"` để gọi hàm xử lý giao diện tương ứng (Switch-case structure).
      * *Ví dụ:* Nếu `command` là `"screen_capture"`, Client sẽ không hiển thị text mà dựng một `<img>` tag bên trong Modal.

### 4.3.3. Tải tài nguyên (Hybrid Data Handling)

Một điểm đặc biệt trong kiến trúc mạng của hệ thống này là sự kết hợp giữa **WebSocket** và **HTTP**:

  * **Lệnh điều khiển:** Đi qua WebSocket.
  * **Dữ liệu File (Ảnh/Video):** Server không gửi binary file qua WebSocket (để tránh nghẽn). Thay vào đó, Server gửi đường dẫn URL (ví dụ: `/captures/img.jpg`).
  * **Client Logic:** Khi nhận được đường dẫn, Client tự động ghép với địa chỉ gốc (`http://IP:PORT/captures/...`) để tải ảnh về hiển thị hoặc cung cấp link Download. Đây là kỹ thuật giúp tối ưu hóa băng thông mạng.

## 4.4. Triển khai
Do kiến trúc Client thuần tĩnh (Static Web), việc triển khai cực kỳ đơn giản và linh hoạt:

1.  **Môi trường:** Client có thể chạy trên bất kỳ trình duyệt hiện đại nào (Chrome, Edge, Firefox) mà không cần cài đặt Node.js hay Web Server phức tạp.
2.  **Khởi chạy:** Người dùng chỉ cần mở file `index.html` trực tiếp (double-click).
3.  **Kết nối thực tế:**
      * Tại máy Server: Chạy file thực thi C++ (lắng nghe Port 9001).
      * Tại máy Client: Mở `index.html`, nhập địa chỉ IP LAN của máy Server (ví dụ: `192.168.1.10`) và Port `9001`.
      * Nhấn **Kết Nối** và bắt đầu điều khiển.

-----

# Phần 5: QUẢN LÝ LỖI & TỐI ƯU HÓA

## 5.1. **Chiến lược:** 

Hệ thống áp dụng mô hình phòng thủ nhiều lớp để xử lý lỗi, từ tầng kết nối mạng thấp nhất đến tầng xử lý dữ liệu ứng dụng.

### 5.1.1. Tầng Mạng & Giao thức (Network Layer)

Chúng ta sử dụng triệt để `beast::error_code` thay vì ném ngoại lệ (exceptions) cho các lỗi mạng thông thường.

* **Cơ chế:** Trong mọi hàm callback (`on_accept`, `on_read`, `on_write`), tham số `ec` luôn được kiểm tra đầu tiên. 

* Điều này giúp khi Client ngắt kết nối đột ngột (RST packet) hoặc timeout, Server không bị crash, chỉ đơn giản là log lỗi (`std::cerr`) và đóng socket một cách an toàn (`return` ngay lập tức).

* **Ví dụ:**
```c++
if (ec == websocket::error::closed || ec == net::error::eof) {
    // Xử lý đóng kết nối
    return;
}
```

### 5.1.2. Tầng Ứng dụng & Dữ liệu (Application Layer)

Đây là nơi dễ gây crash nhất do dữ liệu bẩn từ Client. Ta xử lý như sau:

* JSON Parsing an toàn:

    * Sử dụng khối try-catch bao quanh trình phân tích cú pháp `json::parse(message)`.

    * **Lợi ích:** Nếu Hacker hoặc Client gửi một chuỗi không phải JSON hợp lệ, Server sẽ bắt được lỗi (`std::exception`), in cảnh báo và tiếp tục hoạt động thay vì dừng chương trình.

* Kiểm tra tính hợp lệ của dữ liệu:

    * Sau khi parse, code kiểm tra kỹ: `!j.is_object()`, `!j.count("command")`. Điều này ngăn chặn việc truy cập vào vùng nhớ không tồn tại.

### 5.1.3. Xử lý lỗi File (File System)

Module HTTP Server xử lý các kịch bản lỗi chuẩn mực:

* **404 Not Found**: Khi file không tồn tại.

* **500 Internal Server Error**: Khi file tồn tại nhưng không mở được (do quyền truy cập hoặc file bị khóa).

* **400 Bad Request**: Khi định dạng file không nằm trong whitelist (.jpg, .mp4, .png).

## 5.2. Các kỹ thuật tối ưu hóa

### 5.2.1. Truyền tải File hiệu năng cao

Đây là điểm tối ưu quan trọng nhất trong hàm `handle_http_file_request`.

* Sử dụng `http::file_body` thay vì `http::string_body` để gửi file. `http::file_body` cho phép hệ điều hành gửi file trực tiếp từ đĩa cứng ra socket, làm tiêu tốn cực ít RAM của máy Server ngay cả khi stream một file dung lượng lớn.

### 5.2.2. Cơ chế "Strand"

* **Vấn đề:** Trong môi trường đa luồng (Multi-threading), nếu hai luồng cùng ghi vào một socket socket cùng lúc, dữ liệu sẽ bị hỏng (Data corruption).

* **Giải pháp:** Sử dụng `net::strand`. `strand` đảm bảo rằng các hàm callback (`on_read`, `on_write`) của một session sẽ luôn được thực thi lần lượt, không bao giờ chạy song song, nhưng vẫn không chặn các session khác. Loại bỏ hoàn toàn nhu cầu sử dụng `std::mutex` phức tạp để khóa socket, giảm thiểu overhead của việc context switch.

### 5.2.3. Hàng đợi Gửi

Trong `WebsocketSession`, chúng ta cài đặt `std::vector<std::shared_ptr<...>> queue_`.

* **Lý do:** Boost.Beast cấm việc gọi `async_write` khi một `async_write` khác chưa hoàn thành.

* **Giải pháp:**

    * Nếu Server cần gửi 10 ảnh liên tiếp. ảnh 1 đang gửi, ảnh 2-10 sẽ được xếp vào hàng đợi.

    * Khi ảnh 1 gửi xong (`on_write`), Server tự động lấy ảnh 2 từ hàng đợi ra gửi tiếp.

* **Hiệu quả:** Đảm bảo luồng gửi dữ liệu luôn mượt mà, không bị mất gói tin và tuân thủ đúng chuẩn của thư viện.

# PHẦN 6: HẠN CHẾ VÀ NÂNG CẤP

## 6.1. **Quản lý đa Client**
Hiện tại Server đang dùng chiến thuật "Single-Client" theo yêu cầu của đồ án, nhưng có thể mở rộng mô hình trong tương lai để kết nối nhiều Client trong cùng 1 thời điểm.

* Chuyển `std::shared_ptr<WebsocketSession> active_ws_session_` sang `std::unordered_map<int, std::shared_ptr<WebsocketSession>> sessions_` để quản lý nhiều Client.

* Mỗi khi có kết nối mới, gán cho nó một `ID` và đưa vào Map.

* Sử dụng `std::mutex` để bảo vệ Map này khi thêm/xóa session.

## 6.2 Bảo mật

Hiện tại dữ liệu truyền đi dưới dạng **Clear Text**. Bất kỳ ai bắt gói tin (Sniffing) đều thấy nội dung chat hoặc password.

* Nâng cấp lên `boost::asio::ssl::stream` để hỗ trợ HTTPS và WSS.

* Thêm cơ chế xác thực (Authentication) qua `Token` hoặc `Password`

## 6.3 Xử lý lỗi JSON

Code hiện tại dùng try-catch cho JSON parsing, nhưng nếu Client gửi binary data thay vì text, server có thể tốn tài nguyên xử lý ngoại lệ.


# PHỤ LỤC

**Video demo:** [*https://www.youtube.com/watch?v=zbfBtu9pyog*](https://www.youtube.com/watch?v=zbfBtu9pyog)

**Source code:** [*https://github.com/sjeue/MMT_2025*](https://github.com/sjeue/MMT_2025) (branch `main`)

