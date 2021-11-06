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
	const auto [iterator, isSuccess] = m_Variables.insert(std::make_pair(std::move(name), std::move(initialValue)));
	assert(isSuccess);

	return iterator;
}