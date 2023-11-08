#include "PALComputing.hpp"

#include <ranges>
#include <unordered_map>
#include <utility>

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