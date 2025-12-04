#ifndef PROCESSES_H
#define PROCESSES_H
#include <string>
#include <nlohmann/json.hpp>
nlohmann::json listProcesses();

// Liệt kê các tiến trình nền


// Diệt tiến trình dựa trên PID
bool killProcessByID(int pid);
int getProcessPIDByName(const std::string& procName);
#endif