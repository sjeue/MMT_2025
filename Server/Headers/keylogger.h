#ifndef KEYLOGGER_H
#define KEYLOGGER_H
#include <string>

std::string LogKey(int key_stroke);
std::string AnsiToUtf8(const std::string& ansi_str);
// Code Here

#endif // KEYLOGGER_H