#pragma once

#include "Matrix.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <ostream>
#include <string>

class BinaryAdaptor final {
private:
	std::istream* m_IStream = nullptr;
	std::ostream* m_OStream = nullptr;
	std::endian m_Endian;

public:
	explicit BinaryAdaptor(std::istream& stream, std::endian endian = std::endian::little) noexcept;
	explicit BinaryAdaptor(std::ostream& stream, std::endian endian = std::endian::little) noexcept;
	explicit BinaryAdaptor(std::iostream& stream, std::endian endian = std::endian::little) noexcept;
	BinaryAdaptor(const BinaryAdaptor&) = delete;
	~BinaryAdaptor() = default;

public:
	BinaryAdaptor& operator=(const BinaryAdaptor&) = delete;

public:
	void ReadBytes(void* array, std::size_t byteCount);
	std::string ReadString();
	Matrix ReadMatrix();
	std::int32_t ReadInt32();
	std::int64_t ReadInt64();
	float ReadFloat();

	void Write(const void* array, std::size_t byteCount);
	void Write(const std::string& string);
	void Write(const Matrix& matrix);
	void Write(std::int32_t integer);
	void Write(std::int64_t integer);
	void Write(float decimal);
};