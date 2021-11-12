#pragma once

#include <string>

std::u8string PALEncodeToUTF8(const std::string& ansiString);
std::string PALEncodeToANSI(const std::u8string& utf8String);

std::u8string EncodeToUTF8(const std::string& ansiString);
std::string EncodeToANSI(const std::u8string& utf8String);