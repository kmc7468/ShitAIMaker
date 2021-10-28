#pragma once

#include "Matrix.hpp"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

class Layer {
public:
	Layer() noexcept = default;
	Layer(const Layer&) = delete;
	virtual ~Layer() = default;

public:
	Layer& operator=(const Layer&) = delete;

public:
	virtual Matrix Forward(const Matrix& input) = 0;
	virtual Matrix Backward(const Matrix& gradient) = 0;
	virtual Matrix GetLastInput() const = 0;
	virtual Matrix GetLastOutput() const = 0;

	virtual bool HasVariable() const noexcept = 0;
	virtual std::vector<Matrix> GetVariables() const = 0;
	virtual std::vector<Matrix> GetVariableGradients() const = 0;
	virtual void UpdateVariables(const std::vector<Matrix>& deltas) = 0;
};

class FFLayer final : public Layer {
private:
	Matrix m_Weights, m_Biases;
	Matrix m_LastInput, m_LastOutput;
	Matrix m_WeightGradients, m_BiasGradients;

public:
	FFLayer(std::size_t inputSize, std::size_t outputSize);
	FFLayer(const FFLayer&) = delete;
	virtual ~FFLayer() override = default;

public:
	FFLayer& operator=(const FFLayer&) = delete;

public:
	virtual Matrix Forward(const Matrix& input) override;
	virtual Matrix Backward(const Matrix& gradient) override;
	virtual Matrix GetLastInput() const override;
	virtual Matrix GetLastOutput() const override;

	virtual bool HasVariable() const noexcept override;
	virtual std::vector<Matrix> GetVariables() const override;
	virtual std::vector<Matrix> GetVariableGradients() const override;
	virtual void UpdateVariables(const std::vector<Matrix>& deltas) override;
};

using ActivationFunction = float(*)(float);

float Sigmoid(float x);
float DSigmoid(float x);
float Tanh(float x);
float DTanh(float x);
float ReLU(float x);
float DReLU(float x);
float LeakyReLU(float x);
float DLeakyReLU(float x);

class ActivationLayer final : public Layer {
private:
	ActivationFunction m_PrimitiveFunction, m_DerivativeFunction;
	Matrix m_LastInput, m_LastOutput;

public:
	ActivationLayer(ActivationFunction primitiveFunction, ActivationFunction derivativeFunction) noexcept;
	ActivationLayer(const ActivationLayer&) = delete;
	virtual ~ActivationLayer() override = default;

public:
	FFLayer& operator=(const FFLayer&) = delete;

public:
	virtual Matrix Forward(const Matrix& input) override;
	virtual Matrix Backward(const Matrix& gradient) override;
	virtual Matrix GetLastInput() const override;
	virtual Matrix GetLastOutput() const override;

	virtual bool HasVariable() const noexcept override;
	virtual std::vector<Matrix> GetVariables() const override;
	virtual std::vector<Matrix> GetVariableGradients() const override;
	virtual void UpdateVariables(const std::vector<Matrix>& deltas) override;
};

class Optimizer;

using TrainSample = std::pair<Matrix, Matrix>;
using TrainData = std::vector<TrainSample>;

class Network final {
private:
	std::vector<std::unique_ptr<Layer>> m_Layers;
	std::unique_ptr<Optimizer> m_Optimizer;

public:
	Network() noexcept = default;
	Network(const Network&) = delete;
	~Network() = default;

public:
	Network& operator=(const Network&) = delete;

public:
	Layer* GetLayer(std::size_t index) noexcept;
	std::size_t GetLayerCount() const noexcept;
	void AddLayer(std::unique_ptr<Layer>&& newLayer);

	Matrix Forward(const Matrix& input);
	void Backward(const Matrix& gradient);

	Optimizer* GetOptimizer() noexcept;
	void SetOptimizer(std::unique_ptr<Optimizer>&& optimizer) noexcept;
	void Optimize(const TrainData& trainData, std::size_t epoch);
};