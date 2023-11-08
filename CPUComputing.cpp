#include "PALComputing.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <new>
#include <queue>
#include <thread>
#include <utility>

class CPUDevice final : public Device {
private:
	std::mutex m_Mutex;
	std::condition_variable m_Condition;

	std::thread m_Worker;
	std::queue<std::function<void()>> m_Queue;
	std::atomic_bool m_IsRunning = true;

public:
	CPUDevice()
		: Device("CPU", DeviceType::CPU),
		m_Worker(&CPUDevice::Work, this) {}
	CPUDevice(const CPUDevice&) = delete;
	virtual ~CPUDevice() override {
		m_IsRunning.store(false, std::memory_order_release);
		m_Condition.notify_one();
		m_Worker.join();
	}

public:
	CPUDevice& operator=(const CPUDevice&) = delete;

public:
	virtual BufferRef PALCreateBuffer(std::size_t elementSize,
		std::size_t elementCount, std::size_t elementAlignment) override;
	virtual void PALReadBuffer(void* dest, const BufferRef& src) override {
		std::memcpy(dest, src->GetHandle(), src->GetSize());
	}
	virtual void PALWriteBuffer(const BufferRef& dest, const void* src) override {
		std::memcpy(dest->GetHandle(), src, dest->GetSize());
	}
	virtual void PALCopyBuffer(const BufferRef& dest, const BufferRef& src) override {
		std::memcpy(dest->GetHandle(), src->GetHandle(),
			std::min(dest->GetSize(), src->GetSize()));
	}

	virtual void PALJoin() override {
		assert(m_IsRunning.load(std::memory_order_acquire));

		std::unique_lock lock(m_Mutex);

		if (!m_Queue.empty()) {
			m_Condition.wait(lock, [this]() { return m_Queue.empty(); });
		}
	}

private:
	void Work() {
		assert(m_IsRunning.load(std::memory_order_acquire));

		while (m_IsRunning.load(std::memory_order_acquire)) {
			std::unique_lock lock(m_Mutex);

			if (m_Queue.empty()) {
				m_Condition.notify_one();
				m_Condition.wait(lock, [this]() { return !m_Queue.empty(); });
			}

			const auto function = std::move(m_Queue.front());

			m_Queue.pop();
			lock.unlock();

			function();
		}
	}
	void AddWork(std::function<void()> function) {
		assert(m_IsRunning.load(std::memory_order_acquire));

		std::unique_lock lock(m_Mutex);

		m_Queue.push(std::move(function));
		m_Condition.notify_one();
	}
};

DeviceRef PALInitializeComputingForCPU() {
	return std::make_shared<CPUDevice>();
}
void PALFinalizeComputingForCPU(DeviceRef&) {}

class CPUBuffer final : public Buffer {
private:
	void* m_Buffer;

public:
	CPUBuffer(Device* device, std::size_t size, std::size_t alignment) noexcept
		: Buffer(device, size, alignment),
		m_Buffer(::operator new(size, std::align_val_t(alignment))) {}
	CPUBuffer(const CPUBuffer&) = delete;
	virtual ~CPUBuffer() override {
		::operator delete(m_Buffer, std::align_val_t(GetAlignment()));
	}

public:
	CPUBuffer& operator=(const CPUBuffer&) = delete;

protected:
	virtual void* PALGetHandle() noexcept override {
		return m_Buffer;
	}
};

BufferRef CPUDevice::PALCreateBuffer(std::size_t elementSize,
	std::size_t elementCount, std::size_t elementAlignment) {

	return std::make_shared<CPUBuffer>(this,
		elementSize * elementCount, elementAlignment);
}