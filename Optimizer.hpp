#pragma once

#include "Matrix.hpp"
#include "Network.hpp"

#include <cstddef>
#include <memory>

class LossFunction {
public:
	static const std::shared_ptr<const LossFunction> MSE;

public:
	LossFunction() noexcept = default;
	LossFunction(const LossFunction&) = delete;
	virtual ~LossFunction() = default;

public:
	LossFunction& operator=(const LossFunction&) = delete;

public:
	virtual float Forward(const Matrix& input, const Matrix& target) const = 0;
	virtual Matrix Backward(const Matrix& input, const Matrix& target) const = 0;
};

class Optimizer {
private:
	Network* m_TargetNetwork = nullptr;

	std::shared_ptr<const LossFunction> m_LossFunction;

public:
	Optimizer() noexcept = default;
	Optimizer(const Optimizer&) = delete;
	virtual ~Optimizer() = default;

public:
	Optimizer& operator=(const Optimizer&) = delete;

public:
	void Attach(Network* network) noexcept;
	Network* GetTargetNetwork() noexcept;

	std::shared_ptr<const LossFunction> GetLossFunction() const noexcept;
	void SetLossFunction(const std::shared_ptr<const LossFunction>& lossFunction) noexcept;

	virtual void Optimize(const TrainData& trainData, std::size_t epoch) = 0;
};

class SGDOptimizer final : public Optimizer {
private:
	float m_LearningRate = 0.1f;

public:
	SGDOptimizer() noexcept = default;
	SGDOptimizer(const SGDOptimizer&) = delete;
	virtual ~SGDOptimizer() override = default;

public:
	SGDOptimizer& operator=(const SGDOptimizer&) = delete;

public:
	float GetLearningRate() const noexcept;
	void SetLearingRate(float newLearningRate) noexcept;

	virtual void Optimize(const TrainData& trainData, std::size_t epoch) override;
};