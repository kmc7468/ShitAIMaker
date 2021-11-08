#pragma once

#include "Matrix.hpp"
#include "Network.hpp"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

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
	std::vector<std::pair<std::string, const ResourceObject*>> GetAllObjects() const;
	std::vector<std::pair<std::string, ResourceObject*>> GetAllObjects();
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

class Project final {
private:
	static inline const std::uint8_t m_MagicNumber[] = { 'S', 'H', 'I', 'T', 'A', 'M' };
	static inline const std::uint32_t m_Version = 0x00000000;

private:
	std::string m_Name;
	std::filesystem::path m_Path;

	Network m_Network;
	ResourceDirectory m_Resources;

public:
	Project() = default;
	Project(const Project&) = delete;
	~Project() = default;

public:
	Project& operator=(const Project&) = delete;

public:
	std::string_view GetName() const noexcept;
	void SetName(std::string newName) noexcept;
	const std::filesystem::path& GetPath() const noexcept;
	void SetPath(std::filesystem::path newPath) noexcept;

	const Network& GetNetwork() const noexcept;
	Network& GetNetwork() noexcept;
	const ResourceDirectory& GetResources() const noexcept;
	ResourceDirectory& GetResources() noexcept;

	void Load(std::filesystem::path path);
	void Save() const;
};