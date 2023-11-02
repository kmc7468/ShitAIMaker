#pragma once

#include <memory>
#include <type_traits>
#include <utility>

template<typename T>
class UniqueRef {
	template<typename U>
	friend class UniqueRef;

private:
	std::unique_ptr<T> m_Object;

public:
	UniqueRef(std::unique_ptr<T>&& object) noexcept
		: m_Object(std::move(object)) {}
	template<typename U> requires(std::is_base_of_v<T, U>)
	UniqueRef(UniqueRef<U>&& other) noexcept
		: m_Object(std::move(other.m_Object)) {}
	~UniqueRef() = default;

public:
	template<typename U> requires(std::is_base_of_v<T, U>)
	UniqueRef& operator=(UniqueRef<U>&& other) noexcept {
		m_Object = std::move(other.m_Object);

		return *this;
	}
	bool operator==(std::nullptr_t) const noexcept {
		return m_Object == nullptr;
	}
	T* operator->() const noexcept {
		return m_Object.get();
	}
	T& operator*() const noexcept {
		return *m_Object.get();
	}
	explicit operator bool() const noexcept {
		return m_Object != nullptr;
	}

public:
	bool IsEmpty() const noexcept {
		return m_Object == nullptr;
	}
	T& Get() const noexcept {
		return *m_Object.get();
	}
};

template<typename T>
class SharedRef {
	template<typename U>
	friend class SharedRef;

private:
	std::shared_ptr<T> m_Object;

public:
	SharedRef(std::shared_ptr<T> object) noexcept
		: m_Object(std::move(object)) {}
	template<typename U> requires(std::is_base_of_v<T, U>)
	SharedRef(const SharedRef<U>& other) noexcept
		: m_Object(other.m_Object) {}
	template<typename U> requires(std::is_base_of_v<T, U>)
	SharedRef(SharedRef<U>&& other) noexcept
		: m_Object(std::move(other.m_Object)) {}
	~SharedRef() = default;

public:
	template<typename U> requires(std::is_base_of_v<T, U>)
	SharedRef& operator=(const SharedRef<U>& other) noexcept {
		m_Object = other.m_Object;

		return *this;
	}
	template<typename U> requires(std::is_base_of_v<T, U>)
	SharedRef& operator=(SharedRef<U>&& other) noexcept {
		m_Object = std::move(other.m_Object);

		return *this;
	}
	bool operator==(std::nullptr_t) const noexcept {
		return m_Object == nullptr;
	}
	T* operator->() const noexcept {
		return m_Object.get();
	}
	T& operator*() const noexcept {
		return *m_Object.get();
	}
	explicit operator bool() const noexcept {
		return m_Object != nullptr;
	}

public:
	bool IsEmpty() const noexcept {
		return m_Object == nullptr;
	}
	T& Get() const noexcept {
		return *m_Object.get();
	}
};