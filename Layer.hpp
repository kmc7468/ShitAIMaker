#pragma once

#include "Matrix.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

class ReadonlyVariable;

class Variable final {
	friend class ReadonlyVariable;

private:
	std::map<std::string, Matrix>::iterator m_Iterator;

public:
	Variable(std::map<std::string, Matrix>::iterator iterator) noexcept;
	Variable(const Variable& other) noexcept = default;
	~Variable() = default;

public:
	Variable& operator=(const Variable& other) noexcept = default;
	bool operator==(const Variable& other) noexcept;

public:
	std::string_view GetName() const noexcept;
	Matrix& GetValue() const noexcept;
	Matrix& SetValue(Matrix newValue) const noexcept;
};

class ReadonlyVariable final {
private:
	std::map<std::string, Matrix>::const_iterator m_Iterator;

public:
	ReadonlyVariable(std::map<std::string, Matrix>::const_iterator iterator) noexcept;
	ReadonlyVariable(const Variable& variable) noexcept;
	ReadonlyVariable(const ReadonlyVariable& other) noexcept = default;
	~ReadonlyVariable() = default;

public:
	ReadonlyVariable& operator=(const Variable& variable) noexcept;
	ReadonlyVariable& operator=(const ReadonlyVariable& other) noexcept = default;
	bool operator==(const Variable& variable) noexcept;
	bool operator==(const ReadonlyVariable& other) noexcept;

public:
	std::string_view GetName() const noexcept;
	const Matrix& GetValue() const noexcept;
};

class VariableTable final {
private:
	std::map<std::string, Matrix> m_Variables;

public:
	VariableTable() = default;
	VariableTable(const VariableTable&) = delete;
	~VariableTable() = default;

public:
	VariableTable& operator=(const VariableTable&) = delete;

public:
	ReadonlyVariable GetVariable(const std::string& name) const noexcept;
	Variable GetVariable(const std::string& name) noexcept;
	std::vector<ReadonlyVariable> GetAllVariables() const;
	std::vector<Variable> GetAllVariables();
	Variable AddVariable(std::string name, Matrix initialValue = {});
};

class ReadonlyParameter;

class Parameter final {
	friend class ReadonlyParameter;

private:
	std::map<std::string,
		std::tuple<Matrix, Matrix, std::unique_ptr<VariableTable>>>::iterator m_Iterator;

public:
	Parameter(std::map<std::string,
		std::tuple<Matrix, Matrix, std::unique_ptr<VariableTable>>>::iterator iterator) noexcept;
	Parameter(const Parameter& other) noexcept = default;
	~Parameter() = default;

public:
	Parameter& operator=(const Parameter& other) noexcept = default;
	bool operator==(const Parameter& other) noexcept;

public:
	std::string_view GetName() const noexcept;
	Matrix& GetValue() const noexcept;
	Matrix& SetValue(Matrix newValue) const noexcept;
	Matrix& GetGradient() const noexcept;
	Matrix& SetGradient(Matrix newGradient) const noexcept;
	VariableTable& GetVariableTable() const noexcept;
};

class ReadonlyParameter final {
private:
	std::map<std::string,
		std::tuple<Matrix, Matrix, std::unique_ptr<VariableTable>>>::const_iterator m_Iterator;

public:
	ReadonlyParameter(std::map<std::string,
		std::tuple<Matrix, Matrix, std::unique_ptr<VariableTable>>>::const_iterator iterator) noexcept;
	ReadonlyParameter(const Parameter& parameter) noexcept;
	ReadonlyParameter(const ReadonlyParameter& other) noexcept = default;
	~ReadonlyParameter() = default;

public:
	ReadonlyParameter& operator=(const Parameter& parameter) noexcept;
	ReadonlyParameter& operator=(const ReadonlyParameter& other) noexcept = default;
	bool operator==(const Parameter& parameter) noexcept;
	bool operator==(const ReadonlyParameter& other) noexcept;

public:
	std::string_view GetName() const noexcept;
	const Matrix& GetValue() const noexcept;
	const Matrix& GetGradient() const noexcept;
	const VariableTable& GetVariableTable() const noexcept;
};

class ParameterTable final {
private:
	std::map<std::string, std::tuple<Matrix, Matrix, std::unique_ptr<VariableTable>>> m_Parameters;

public:
	ParameterTable() = default;
	ParameterTable(const ParameterTable&) = delete;
	~ParameterTable() = default;

public:
	ParameterTable& operator=(const ParameterTable&) = delete;

public:
	ReadonlyParameter GetParameter(const std::string& name) const noexcept;
	Parameter GetParameter(const std::string& name) noexcept;
	std::vector<ReadonlyParameter> GetAllParameters() const;
	std::vector<Parameter> GetAllParameters();
	Parameter AddParameter(std::string name, Matrix initialValue = {});
};

class Layer {
private:
	std::string m_Name;
	VariableTable m_VariableTable;
	ParameterTable m_ParameterTable;

	Variable m_LastForwardInput, m_LastForwardOutput;
	Variable m_LastBackwardInput, m_LastBackwardOutput;

public:
	Layer(std::string name);
	Layer(const Layer&) = delete;
	virtual ~Layer() = default;

public:
	Layer& operator=(const Layer&) = delete;

public:
	std::string_view GetName() const noexcept;

	Matrix Forward(const Matrix& input);
	const Matrix& GetLastForwardInput() const noexcept;
	const Matrix& GetLastForwardOutput() const noexcept;
	virtual std::size_t GetForwardInputSize() const noexcept = 0;
	virtual std::size_t GetForwardOutputSize() const noexcept = 0;

	Matrix Backward(const Matrix& input);
	const Matrix& GetLastBackwardInput() const noexcept;
	const Matrix& GetLastBackwardOutput() const noexcept;

	const VariableTable& GetVariableTable() const noexcept;
	VariableTable& GetVariableTable() noexcept;
	const ParameterTable& GetParameterTable() const noexcept;
	ParameterTable& GetParameterTable() noexcept;

	virtual void ResetAllParameters() = 0;

protected:
	virtual Matrix ForwardImpl(const Matrix& input) = 0;
	virtual Matrix BackwardImpl(const Matrix& input) = 0;
};

class FCLayer final : public Layer {
private:
	Parameter m_Weights, m_Biases;

public:
	FCLayer(std::size_t inputSize, std::size_t outputSize);
	FCLayer(const FCLayer&) = delete;
	virtual ~FCLayer() override = default;

public:
	FCLayer& operator=(const FCLayer&) = delete;

public:
	virtual std::size_t GetForwardInputSize() const noexcept override;
	virtual std::size_t GetForwardOutputSize() const noexcept override;

	virtual void ResetAllParameters() override;

protected:
	virtual Matrix ForwardImpl(const Matrix& input) override;
	virtual Matrix BackwardImpl(const Matrix& input) override;
};

enum class AFunction {
	Sigmoid,
	Tanh,
	ReLU,
	LeakyReLU,
};

class ALayer final : public Layer {
private:
	AFunction m_AFunction;
	float (*m_Primitive)(float) = nullptr;
	float (*m_Derivative)(float) = nullptr;

public:
	ALayer(AFunction aFunction);
	ALayer(const ALayer&) = delete;
	virtual ~ALayer() override = default;

public:
	ALayer& operator=(const ALayer&) = delete;

public:
	virtual std::size_t GetForwardInputSize() const noexcept override;
	virtual std::size_t GetForwardOutputSize() const noexcept override;

	virtual void ResetAllParameters() override;

protected:
	virtual Matrix ForwardImpl(const Matrix& input) override;
	virtual Matrix BackwardImpl(const Matrix& input) override;

public:
	AFunction GetAFunction() const noexcept;
};

float Sigmoid(float x);
float SigmoidDerivative(float x);
float Tanh(float x);
float TanhDerivative(float x);
float ReLU(float x);
float ReLUDerivative(float x);
float LeakyReLU(float x);
float LeakyReLUDerivative(float x);