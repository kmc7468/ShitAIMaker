#pragma once

#include "Matrix.hpp"

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <variant>

class ResourceDirectory;
class ResourceFile;

class ResourceObject {
private:
	std::string m_Name;
	ResourceDirectory* m_Parent = nullptr;
	std::chrono::system_clock::time_point m_CreationTime;
	std::chrono::system_clock::time_point m_LastEditTime;

public:
	ResourceObject(const ResourceObject&) = delete;
	virtual ~ResourceObject() = 0;

protected:
	ResourceObject(std::string name, ResourceDirectory* parent,
		std::chrono::system_clock::time_point creationTime);

public:
	ResourceObject& operator=(const ResourceObject&) = delete;

public:
	std::string_view GetName() const noexcept;
	bool IsRootObject() const noexcept;
	const ResourceDirectory& GetParent() const noexcept;
	ResourceDirectory& GetParent() noexcept;
	std::chrono::system_clock::time_point GetCreationTime() const;
	std::chrono::system_clock::time_point GetLastEditTime() const;
	void SetLastEditTime(std::chrono::system_clock::time_point newLastEditTime);

	const ResourceDirectory* IsDirectory() const noexcept;
	ResourceDirectory* IsDirectory() noexcept;
	const ResourceFile* IsFile() const noexcept;
	ResourceFile* IsFile() noexcept;
};

class ResourceDirectory final : public ResourceObject {
private:
	std::map<std::string, std::unique_ptr<ResourceObject>> m_Objects;

public:
	ResourceDirectory();
	ResourceDirectory(const ResourceDirectory&) = delete;
	virtual ~ResourceDirectory() override = default;

private:
	ResourceDirectory(std::string name, ResourceDirectory* parent,
		std::chrono::system_clock::time_point creationTime);

public:
	ResourceDirectory& operator=(const ResourceDirectory&) = delete;

public:
	ResourceDirectory& CreateDirectory(std::string name,
		std::chrono::system_clock::time_point creationTime = std::chrono::system_clock::now());
	ResourceFile& CreateFile(std::string name,
		std::chrono::system_clock::time_point creationTime = std::chrono::system_clock::now());
};

class ResourceFile final : public ResourceObject {
	friend class ResourceDirectory;

private:
	std::variant<std::monostate, Matrix> m_Content;

public:
	ResourceFile(const ResourceFile&) = delete;
	virtual ~ResourceFile() override = default;

private:
	ResourceFile(std::string name, ResourceDirectory* parent,
		std::chrono::system_clock::time_point creationTime);

public:
	ResourceFile& operator=(Matrix matrix);
	ResourceFile& operator=(const ResourceFile&) = delete;

public:
	bool IsEmpty() const noexcept;
	const Matrix* IsMatrix() const noexcept;
};