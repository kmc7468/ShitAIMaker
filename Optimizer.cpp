#include "Optimizer.hpp"

#include "Layer.hpp"

#include <cassert>
#include <cmath>

class MSEImpl final : public LossFunction {
public:
	MSEImpl() noexcept = default;
	MSEImpl(const MSEImpl&) = delete;
	virtual ~MSEImpl() override = default;

public:
	MSEImpl& operator=(const MSEImpl&) = delete;

public:
	virtual float Forward(const Matrix& input, const Matrix& target) const override {
		const auto [row, column] = input.GetSize();
		const Matrix error = input - target;
		float result = 0;

		for (std::size_t i = 0; i < column; ++i) {
			float squared = 0;

			for (std::size_t j = 0; j < row; ++j) {
				squared += std::powf(error(j, i), 2);
			}

			result += std::sqrtf(squared);
		}

		return result / column;
	}
	virtual Matrix Backward(const Matrix& input, const Matrix& target) const override {
		return (2.f / input.GetColumnSize()) * (input - target);
	}
};

const std::shared_ptr<const LossFunction> MSE = std::make_shared<MSEImpl>();

void Optimizer::Attach(Network& network) noexcept {
	assert(m_TargetNetwork == nullptr);

	m_TargetNetwork = &network;
}
const Network& Optimizer::GetTargetNetwork() const noexcept {
	return *m_TargetNetwork;
}
Network& Optimizer::GetTargetNetwork() noexcept {
	return *m_TargetNetwork;
}

std::shared_ptr<const LossFunction> Optimizer::GetLossFunction() const noexcept {
	return m_LossFunction;
}
void Optimizer::SetLossFunction(const std::shared_ptr<const LossFunction>& lossFunction) noexcept {
	assert(m_LossFunction == nullptr);

	m_LossFunction = lossFunction;
}

float SGDOptimizer::GetLearningRate() const noexcept {
	return m_LearningRate;
}
void SGDOptimizer::SetLearningRate(float newLearningRate) noexcept {
	assert(newLearningRate > 0.f);
	assert(newLearningRate <= 1.f);

	m_LearningRate = newLearningRate;
}

void SGDOptimizer::Optimize(const TrainData& trainData, std::size_t epoch) {
	Network& network = GetTargetNetwork();
	const std::size_t layerCount = network.GetLayerCount();

	const auto lossFunction = GetLossFunction();
	const std::size_t sampleCount = trainData.size();

	for (std::size_t i = 0; i < epoch; ++i) {
		for (const auto& sample : trainData) {
			const Matrix output = network.Forward(sample.first);
			const Matrix gradient = lossFunction->Backward(output, sample.second);
			network.Backward(gradient);

			for (std::size_t j = 0; j < layerCount; ++j) {
				for (auto& parameter : network.GetLayer(layerCount - j - 1).GetParameterTable().GetAllParameters()) {
					parameter.GetValue() -= m_LearningRate * parameter.GetGradient();
				}
			}
		}
	}
}