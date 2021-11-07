#pragma once

#include <string>

std::u8string EncodeToUTF8(const std::string& ansiString);
std::string EncodeToANSI(const std::u8string& utf8String);