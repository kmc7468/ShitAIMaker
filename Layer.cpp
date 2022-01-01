#include "Layer.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

Variable::Variable(std::map<std::string, Matrix>::iterator iterator) noexcept
	: m_Iterator(iterator) {}

bool Variable::operator==(const Variable& other) noexcept {
	return m_Iterator == other.m_Iterator;
}

std::string_view Variable::GetName() const noexcept {
	return m_Iterator->first;
}
Matrix& Variable::GetValue() const noexcept {
	return m_Iterator->second;
}
Matrix& Variable::SetValue(Matrix newValue) const noexcept {
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
std::vector<ReadonlyVariable> VariableTable::GetAllVariables() const {
	std::vector<ReadonlyVariable> result;

	for (auto iter = m_Variables.begin(); iter != m_Variables.end(); ++iter) {
		result.emplace_back(iter);
	}

	return result;
}
std::vector<Variable> VariableTable::GetAllVariables() {
	std::vector<Variable> result;

	for (auto iter = m_Variables.begin(); iter != m_Variables.end(); ++iter) {
		result.emplace_back(iter);
	}

	return result;
}
Variable VariableTable::AddVariable(std::string name, Matrix initialValue) {
	return m_Variables.insert_or_assign(std::move(name), std::move(initialValue)).first;
}

Parameter::Parameter(std::map<std::string,
	std::tuple<Matrix, Matrix, std::unique_ptr<VariableTable>>>::iterator iterator) noexcept
	: m_Iterator(iterator) {}

bool Parameter::operator==(const Parameter& other) noexcept {
	return m_Iterator == other.m_Iterator;
}

std::string_view Parameter::GetName() const noexcept {
	return m_Iterator->first;
}
Matrix& Parameter::GetValue() const noexcept {
	return std::get<0>(m_Iterator->second);
}
Matrix& Parameter::SetValue(Matrix newValue) const noexcept {
	return std::get<0>(m_Iterator->second) = std::move(newValue);
}
Matrix& Parameter::GetGradient() const noexcept {
	return std::get<1>(m_Iterator->second);
}
Matrix& Parameter::SetGradient(Matrix newGradient) const noexcept {
	return std::get<1>(m_Iterator->second) = std::move(newGradient);
}
VariableTable& Parameter::GetVariableTable() const noexcept {
	return *std::get<2>(m_Iterator->second);
}

ReadonlyParameter::ReadonlyParameter(std::map<std::string,
	std::tuple<Matrix, Matrix, std::unique_ptr<VariableTable>>>::const_iterator iterator) noexcept
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
	return m_Parameters.insert_or_assign(std::move(name),
		std::make_tuple(initialValue, Matrix{}, std::make_unique<VariableTable>())).first;
}

Layer::Layer(std::string name)
	: m_Name(std::move(name)),
	m_LastForwardInput(m_VariableTable.AddVariable("LastForwardInput")),
	m_LastForwardOutput(m_VariableTable.AddVariable("LastForwardOutput")),
	m_LastBackwardInput(m_VariableTable.AddVariable("LastBackwardInput")),
	m_LastBackwardOutput(m_VariableTable.AddVariable("LastBackwardOutput")) {}

std::string_view Layer::GetName() const noexcept {
	return m_Name;
}

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

std::size_t FCLayer::GetForwardInputSize() const noexcept {
	return m_Weights.GetValue().GetColumnSize();
}
std::size_t FCLayer::GetForwardOutputSize() const noexcept {
	return m_Weights.GetValue().GetRowSize();
}

LayerDump FCLayer::GetDump(const LayerDump& prevLayerDump) const {
	const std::size_t inputSize = GetForwardInputSize();
	const std::size_t outputSize = GetForwardOutputSize();
	std::vector<std::vector<float>> units;

	for (std::size_t i = 0; i < outputSize; ++i) {
		std::vector<float>& weights = units.emplace_back();

		for (std::size_t j = 0; j < inputSize; ++j) {
			weights.push_back(m_Weights.GetValue()(i, j));
		}
	}

	std::vector<std::size_t> drawnUnits(outputSize);

	for (std::size_t i = 0; i < outputSize; ++i) {
		drawnUnits[i] = i;
	}

	return LayerDump("��������", prevLayerDump, units, drawnUnits);
}
void FCLayer::ResetAllParameters() {
	const std::size_t inputSize = GetForwardInputSize();
	const std::size_t outputSize = GetForwardOutputSize();

	m_Weights.SetValue(RandomMatrix(outputSize, inputSize));
	m_Biases.SetValue(RandomMatrix(outputSize, 1));
}

Matrix FCLayer::ForwardImpl(const Matrix& input) {
	return m_Weights.GetValue() * input + m_Biases.GetValue() * Matrix(1, input.GetColumnSize(), 1);
}
Matrix FCLayer::BackwardImpl(const Matrix& input) {
	m_Weights.SetGradient(input * Transpose(GetLastForwardInput()));
	m_Biases.SetGradient(input * Matrix(1, GetLastForwardInput().GetColumnSize(), 1));

	return Transpose(m_Weights.GetValue()) * input;
}

ALayer::ALayer(AFunction aFunction)
	: Layer("ALayer"), m_AFunction(aFunction) {
	switch (aFunction) {
	case AFunction::Sigmoid:
		m_Primitive = Sigmoid;
		m_Derivative = SigmoidDerivative;
		break;

	case AFunction::Tanh:
		m_Primitive = Tanh;
		m_Derivative = TanhDerivative;
		break;

	case AFunction::ReLU:
		m_Primitive = ReLU;
		m_Derivative = ReLUDerivative;
		break;

	case AFunction::LeakyReLU:
		m_Primitive = LeakyReLU;
		m_Derivative = LeakyReLUDerivative;
		break;

	default:
		assert(false);
		break;
	}
}

std::size_t ALayer::GetForwardInputSize() const noexcept {
	return 0;
}
std::size_t ALayer::GetForwardOutputSize() const noexcept {
	return 0;
}

LayerDump ALayer::GetDump(const LayerDump& prevLayerDump) const {
	std::string_view name;

	switch (m_AFunction) {
	case AFunction::Sigmoid:
		name = "Sigmoid Ȱ��ȭ��";
		break;

	case AFunction::Tanh:
		name = "Tanh Ȱ��ȭ��";
		break;

	case AFunction::ReLU:
		name = "ReLU Ȱ��ȭ��";
		break;

	case AFunction::LeakyReLU:
		name = "LeakyReLU Ȱ��ȭ��";
		break;
	}

	const std::size_t inputSize = prevLayerDump.GetDrawnUnits().size();
	std::vector<std::vector<float>> units;

	for (std::size_t i = 0; i < inputSize; ++i) {
		units.push_back(std::vector<float>(inputSize, 0.f));
		units.back()[i] = 1.f;
	}

	std::vector<std::size_t> drawnUnits(inputSize);

	for (std::size_t i = 0; i < inputSize; ++i) {
		drawnUnits[i] = i;
	}

	return LayerDump(name, prevLayerDump, units, drawnUnits);
}
void ALayer::ResetAllParameters() {}

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

AFunction ALayer::GetAFunction() const noexcept {
	return m_AFunction;
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

SMLayer::SMLayer()
	: Layer("SMLayer") {}

std::size_t SMLayer::GetForwardInputSize() const noexcept {
	return 0;
}
std::size_t SMLayer::GetForwardOutputSize() const noexcept {
	return 0;
}

LayerDump SMLayer::GetDump(const LayerDump& prevLayerDump) const {
	const std::size_t inputSize = prevLayerDump.GetDrawnUnits().size();
	std::vector<std::vector<float>> units;

	for (std::size_t i = 0; i < inputSize; ++i) {
		units.push_back(std::vector<float>(inputSize, 1.f));
	}

	std::vector<std::size_t> drawnUnits(inputSize);

	for (std::size_t i = 0; i < inputSize; ++i) {
		drawnUnits[i] = i;
	}

	return LayerDump("Softmax Ȱ��ȭ��", prevLayerDump, units, drawnUnits);
}
void SMLayer::ResetAllParameters() {}

Matrix SMLayer::ForwardImpl(const Matrix& input) {
	const auto [row, column] = input.GetSize();
	Matrix result = input;

	for (std::size_t i = 0; i < column; ++i) {
		float sum = 0;

		for (std::size_t j = 0; j < row; ++j) {
			result(j, i) = std::expf(result(j, i));

			sum += result(j, i);
		}

		for (std::size_t j = 0; j < row; ++j) {
			result(j, i) /= sum;
		}
	}

	return result;
}
Matrix SMLayer::BackwardImpl(const Matrix& input) {
	const Matrix& lastOutput = GetLastForwardOutput();
	const auto [row, column] = lastOutput.GetSize();

	Matrix result(row, column);

	for (std::size_t i = 0; i < column; ++i) {
		Matrix gradient(row, row);

		for (std::size_t j = 0; j < row; ++j) {
			for (std::size_t k = 0; k < row; ++k) {
				if (j == k) {
					gradient(j, k) = lastOutput(j, i) * (1 - lastOutput(j, i));
				} else {
					gradient(j, k) = -lastOutput(j, i) * lastOutput(k, i);
				}
			}
		}

		Matrix oneInput(row, 1);

		for (std::size_t j = 0; j < row; ++j) {
			oneInput(j, 0) = input(j, i);
		}

		const Matrix oneResult = gradient * oneInput;

		for (std::size_t j = 0; j < row; ++j) {
			result(j, i) = oneResult(j, 0);
		}
	}

	return result;
}

LayerDump::LayerDump(std::size_t inputSize)
	: m_Name("�Է���") {
	for (std::size_t i = 0; i < inputSize; ++i) {
		m_DrawnUnits.push_back(std::make_pair(i, std::vector<float>{}));
	}
}
LayerDump::LayerDump(const std::string_view& name, const LayerDump& prevLayerDump,
	const std::vector<std::vector<float>>& units, const std::vector<std::size_t>& drawnUnits)
	: m_Name(name) {
	const auto& prevDrawnUnits = prevLayerDump.GetDrawnUnits();

	float maxWeight = 0;

	for (const auto& unitIndex : drawnUnits) {
		std::pair<std::size_t, std::vector<float>> unit;

		unit.first = unitIndex;

		for (const auto& [prevUnitIndex, prevUnitWeights] : prevDrawnUnits) {
			const float weight = std::abs(units[unitIndex][prevUnitIndex]);

			unit.second.push_back(weight);

			maxWeight = std::max(weight, maxWeight);
		}

		m_DrawnUnits.push_back(std::move(unit));
	}

	if (maxWeight == 0) return;

	for (auto& [unitIndex, weights] : m_DrawnUnits) {
		for (auto& weight : weights) {
			weight /= maxWeight;
		}
	}
}

std::string_view LayerDump::GetName() const noexcept {
	return m_Name;
}
const std::vector<std::pair<std::size_t, std::vector<float>>>& LayerDump::GetDrawnUnits() const noexcept {
	return m_DrawnUnits;
}