#pragma once

#include "Layer.hpp"
#include "Matrix.hpp"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

class Optimizer;

using TrainSample = std::pair<Matrix, Matrix>;
using TrainData = std::vector<TrainSample>;

class NetworkDump;

class Network final {
private:
	std::vector<std::unique_ptr<Layer>> m_Layers;
	std::unique_ptr<Optimizer> m_Optimizer;

public:
	Network() noexcept = default;
	Network(const Network&) = delete;
	~Network() = default;

public:
	Network& operator=(const Network&) = delete;

public:
	const Layer& GetLayer(std::size_t index) const noexcept;
	Layer& GetLayer(std::size_t index) noexcept;
	std::size_t GetLayerCount() const noexcept;
	void AddLayer(std::unique_ptr<Layer>&& newLayer);
	void RemoveLayer(std::size_t index) noexcept;

	Matrix Forward(const Matrix& input);
	void Backward(const Matrix& input);
	std::size_t GetInputSize() const noexcept;
	std::size_t GetOutputSize() const noexcept;
	std::size_t GetOutputSize(std::size_t layerIndex) const noexcept;

	NetworkDump GetDump() const;

	bool HasOptimizer() const noexcept;
	const Optimizer& GetOptimizer() const noexcept;
	Optimizer& GetOptimizer() noexcept;
	void SetOptimizer(std::unique_ptr<Optimizer>&& optimizer) noexcept;
	void Optimize(const TrainData& trainData, std::size_t epoch);
};

class NetworkDump final {
private:
	std::vector<LayerDump> m_Layers;

public:
	NetworkDump(std::vector<LayerDump>&& layers) noexcept;
	NetworkDump(const NetworkDump&) = delete;
	NetworkDump(NetworkDump&& networkDump) noexcept = default;
	~NetworkDump() = default;

public:
	NetworkDump& operator=(const NetworkDump&) = delete;
	NetworkDump& operator=(NetworkDump&& networkDump) noexcept = default;

public:
	const std::vector<LayerDump>& GetLayers() const noexcept;
};