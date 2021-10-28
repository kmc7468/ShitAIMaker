#include "Matrix.hpp"

#include <algorithm>
#include <cassert>
#include <utility>

Matrix::Matrix(std::size_t rowSize, std::size_t columnSize, float data)
	: m_Elements(rowSize * columnSize, data), m_RowSize(rowSize), m_ColumnSize(columnSize) {
	assert(rowSize > 0);
	assert(columnSize > 0);
}
Matrix::Matrix(std::size_t rowSize, std::size_t columnSize, std::vector<float> elements) noexcept
	: m_Elements(std::move(elements)), m_RowSize(rowSize), m_ColumnSize(columnSize) {}

bool Matrix::operator==(const Matrix& other) const noexcept {
	return GetSize() == other.GetSize() &&
		std::equal(m_Elements.begin(), m_Elements.end(), other.m_Elements.begin(), other.m_Elements.end());
}
Matrix Matrix::operator+(const Matrix& other) const {
	return Matrix(*this) += other;
}
Matrix Matrix::operator-(const Matrix& other) const {
	return Matrix(*this) -= other;
}
Matrix Matrix::operator*(float scalar) const {
	return Matrix(*this) *= scalar;
}
Matrix Matrix::operator*(const Matrix& other) const {
	assert(m_ColumnSize == other.m_RowSize);

	Matrix result(m_RowSize, other.m_ColumnSize);

	for (std::size_t i = 0; i < m_RowSize; ++i) {
		for (std::size_t j = 0; j < other.m_ColumnSize; ++j) {
			for (std::size_t k = 0; k < m_ColumnSize; ++k) {
				result(i, j) += (*this)(i, k) * other(k, j);
			}
		}
	}

	return result;
}
Matrix& Matrix::operator+=(const Matrix& other) noexcept {
	assert(GetSize() == other.GetSize());

	for (std::size_t i = 0; i < m_Elements.size(); ++i) {
		m_Elements[i] += other.m_Elements[i];
	}

	return *this;
}
Matrix& Matrix::operator-=(const Matrix& other) noexcept {
	assert(GetSize() == other.GetSize());

	for (std::size_t i = 0; i < m_Elements.size(); ++i) {
		m_Elements[i] -= other.m_Elements[i];
	}

	return *this;
}
Matrix& Matrix::operator*=(float scalar) noexcept {
	for (std::size_t i = 0; i < m_Elements.size(); ++i) {
		m_Elements[i] *= scalar;
	}

	return *this;
}
Matrix& Matrix::operator*=(const Matrix& other) {
	return *this = *this * other;
}
float Matrix::operator[](const std::pair<std::size_t, std::size_t>& index) const noexcept {
	return (*this)(index.first, index.second);
}
float& Matrix::operator[](const std::pair<std::size_t, std::size_t>& index) noexcept {
	return (*this)(index.first, index.second);
}
float Matrix::operator()(std::size_t rowIndex, std::size_t columnIndex) const noexcept {
	assert(rowIndex < m_RowSize);
	assert(columnIndex < m_ColumnSize);

	return m_Elements[rowIndex * m_ColumnSize + columnIndex];
}
float& Matrix::operator()(std::size_t rowIndex, std::size_t columnIndex) noexcept {
	assert(rowIndex < m_RowSize);
	assert(columnIndex < m_ColumnSize);

	return m_Elements[rowIndex * m_ColumnSize + columnIndex];
}

std::pair<std::size_t, std::size_t> Matrix::GetSize() const noexcept {
	return { m_RowSize, m_ColumnSize };
}
std::size_t Matrix::GetRowSize() const noexcept {
	return m_RowSize;
}
std::size_t Matrix::GetColumnSize() const noexcept {
	return m_ColumnSize;
}
bool Matrix::IsZeroMatrix() const noexcept {
	return m_RowSize == 0;
}

Matrix& Matrix::HadamardProduct(const Matrix& other) noexcept {
	assert(GetSize() == other.GetSize());

	for (std::size_t i = 0; i < m_Elements.size(); ++i) {
		m_Elements[i] *= other.m_Elements[i];
	}

	return *this;
}
Matrix& Matrix::Transpose() {
	return *this = ::Transpose(*this);
}

Matrix operator*(float scalar, const Matrix& matrix) {
	return matrix * scalar;
}
std::ostream& operator<<(std::ostream& stream, const Matrix& matrix) {
	if (matrix.IsZeroMatrix()) {
		return stream << "[]";
	}

	const auto [row, column] = matrix.GetSize();

	stream << "[ ";
	for (std::size_t i = 0; i < row; ++i) {
		for (std::size_t j = 0; j < column; ++j) {
			stream << matrix(i, j) << ' ';
		}

		if (i == row - 1) {
			stream << ']';
		} else {
			stream << "\n  ";
		}
	}

	return stream;
}

Matrix HadamardProduct(const Matrix& lhsMatrix, const Matrix& rhsMatrix) {
	return Matrix(lhsMatrix).HadamardProduct(rhsMatrix);
}
Matrix Transpose(const Matrix& matrix) {
	const auto [row, column] = matrix.GetSize();
	Matrix result(column, row);

	for (std::size_t i = 0; i < row; ++i) {
		for (std::size_t j = 0; j < column; ++j) {
			result(j, i) = matrix(i, j);
		}
	}

	return result;
}