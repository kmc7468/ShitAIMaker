#include "Network.hpp"

#include "Optimizer.hpp"

#include <cassert>
#include <random>

FFLayer::FFLayer(std::size_t inputSize, std::size_t outputSize)
	: m_Weights(inputSize, outputSize), m_Biases(1, outputSize) {
	std::mt19937 mt(std::random_device{}());
	std::uniform_real_distribution<float> dist(-1.f, 1.f);

	for (std::size_t i = 0; i < inputSize; ++i) {
		for (std::size_t j = 0; j < outputSize; ++j) {
			m_Weights(i, j) = dist(mt);

			if (i == 0) {
				m_Biases(i, j) = dist(mt);
			}
		}
	}
}

Matrix FFLayer::Forward(const Matrix& input) {
	m_LastInput = input;
	m_LastOutput = m_Weights * input + m_Biases;

	return m_LastOutput;
}
Matrix FFLayer::Backward(const Matrix& gradient) {
	m_WeightGradients = gradient * Transpose(m_LastInput);
	m_BiasGradients = gradient * Matrix(m_LastInput.GetRowSize(), 1, 1.f);

	return Transpose(m_LastInput) * gradient;
}
Matrix FFLayer::GetLastInput() const {
	return m_LastInput;
}
Matrix FFLayer::GetLastOutput() const {
	return m_LastOutput;
}

bool FFLayer::HasVariable() const noexcept {
	return true;
}
std::vector<Matrix> FFLayer::GetVariables() const {
	return { m_Weights, m_Biases };
}
std::vector<Matrix> FFLayer::GetVariableGradients() const {
	return { m_WeightGradients, m_BiasGradients };
}
void FFLayer::UpdateVariables(const std::vector<Matrix>& deltas) {
	assert(deltas.size() == 2);

	m_Weights += deltas[0];
	m_Biases += deltas[1];
}

ActivationLayer::ActivationLayer(ActivationFunction primitiveFunction, ActivationFunction derivativeFunction) noexcept
	: m_PrimitiveFunction(primitiveFunction), m_DerivativeFunction(derivativeFunction) {}

Layer* Network::GetLayer(std::size_t index) noexcept {
	return m_Layers[index].get();
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
void Network::Backward(const Matrix& gradient) {
	assert(!m_Layers.empty());

	Matrix nextGradient = gradient;

	for (auto& layer : m_Layers) {
		nextGradient = layer->Backward(nextGradient);
	}
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

Matrix ActivationLayer::Forward(const Matrix& input) {
	m_LastInput = input;
	m_LastOutput = input;

	const auto [row, column] = input.GetSize();

	for (std::size_t i = 0; i < row; ++i) {
		for (std::size_t j = 0; j < column; ++j) {
			m_LastOutput(i, j) = m_PrimitiveFunction(m_LastOutput(i, j));
		}
	}

	return m_LastOutput;
}
Matrix ActivationLayer::Backward(const Matrix& gradient) {
	Matrix myGradient = m_LastInput;
	const auto [row, column] = m_LastInput.GetSize();

	for (std::size_t i = 0; i < row; ++i) {
		for (std::size_t j = 0; j < column; ++j) {
			myGradient(i, j) = m_DerivativeFunction(myGradient(i, j));
		}
	}

	return myGradient.HadamardProduct(gradient);
}
Matrix ActivationLayer::GetLastInput() const {
	return m_LastInput;
}
Matrix ActivationLayer::GetLastOutput() const {
	return m_LastOutput;
}

bool ActivationLayer::HasVariable() const noexcept {
	return false;
}
std::vector<Matrix> ActivationLayer::GetVariables() const {
	return {};
}
std::vector<Matrix> ActivationLayer::GetVariableGradients() const {
	return {};
}
void ActivationLayer::UpdateVariables(const std::vector<Matrix>&) {}