#include "Project.hpp"

#include "IO.hpp"
#include "Layer.hpp"
#include "Matrix.hpp"
#include "Optimizer.hpp"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iterator>
#include <stdexcept>

ResourceObject::~ResourceObject() {}

ResourceObject::ResourceObject(std::string name, ResourceDirectory* parent,
	std::chrono::system_clock::time_point creationTime)
	: m_Name(std::move(name)), m_Parent(parent), m_CreationTime(std::move(creationTime)),
	m_LastEditTime(m_CreationTime) {}

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

std::vector<std::pair<std::string, const ResourceObject*>> ResourceDirectory::GetAllObjects() const {
	std::vector<std::pair<std::string, const ResourceObject*>> result;

	for (const auto& [name, object] : m_Objects) {
		result.push_back(std::make_pair(name, object.get()));
	}

	return result;
}
std::vector<std::pair<std::string, ResourceObject*>> ResourceDirectory::GetAllObjects() {
	std::vector<std::pair<std::string, ResourceObject*>> result;

	for (const auto& [name, object] : m_Objects) {
		result.push_back(std::make_pair(name, object.get()));
	}

	return result;
}
ResourceDirectory& ResourceDirectory::CreateDirectory(std::string name,
	std::chrono::system_clock::time_point creationTime) {
	assert(name.size() > 0);

	auto directory = std::unique_ptr<ResourceDirectory>(
		new ResourceDirectory(name, this, std::move(creationTime)));
	const auto [iterator, isSuccess] = m_Objects.insert(
		std::make_pair(std::move(name), std::move(directory)));
	assert(isSuccess);

	return *static_cast<ResourceDirectory*>(iterator->second.get());
}
ResourceFile& ResourceDirectory::CreateFile(std::string name,
	std::chrono::system_clock::time_point creationTime) {
	assert(name.size() > 0);

	auto file = std::unique_ptr<ResourceFile>(
		new ResourceFile(name, this, std::move(creationTime)));
	const auto [iterator, isSuccess] = m_Objects.insert(
		std::make_pair(std::move(name), std::move(file)));
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
ResourceFile& ResourceFile::operator=(TrainSample trainSample) {
	m_Content = std::move(trainSample);
	SetLastEditTime(std::chrono::system_clock::now());

	return *this;
}
ResourceFile& ResourceFile::operator=(TrainData trainData) {
	m_Content = std::move(trainData);
	SetLastEditTime(std::chrono::system_clock::now());

	return *this;
}

bool ResourceFile::IsEmpty() const noexcept {
	return m_Content.index() == 0;
}
const Matrix* ResourceFile::IsMatrix() const noexcept {
	return m_Content.index() == 1 ? &std::get<1>(m_Content) : nullptr;
}
const TrainSample* ResourceFile::IsTrainSample() const noexcept {
	return m_Content.index() == 2 ? &std::get<2>(m_Content) : nullptr;
}
const TrainData* ResourceFile::IsTrainData() const noexcept {
	return m_Content.index() == 3 ? &std::get<3>(m_Content) : nullptr;
}

std::string_view Project::GetName() const noexcept {
	return m_Name;
}
void Project::SetName(std::string newName) noexcept {
	m_Name = std::move(newName);
}
const std::filesystem::path& Project::GetPath() const noexcept {
	return m_Path;
}
void Project::SetPath(std::filesystem::path newPath) noexcept {
	assert(!newPath.empty());

	m_Path = std::move(newPath);
}

const Network& Project::GetNetwork() const noexcept {
	return m_Network;
}
Network& Project::GetNetwork() noexcept {
	return m_Network;
}
const ResourceDirectory& Project::GetResources() const noexcept {
	return m_Resources;
}
ResourceDirectory& Project::GetResources() noexcept {
	return m_Resources;
}

namespace {
	void ReadVariableTable(BinaryAdaptor& bin, VariableTable& variableTable) {
		const std::uint32_t variableCount = bin.ReadInt32();

		for (std::uint32_t i = 0; i < variableCount; ++i) {
			std::string variableName = bin.ReadString();
			Matrix variableValue = bin.ReadMatrix();

			variableTable.AddVariable(std::move(variableName), std::move(variableValue));
		}
	}
	void WriteVariableTable(BinaryAdaptor& bin, const VariableTable& variableTable) {
		const auto variables = variableTable.GetAllVariables();

		bin.Write(static_cast<std::int32_t>(variables.size()));
		for (const auto& variable : variables) {
			bin.Write(std::string(variable.GetName()));
			bin.Write(variable.GetValue());
		}
	}

	void ReadParameterTable(BinaryAdaptor& bin, ParameterTable& parameterTable) {
		const std::uint32_t parameterCount = bin.ReadInt32();

		for (std::uint32_t i = 0; i < parameterCount; ++i) {
			std::string parameterName = bin.ReadString();
			Matrix parameterValue = bin.ReadMatrix();
			Matrix parameterGradient = bin.ReadMatrix();

			const Parameter parameter =
				parameterTable.AddParameter(std::move(parameterName), std::move(parameterValue));
			parameter.SetGradient(std::move(parameterGradient));

			ReadVariableTable(bin, parameter.GetVariableTable());
		}
	}
	void WriteParameterTable(BinaryAdaptor& bin, const ParameterTable& parameterTable) {
		const auto parameters = parameterTable.GetAllParameters();

		bin.Write(static_cast<std::int32_t>(parameters.size()));
		for (const auto& parameter : parameters) {
			bin.Write(std::string(parameter.GetName()));
			bin.Write(parameter.GetValue());
			bin.Write(parameter.GetGradient());
			WriteVariableTable(bin, parameter.GetVariableTable());
		}
	}

	void ReadTrainSample(BinaryAdaptor& bin, TrainSample& trainSample) {
		trainSample.first = bin.ReadMatrix();
		trainSample.second = bin.ReadMatrix();
	}
	void WriteTrainSample(BinaryAdaptor& bin, const TrainSample& trainSample) {
		bin.Write(trainSample.first);
		bin.Write(trainSample.second);
	}
	void ReadTrainData(BinaryAdaptor& bin, TrainData& trainData) {
		const std::uint32_t sampleCount = bin.ReadInt32();

		for (std::uint32_t i = 0; i < sampleCount; ++i) {
			ReadTrainSample(bin, trainData.emplace_back());
		}
	}
	void WriteTrainData(BinaryAdaptor& bin, const TrainData& trainData) {
		bin.Write(static_cast<std::int32_t>(trainData.size()));
		for (const auto& trainSample : trainData) {
			WriteTrainSample(bin, trainSample);
		}
	}

	void ReadNetwork(BinaryAdaptor& bin, Network& network) {
		const std::uint32_t layerCount = bin.ReadInt32();

		for (std::uint32_t i = 0; i < layerCount; ++i) {
			const std::string layerName = bin.ReadString();
			std::unique_ptr<Layer> layer;

			if (layerName == "FCLayer") {
				const std::uint32_t inputSize = bin.ReadInt32();
				const std::uint32_t outputSize = bin.ReadInt32();

				layer = std::make_unique<FCLayer>(inputSize, outputSize);
			} else if (layerName == "ALayer") {
				const AFunction aFunction = static_cast<AFunction>(bin.ReadInt32());

				layer = std::make_unique<ALayer>(aFunction);
			} else throw std::runtime_error("Invalid layer name");

			ReadVariableTable(bin, layer->GetVariableTable());
			ReadParameterTable(bin, layer->GetParameterTable());

			network.AddLayer(std::move(layer));
		}

		const std::string optimizerName = bin.ReadString();
		std::unique_ptr<Optimizer> optimizer;

		if (optimizerName == "SGDOptimizer") {
			const float learningRate = bin.ReadFloat();

			auto sgdOptimizer = std::make_unique<SGDOptimizer>();
			sgdOptimizer->SetLearningRate(learningRate);

			optimizer = std::move(sgdOptimizer);
		} else throw std::runtime_error("Invalid optimizer name");

		const std::string lossFunctionName = bin.ReadString();

		if (lossFunctionName == "MSE") {
			optimizer->SetLossFunction(MSE);
		} else throw std::runtime_error("Invalid loss function name");

		network.SetOptimizer(std::move(optimizer));
	}
	void WriteNetwork(BinaryAdaptor& bin, const Network& network) {
		const std::size_t layerCount = network.GetLayerCount();

		bin.Write(static_cast<std::int32_t>(layerCount));
		for (std::size_t i = 0; i < layerCount; ++i) {
			const Layer& layer = network.GetLayer(i);
			const std::string layerName(layer.GetName());

			bin.Write(layerName);
			if (layerName == "FCLayer") {
				const auto& fcLayer = static_cast<const FCLayer&>(layer);

				bin.Write(static_cast<std::int32_t>(fcLayer.GetForwardInputSize()));
				bin.Write(static_cast<std::int32_t>(fcLayer.GetForwardOutputSize()));
			} else if (layerName == "ALayer") {
				const auto& aLayer = static_cast<const ALayer&>(layer);

				bin.Write(static_cast<std::int32_t>(aLayer.GetAFunction()));
			}

			WriteVariableTable(bin, layer.GetVariableTable());
			WriteParameterTable(bin, layer.GetParameterTable());
		}

		const Optimizer& optimizer = network.GetOptimizer();
		const std::string optimizerName(optimizer.GetName());

		bin.Write(optimizerName);
		if (optimizerName == "SGDOptimizer") {
			const auto& sgdOptimizer = static_cast<const SGDOptimizer&>(optimizer);

			bin.Write(sgdOptimizer.GetLearningRate());
		}

		bin.Write(std::string(optimizer.GetLossFunction()->GetName()));
	}

	void ReadResourceObjects(BinaryAdaptor& bin, ResourceDirectory& resources);
	void ReadResourceObject(BinaryAdaptor& bin, ResourceDirectory& parent) {
		std::string objectName = bin.ReadString();
		std::chrono::system_clock::time_point objectCreationTime(std::chrono::seconds(bin.ReadInt64()));
		std::chrono::system_clock::time_point objectLastEditTime(std::chrono::seconds(bin.ReadInt64()));

		const std::string objectType = bin.ReadString();

		if (objectType == "Directory") {
			ResourceDirectory& directory = parent.CreateDirectory(std::move(objectName), std::move(objectCreationTime));
			directory.SetLastEditTime(std::move(objectLastEditTime));

			ReadResourceObjects(bin, directory);
		} else if (objectType == "File") {
			ResourceFile& file = parent.CreateFile(std::move(objectName), std::move(objectCreationTime));
			file.SetLastEditTime(std::move(objectLastEditTime));

			const std::string valueType = bin.ReadString();

			if (valueType == "Empty") {}
			else if (valueType == "Matrix") {
				file = bin.ReadMatrix();
			} else if (valueType == "TrainSample") {
				TrainSample trainSample;

				ReadTrainSample(bin, trainSample);
				file = std::move(trainSample);
			} else if (valueType == "TrainData") {
				TrainData trainData;

				ReadTrainData(bin, trainData);
				file = std::move(trainData);
			} else throw std::runtime_error("Invalid resource file type name");
		} else throw std::runtime_error("Invalid resource object name");
	}
	void WriteResourceObjects(BinaryAdaptor& bin, const ResourceDirectory& resources);
	void WriteResourceObject(BinaryAdaptor& bin, const ResourceObject& resourceObject) {
		bin.Write(std::string(resourceObject.GetName()));
		bin.Write(static_cast<std::int64_t>(std::chrono::time_point_cast<std::chrono::seconds>(
			resourceObject.GetCreationTime()).time_since_epoch().count()));
		bin.Write(static_cast<std::int64_t>(std::chrono::time_point_cast<std::chrono::seconds>(
			resourceObject.GetLastEditTime()).time_since_epoch().count()));

		if (const auto directory = resourceObject.IsDirectory(); directory) {
			const auto objects = directory->GetAllObjects();

			bin.Write("Directory");
			WriteResourceObjects(bin, *directory);
		} else if (const auto file = resourceObject.IsFile(); file) {
			bin.Write("File");
			if (file->IsEmpty()) {
				bin.Write("Empty");
			} else if (const auto matrix = file->IsMatrix(); matrix) {
				bin.Write("Matrix");
				bin.Write(*matrix);
			} else if (const auto trainSample = file->IsTrainSample(); trainSample) {
				bin.Write("TrainSample");
				WriteTrainSample(bin, *trainSample);
			} else if (const auto trainData = file->IsTrainData(); trainData) {
				bin.Write("TrainData");
				WriteTrainData(bin, *trainData);
			}
		}
	}
	void ReadResourceObjects(BinaryAdaptor& bin, ResourceDirectory& resources) {
		const std::uint32_t objectCount = bin.ReadInt32();

		for (std::uint32_t i = 0; i < objectCount; ++i) {
			ReadResourceObject(bin, resources);
		}
	}
	void WriteResourceObjects(BinaryAdaptor& bin, const ResourceDirectory& resources) {
		const auto objects = resources.GetAllObjects();

		bin.Write(static_cast<std::int32_t>(objects.size()));
		for (const auto& [name, object] : objects) {
			WriteResourceObject(bin, *object);
		}
	}
}

void Project::Load(std::filesystem::path path) {
	assert(m_Path.empty());

	std::ifstream stream(path, std::ios::binary);
	if (!stream) throw std::runtime_error("Failed to open a file");

	BinaryAdaptor bin(stream);

	std::uint8_t magicNumber[sizeof(m_MagicNumber)];
	bin.ReadBytes(magicNumber, sizeof(magicNumber));
	if (!std::equal(std::begin(magicNumber), std::end(magicNumber), m_MagicNumber))
		throw std::runtime_error("Invalid magic number");

	const std::uint32_t version = bin.ReadInt32();
	switch (version) {
	case 0x00000000: {
		m_Name = bin.ReadString();

		ReadNetwork(bin, m_Network);
		ReadResourceObjects(bin, m_Resources);

		break;
	}

	default:
		throw std::runtime_error("Incompatible version");
	}

	m_Path = std::move(path);
}
void Project::Save() const {
	assert(!m_Path.empty());

	std::ofstream stream(m_Path, std::ios::binary);
	if (!stream) throw std::runtime_error("Failed to open a file");

	BinaryAdaptor bin(stream);

	bin.Write(m_MagicNumber, sizeof(m_MagicNumber));
	bin.Write(static_cast<std::int32_t>(m_Version));

	bin.Write(m_Name);

	WriteNetwork(bin, m_Network);
	WriteResourceObjects(bin, m_Resources);
}