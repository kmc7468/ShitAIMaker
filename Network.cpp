#include "Network.hpp"

#include "Optimizer.hpp"

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
void Network::RemoveLayer(std::size_t index) noexcept {
	assert(index < m_Layers.size());

	m_Layers.erase(m_Layers.begin() + index);
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
std::size_t Network::GetOutputSize(std::size_t layerIndex) const noexcept {
	std::size_t result = m_Layers[layerIndex]->GetForwardOutputSize();

	if (result > 0) return result;

	for (const auto& layer : std::ranges::views::reverse(
		std::ranges::views::counted(m_Layers.begin(), layerIndex))) {
		result = layer->GetForwardOutputSize();

		if (result > 0) return result;
	}

	for (const auto& layer : std::ranges::subrange(m_Layers.begin() + layerIndex + 1, m_Layers.end())) {
		result = layer->GetForwardInputSize();

		if (result > 0) return result;
	}

	return 0;
}

NetworkDump Network::GetDump() const {
	std::vector<LayerDump> layers;

	layers.emplace_back(GetInputSize());

	for (auto& layer : m_Layers) {
		layers.push_back(layer->GetDump(layers.back()));
	}

	return NetworkDump(std::move(layers));
}

bool Network::HasOptimizer() const noexcept {
	return m_Optimizer != nullptr;
}
const Optimizer& Network::GetOptimizer() const noexcept {
	return *m_Optimizer.get();
}
Optimizer& Network::GetOptimizer() noexcept {
	return *m_Optimizer.get();
}
void Network::SetOptimizer(std::unique_ptr<Optimizer>&& optimizer) noexcept {
	m_Optimizer = std::move(optimizer);
	m_Optimizer->Attach(*this);
}

void Network::Optimize(const TrainData& trainData, std::size_t epoch) {
	assert(!m_Layers.empty());
	assert(m_Optimizer != nullptr);

	m_Optimizer->Optimize(trainData, epoch);
}

NetworkDump::NetworkDump(std::vector<LayerDump>&& layers) noexcept
	: m_Layers(std::move(layers)) {}

const std::vector<LayerDump>& NetworkDump::GetLayers() const noexcept {
	return m_Layers;
}