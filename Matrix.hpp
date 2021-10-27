#pragma once

#include <cstddef>
#include <ostream>
#include <utility>
#include <vector>

class Matrix final {
private:
	std::vector<float> m_Elements;
	std::size_t m_RowSize = 0, m_ColumnSize = 0;

public:
	Matrix() noexcept = default;
	Matrix(std::size_t rowSize, std::size_t columnSize, float data = 0.f);
	Matrix(std::size_t rowSize, std::size_t columnSize, std::vector<float> elements) noexcept;
	Matrix(const Matrix& other) = default;
	Matrix(Matrix&& other) noexcept = default;
	~Matrix() = default;

public:
	Matrix& operator=(const Matrix& other) = default;
	Matrix& operator=(Matrix&& other) noexcept = default;
	bool operator==(const Matrix& other) const noexcept;
	Matrix operator+(const Matrix& other) const;
	Matrix operator-(const Matrix& other) const;
	Matrix operator*(float scalar) const;
	Matrix operator*(const Matrix& other) const;
	Matrix& operator+=(const Matrix& other) noexcept;
	Matrix& operator-=(const Matrix& other) noexcept;
	Matrix& operator*=(float scalar) noexcept;
	Matrix& operator*=(const Matrix& other);
	float operator[](const std::pair<std::size_t, std::size_t>& index) const noexcept;
	float& operator[](const std::pair<std::size_t, std::size_t>& index) noexcept;
	float operator()(std::size_t rowIndex, std::size_t columnIndex) const noexcept;
	float& operator()(std::size_t rowIndex, std::size_t columnIndex) noexcept;

public:
	std::pair<std::size_t, std::size_t> GetSize() const noexcept;
	std::size_t GetRowSize() const noexcept;
	std::size_t GetColumnSize() const noexcept;
	bool IsZeroMatrix() const noexcept;

	Matrix& HadamardProduct(const Matrix& other) noexcept;
	Matrix& Transpose();
};

Matrix operator*(float scalar, const Matrix& matrix);
std::ostream& operator<<(std::ostream& stream, const Matrix& matrix);

Matrix HadamardProduct(const Matrix& lhsMatrix, const Matrix& rhsMatrix);
Matrix Transpose(const Matrix& matrix);