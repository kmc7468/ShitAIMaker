#pragma once

#include "Matrix.hpp"
#include "Network.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

class LossFunction {
private:
	std::string m_Name;

public:
	LossFunction(std::string name) noexcept;
	LossFunction(const LossFunction&) = delete;
	virtual ~LossFunction() = default;

public:
	LossFunction& operator=(const LossFunction&) = delete;

public:
	std::string_view GetName() const noexcept;

	virtual float Forward(const Matrix& input, const Matrix& target) const = 0;
	virtual Matrix Backward(const Matrix& input, const Matrix& target) const = 0;
};

extern const std::shared_ptr<const LossFunction> MSE;

class Optimizer {
private:
	std::string m_Name;
	Network* m_TargetNetwork = nullptr;

	std::shared_ptr<const LossFunction> m_LossFunction;

public:
	Optimizer(std::string name) noexcept;
	Optimizer(const Optimizer&) = delete;
	virtual ~Optimizer() = default;

public:
	Optimizer& operator=(const Optimizer&) = delete;

public:
	std::string_view GetName() const noexcept;
	void Attach(Network& network) noexcept;
	const Network& GetTargetNetwork() const noexcept;
	Network& GetTargetNetwork() noexcept;

	std::shared_ptr<const LossFunction> GetLossFunction() const noexcept;
	void SetLossFunction(const std::shared_ptr<const LossFunction>& lossFunction) noexcept;

	virtual void Optimize(const TrainData& trainData, std::size_t epoch) = 0;
};

class SGDOptimizer final : public Optimizer {
private:
	float m_LearningRate = 0.1f;

public:
	SGDOptimizer();
	SGDOptimizer(const SGDOptimizer&) = delete;
	virtual ~SGDOptimizer() override = default;

public:
	SGDOptimizer& operator=(const SGDOptimizer&) = delete;

public:
	float GetLearningRate() const noexcept;
	void SetLearningRate(float newLearningRate) noexcept;

	virtual void Optimize(const TrainData& trainData, std::size_t epoch) override;
};