#include "PALComputing.hpp"

#include <ranges>
#include <unordered_map>
#include <utility>

std::size_t GetDataTypeSize(DataType type) noexcept {
	switch (type) {
	case DataType::Float32:
		return 4;
	}
}

namespace {
	enum class DeviceKey {
		CPU,
		NVIDIA,
	};

	static std::unordered_map<DeviceKey, DeviceRef> s_Devices;
}

bool InitializeComputing() {
	if (auto device = PALInitializeComputingForCPU(); device) {
		s_Devices[DeviceKey::CPU] = std::move(device);
	} else {
		return false;
	}

	if (auto device = PALInitializeComputingForNVIDIA(); device) {
		s_Devices[DeviceKey::NVIDIA] = std::move(device);
	}

	return true;
}
void FinalizeComputing() {
	if (s_Devices.contains(DeviceKey::CPU)) {
		PALFinalizeComputingForCPU(s_Devices.at(DeviceKey::CPU));
	}

	if (s_Devices.contains(DeviceKey::NVIDIA)) {
		PALFinalizeComputingForNVIDIA(s_Devices.at(DeviceKey::NVIDIA));
	}

	s_Devices.clear();
}

Device::Device(std::string name, DeviceType type) noexcept
	: m_Name(std::move(name)), m_Type(type) {

	assert(!m_Name.empty());
}

std::string_view Device::GetName() const noexcept {
	return m_Name;
}
DeviceType Device::GetType() const noexcept {
	return m_Type;
}

BufferRef Device::CreateBuffer(const BufferRef& buffer) {
	assert(buffer);
	assert(buffer->GetDevice() == this);

	return PALCreateBuffer(buffer);
}
void Device::ReadBuffer(void* dest, const BufferRef& src) {
	assert(dest);
	assert(src);
	assert(src->GetDevice() == this);

	PALReadBuffer(dest, src);
}
void Device::ReadBufferAsync(void* dest, const BufferRef& src) {
	assert(dest);
	assert(src);
	assert(src->GetDevice() == this);

	PALReadBufferAsync(dest, src);
}
void Device::WriteBuffer(const BufferRef& dest, const void* src) {
	assert(dest);
	assert(dest->GetDevice() == this);
	assert(src);

	PALWriteBuffer(dest, src);
}
void Device::WriteBufferAsync(const BufferRef& dest, const void* src) {
	assert(dest);
	assert(dest->GetDevice() == this);
	assert(src);

	PALWriteBufferAsync(dest, src);
}
void Device::CopyBuffer(const BufferRef& dest, const BufferRef& src) {
	assert(dest);
	assert(dest->GetDevice() == this);
	assert(src);
	assert(src->GetDevice() == this);

	PALCopyBuffer(dest, src);
}
void Device::CopyBufferAsync(const BufferRef& dest, const BufferRef& src) {
	assert(dest);
	assert(dest->GetDevice() == this);
	assert(src);
	assert(src->GetDevice() == this);

	PALCopyBufferAsync(dest, src);
}

void Device::MultiplyMatrixAsync(
	std::size_t m, std::size_t n,
	const BufferRef& a, DataType aDataType, MatrixOrderType aOrderType,
	const BufferRef& b, DataType bDataType, MatrixOrderType bOrderType,
	const BufferRef& c, DataType cDataType, MatrixOrderType cOrderType
) {

	assert(m > 0);
	assert(n > 0);

	assert(a);
	assert(a->GetDevice() == this);
	assert(b);
	assert(b->GetDevice() == this);
	assert(c);
	assert(c->GetDevice() == this);

	const auto aDataSize = GetDataTypeSize(aDataType);
	const auto bDataSize = GetDataTypeSize(bDataType);
	const auto cDataSize = GetDataTypeSize(cDataType);

	const auto k = b->GetSize() / bDataSize / n;

	assert(k > 0);

	assert(a->GetSize() == m * n * aDataSize);
	assert(b->GetSize() == n * k * bDataSize);
	assert(c->GetSize() == m * k * cDataSize);

	PALMultiplyMatrixAsync(
		m, n, k,
		a, aDataType, aOrderType,
		b, bDataType, bOrderType,
		c, cDataType, cOrderType
	);
}
void Device::MultiplyMatrixAsync(
	std::size_t m, std::size_t n,
	const BufferRef& a, DataType aDataType, MatrixOrderType aOrderType,
	const BufferRef& b, DataType bDataType, MatrixOrderType bOrderType,
	const BufferRef& c, DataType cDataType, MatrixOrderType cOrderType,
	const BufferRef& d, DataType dDataType, MatrixOrderType dOrderType
) {

	assert(m > 0);
	assert(n > 0);

	assert(a);
	assert(a->GetDevice() == this);
	assert(b);
	assert(b->GetDevice() == this);
	assert(c);
	assert(c->GetDevice() == this);
	assert(d);
	assert(d->GetDevice() == this);

	const auto aDataSize = GetDataTypeSize(aDataType);
	const auto bDataSize = GetDataTypeSize(bDataType);
	const auto cDataSize = GetDataTypeSize(cDataType);
	const auto dDataSize = GetDataTypeSize(dDataType);

	const auto k = b->GetSize() / bDataSize / n;

	assert(k > 0);

	assert(a->GetSize() == m * n * aDataSize);
	assert(b->GetSize() == n * k * bDataSize);
	assert(c->GetSize() == m * k * cDataSize);
	assert(d->GetSize() == m * k * dDataSize);

	PALMultiplyMatrixAsync(
		m, n, k,
		a, aDataType, aOrderType,
		b, bDataType, bOrderType,
		c, cDataType, cOrderType,
		d, dDataType, dOrderType
	);
}
void Device::TransposeMatrixAsync(
	std::size_t m,
	const BufferRef& a, DataType aDataType, MatrixOrderType aOrderType
) {

	assert(m > 0);

	assert(a);
	assert(a->GetDevice() == this);

	const auto aDataSize = GetDataTypeSize(aDataType);

	const auto n = a->GetSize() / aDataSize / m;

	assert(n > 0);

	assert(a->GetSize() == m * n * aDataSize);

	PALTransposeMatrixAsync(
		m, n,
		a, aDataType, aOrderType
	);
}
void Device::TransposeMatrixAsync(
	std::size_t m,
	const BufferRef& a, DataType aDataType, MatrixOrderType aOrderType,
	const BufferRef& b, DataType bDataType, MatrixOrderType bOrderType
) {

	assert(m > 0);

	assert(a);
	assert(a->GetDevice() == this);
	assert(b);
	assert(b->GetDevice() == this);

	const auto aDataSize = GetDataTypeSize(aDataType);
	const auto bDataSize = aDataSize;

	const auto n = a->GetSize() / aDataSize / m;

	assert(n > 0);

	assert(a->GetSize() == m * n * aDataSize);
	assert(b->GetSize() == n * m * bDataSize);

	if (aOrderType == MatrixOrderType::RowMajor &&
		bOrderType == MatrixOrderType::ColumnMajor ||
		aOrderType == MatrixOrderType::ColumnMajor &&
		bOrderType == MatrixOrderType::RowMajor) {

		CopyBufferAsync(b, a);
	} else {
		PALTransposeMatrixAsync(
			m, n,
			a, aDataType, aOrderType,
			b, bDataType, bOrderType
		);
	}
}

void Device::Join() {
	PALJoin();
}

std::vector<DeviceRef> Device::GetAllDevices() {
	return std::ranges::to<std::vector>(
		s_Devices |
		std::ranges::views::transform([](const auto& pair) {
			return pair.second;
		})
	);
}

Buffer::Buffer(Device* device, std::size_t size, std::size_t alignment) noexcept
	: m_Device(device), m_Size(size), m_Alignment(alignment) {}

const Device* Buffer::GetDevice() const noexcept {
	return m_Device;
}
Device* Buffer::GetDevice() noexcept {
	return m_Device;
}
std::size_t Buffer::GetSize() const noexcept {
	return m_Size;
}
std::size_t Buffer::GetAlignment() const noexcept {
	return m_Alignment;
}

void* Buffer::GetHandle() noexcept {
	return PALGetHandle();
}