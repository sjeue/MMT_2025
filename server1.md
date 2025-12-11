# PHẦN 1: KIẾN TRÚC HỆ THỐNG SERVER

## 1.1. Mô hình Asynchronous Event-Loop

Hệ thống Server được xây dựng theo mô hình **Asynchronous I/O (Bất đồng bộ)** sử dụng thư viện `Boost.Asio`, cho phép xử lý đồng thời nhiều kết nối mạng mà không bị tắc nghẽn (non-blocking). Điểm đặc biệt của Server này là khả năng **Hybrid Protocol Handling**: nó có thể xử lý cả giao thức **HTTP** (để tải file) và **WebSocket** (để điều khiển thời gian thực) trên cùng một cổng duy nhất (Port `9001`).

Server hoạt động đa luồng (Multi-threading) dựa trên số lõi CPU của phần cứng, đảm bảo hiệu năng cao khi chịu tải.
### 1.2. Phân tích cơ chế hoạt đông

#### 1.2.1. Quản lý vòng đời kết nối
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

        * **Trường hợp 2 (HTTP Request):** Nếu là yêu cầu HTTP thông thường (GET file ảnh, video...), nó gọi hàm `handle_http_file_request` để trả file và đóng kết nối ngay lập tức (Stateless).


#### 1.2.2. Phân Hệ WebSocket
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

    * Dùng `LogKey()` để xác định phím nào được bấm, sau đó `AnsiToUtf8()`chuyển phím được bấm thành 1 string **UTF-8** để truyền vào log.

    * `key_status` để đảm bảo 1 kí tự sẽ không xuất hiện trong log nhiều lần nếu nó bị giữ.

**Kết quả:** Trả về cho **Client** 1 chuỗi kí tự (string), biểu thị cho các phím bấm được ghi lại.

## 3.3. Module Webcam (`webcam.cpp`)

**Chức năng:** Sử dụng webcam để chụp hoặc quay trong 1 khoảng thời gian (duration) nhất định rồi trả về cho **Client** kết quả tương ứng.

**Kĩ thuật:** Sử dụng OpenCV để tương tác phần cứng.

* **Khởi tạo:** `VideoCapture(0, CAP_DSHOW)` giúp tăng tốc khởi động camera trên Windows.
* **Nén Video:** Sử dụng codec **H.264** (`VideoWriter::fourcc('H', '2', '6', '4')`). Việc nén này cực kỳ quan trọng để giảm dung lượng video truyền qua mạng (từ hàng GB xuống vài MB).
* **Timing:** Sử dụng `cv::getTickCount()` để đảm bảo thời gian quay video chính xác đến mili-giây.
* **Write**: Sử dụng `write()` (hay `imwrite()` cho `capture`) để ghi từng khung hình vào trong file.

**Kết quả:** Trả về cho **Client** đường dẫn tương đối với file `.exe` của **Server** (`/captures/anh_chup.jpg` cho ảnh, và `/captures/video_recording.mp4` cho video)

# Phần 4: QUẢN LÝ LỖI & TỐI ƯU HÓA

## 4.1. **Chiến lược:** 

Hệ thống áp dụng mô hình phòng thủ nhiều lớp để xử lý lỗi, từ tầng kết nối mạng thấp nhất đến tầng xử lý dữ liệu ứng dụng.

### 4.1.1. Tầng Mạng & Giao thức (Network Layer)

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

### 4.1.2. Tầng Ứng dụng & Dữ liệu (Application Layer)

Đây là nơi dễ gây crash nhất do dữ liệu bẩn từ Client. Ta xử lý như sau:

* JSON Parsing an toàn:

    * Sử dụng khối try-catch bao quanh trình phân tích cú pháp `json::parse(message)`.

    * **Lợi ích:** Nếu Hacker hoặc Client gửi một chuỗi không phải JSON hợp lệ, Server sẽ bắt được lỗi (`std::exception`), in cảnh báo và tiếp tục hoạt động thay vì dừng chương trình.

* Kiểm tra tính hợp lệ của dữ liệu:

    * Sau khi parse, code kiểm tra kỹ: `!j.is_object()`, `!j.count("command")`. Điều này ngăn chặn việc truy cập vào vùng nhớ không tồn tại.

### 4.1.3. Xử lý lỗi File (File System)

Module HTTP Server xử lý các kịch bản lỗi chuẩn mực:

* **404 Not Found**: Khi file không tồn tại.

* **500 Internal Server Error**: Khi file tồn tại nhưng không mở được (do quyền truy cập hoặc file bị khóa).

* **400 Bad Request**: Khi định dạng file không nằm trong whitelist (.jpg, .mp4, .png).

## 4.2. Các kỹ thuật tối ưu hóa

### 4.2.1. Truyền tải File hiệu năng cao

Đây là điểm tối ưu quan trọng nhất trong hàm `handle_http_file_request`.

* Sử dụng `http::file_body` thay vì `http::string_body` để gửi file. `http::file_body` cho phép hệ điều hành gửi file trực tiếp từ đĩa cứng ra socket, làm tiêu tốn cực ít RAM của máy Server ngay cả khi stream một file dung lượng lớn.

### 4.2.2. Cơ chế "Strand"

* **Vấn đề:** Trong môi trường đa luồng (Multi-threading), nếu hai luồng cùng ghi vào một socket socket cùng lúc, dữ liệu sẽ bị hỏng (Data corruption).

* **Giải pháp:** Sử dụng `net::strand`. `strand` đảm bảo rằng các hàm callback (`on_read`, `on_write`) của một session sẽ luôn được thực thi lần lượt, không bao giờ chạy song song, nhưng vẫn không chặn các session khác. Loại bỏ hoàn toàn nhu cầu sử dụng `std::mutex` phức tạp để khóa socket, giảm thiểu overhead của việc context switch.

### 4.2.3. Hàng đợi Gửi

Trong `WebsocketSession`, chúng ta cài đặt `std::vector<std::shared_ptr<...>> queue_`.

* **Lý do:** Boost.Beast cấm việc gọi `async_write` khi một `async_write` khác chưa hoàn thành.

* **Giải pháp:**

    * Nếu Server cần gửi 10 ảnh liên tiếp. ảnh 1 đang gửi, ảnh 2-10 sẽ được xếp vào hàng đợi.

    * Khi ảnh 1 gửi xong (`on_write`), Server tự động lấy ảnh 2 từ hàng đợi ra gửi tiếp.

* **Hiệu quả:** Đảm bảo luồng gửi dữ liệu luôn mượt mà, không bị mất gói tin và tuân thủ đúng chuẩn của thư viện.

# PHẦN 5: HẠN CHẾ VÀ NÂNG CẤP

## 5.1. **Quản lý đa Client**

Hiện tại Server đang dùng chiến thuật "Single-Client" theo yêu cầu của đồ án, nhưng có thể mở rộng mô hình trong tương lai để kết nối nhiều Client trong cùng 1 thời điểm.

* Chuyển `std::shared_ptr<WebsocketSession> active_ws_session_` sang `std::unordered_map<int, std::shared_ptr<WebsocketSession>> sessions_` để quản lý nhiều Client.

* Mỗi khi có kết nối mới, gán cho nó một `ID` và đưa vào Map.

* Sử dụng `std::mutex` để bảo vệ Map này khi thêm/xóa session.

## 5.2 Bảo mật

Hiện tại dữ liệu truyền đi dưới dạng **Clear Text**. Bất kỳ ai bắt gói tin (Sniffing) đều thấy nội dung chat hoặc password.

* Nâng cấp lên `boost::asio::ssl::stream` để hỗ trợ HTTPS và WSS.

* Thêm cơ chế xác thực (Authentication) qua `Token` hoặc `Password`

## 5.3 Xử lý lỗi JSON

Code hiện tại dùng try-catch cho JSON parsing, nhưng nếu Client gửi binary data thay vì text, server có thể tốn tài nguyên xử lý ngoại lệ.




