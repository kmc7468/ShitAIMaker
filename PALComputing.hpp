#pragma once

#include "Reference.hpp"

#include <cassert>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

bool InitializeComputing();
void FinalizeComputing();

class BufferRef;
class DeviceRef;

enum class DeviceType {
	CPU,
	GPU,
	Others,
};

enum class DataType {
	Float32,
};

std::size_t GetDataTypeSize(DataType type) noexcept;

enum class MatrixOrderType {
	Default,
	RowMajor,
	ColumnMajor,
};

class Device {
private:
	std::string m_Name;
	DeviceType m_Type;

	std::vector<BufferRef> m_Buffers;

public:
	Device(const Device&) = delete;
	virtual ~Device() = default;

protected:
	Device(std::string name, DeviceType type) noexcept;

public:
	Device& operator=(const Device&) = delete;

public:
	std::string_view GetName() const noexcept;
	DeviceType GetType() const noexcept;

public:
	template<typename T>
	BufferRef CreateBuffer(std::size_t elementCount = 1) {
		assert(elementCount > 0);

		const auto buffer = PALCreateBuffer(sizeof(T), elementCount, alignof(T));

		if (buffer) {
			m_Buffers.push_back(buffer);
		}

		return buffer;
	}
	void ReadBuffer(void* dest, const BufferRef& src);
	void ReadBufferAsync(void* dest, const BufferRef& src);
	void WriteBuffer(const BufferRef& dest, const void* src);
	void WriteBufferAsync(const BufferRef& dest, const void* src);
	void CopyBuffer(const BufferRef& dest, const BufferRef& src);
	void CopyBufferAsync(const BufferRef& dest, const BufferRef& src);

	void MultiplyMatrixAsync(
		std::size_t m, std::size_t n,
		const BufferRef& a, DataType aDataType, MatrixOrderType aOrderType,
		const BufferRef& b, DataType bDataType, MatrixOrderType bOrderType,
		const BufferRef& c, DataType cDataType, MatrixOrderType cOrderType
	);
	void MultiplyMatrixAsync(
		std::size_t m, std::size_t n,
		const BufferRef& a, DataType aDataType, MatrixOrderType aOrderType,
		const BufferRef& b, DataType bDataType, MatrixOrderType bOrderType,
		const BufferRef& c, DataType cDataType, MatrixOrderType cOrderType,
		const BufferRef& d, DataType dDataType, MatrixOrderType dOrderType
	);

	void Join();

protected:
	virtual BufferRef PALCreateBuffer(std::size_t elementSize,
		std::size_t elementCount, std::size_t elementAlignment) = 0;
	virtual void PALReadBuffer(void* dest, const BufferRef& src) = 0;
	virtual void PALReadBufferAsync(void* dest, const BufferRef& src) = 0;
	virtual void PALWriteBuffer(const BufferRef& dest, const void* src) = 0;
	virtual void PALWriteBufferAsync(const BufferRef& dest, const void* src) = 0;
	virtual void PALCopyBuffer(const BufferRef& dest, const BufferRef& src) = 0;
	virtual void PALCopyBufferAsync(const BufferRef& dest, const BufferRef& src) = 0;

	virtual void PALMultiplyMatrixAsync(
		std::size_t m, std::size_t n, std::size_t k,
		const BufferRef& a, DataType aDataType, MatrixOrderType aOrderType,
		const BufferRef& b, DataType bDataType, MatrixOrderType bOrderType,
		const BufferRef& c, DataType cDataType, MatrixOrderType cOrderType
	) = 0;
	virtual void PALMultiplyMatrixAsync(
		std::size_t m, std::size_t n, std::size_t k,
		const BufferRef& a, DataType aDataType, MatrixOrderType aOrderType,
		const BufferRef& b, DataType bDataType, MatrixOrderType bOrderType,
		const BufferRef& c, DataType cDataType, MatrixOrderType cOrderType,
		const BufferRef& d, DataType dDataType, MatrixOrderType dOrderType
	) = 0;

	virtual void PALJoin() = 0;

public:
	static std::vector<DeviceRef> GetAllDevices();
};

class DeviceRef final : public SharedRef<Device> {
public:
	using SharedRef::SharedRef;
};

DeviceRef PALInitializeComputingForCPU();
void PALFinalizeComputingForCPU(DeviceRef& device);

DeviceRef PALInitializeComputingForNVIDIA();
void PALFinalizeComputingForNVIDIA(DeviceRef& device);

class Buffer {
private:
	Device* m_Device;
	std::size_t m_Size, m_Alignment;

public:
	Buffer(const Buffer&) = delete;
	virtual ~Buffer() = default;

protected:
	Buffer(Device* device, std::size_t size, std::size_t alignment) noexcept;

public:
	Buffer& operator=(const Buffer&) = delete;

public:
	const Device* GetDevice() const noexcept;
	Device* GetDevice() noexcept;
	std::size_t GetSize() const noexcept;
	std::size_t GetAlignment() const noexcept;

public:
	void* GetHandle() noexcept;

protected:
	virtual void* PALGetHandle() noexcept = 0;
};

class BufferRef final : public SharedRef<Buffer> {
public:
	using SharedRef::SharedRef;
};