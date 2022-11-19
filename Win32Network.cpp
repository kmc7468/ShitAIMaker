#include "PALNetwork.hpp"

#include <Windows.h>

void PALOpenURL(const std::string& url) {
	ShellExecuteA(nullptr, nullptr, url.c_str(), nullptr, nullptr, SW_SHOW);
}