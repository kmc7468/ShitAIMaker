#include "Network.hpp"

#include "Optimizer.hpp"

#include <algorithm>
#include <cassert>
#include <ranges>

const Layer& Network::GetLayer(std::size_t index) const noexcept {
	return *m_Layers[index].get();
}
Layer& Network::GetLayer(std::size_t index) noexcept {
	return *m_Layers[index].get();
}
std::size_t Network::GetLayerCount() const noexcept {
	return m_Layers.size();
}
void Network::AddLayer(std::unique_ptr<Layer>&& newLayer) {
	assert(newLayer != nullptr);

	m_Layers.push_back(std::move(newLayer));
}

Matrix Network::Forward(const Matrix& input) {
	assert(!m_Layers.empty());

	Matrix nextInput = input;

	for (auto& layer : m_Layers) {
		nextInput = layer->Forward(nextInput);
	}

	return nextInput;
}
void Network::Backward(const Matrix& input) {
	assert(!m_Layers.empty());

	Matrix nextInput = input;

	for (auto& layer : std::ranges::views::reverse(m_Layers)) {
		nextInput = layer->Backward(nextInput);
	}
}
std::size_t Network::GetInputSize() const noexcept {
	assert(!m_Layers.empty());

	for (const auto& layer : m_Layers) {
		const std::size_t layerInputSize = layer->GetForwardInputSize();

		if (layerInputSize > 0) return layerInputSize;
	}

	return 0;
}
std::size_t Network::GetOutputSize() const noexcept {
	assert(!m_Layers.empty());

	for (const auto& layer : std::ranges::views::reverse(m_Layers)) {
		const std::size_t layerOutputSize = layer->GetForwardOutputSize();

		if (layerOutputSize > 0) return layerOutputSize;
	}

	return 0;
}

const Optimizer& Network::GetOptimizer() const noexcept {
	return *m_Optimizer.get();
}
Optimizer& Network::GetOptimizer() noexcept {
	return *m_Optimizer.get();
}
void Network::SetOptimizer(std::unique_ptr<Optimizer>&& optimizer) noexcept {
	assert(m_Optimizer == nullptr);

	m_Optimizer = std::move(optimizer);
	m_Optimizer->Attach(*this);
}

void Network::Optimize(const TrainData& trainData, std::size_t epoch) {
	assert(!m_Layers.empty());
	assert(m_Optimizer != nullptr);

	m_Optimizer->Optimize(trainData, epoch);
}