#pragma once

#include "Network.hpp"
#include "PALGraphics.hpp"
#include "Project.hpp"

#include <any>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <thread>

class MainWindowHandler final : public WindowEventHandler {
private:
	Window* m_Window = nullptr;
	FontRef m_Font = std::shared_ptr<Font>();

	std::unique_ptr<Project> m_Project;
	bool m_IsSaved = true;

	Panel* m_NetworkViewer = nullptr;

	std::optional<std::jthread> m_Thread;

public:
	MainWindowHandler() noexcept = default;
	MainWindowHandler(const MainWindowHandler&) = delete;
	virtual ~MainWindowHandler() override = default;

public:
	virtual void OnCreate(Control& control) override;
	virtual void OnClose(Window& window, bool& cancel) override;

	virtual void OnResize(Control& control) override;

	virtual void OnReceiveMessage(Control& control, std::size_t message, std::optional<std::any> argument) override;

private:
	MenuRef CreateMenu();
	void UpdateText();

private:
	DialogResult AskDiscardChanges();
	void CreateNewProject();
	bool SaveProject(bool saveAs = false);

	std::optional<TrainData> AskTrainData(std::string dialogTitle);
	std::optional<float> AskLearningRate(std::string dialogTitle);
	std::optional<std::size_t> AskEpoch(std::string dialogTitle);
	std::optional<std::size_t> AskInputOrOutputSize(std::string dialogTitle);

	void StartOperation();
	void DoneOperation();
	void DoneTestOperation(std::string result);
	void DoneFastOptimizingOperation(std::string result);
	void DoneOptimizingOperation();
	void DoneOptimizingOperation(std::string result);
};