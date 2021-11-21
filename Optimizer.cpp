#include "Optimizer.hpp"

#include "Layer.hpp"

#include <cassert>
#include <cmath>
#include <utility>

LossFunction::LossFunction(std::string name) noexcept
	: m_Name(std::move(name)) {}

std::string_view LossFunction::GetName() const noexcept {
	return m_Name;
}

class MSEImpl final : public LossFunction {
public:
	MSEImpl()
		: LossFunction("MSE") {}
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
			for (std::size_t j = 0; j < row; ++j) {
				result += std::powf(error(j, i), 2);
			}
		}

		return result / column;
	}
	virtual Matrix Backward(const Matrix& input, const Matrix& target) const override {
		return (2.f / input.GetColumnSize()) * (input - target);
	}
};

const std::shared_ptr<const LossFunction> MSE = std::make_shared<MSEImpl>();

Optimizer::Optimizer(std::string name) noexcept
	: m_Name(std::move(name)) {}
Optimizer::Optimizer(const Optimizer& other)
	: m_Name(other.m_Name), m_LossFunction(other.m_LossFunction) {}

std::string_view Optimizer::GetName() const noexcept {
	return m_Name;
}
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
	m_LossFunction = lossFunction;
}

SGDOptimizer::SGDOptimizer()
	: Optimizer("SGDOptimizer") {}
SGDOptimizer::SGDOptimizer(const SGDOptimizer& other)
	: Optimizer(other), m_LearningRate(other.m_LearningRate) {}

float SGDOptimizer::GetLearningRate() const noexcept {
	return m_LearningRate;
}
void SGDOptimizer::SetLearningRate(float newLearningRate) noexcept {
	assert(newLearningRate > 0.f);
	assert(newLearningRate <= 1.f);

	m_LearningRate = newLearningRate;
}

std::unique_ptr<Optimizer> SGDOptimizer::Copy() const {
	return std::make_unique<SGDOptimizer>(*this);
}

void SGDOptimizer::Optimize(const TrainData& trainData, std::size_t epoch) {
	assert(trainData.size() > 0);
	assert(epoch > 0);

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