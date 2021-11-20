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

	void StartOperation();
	void DoneOperation();
	void DoneTestOperation(std::string result);
	void DoneFastOptimizingOperation(std::string result);
	void DoneOptimizingOperation();
	void DoneOptimizingOperation(std::string result);
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

class LearningRateInputDialogHandler final : public WindowDialogEventHandler {
private:
	WindowDialog* m_WindowDialog = nullptr;

	TextBox* m_LearningRateTextBox = nullptr;
	Button* m_OkButton = nullptr;
	Button* m_CancelButton = nullptr;

	std::optional<float> m_LearningRate;

public:
	LearningRateInputDialogHandler() noexcept = default;
	LearningRateInputDialogHandler(const LearningRateInputDialogHandler&) = delete;
	virtual ~LearningRateInputDialogHandler() override = default;

public:
	LearningRateInputDialogHandler& operator=(const LearningRateInputDialogHandler&) = delete;

public:
	bool HasLearningRate() const noexcept;
	float GetLearningRate() const noexcept;

public:
	virtual void OnCreate(WindowDialog& dialog) override;

	virtual void OnResize(WindowDialog& dialog) override;

public:
	void OnOkButtonClick();
	void OnCancelButtonClick();
};

class EpochInputDialogHandler final : public WindowDialogEventHandler {
private:
	WindowDialog* m_WindowDialog = nullptr;

	TextBox* m_EpochTextBox = nullptr;
	Button* m_OkButton = nullptr;
	Button* m_CancelButton = nullptr;

	std::optional<std::size_t> m_Epoch;

public:
	EpochInputDialogHandler() noexcept = default;
	EpochInputDialogHandler(const EpochInputDialogHandler&) = delete;
	virtual ~EpochInputDialogHandler() override = default;

public:
	EpochInputDialogHandler& operator=(const EpochInputDialogHandler&) = delete;

public:
	bool HasEpoch() const noexcept;
	std::size_t GetEpoch() const noexcept;

public:
	virtual void OnCreate(WindowDialog& dialog) override;

	virtual void OnResize(WindowDialog& dialog) override;

public:
	void OnOkButtonClick();
	void OnCancelButtonClick();
};