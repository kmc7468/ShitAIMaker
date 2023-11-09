#include "PALComputing.hpp"

#include <cublas_v2.h>
#include <cuda_runtime.h>
#include <memory>
#include <stdexcept>

#ifdef SAI_USE_CUBLAS
class NVIDIADevice final : public Device {
private:
	cublasHandle_t m_CuBLASHandle;
	cudaStream_t m_CudaStream;

public:
	NVIDIADevice()
		: Device("NVIDIA GPU", DeviceType::GPU) {

		if (cublasCreate(&m_CuBLASHandle) != CUBLAS_STATUS_SUCCESS) {
			throw std::runtime_error("Failed to create a cuBLAS handle");
		} else if (cudaStreamCreate(&m_CudaStream) != cudaSuccess) {
			cublasDestroy(m_CuBLASHandle);

			throw std::runtime_error("Failed to create a CUDA stream");
		} else if (cublasSetStream(m_CuBLASHandle, m_CudaStream) != CUBLAS_STATUS_SUCCESS) {
			cublasDestroy(m_CuBLASHandle);
			cudaStreamDestroy(m_CudaStream);

			throw std::runtime_error("Failed to set a CUDA stream to a cuBLAS handle");
		}
	}
	NVIDIADevice(const NVIDIADevice&) = delete;
	virtual ~NVIDIADevice() override {
		cublasDestroy(m_CuBLASHandle);
		cudaStreamDestroy(m_CudaStream);
	}

public:
	NVIDIADevice& operator=(const NVIDIADevice&) = delete;

protected:
	virtual BufferRef PALCreateBuffer(std::size_t elementSize,
		std::size_t elementCount, std::size_t elementAlignment) override;
	virtual void PALReadBuffer(void* dest, const BufferRef& src) override {
		if (cudaMemcpy(dest, src->GetHandle(), src->GetSize(),
			cudaMemcpyDeviceToHost) != cudaSuccess) {

			throw std::runtime_error("Failed to read a buffer");
		}
	}
	virtual void PALReadBufferAsync(void* dest, const BufferRef& src) override {
		if (cudaMemcpyAsync(dest, src->GetHandle(), src->GetSize(),
			cudaMemcpyDeviceToHost, m_CudaStream) != cudaSuccess) {
			throw std::runtime_error("Failed to read a buffer asynchronously");
		}
	}
	virtual void PALWriteBuffer(const BufferRef& dest, const void* src) override {
		if (cudaMemcpy(dest->GetHandle(), src, dest->GetSize(),
			cudaMemcpyHostToDevice) != cudaSuccess) {

			throw std::runtime_error("Failed to write a buffer");
		}
	}
	virtual void PALWriteBufferAsync(const BufferRef& dest, const void* src) override {
		if (cudaMemcpyAsync(dest->GetHandle(), src, dest->GetSize(),
			cudaMemcpyHostToDevice, m_CudaStream) != cudaSuccess) {

			throw std::runtime_error("Failed to write a buffer asynchronously");
		}
	}
	virtual void PALCopyBuffer(const BufferRef& dest, const BufferRef& src) override {
		if (cudaMemcpy(dest->GetHandle(), src->GetHandle(),
			std::min(dest->GetSize(), src->GetSize()),
			cudaMemcpyDeviceToDevice) != cudaSuccess) {

			throw std::runtime_error("Failed to copy a buffer");
		}
	}
	virtual void PALCopyBufferAsync(const BufferRef& dest, const BufferRef& src) override {
		if (cudaMemcpyAsync(dest->GetHandle(), src->GetHandle(),
			std::min(dest->GetSize(), src->GetSize()),
			cudaMemcpyDeviceToDevice, m_CudaStream) != cudaSuccess) {

			throw std::runtime_error("Failed to copy a buffer asynchronously");
		}
	}

	virtual void PALMultiplyMatrixAsync(
		std::size_t m, std::size_t n, std::size_t k,
		const BufferRef& a, DataType aDataType, MatrixOrderType aOrderType,
		const BufferRef& b, DataType bDataType, MatrixOrderType bOrderType,
		const BufferRef& c, DataType cDataType, MatrixOrderType cOrderType
	) override {

		if (aDataType != DataType::Float32 ||
			bDataType != DataType::Float32 ||
			cDataType != DataType::Float32) {

			throw std::runtime_error("Unsupported data type");
		} else if (cOrderType == MatrixOrderType::RowMajor) {
			throw std::runtime_error("Unsupported matrix order type");
		}

		const float alpha = 1.f;
		const float beta = 0.f;

		if (cublasGemmEx(
			m_CuBLASHandle,
			ToCuBLASOperation(aOrderType), ToCuBLASOperation(bOrderType),
			static_cast<int>(m), static_cast<int>(k), static_cast<int>(n),
			&alpha,
			a->GetHandle(), ToCudaDataType(aDataType), static_cast<int>(n),
			b->GetHandle(), ToCudaDataType(bDataType), static_cast<int>(k),
			&beta,
			c->GetHandle(), ToCudaDataType(cDataType), static_cast<int>(m),
			CUBLAS_COMPUTE_32F, CUBLAS_GEMM_DEFAULT_TENSOR_OP
		) != CUBLAS_STATUS_SUCCESS) {

			throw std::runtime_error("Failed to multiply matrices");
		}
	}
	virtual void PALMultiplyMatrixAsync(
		std::size_t m, std::size_t n, std::size_t k,
		const BufferRef& a, DataType aDataType, MatrixOrderType aOrderType,
		const BufferRef& b, DataType bDataType, MatrixOrderType bOrderType,
		const BufferRef& c, DataType cDataType, MatrixOrderType cOrderType,
		const BufferRef& d, DataType dDataType, MatrixOrderType dOrderType
	) override {

		if (aDataType != DataType::Float32 ||
			bDataType != DataType::Float32 ||
			cDataType != DataType::Float32 ||
			dDataType != DataType::Float32) {

			throw std::runtime_error("Unsupported data type");
		} else if (cOrderType == MatrixOrderType::RowMajor ||
			dOrderType == MatrixOrderType::RowMajor) {

			throw std::runtime_error("Unsupported matrix order type");
		}

		PALCopyBufferAsync(d, c);

		const float alpha = 1.f;
		const float beta = 1.f;

		if (cublasGemmEx(
			m_CuBLASHandle,
			ToCuBLASOperation(aOrderType), ToCuBLASOperation(bOrderType),
			static_cast<int>(m), static_cast<int>(k), static_cast<int>(n),
			&alpha,
			a->GetHandle(), ToCudaDataType(aDataType), static_cast<int>(n),
			b->GetHandle(), ToCudaDataType(bDataType), static_cast<int>(k),
			&beta,
			d->GetHandle(), ToCudaDataType(dDataType), static_cast<int>(m),
			CUBLAS_COMPUTE_32F, CUBLAS_GEMM_DEFAULT_TENSOR_OP
		) != CUBLAS_STATUS_SUCCESS) {

			throw std::runtime_error("Failed to multiply matrices");
		}
	}

	virtual void PALJoin() override {
		if (cudaStreamSynchronize(m_CudaStream) != cudaSuccess) {
			throw std::runtime_error("Failed to join a CUDA stream");
		}
	}

private:
	static cudaDataType_t ToCudaDataType(DataType dataType) noexcept {
		switch (dataType) {
		case DataType::Float32:
			return CUDA_R_32F;
		}
	}
	static cublasOperation_t ToCuBLASOperation(MatrixOrderType orderType) noexcept {
		switch (orderType) {
		case MatrixOrderType::RowMajor:
			return CUBLAS_OP_T;

		case MatrixOrderType::Default:
		case MatrixOrderType::ColumnMajor:
			return CUBLAS_OP_N;
		}
	}
};

DeviceRef PALInitializeComputingForNVIDIA() {
	return std::make_shared<NVIDIADevice>();
}
void PALFinalizeComputingForNVIDIA(DeviceRef&) {}

class NVIDIABuffer final : public Buffer {
private:
	void* m_Buffer;

public:
	NVIDIABuffer(Device* device, std::size_t size, std::size_t)
		: Buffer(device, size, 0) {

		if (cudaMalloc(&m_Buffer, size) != cudaSuccess)
			throw std::runtime_error("Failed to allocate a buffer");
	}
	NVIDIABuffer(const NVIDIABuffer&) = delete;
	virtual ~NVIDIABuffer() override {
		cudaFree(m_Buffer);
	}

public:
	NVIDIABuffer& operator=(const NVIDIABuffer&) = delete;

protected:
	virtual void* PALGetHandle() noexcept override {
		return m_Buffer;
	}
};

BufferRef NVIDIADevice::PALCreateBuffer(std::size_t elementSize,
	std::size_t elementCount, std::size_t elementAlignment) {

	return std::make_shared<NVIDIABuffer>(this,
		elementSize * elementCount, elementAlignment);
}
#else
DeviceRef PALInitializeComputingForNVIDIA() {
	return nullptr;
}
void PALFinalizeComputingForNVIDIA(DeviceRef&) {}
#endif