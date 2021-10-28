#include "Network.hpp"

#include <utility>

void Network::AddLayer(std::unique_ptr<Layer>&& newLayer) {
	m_Layers.push_back(std::move(newLayer));
}

Matrix Network::Run(const Matrix& input) {
	Matrix output = input;

	for (auto& layer : m_Layers) {
		output = layer->Forward(output);
	}

	return output;
}