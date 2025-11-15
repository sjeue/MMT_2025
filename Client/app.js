// --- Biến toàn cục để giữ kết nối ---
let socket = null;

// --- Lấy các phần tử DOM ---
// Giao diện
const loginView = document.getElementById("login-view");
const controlView = document.getElementById("control-view");

// Giao diện Login
const ipInput = document.getElementById("server-ip");
const portInput = document.getElementById("server-port");
const connectButton = document.getElementById("btn-connect");
const loginStatusText = document.getElementById("login-status-text");
const loginStatusLight = document.getElementById("login-status-light");

// Giao diện Điều khiển
const controlStatusText = document.getElementById("control-status-text");
const controlStatusLight = document.getElementById("control-status-light");
const shutdownButton = document.getElementById("btn-shutdown");
const disconnectButton = document.getElementById("btn-disconnect");


/**
 * Hàm cập nhật UI để CHUYỂN SANG GIAO DIỆN ĐIỀU KHIỂN
 */
function showControlView() {
    loginView.classList.add("hidden"); // Ẩn Login
    controlView.classList.remove("hidden"); // Hiện Điều khiển

    // Cập nhật trạng thái trên màn hình điều khiển
    controlStatusText.innerText = "Đã kết nối";
    controlStatusLight.className = "connected";
}

/**
 * Hàm cập nhật UI để QUAY LẠI GIAO DIỆN LOGIN
 * (Dùng khi ngắt kết nối hoặc kết nối lỗi)
 */
function showLoginView(message, isError = false) {
    loginView.classList.remove("hidden"); // Hiện Login
    controlView.classList.add("hidden"); // Ẩn Điều khiển

    // Cập nhật trạng thái trên màn hình login
    loginStatusText.innerText = message;
    loginStatusLight.className = isError ? "disconnected" : "disconnected";

    // Kích hoạt lại nút kết nối
    connectButton.disabled = false;
    connectButton.innerText = "Kết nối";
}

/**
 * Hàm khởi tạo kết nối WebSocket
 */
function connect() {
    // Lấy IP và Port từ ô nhập liệu
    const ip = ipInput.value;
    const port = portInput.value;
    if (!ip || !port) {
        alert("Vui lòng nhập cả IP và Port.");
        return;
    }
    const serverUrl = `ws://${ip}:${port}`;
    console.log(`Đang thử kết nối tới: ${serverUrl}`);

    // Cập nhật UI (trên màn hình Login)
    connectButton.disabled = true;
    connectButton.innerText = "Đang kết nối...";
    loginStatusText.innerText = "Đang kết nối...";
    loginStatusLight.className = "disconnected";

    // Tạo đối tượng WebSocket mới
    socket = new WebSocket(serverUrl);

    // Xử lý khi kết nối được mở thành công
    socket.onopen = function(event) {
        console.log("Đã kết nối tới server.");
        showControlView(); // <-- Chuyển giao diện
    };

    // Xử lý khi kết nối bị đóng
    socket.onclose = function(event) {
        console.log("Kết nối đã bị đóng.");
        showLoginView("Đã ngắt kết nối"); // <-- Quay lại Login
        socket = null;
    };

    // Xử lý khi có lỗi kết nối
    socket.onerror = function(error) {
        console.error("Lỗi WebSocket:", error);
        showLoginView("Lỗi kết nối", true); // <-- Quay lại Login
        socket = null;
    };
}

// --- Gắn sự kiện cho các nút ---

// 1. Nút "Kết nối" trên màn hình Login
connectButton.addEventListener("click", function() {
    // Chỉ chạy hàm connect nếu chưa kết nối
    if (!socket || socket.readyState === WebSocket.CLOSED) {
        connect();
    }
});

// 2. Nút "Ngắt kết nối" trên màn hình Điều khiển
disconnectButton.addEventListener("click", function() {
    if (socket && socket.readyState === WebSocket.OPEN) {
        socket.close();
    }
});

// 3. Nút "Gửi lệnh Shutdown" trên màn hình Điều khiển
shutdownButton.addEventListener("click", function() {
    if (socket && socket.readyState === WebSocket.OPEN) {
        const message = "shutdown";
        socket.send(message);
        console.log(`Đã gửi tin nhắn: ${message}`);
        alert("Đã gửi lệnh 'shutdown'!");
    } else {
        alert("Không thể gửi tin nhắn. Đã mất kết nối tới server.");
    }
});