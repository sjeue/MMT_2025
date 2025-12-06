#ifndef APPS_H
#define APPS_H
#include <string>
#include <nlohmann/json.hpp> 

// Trả về JSON Array để Client hiển thị bảng
nlohmann::json listApps();

bool killAppByID(int pid);

// Hàm mở App
bool startApp(const std::string& appName);
int getAppPIDByName(const std::string& appName);
#endif