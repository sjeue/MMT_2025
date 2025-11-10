// --- Cấu hình ---
// Thay đổi IP và Port này cho đúng với Server C++ của bạn
const SERVER_URL = "ws://127.0.0.1:9001"; // (hoặc "ws://localhost:9001")
const RECONNECT_DELAY = 3000; // Thử kết nối lại sau 3 giây

// --- Lấy các phần tử DOM ---
const statusLight = document.getElementById("status-light");
const statusText = document.getElementById("status-text");
const cpuDisplay = document.getElementById("cpu-display");
const ramDisplay = document.getElementById("ram-display");

/**
 * Hàm khởi tạo kết nối WebSocket
 */
function connect() {
    // Cập nhật UI
    statusText.innerText = "Đang kết nối...";
    statusLight.className = "disconnected";
    cpuDisplay.innerText = "-- %";
    ramDisplay.innerText = "-- MB";

    // Tạo đối tượng WebSocket mới
    const socket = new WebSocket(SERVER_URL);

    // Xử lý khi kết nối được mở thành công
    socket.onopen = function(event) {
        console.log("Đã kết nối tới server.");
        statusText.innerText = "Đã kết nối";
        statusLight.className = "connected";
        
        // (Tùy chọn) Gửi một tin nhắn khởi đầu nếu server yêu cầu
        // socket.send(JSON.stringify({ command: "START_MONITORING" }));
    };

    // Xử lý khi nhận được tin nhắn từ server
    socket.onmessage = function(event) {
        try {
            // Parse dữ liệu JSON nhận được
            const data = JSON.parse(event.data);

            // Giả sử server gửi về dạng: {"cpu": 15.4, "ram": 4096}
            if (data.cpu !== undefined && data.ram !== undefined) {
                // Cập nhật giao diện
                cpuDisplay.innerText = data.cpu.toFixed(1) + " %";
                ramDisplay.innerText = data.ram.toFixed(0) + " MB";
            } else {
                console.warn("Dữ liệu nhận được không đúng định dạng:", data);
            }

        } catch (error) {
            console.error("Lỗi khi parse JSON:", error);
        }
    };

    // Xử lý khi kết nối bị đóng
    socket.onclose = function(event) {
        console.log("Kết nối đã bị đóng. Đang thử kết nối lại...");
        statusText.innerText = "Đã ngắt kết nối";
        statusLight.className = "disconnected";
        
        // Tự động kết nối lại sau một khoảng thời gian
        setTimeout(connect, RECONNECT_DELAY);
    };

    // Xử lý khi có lỗi kết nối
    socket.onerror = function(error) {
        console.error("Lỗi WebSocket:", error);
        statusText.innerText = "Lỗi kết nối";
        // onclose sẽ tự động được gọi ngay sau onerror, 
        // nên logic kết nối lại sẽ được xử lý ở onclose.
    };
}

// Bắt đầu chạy ứng dụng
connect();