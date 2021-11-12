#include "PALString.hpp"

std::u8string EncodeToUTF8(const std::string& ansiString) {
	return PALEncodeToUTF8(ansiString);
}
std::string EncodeToANSI(const std::u8string& utf8String) {
	return PALEncodeToANSI(utf8String);
}