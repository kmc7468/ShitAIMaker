#ifdef _WIN32
#include "PALString.hpp"

#include <cassert>
#include <Windows.h>

namespace {
	template<typename Char>
	std::wstring EncodeToWideCharString(const std::basic_string<Char>& multiByteString, UINT codePage) {
		if (multiByteString.empty()) return {};

		const int length = MultiByteToWideChar(codePage, 0,
			reinterpret_cast<const char*>(multiByteString.data()), static_cast<int>(multiByteString.size()),
			nullptr, 0);
		assert(length > 0);

		std::wstring result(length, 0);
		MultiByteToWideChar(codePage, 0,
			reinterpret_cast<const char*>(multiByteString.data()), static_cast<int>(multiByteString.size()),
			result.data(), length + 1);

		return result;
	}
	template<typename Char>
	std::basic_string<Char> EncodeToMultiByteString(const std::wstring& wideCharString, UINT codePage) {
		if (wideCharString.empty()) return {};

		const int length = WideCharToMultiByte(codePage, 0,
			wideCharString.data(), static_cast<int>(wideCharString.size()),
			nullptr, 0, nullptr, nullptr);
		assert(length > 0);

		std::basic_string<Char> result(length, 0);
		WideCharToMultiByte(codePage, 0,
			wideCharString.data(), static_cast<int>(wideCharString.size()),
			reinterpret_cast<char*>(result.data()), length + 1, nullptr, nullptr);

		return result;
	}
}

std::u8string PALEncodeToUTF8(const std::string& ansiString) {
	return EncodeToMultiByteString<char8_t>(EncodeToWideCharString(ansiString, CP_ACP), CP_UTF8);
}
std::string PALEncodeToANSI(const std::u8string& utf8String) {
	return EncodeToMultiByteString<char>(EncodeToWideCharString(utf8String, CP_UTF8), CP_ACP);
}
#endif