#include "Optimizer.hpp"

#include <cassert>
#include <cmath>

class MSE final : public LossFunction {
public:
	MSE() noexcept = default;
	MSE(const MSE&) = delete;
	virtual ~MSE() override = default;

public:
	MSE& operator=(const MSE&) = delete;

public:
	virtual float Forward(const Matrix& input, const Matrix& target) const override {
		const auto [row, column] = input.GetSize();
		const Matrix error = input - target;
		float result = 0;

		for (std::size_t i = 0; i < row; ++i) {
			float squared = 0;

			for (std::size_t j = 0; j < column; ++j) {
				squared += std::powf(error(i, j), 2);
			}

			result += std::sqrtf(squared);
		}

		return result / row;
	}
	virtual Matrix Backward(const Matrix& input, const Matrix& target) const override {
		return 2 * (input - target);
	}
};

const std::shared_ptr<const LossFunction> LossFunction::MSE = std::make_shared<::MSE>();

void Optimizer::Attach(Network* network) noexcept {
	assert(m_TargetNetwork == nullptr);

	m_TargetNetwork = network;
}
Network* Optimizer::GetTargetNetwork() noexcept {
	return m_TargetNetwork;
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
void SGDOptimizer::SetLearingRate(float newLearningRate) noexcept {
	assert(newLearningRate > 0.f);
	assert(newLearningRate < 1.f);

	m_LearningRate = newLearningRate;
}

void SGDOptimizer::Optimize(const TrainData& trainData, std::size_t epoch) {
	Network* const network = GetTargetNetwork();
	const std::size_t layerCount = network->GetLayerCount();

	const std::shared_ptr<const LossFunction> lossFunction = GetLossFunction();
	const std::size_t sampleCount = trainData.size();

	for (std::size_t i = 0; i < epoch; ++i) {
		for (const auto& sample : trainData) {
			const Matrix output = network->Forward(sample.first);
			const Matrix gradient = lossFunction->Backward(output, sample.second);
			network->Backward(gradient);

			for (std::size_t j = 0; j < layerCount; ++j) {
				Layer* const layer = network->GetLayer(layerCount - j - 1);
				if (!layer->HasVariable()) continue;

				const Matrix layerGradient = layer->GetVariableGradient();
				const Matrix layerDelta = -m_LearningRate * layerGradient;
				layer->UpdateVariable(layerDelta);
			}
		}
	}
}