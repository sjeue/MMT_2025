// ==========================================
// 1. KHỞI TẠO BIẾN VÀ LẤY DOM ELEMENTS
// ==========================================
let socket = null;

// Views
const loginView = document.getElementById("login-view");
const controlView = document.getElementById("control-view");

// Inputs kết nối
const ipInput = document.getElementById("server-ip");
const portInput = document.getElementById("server-port");
const connectButton = document.getElementById("btn-connect");
const disconnectButton = document.getElementById("btn-disconnect");

// Status indicators
const loginStatusText = document.getElementById("login-status-text");
const loginStatusLight = document.getElementById("login-status-light");
const controlStatusText = document.getElementById("control-status-text");
const controlStatusLight = document.getElementById("control-status-light");

// Modal Elements
const resultModal = document.getElementById("result-modal");
const modalTitle = document.getElementById("modal-title");
const modalBody = document.getElementById("modal-body");
const closeModal = document.getElementById("close-modal");
const modalFooter = document.getElementById("modal-footer");
const downloadLink = document.getElementById("download-link");

// ==========================================
// 2. HÀM XỬ LÝ GIAO DIỆN (UI)
// ==========================================

function showControlView() {
    loginView.classList.add("hidden");
    controlView.classList.remove("hidden");
    controlStatusText.innerText = "Đã kết nối";
    controlStatusLight.className = "connected";
}

function showLoginView(message, isError = false) {
    loginView.classList.remove("hidden");
    controlView.classList.add("hidden");
    resultModal.classList.add("hidden"); // Đóng modal nếu đang mở
    
    loginStatusText.innerText = message;
    loginStatusLight.className = "disconnected";
    
    connectButton.disabled = false;
    connectButton.innerText = "Kết nối";
}

// Xử lý đóng Modal
closeModal.onclick = () => { resultModal.classList.add("hidden"); };
window.onclick = (event) => {
    if (event.target == resultModal) resultModal.classList.add("hidden");
};

// ==========================================
// 3. HÀM KẾT NỐI WEBSOCKET
// ==========================================

function connect() {
    const ip = ipInput.value;
    const port = portInput.value;

    if (!ip || !port) return alert("Vui lòng nhập IP và Port server.");

    // Update UI trạng thái
    connectButton.disabled = true;
    connectButton.innerText = "Đang kết nối...";
    loginStatusText.innerText = "Đang thử kết nối...";

    // Tạo kết nối WebSocket
    // Lưu ý: "ws://" cho WebSocket thường
    const serverUrl = `ws://${ip}:${port}`;
    console.log(`Connecting to: ${serverUrl}`);
    
    socket = new WebSocket(serverUrl);

    // --- SỰ KIỆN SOCKET ---

    socket.onopen = function() {
        console.log("WebSocket Connected");
        showControlView();
    };

    socket.onclose = function() {
        console.log("WebSocket Closed");
        showLoginView("Đã ngắt kết nối");
        socket = null;
    };

    socket.onerror = function(error) {
        console.error("WebSocket Error", error);
        showLoginView("Lỗi kết nối", true);
        socket = null;
    };

    socket.onmessage = function(event) {
        // Nhận dữ liệu từ server
        console.log(`[RECEIVED]: ${event.data}`);
        try {
            const data = JSON.parse(event.data);
            handleServerResponse(data);
        } catch (e) {
            console.error("Non-JSON received:", event.data);
            showModal("Thông báo từ Server", `<p>${event.data}</p>`);
        }
    };
}

// ==========================================
// 4. HÀM GỬI LỆNH (CLIENT -> SERVER)
// ==========================================

function sendSocketMessage(command, payload = null) {
    if (socket && socket.readyState === WebSocket.OPEN) {
        const message = JSON.stringify({ command: command, payload: payload });
        socket.send(message);
        console.log(`[SENT]: ${message}`);
    } else {
        alert("Mất kết nối tới server!");
        showLoginView("Mất kết nối", true);
    }
}

// ==========================================
// 5. HÀM XỬ LÝ PHẢN HỒI (SERVER -> CLIENT)
// ==========================================

function handleServerResponse(data) {
    const cmd = data.command;
    const payload = data.payload;

    // Lấy Base URL để tải ảnh/video qua HTTP
    const ip = ipInput.value;
    const port = portInput.value;
    const httpBaseUrl = `http://${ip}:${port}`;

    switch (cmd) {
        case "list_apps":
            showTableModal("Danh sách Ứng dụng (Apps)", payload);
            break;
            
        case "list_processes":
            showTableModal("Danh sách Processes", payload);
            break;

        case "screen_capture":
            // Payload mong đợi: "/captures/screenshot_xxx.png"
            const imgUrl = httpBaseUrl + payload;
            showImageModal("Ảnh chụp màn hình", imgUrl);
            break;

        case "screen_record":
            // Payload mong đợi: "/captures/video_xxx.mp4"
            const vidUrl = httpBaseUrl + payload;
            showVideoModal("Video quay màn hình", vidUrl);
            break;

        case "error":
            showModal("Lỗi", `<p style="color:red; font-weight:bold;">${payload}</p>`);
            break;

        default:
            // Các tin nhắn text thông thường
            showModal("Thông báo", `<pre>${JSON.stringify(payload, null, 2)}</pre>`);
            break;
    }
}

// ==========================================
// 6. CÁC HÀM HIỂN THỊ MODAL
// ==========================================

function showModal(title, htmlContent) {
    modalTitle.innerText = title;
    modalBody.innerHTML = htmlContent;
    modalFooter.classList.add("hidden"); // Ẩn footer mặc định
    resultModal.classList.remove("hidden");
}

function showTableModal(title, listData) {
    if (!Array.isArray(listData) || listData.length === 0) {
        showModal(title, "<p>Danh sách trống.</p>");
        return;
    }
    // Tạo bảng động từ keys của object đầu tiên
    let html = '<table class="result-table"><thead><tr>';
    const headers = Object.keys(listData[0]);
    headers.forEach(h => html += `<th>${h.toUpperCase()}</th>`);
    html += '</tr></thead><tbody>';
    
    listData.forEach(row => {
        html += '<tr>';
        headers.forEach(key => html += `<td>${row[key]}</td>`);
        html += '</tr>';
    });
    html += '</tbody></table>';
    showModal(title, html);
}

function showImageModal(title, url) {
    const html = `<img src="${url}" class="screenshot-img" alt="Screenshot">`;
    showModal(title, html);
    setupDownloadLink(url, `screenshot_${Date.now()}.png`);
}

function showVideoModal(title, url) {
    const html = `
        <div style="text-align:center">
            <video controls autoplay style="max-height: 60vh;">
                <source src="${url}" type="video/mp4">
                Trình duyệt không hỗ trợ video này.
            </video>
        </div>`;
    showModal(title, html);
    setupDownloadLink(url, `record_${Date.now()}.mp4`);
}

function setupDownloadLink(url, filename) {
    downloadLink.href = url;
    downloadLink.download = filename;
    modalFooter.classList.remove("hidden");
}

// ==========================================
// 7. GẮN SỰ KIỆN CHO CÁC NÚT (EVENT LISTENERS)
// ==========================================

// Nút Kết nối / Ngắt kết nối
connectButton.addEventListener("click", () => { if (!socket) connect(); });
disconnectButton.addEventListener("click", () => { if (socket) socket.close(); });

// Nhóm APPS
document.getElementById("btn-list-apps").onclick = () => sendSocketMessage("list_apps");
document.getElementById("btn-start-app").onclick = () => {
    const name = document.getElementById("app-name-start").value;
    if(name) sendSocketMessage("start_app", { name: name });
};
document.getElementById("btn-stop-app").onclick = () => {
    const id = document.getElementById("app-pid-stop").value;
    if(id) sendSocketMessage("stop_app", { id: id });
};

// Nhóm PROCESSES
document.getElementById("btn-list-process").onclick = () => sendSocketMessage("list_processes");
document.getElementById("btn-start-process").onclick = () => {
    const name = document.getElementById("process-name-start").value;
    if(name) sendSocketMessage("start_process", { name: name });
};
document.getElementById("btn-stop-process").onclick = () => {
    const id = document.getElementById("process-pid-stop").value;
    if(id) sendSocketMessage("stop_process", { id: id });
};

// Nhóm SCREEN
document.getElementById("btn-screen-capture").onclick = () => sendSocketMessage("screen_capture");
document.getElementById("btn-screen-record").onclick = () => {
    const sec = parseInt(document.getElementById("record-duration").value);
    if(sec > 0) {
        sendSocketMessage("screen_record", { duration: sec });
        alert(`Đang yêu cầu quay video trong ${sec} giây. Vui lòng đợi...`);
    }
};

// Nhóm MESSAGE & SYSTEM
document.getElementById("btn-send-message").onclick = () => {
    const msg = document.getElementById("message-text").value;
    if(msg) sendSocketMessage("send_message", { text: msg });
};
document.getElementById("btn-shutdown").onclick = () => {
    if(confirm("CẢNH BÁO: Bạn có chắc chắn muốn tắt Server không?")) {
        sendSocketMessage("shutdown");
    }
};