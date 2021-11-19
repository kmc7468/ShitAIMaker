#ifdef _WIN32
#include "PALString.hpp"

#include "Win32String.hpp"

std::u8string PALEncodeToUTF8(const std::string& ansiString) {
	return EncodeToMultiByteString<char8_t>(EncodeToWideCharString(ansiString, CP_ACP), CP_UTF8);
}
std::string PALEncodeToANSI(const std::u8string& utf8String) {
	return EncodeToMultiByteString<char>(EncodeToWideCharString(utf8String, CP_UTF8), CP_ACP);
}
#endif