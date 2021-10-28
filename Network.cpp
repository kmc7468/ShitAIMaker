#include "Network.hpp"

#include "Optimizer.hpp"

#include <cassert>

void Network::AddLayer(std::unique_ptr<Layer>&& newLayer) {
	assert(newLayer != nullptr);

	m_Layers.push_back(std::move(newLayer));
}

Matrix Network::Run(const Matrix& input) {
	assert(!m_Layers.empty());

	Matrix nextInput = input;

	for (auto& layer : m_Layers) {
		nextInput = layer->Forward(nextInput);
	}

	return nextInput;
}

Optimizer* Network::GetOptimizer() noexcept {
	return m_Optimizer.get();
}
void Network::SetOptimizer(std::unique_ptr<Optimizer>&& optimizer) noexcept {
	assert(m_Optimizer == nullptr);

	m_Optimizer = std::move(optimizer);
	m_Optimizer->Attach(this);
}

void Network::Optimize(const TrainData& trainData, std::size_t epoch) {
	assert(!m_Layers.empty());
	assert(m_Optimizer != nullptr);

	m_Optimizer->Optimize(trainData, epoch);
}