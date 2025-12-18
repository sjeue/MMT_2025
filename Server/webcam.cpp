#include "Headers/webcam.h"
using namespace cv;
using namespace std;

string capture() {
    // Mở kết nối với camera 
    setBreakOnError(true);
    VideoCapture cap(0, CAP_DSHOW); 
    
    // Kiểm tra xem camera đã mở thành công chưa
    if (!cap.isOpened()) {
        cerr << "Lỗi: Không thể mở camera." << endl;
        return "";
    }
    
    // Biến lưu trữ khung hình từ camera
    Mat frame; 
    
    cap >> frame;
    // Biến đếm để đặt tên cho ảnh chụp
    string filename = "captures/anh_chup.jpg";
            
    // Lưu khung hình hiện tại thành file ảnh
    bool success = imwrite(filename, frame); 
    
    if (success) {
        cout << "Đã chụp và lưu ảnh thành công: " << filename << endl;
    } else {
        cerr << "Lỗi: Không thể lưu ảnh." << endl;
    } 

    // Vòng lặp chính để đọc và hiển thị luồng video
    cap.release();
    return "/captures/anh_chup.jpg";
}

// ĐỊNH NGHĨA THỜI GIAN QUAY (tính bằng giây) 

string record(double sec_record) {
    // Khởi tạo Camera 
    VideoCapture cap(0, CAP_DSHOW); 

    if (!cap.isOpened()) {
        cerr << "Lỗi: Khong the mo camera." << endl;
        return "";
    }

    // Lấy thông số của luồng video
    double fps = cap.get(CAP_PROP_FPS); 
    if (fps == 0) { 
        fps = 15.0;
    }
    int frame_width = static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH));
    int frame_height = static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT));
    Size frame_size(frame_width, frame_height);

    // Ghi Video
    string output_filename = "captures/video_recording.mp4";
    
    // Đặt codec: Sử dụng H.264.
    int fourcc = VideoWriter::fourcc('H', '2', '6', '4'); 

    VideoWriter writer(output_filename, fourcc, fps, frame_size, true);

    if (!writer.isOpened()) {
        cerr << "Lỗi: Khong the mo VideoWriter hoac codec khong duoc ho tro." << endl;
        return "";
    }

    Mat frame;
    
    // Biến đo thời gian bắt đầu
    double start_time = (double)cv::getTickCount(); 
    double tick_frequency = cv::getTickFrequency();
    double current_time = 0.0;

    cout << "Bat dau quay video trong " << sec_record << " giay..." << endl;

    // Vòng lặp Quay và Ghi Video
    while (true) {
        
        current_time = ((double)cv::getTickCount() - start_time) / tick_frequency;

        // Thoát nếu đã đạt thời gian quay yêu cầu
        if (current_time >= sec_record) {
            cout << "Da hoan thanh quay sau " << current_time << " giay." << endl;
            break;
        }

        // Đọc khung hình
        cap >> frame; 

        if (frame.empty()) {
            cerr << "Lỗi: Khung hinh trong." << endl;
            break;
        }

        // Ghi khung hình vào file
        writer.write(frame); 

        waitKey(1);
        imshow("Dang Ghi Hinh", frame); 

    }

    cap.release();
    writer.release();
    destroyAllWindows();

    return "/captures/video_recording.mp4";
}
