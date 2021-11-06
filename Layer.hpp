#pragma once

#include "Matrix.hpp"

#include <map>
#include <string>
#include <string_view>

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
	const Matrix& GetValue() const noexcept;
	Matrix& GetValue() noexcept;
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
	Variable AddVariable(std::string name, Matrix initialValue = {});
};