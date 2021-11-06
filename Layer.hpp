#pragma once

#include "Matrix.hpp"

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

class ReadonlyParameter;

class Parameter final {
	friend class ReadonlyParameter;

private:
	std::map<std::string, std::tuple<Matrix, Matrix, std::unique_ptr<VariableTable>>>::iterator m_Iterator;

public:
	Parameter(
		std::map<std::string, std::tuple<Matrix, Matrix, std::unique_ptr<VariableTable>>>::iterator iterator) noexcept;
	Parameter(const Parameter& other) noexcept = default;
	~Parameter() = default;

public:
	Parameter& operator=(const Parameter& other) noexcept = default;
	bool operator==(const Parameter& other) noexcept;

public:
	std::string_view GetName() const noexcept;
	const Matrix& GetValue() const noexcept;
	Matrix& GetValue() noexcept;
	const Matrix& GetGradient() const noexcept;
	Matrix& GetGradient() noexcept;
	const VariableTable& GetVariableTable() const noexcept;
	VariableTable& GetVariableTable() noexcept;
};

class ReadonlyParameter final {
private:
	std::map<std::string, std::tuple<Matrix, Matrix, std::unique_ptr<VariableTable>>>::const_iterator m_Iterator;

public:
	ReadonlyParameter(
		std::map<std::string, std::tuple<Matrix, Matrix, std::unique_ptr<VariableTable>>>::const_iterator iterator) noexcept;
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