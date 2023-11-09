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
	template<typename T>
	struct RowMajorMatrixViewer final {
		using Type = T;

		static T Get(
			const T* matrix, std::size_t, std::size_t n,
			std::size_t i, std::size_t j
		) {

			return matrix[i * n + j];
		}
		static void Set(
			T* matrix, std::size_t, std::size_t n,
			std::size_t i, std::size_t j, T value
		) {

			matrix[i * n + j] = value;
		}
	};
	template<typename T>
	struct ColumnMajorMatrixViewer final {
		using Type = T;

		static T Get(
			const T* matrix, std::size_t m, std::size_t,
			std::size_t i, std::size_t j
		) {

			return matrix[j * m + i];
		}
		static void Set(
			T* matrix, std::size_t m, std::size_t,
			std::size_t i, std::size_t j, T value
		) {

			matrix[j * m + i] = value;
		}
	};

	template<std::size_t I, typename T, typename... Ts>
	struct GetNthType final {
		using Type = typename GetNthType<I - 1, Ts...>::Type;
	};
	template<typename T, typename... Ts>
	struct GetNthType<0, T, Ts...> final {
		using Type = T;
	};

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

protected:
	virtual BufferRef PALCreateBuffer(std::size_t elementSize,
		std::size_t elementCount, std::size_t elementAlignment) override;
	virtual void PALReadBuffer(void* dest, const BufferRef& src) override {
		std::memcpy(dest, src->GetHandle(), src->GetSize());
	}
	virtual void PALReadBufferAsync(void* dest, const BufferRef& src) override {
		AddWork([dest, src]() {
			std::memcpy(dest, src->GetHandle(), src->GetSize());
		});
	}
	virtual void PALWriteBuffer(const BufferRef& dest, const void* src) override {
		std::memcpy(dest->GetHandle(), src, dest->GetSize());
	}
	virtual void PALWriteBufferAsync(const BufferRef& dest, const void* src) override {
		AddWork([dest, src]() {
			std::memcpy(dest->GetHandle(), src, dest->GetSize());
		});
	}
	virtual void PALCopyBuffer(const BufferRef& dest, const BufferRef& src) override {
		std::memcpy(dest->GetHandle(), src->GetHandle(),
			std::min(dest->GetSize(), src->GetSize()));
	}
	virtual void PALCopyBufferAsync(const BufferRef& dest, const BufferRef& src) override {
		AddWork([dest, src]() {
			std::memcpy(dest->GetHandle(), src->GetHandle(),
				std::min(dest->GetSize(), src->GetSize()));
		});
	}

	virtual void PALMultiplyMatrixAsync(
		std::size_t m, std::size_t n, std::size_t k,
		const BufferRef& a, DataType aDataType, MatrixOrderType aOrderType,
		const BufferRef& b, DataType bDataType, MatrixOrderType bOrderType,
		const BufferRef& c, DataType cDataType, MatrixOrderType cOrderType
	) override {

		MultiplyMatrix<>(
			m, n, k,
			a, aDataType, aOrderType,
			b, bDataType, bOrderType,
			c, cDataType, cOrderType
		);
	}
	virtual void PALMultiplyMatrixAsync(
		std::size_t m, std::size_t n, std::size_t k,
		const BufferRef& a, DataType aDataType, MatrixOrderType aOrderType,
		const BufferRef& b, DataType bDataType, MatrixOrderType bOrderType,
		const BufferRef& c, DataType cDataType, MatrixOrderType cOrderType,
		const BufferRef& d, DataType dDataType, MatrixOrderType dOrderType
	) override {

		MultiplyMatrix<>(
			m, n, k,
			a, aDataType, aOrderType,
			b, bDataType, bOrderType,
			c, cDataType, cOrderType,
			d, dDataType, dOrderType
		);
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

	template<typename... Ts>
	void MultiplyMatrix(
		std::size_t m, std::size_t n, std::size_t k,
		const BufferRef& a, DataType aDataType, MatrixOrderType aOrderType,
		const BufferRef& b, DataType bDataType, MatrixOrderType bOrderType,
		const BufferRef& c, DataType cDataType, MatrixOrderType cOrderType
	) {

		if constexpr (sizeof...(Ts) == 3) {
			using A = typename GetNthType<0, Ts...>::Type;
			using B = typename GetNthType<1, Ts...>::Type;
			using C = typename GetNthType<2, Ts...>::Type;

			const auto aPtr = static_cast<const typename A::Type*>(a->GetHandle());
			const auto bPtr = static_cast<const typename B::Type*>(b->GetHandle());
			const auto cPtr = static_cast<typename C::Type*>(c->GetHandle());

			AddWork([m, n, k, aPtr, bPtr, cPtr]() {
				for (std::size_t row = 0; row < m; ++row) {
					for (std::size_t column = 0; column < k; ++column) {
						typename C::Type result = 0;

						for (std::size_t i = 0; i < n; ++i) {
							result += A::Get(aPtr, m, n, row, i) * B::Get(bPtr, n, k, i, column);
						}

						C::Set(cPtr, m, k, row, column, result);
					}
				}
			});
		} else {
			const DataType dataTypes[] = { aDataType, bDataType, cDataType };
			const MatrixOrderType orderTypes[] = { aOrderType, bOrderType, cOrderType };
			constexpr int index = sizeof...(Ts);

			switch (dataTypes[index]) {
			case DataType::Float32:
				switch (orderTypes[index]) {
				case MatrixOrderType::Default:
				case MatrixOrderType::RowMajor:
					MultiplyMatrix<Ts..., RowMajorMatrixViewer<float>>(
						m, n, k,
						a, aDataType, aOrderType,
						b, bDataType, bOrderType,
						c, cDataType, cOrderType
					);

					break;

				case MatrixOrderType::ColumnMajor:
					MultiplyMatrix<Ts..., ColumnMajorMatrixViewer<float>>(
						m, n, k,
						a, aDataType, aOrderType,
						b, bDataType, bOrderType,
						c, cDataType, cOrderType
					);

					break;
				}

				break;
			}
		}
	}
	template<typename... Ts>
	void MultiplyMatrix(
		std::size_t m, std::size_t n, std::size_t k,
		const BufferRef& a, DataType aDataType, MatrixOrderType aOrderType,
		const BufferRef& b, DataType bDataType, MatrixOrderType bOrderType,
		const BufferRef& c, DataType cDataType, MatrixOrderType cOrderType,
		const BufferRef& d, DataType dDataType, MatrixOrderType dOrderType
	) {

		if constexpr (sizeof...(Ts) == 4) {
			using A = typename GetNthType<0, Ts...>::Type;
			using B = typename GetNthType<1, Ts...>::Type;
			using C = typename GetNthType<2, Ts...>::Type;
			using D = typename GetNthType<3, Ts...>::Type;

			const auto aPtr = static_cast<const typename A::Type*>(a->GetHandle());
			const auto bPtr = static_cast<const typename B::Type*>(b->GetHandle());
			const auto cPtr = static_cast<const typename C::Type*>(c->GetHandle());
			const auto dPtr = static_cast<typename D::Type*>(d->GetHandle());

			AddWork([m, n, k, aPtr, bPtr, cPtr, dPtr]() {
				for (std::size_t row = 0; row < m; ++row) {
					for (std::size_t column = 0; column < k; ++column) {
						typename D::Type result = C::Get(cPtr, m, k, row, column);

						for (std::size_t i = 0; i < n; ++i) {
							result += A::Get(aPtr, m, n, row, i) * B::Get(bPtr, n, k, i, column);
						}

						D::Set(dPtr, m, k, row, column, result);
					}
				}
			});
		} else {
			const DataType dataTypes[] = { aDataType, bDataType, cDataType, dDataType };
			const MatrixOrderType orderTypes[] = { aOrderType, bOrderType, cOrderType, dOrderType };
			constexpr int index = sizeof...(Ts);

			switch (dataTypes[index]) {
			case DataType::Float32:
				switch (orderTypes[index]) {
				case MatrixOrderType::Default:
				case MatrixOrderType::RowMajor:
					MultiplyMatrix<Ts..., RowMajorMatrixViewer<float>>(
						m, n, k,
						a, aDataType, aOrderType,
						b, bDataType, bOrderType,
						c, cDataType, cOrderType,
						d, dDataType, dOrderType
					);

					break;

				case MatrixOrderType::ColumnMajor:
					MultiplyMatrix<Ts..., ColumnMajorMatrixViewer<float>>(
						m, n, k,
						a, aDataType, aOrderType,
						b, bDataType, bOrderType,
						c, cDataType, cOrderType,
						d, dDataType, dOrderType
					);

					break;
				}

				break;
			}
		}
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
	CPUBuffer(Device* device, std::size_t size, std::size_t alignment)
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