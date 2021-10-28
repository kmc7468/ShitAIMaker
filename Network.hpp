#pragma once

#include "Matrix.hpp"

#include <cstddef>
#include <memory>
#include <vector>

class Layer {
public:
	Layer() noexcept = default;
	Layer(const Layer&) = delete;
	virtual ~Layer() = 0;

public:
	Layer& operator=(const Layer&) = delete;

public:
	virtual Matrix Forward(const Matrix& input) = 0;
	virtual Matrix Backward(const Matrix& gradient) = 0;
	virtual Matrix GetLastInput() const = 0;
	virtual Matrix GetLastOutput() const = 0;

	virtual bool HasVariable() const noexcept = 0;
	virtual Matrix GetVariable() const = 0;
	virtual Matrix GetVariableGradient() const = 0;
	virtual void UpdateVariable(const Matrix& delta) = 0;
};

class Optimizer;

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
	void AddLayer(std::unique_ptr<Layer>&& newLayer);

	Matrix Run(const Matrix& input);

	Optimizer* GetOptimizer() noexcept;
	void SetOptimizer(std::unique_ptr<Optimizer>&& optimizer) noexcept;
	void Optimize(std::size_t epochCount);
};