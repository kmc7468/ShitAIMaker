#include "IO.hpp"

#include "PAL.hpp"

#include <algorithm>
#include <cassert>
#include <climits>
#include <iterator>
#include <limits>

static_assert(CHAR_BIT == 8);
static_assert(std::endian::native == std::endian::little || std::endian::native == std::endian::big);
static_assert(std::numeric_limits<float>::is_iec559);

BinaryAdaptor::BinaryAdaptor(std::istream& stream, std::endian endian) noexcept
	: m_IStream(&stream), m_Endian(endian) {
	assert(endian == std::endian::little || endian == std::endian::big);
}
BinaryAdaptor::BinaryAdaptor(std::ostream& stream, std::endian endian) noexcept
	: m_OStream(&stream), m_Endian(endian) {
	assert(endian == std::endian::little || endian == std::endian::big);
}
BinaryAdaptor::BinaryAdaptor(std::iostream& stream, std::endian endian) noexcept
	: m_IStream(&stream), m_OStream(&stream), m_Endian(endian) {
	assert(endian == std::endian::little || endian == std::endian::big);
}

void BinaryAdaptor::ReadBytes(void* array, std::size_t byteCount) {
	assert(m_IStream != nullptr);

	m_IStream->read(static_cast<char*>(array), static_cast<std::streamsize>(byteCount));
}
std::string BinaryAdaptor::ReadString() {
	const std::uint32_t utf8Length = ReadInt32();
	std::u8string utf8(utf8Length, 0);

	ReadBytes(utf8.data(), utf8Length);

	return EncodeToANSI(utf8);
}
Matrix BinaryAdaptor::ReadMatrix() {
	const std::uint32_t row = ReadInt32();
	const std::uint32_t column = ReadInt32();
	Matrix result(row, column);

	for (std::uint32_t i = 0; i < row; ++i) {
		for (std::uint32_t j = 0; j < column; ++j) {
			result(i, j) = ReadFloat();
		}
	}

	return result;
}
std::int32_t BinaryAdaptor::ReadInt32() {
	std::uint8_t buffer[sizeof(std::int32_t)];

	ReadBytes(buffer, sizeof(buffer));
	if (m_Endian != std::endian::native) {
		std::reverse(std::begin(buffer), std::end(buffer));
	}

	return *reinterpret_cast<std::int32_t*>(buffer);
}
std::int64_t BinaryAdaptor::ReadInt64() {
	std::uint8_t buffer[sizeof(std::int64_t)];

	ReadBytes(buffer, sizeof(buffer));
	if (m_Endian != std::endian::native) {
		std::reverse(std::begin(buffer), std::end(buffer));
	}

	return *reinterpret_cast<std::int64_t*>(buffer);
}
float BinaryAdaptor::ReadFloat() {
	std::uint8_t buffer[sizeof(float)];

	ReadBytes(buffer, sizeof(buffer));
	if (m_Endian != std::endian::native) {
		std::reverse(std::begin(buffer), std::end(buffer));
	}

	return *reinterpret_cast<float*>(buffer);
}

void BinaryAdaptor::Write(const void* array, std::size_t byteCount) {
	assert(m_OStream != nullptr);

	m_OStream->write(static_cast<const char*>(array), static_cast<std::streamsize>(byteCount));
}
void BinaryAdaptor::Write(const std::string& string) {
	const std::u8string utf8 = EncodeToUTF8(string);

	Write(static_cast<std::int32_t>(utf8.size()));
	Write(utf8.data(), utf8.size());
}
void BinaryAdaptor::Write(const Matrix& matrix) {
	const auto [row, column] = matrix.GetSize();

	Write(static_cast<std::int32_t>(row));
	Write(static_cast<std::int32_t>(column));

	for (std::size_t i = 0; i < row; ++i) {
		for (std::size_t j = 0; j < column; ++j) {
			Write(matrix(i, j));
		}
	}
}
void BinaryAdaptor::Write(std::int32_t integer) {
	std::uint8_t buffer[sizeof(std::int32_t)];
	*reinterpret_cast<std::int32_t*>(buffer) = integer;

	if (m_Endian != std::endian::native) {
		std::reverse(std::begin(buffer), std::end(buffer));
	}
	Write(buffer, sizeof(buffer));
}
void BinaryAdaptor::Write(std::int64_t integer) {
	std::uint8_t buffer[sizeof(std::int64_t)];
	*reinterpret_cast<std::int64_t*>(buffer) = integer;

	if (m_Endian != std::endian::native) {
		std::reverse(std::begin(buffer), std::end(buffer));
	}
	Write(buffer, sizeof(buffer));
}
void BinaryAdaptor::Write(float decimal) {
	std::uint8_t buffer[sizeof(float)];
	*reinterpret_cast<float*>(buffer) = decimal;

	if (m_Endian != std::endian::native) {
		std::reverse(std::begin(buffer), std::end(buffer));
	}
	Write(buffer, sizeof(buffer));
}