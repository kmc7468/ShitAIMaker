#include "Layer.hpp"

#include <cassert>
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
const Matrix& Parameter::GetGradient() const noexcept {
	return std::get<1>(m_Iterator->second);
}
Matrix& Parameter::GetGradient() noexcept {
	return std::get<1>(m_Iterator->second);
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
Parameter ParameterTable::AddParameter(std::string name) {
	const auto [iterator, isSuccess] =
		m_Parameters.insert(std::make_pair(std::move(name),
			std::make_tuple(Matrix{}, Matrix{}, std::make_unique<VariableTable>())));
	assert(isSuccess);

	return iterator;
}