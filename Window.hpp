#pragma once

#include "Network.hpp"
#include "PALGraphics.hpp"
#include "Project.hpp"

#include <functional>
#include <memory>
#include <optional>

class MainWindowHandler final : public WindowEventHandler {
private:
	Window* m_Window = nullptr;

	std::unique_ptr<Project> m_Project;
	bool m_IsSaved = true;

	Panel* m_NetworkViewer = nullptr;

public:
	MainWindowHandler() noexcept = default;
	MainWindowHandler(const MainWindowHandler&) = delete;
	virtual ~MainWindowHandler() override = default;

public:
	virtual void OnCreate(Control& control) override;
	virtual void OnClose(Window& window, bool& cancel) override;

	virtual void OnResize(Control& control) override;

private:
	MenuRef CreateMenu();
	void UpdateText();

private:
	DialogResult AskDiscardChanges();
	void CreateNewProject();
	bool SaveProject(bool saveAs = false);

	std::optional<TrainData> AskTrainData(std::string dialogTitle);
};

class TrainDataInputDialogHandler final : public WindowDialogEventHandler {
private:
	WindowDialog* m_WindowDialog = nullptr;

	TextBox* m_TrainDataTextBox = nullptr;
	Button* m_OkButton = nullptr;
	Button* m_CancelButton = nullptr;

	std::size_t m_InputSize, m_OutputSize;
	std::optional<TrainData> m_TrainData;

public:
	TrainDataInputDialogHandler(std::size_t inputSize, std::size_t outputSize) noexcept;
	TrainDataInputDialogHandler(const TrainDataInputDialogHandler&) = delete;
	virtual ~TrainDataInputDialogHandler() override = default;

public:
	TrainDataInputDialogHandler& operator=(const TrainDataInputDialogHandler&) = delete;

public:
	bool HasTrainData() const noexcept;
	const TrainData& GetTrainData() const noexcept;

public:
	virtual void OnCreate(WindowDialog& dialog) override;

	virtual void OnResize(WindowDialog& dialog) override;

public:
	void OnOkButtonClick();
	void OnCancelButtonClick();
};