#include "Layer.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <utility>

Variable::Variable(std::map<std::string, Matrix>::iterator iterator) noexcept
	: m_Iterator(iterator) {}

bool Variable::operator==(const Variable& other) noexcept {
	return m_Iterator == other.m_Iterator;
}

std::string_view Variable::GetName() const noexcept {
	return m_Iterator->first;
}
const Matrix& Variable::GetValue() const noexcept {
	return m_Iterator->second;
}
Matrix& Variable::GetValue() noexcept {
	return m_Iterator->second;
}
Matrix& Variable::SetValue(Matrix newValue) noexcept {
	return m_Iterator->second = std::move(newValue);
}

ReadonlyVariable::ReadonlyVariable(std::map<std::string, Matrix>::const_iterator iterator) noexcept
	: m_Iterator(iterator) {}
ReadonlyVariable::ReadonlyVariable(const Variable& variable) noexcept
	: m_Iterator(variable.m_Iterator) {}

ReadonlyVariable& ReadonlyVariable::operator=(const Variable& variable) noexcept {
	m_Iterator = variable.m_Iterator;

	return *this;
}
bool ReadonlyVariable::operator==(const Variable& variable) noexcept {
	return m_Iterator == variable.m_Iterator;
}
bool ReadonlyVariable::operator==(const ReadonlyVariable& other) noexcept {
	return m_Iterator == other.m_Iterator;
}

std::string_view ReadonlyVariable::GetName() const noexcept {
	return m_Iterator->first;
}
const Matrix& ReadonlyVariable::GetValue() const noexcept {
	return m_Iterator->second;
}

ReadonlyVariable VariableTable::GetVariable(const std::string& name) const noexcept {
	const auto iterator = m_Variables.find(name);
	assert(iterator != m_Variables.end());

	return iterator;
}
Variable VariableTable::GetVariable(const std::string& name) noexcept {
	const auto iterator = m_Variables.find(name);
	assert(iterator != m_Variables.end());

	return iterator;
}
Variable VariableTable::AddVariable(std::string name, Matrix initialValue) {
	const auto [iterator, isSuccess] =
		m_Variables.insert(std::make_pair(std::move(name), std::move(initialValue)));
	assert(isSuccess);

	return iterator;
}

Parameter::Parameter(
	std::map<std::string, std::tuple<Matrix, Matrix, std::unique_ptr<VariableTable>>>::iterator iterator) noexcept
	: m_Iterator(iterator) {}

bool Parameter::operator==(const Parameter& other) noexcept {
	return m_Iterator == other.m_Iterator;
}

std::string_view Parameter::GetName() const noexcept {
	return m_Iterator->first;
}
const Matrix& Parameter::GetValue() const noexcept {
	return std::get<0>(m_Iterator->second);
}
Matrix& Parameter::GetValue() noexcept {
	return std::get<0>(m_Iterator->second);
}
Matrix& Parameter::SetValue(Matrix newValue) noexcept {
	return std::get<0>(m_Iterator->second) = std::move(newValue);
}
const Matrix& Parameter::GetGradient() const noexcept {
	return std::get<1>(m_Iterator->second);
}
Matrix& Parameter::GetGradient() noexcept {
	return std::get<1>(m_Iterator->second);
}
Matrix& Parameter::SetGradient(Matrix newGradient) noexcept {
	return std::get<1>(m_Iterator->second) = std::move(newGradient);
}
const VariableTable& Parameter::GetVariableTable() const noexcept {
	return *std::get<2>(m_Iterator->second);
}
VariableTable& Parameter::GetVariableTable() noexcept {
	return *std::get<2>(m_Iterator->second);
}

ReadonlyParameter::ReadonlyParameter(
	std::map<std::string, std::tuple<Matrix, Matrix, std::unique_ptr<VariableTable>>>::const_iterator iterator) noexcept
	: m_Iterator(iterator) {}
ReadonlyParameter::ReadonlyParameter(const Parameter& parameter) noexcept
	: m_Iterator(parameter.m_Iterator) {}

ReadonlyParameter& ReadonlyParameter::operator=(const Parameter& parameter) noexcept {
	m_Iterator = parameter.m_Iterator;

	return *this;
}
bool ReadonlyParameter::operator==(const Parameter& parameter) noexcept {
	return m_Iterator == parameter.m_Iterator;
}
bool ReadonlyParameter::operator==(const ReadonlyParameter& other) noexcept {
	return m_Iterator == other.m_Iterator;
}

std::string_view ReadonlyParameter::GetName() const noexcept {
	return m_Iterator->first;
}
const Matrix& ReadonlyParameter::GetValue() const noexcept {
	return std::get<0>(m_Iterator->second);
}
const Matrix& ReadonlyParameter::GetGradient() const noexcept {
	return std::get<1>(m_Iterator->second);
}
const VariableTable& ReadonlyParameter::GetVariableTable() const noexcept {
	return *std::get<2>(m_Iterator->second);
}

ReadonlyParameter ParameterTable::GetParameter(const std::string& name) const noexcept {
	const auto iterator = m_Parameters.find(name);
	assert(iterator != m_Parameters.end());

	return iterator;
}
Parameter ParameterTable::GetParameter(const std::string& name) noexcept {
	const auto iterator = m_Parameters.find(name);
	assert(iterator != m_Parameters.end());

	return iterator;
}
std::vector<ReadonlyParameter> ParameterTable::GetAllParameters() const {
	std::vector<ReadonlyParameter> result;

	for (auto iter = m_Parameters.begin(); iter != m_Parameters.end(); ++iter) {
		result.emplace_back(iter);
	}

	return result;
}
std::vector<Parameter> ParameterTable::GetAllParameters() {
	std::vector<Parameter> result;

	for (auto iter = m_Parameters.begin(); iter != m_Parameters.end(); ++iter) {
		result.emplace_back(iter);
	}

	return result;
}
Parameter ParameterTable::AddParameter(std::string name, Matrix initialValue) {
	const auto [iterator, isSuccess] =
		m_Parameters.insert(std::make_pair(std::move(name),
			std::make_tuple(initialValue, Matrix{}, std::make_unique<VariableTable>())));
	assert(isSuccess);

	return iterator;
}

Layer::Layer(std::string name)
	: m_Name(std::move(name)),
	m_LastForwardInput(m_VariableTable.AddVariable("LastForwardInput")),
	m_LastForwardOutput(m_VariableTable.AddVariable("LastForwardOutput")),
	m_LastBackwardInput(m_VariableTable.AddVariable("LastBackwardInput")),
	m_LastBackwardOutput(m_VariableTable.AddVariable("LastBackwardOutput")) {}

Matrix Layer::Forward(const Matrix& input) {
	return m_LastForwardOutput.SetValue(ForwardImpl(m_LastForwardInput.SetValue(input)));
}
const Matrix& Layer::GetLastForwardInput() const noexcept {
	return m_LastForwardInput.GetValue();
}
const Matrix& Layer::GetLastForwardOutput() const noexcept {
	return m_LastForwardOutput.GetValue();
}

Matrix Layer::Backward(const Matrix& input) {
	return m_LastBackwardOutput.SetValue(BackwardImpl(m_LastBackwardInput.SetValue(input)));
}
const Matrix& Layer::GetLastBackwardInput() const noexcept {
	return m_LastBackwardInput.GetValue();
}
const Matrix& Layer::GetLastBackwardOutput() const noexcept {
	return m_LastBackwardOutput.GetValue();
}

const VariableTable& Layer::GetVariableTable() const noexcept {
	return m_VariableTable;
}
VariableTable& Layer::GetVariableTable() noexcept {
	return m_VariableTable;
}
const ParameterTable& Layer::GetParameterTable() const noexcept {
	return m_ParameterTable;
}
ParameterTable& Layer::GetParameterTable() noexcept {
	return m_ParameterTable;
}

FCLayer::FCLayer(std::size_t inputSize, std::size_t outputSize)
	: Layer("FCLayer"),
	m_Weights(GetParameterTable().AddParameter("Weights", RandomMatrix(outputSize, inputSize))),
	m_Biases(GetParameterTable().AddParameter("Biases", RandomMatrix(outputSize, 1))) {}

Matrix FCLayer::ForwardImpl(const Matrix& input) {
	return m_Weights.GetValue() * input + m_Biases.GetValue();
}
Matrix FCLayer::BackwardImpl(const Matrix& input) {
	m_Weights.SetGradient(input * Transpose(GetLastForwardInput()));
	m_Biases.SetGradient(input * Matrix(1, GetLastForwardInput().GetColumnSize(), 1));

	return Transpose(m_Weights.GetValue()) * input;
}

ALayer::ALayer(AFunction primitive, AFunction derivative)
	: Layer("ALayer"), m_Primitive(primitive), m_Derivative(derivative) {
	assert(primitive != nullptr);
	assert(derivative != nullptr);
}

Matrix ALayer::ForwardImpl(const Matrix& input) {
	const auto [row, column] = input.GetSize();
	Matrix result = input;

	for (std::size_t i = 0; i < row; ++i) {
		for (std::size_t j = 0; j < column; ++j) {
			result(i, j) = m_Primitive(result(i, j));
		}
	}

	return result;
}
Matrix ALayer::BackwardImpl(const Matrix& input) {
	Matrix result = GetLastForwardInput();
	const auto [row, column] = result.GetSize();

	for (std::size_t i = 0; i < row; ++i) {
		for (std::size_t j = 0; j < column; ++j) {
			result(i, j) = m_Derivative(result(i, j));
		}
	}

	return HadamardProduct(result, input);
}

float Sigmoid(float x) {
	return 1 / (1 + std::expf(-x));
}
float SigmoidDerivative(float x) {
	const float y = Sigmoid(x);
	return y * (1 - y);
}
float Tanh(float x) {
	return std::tanhf(x);
}
float TanhDerivative(float x) {
	return 1 - std::powf(std::tanhf(x), 2);
}
float ReLU(float x) {
	return std::max(x, 0.f);
}
float ReLUDerivative(float x) {
	return x >= 0 ? 1.f : 0.f;
}
float LeakyReLU(float x) {
	return std::max(0.01f * x, x);
}
float LeakyReLUDerivative(float x) {
	return x >= 0.f ? 1.f : 0.01f;
}