// --- Biến toàn cục để giữ kết nối ---
let socket = null;

// --- Lấy các phần tử DOM ---
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
const disconnectButton = document.getElementById("btn-disconnect");

// Nút bấm Process
const btnListProcess = document.getElementById("btn-list-process");
const btnStartProcess = document.getElementById("btn-start-process");
const btnStopProcess = document.getElementById("btn-stop-process");
const processNameInput = document.getElementById("process-name-start");
const processPidInput = document.getElementById("process-pid-stop");

// Nút bấm Screen
const btnScreenCapture = document.getElementById("btn-screen-capture");
const btnScreenRecord = document.getElementById("btn-screen-record");
const recordDurationInput = document.getElementById("record-duration");

// Nút bấm Message
const btnSendMessage = document.getElementById("btn-send-message");
const messageInput = document.getElementById("message-text");

// Nút bấm System
const btnShutdown = document.getElementById("btn-shutdown");


// --- HÀM TRỢ GIÚP GỬI LỆNH JSON ---
/**
 * Gửi một lệnh có cấu trúc JSON đến server.
 * @param {string} command - Tên lệnh (vd: "shutdown", "list_process")
 * @param {object|null} payload - Dữ liệu đi kèm (vd: { name: "notepad.exe" })
 */
function sendSocketMessage(command, payload = null) {
    if (socket && socket.readyState === WebSocket.OPEN) {
        const message = JSON.stringify({
            command: command,
            payload: payload
        });
        
        socket.send(message);
        console.log(`Đã gửi: ${message}`);
    } else {
        alert("Lỗi: Mất kết nối tới server!");
        showLoginView("Đã mất kết nối", true); // Quay về màn hình login
    }
}


// --- HÀM QUẢN LÝ GIAO DIỆN (Giữ nguyên) ---
function showControlView() {
    loginView.classList.add("hidden");
    controlView.classList.remove("hidden");
    controlStatusText.innerText = "Đã kết nối";
    controlStatusLight.className = "connected";
}

function showLoginView(message, isError = false) {
    loginView.classList.remove("hidden");
    controlView.classList.add("hidden");
    loginStatusText.innerText = message;
    loginStatusLight.className = isError ? "disconnected" : "disconnected";
    connectButton.disabled = false;
    connectButton.innerText = "Kết nối";
}

// --- HÀM KẾT NỐI (Gần như giữ nguyên) ---
function connect() {
    const ip = ipInput.value;
    const port = portInput.value;
    if (!ip || !port) {
        alert("Vui lòng nhập cả IP và Port.");
        return;
    }
    const serverUrl = `ws://${ip}:${port}`;
    console.log(`Đang thử kết nối tới: ${serverUrl}`);

    connectButton.disabled = true;
    connectButton.innerText = "Đang kết nối...";
    loginStatusText.innerText = "Đang kết nối...";
    loginStatusLight.className = "disconnected";

    socket = new WebSocket(serverUrl);

    socket.onopen = function(event) {
        console.log("Đã kết nối tới server.");
        showControlView();
    };

    socket.onclose = function(event) {
        console.log("Kết nối đã bị đóng.");
        showLoginView("Đã ngắt kết nối");
        socket = null;
    };

    socket.onerror = function(error) {
        console.error("Lỗi WebSocket:", error);
        showLoginView("Lỗi kết nối", true);
        socket = null;
    };

    // CHÚ Ý: Bạn có thể thêm logic `socket.onmessage` ở đây
    // để nhận phản hồi từ server (vd: danh sách process)
    // và hiển thị lên UI.
}

// --- GẮN SỰ KIỆN CHO CÁC NÚT ĐIỀU KHIỂN ---

// Nút kết nối
connectButton.addEventListener("click", function() {
    if (!socket || socket.readyState === WebSocket.CLOSED) {
        connect();
    }
});

// Nút ngắt kết nối
disconnectButton.addEventListener("click", function() {
    if (socket && socket.readyState === WebSocket.OPEN) {
        socket.close();
    }
});

// --- Nhóm Process ---
btnListProcess.addEventListener("click", function() {
    // Về "Start/Stop Apps" / "List running Apps":
    // Trong thực tế, "App" và "Process" là gần như tương đồng.
    // Tôi gộp chúng lại thành "Process" cho rõ ràng.
    sendSocketMessage("list_processes");
});

btnStartProcess.addEventListener("click", function() {
    const processName = processNameInput.value;
    if (processName) {
        sendSocketMessage("start_process", { name: processName });
        processNameInput.value = ""; // Xóa input
    } else {
        alert("Vui lòng nhập tên process (ví dụ: notepad.exe)");
    }
});

btnStopProcess.addEventListener("click", function() {
    const processId = processPidInput.value;
    if (processId) {
        sendSocketMessage("stop_process", { id: processId });
        processPidInput.value = ""; // Xóa input
    } else {
        alert("Vui lòng nhập PID hoặc Tên process để dừng.");
    }
});

// --- Nhóm Screen ---
btnScreenCapture.addEventListener("click", function() {
    sendSocketMessage("screen_capture");
});

btnScreenRecord.addEventListener("click", function() {
    const duration = parseInt(recordDurationInput.value, 10);
    if (duration > 0) {
        sendSocketMessage("screen_record", { duration: duration });
    } else {
        alert("Thời gian quay phải lớn hơn 0 giây.");
    }
});

// --- Nhóm Message ---
btnSendMessage.addEventListener("click", function() {
    const messageText = messageInput.value;
    if (messageText) {
        sendSocketMessage("send_message", { text: messageText });
        messageInput.value = ""; // Xóa input
    } else {
        alert("Vui lòng nhập tin nhắn.");
    }
});

// --- Nhóm System ---
btnShutdown.addEventListener("click", function() {
    // Thêm một bước xác nhận cho hành động nguy hiểm
    if (confirm("Bạn có CHẮC CHẮN muốn shutdown server không?")) {
        sendSocketMessage("shutdown");
    }
});