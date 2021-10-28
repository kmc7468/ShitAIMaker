#pragma once

#include "Matrix.hpp"

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

class Network final {
private:
	std::vector<std::unique_ptr<Layer>> m_Layers;

public:
	Network() noexcept = default;
	Network(Network&& network) noexcept = default;
	~Network() = default;

public:
	Network& operator=(Network&& network) noexcept = default;

public:
	void AddLayer(std::unique_ptr<Layer>&& newLayer);

	Matrix Run(const Matrix& input);
};