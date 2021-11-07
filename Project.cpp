#include "Project.hpp"

#include <cassert>
#include <cctype>
#include <utility>

ResourceObject::~ResourceObject() {}

ResourceObject::ResourceObject(std::string name, ResourceDirectory* parent,
	std::chrono::system_clock::time_point creationTime)
	: m_Name(std::move(name)), m_Parent(parent), m_CreationTime(std::move(creationTime)) {}

std::string_view ResourceObject::GetName() const noexcept {
	return m_Name;
}
bool ResourceObject::IsRootObject() const noexcept {
	return m_Parent == nullptr;
}
const ResourceDirectory& ResourceObject::GetParent() const noexcept {
	return *m_Parent;
}
ResourceDirectory& ResourceObject::GetParent() noexcept {
	return *m_Parent;
}
std::chrono::system_clock::time_point ResourceObject::GetCreationTime() const {
	return m_CreationTime;
}
std::chrono::system_clock::time_point ResourceObject::GetLastEditTime() const {
	return m_LastEditTime;
}
void ResourceObject::SetLastEditTime(std::chrono::system_clock::time_point newLastEditTime) {
	m_LastEditTime = std::move(newLastEditTime);
}

const ResourceDirectory* ResourceObject::IsDirectory() const noexcept {
	return dynamic_cast<const ResourceDirectory*>(this);
}
ResourceDirectory* ResourceObject::IsDirectory() noexcept {
	return dynamic_cast<ResourceDirectory*>(this);
}
const ResourceFile* ResourceObject::IsFile() const noexcept {
	return dynamic_cast<const ResourceFile*>(this);
}
ResourceFile* ResourceObject::IsFile() noexcept {
	return dynamic_cast<ResourceFile*>(this);
}

ResourceDirectory::ResourceDirectory()
	: ResourceObject("$Root", nullptr, {}) {}

ResourceDirectory::ResourceDirectory(std::string name, ResourceDirectory* parent,
	std::chrono::system_clock::time_point creationTime)
	: ResourceObject(std::move(name), parent, std::move(creationTime)) {}

ResourceDirectory& ResourceDirectory::CreateDirectory(std::string name,
	std::chrono::system_clock::time_point creationTime) {
	assert(name.size() > 0);
	assert(std::ispunct(static_cast<unsigned char>(name.front())) == 0);

	auto directory = std::unique_ptr<ResourceDirectory>(
		new ResourceDirectory(name, this, std::move(creationTime)));
	const auto [iterator, isSuccess] = m_Objects.insert(std::make_pair(std::move(name), std::move(directory)));
	assert(isSuccess);

	return *static_cast<ResourceDirectory*>(iterator->second.get());
}
ResourceFile& ResourceDirectory::CreateFile(std::string name,
	std::chrono::system_clock::time_point creationTime) {
	assert(name.size() > 0);
	assert(std::ispunct(static_cast<unsigned char>(name.front())) == 0);

	auto file = std::unique_ptr<ResourceFile>(
		new ResourceFile(name, this, std::move(creationTime)));
	const auto [iterator, isSuccess] = m_Objects.insert(std::make_pair(std::move(name), std::move(file)));
	assert(isSuccess);

	return *static_cast<ResourceFile*>(iterator->second.get());
}

ResourceFile::ResourceFile(std::string name, ResourceDirectory* parent,
	std::chrono::system_clock::time_point creationTime)
	: ResourceObject(std::move(name), parent, std::move(creationTime)) {}

ResourceFile& ResourceFile::operator=(Matrix matrix) {
	m_Content = std::move(matrix);
	SetLastEditTime(std::chrono::system_clock::now());

	return *this;
}

bool ResourceFile::IsEmpty() const noexcept {
	return m_Content.index() == 0;
}
const Matrix* ResourceFile::IsMatrix() const noexcept {
	return m_Content.index() == 1 ? &std::get<1>(m_Content) : nullptr;
}