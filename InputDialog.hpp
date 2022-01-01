#pragma once

#include "PALGraphics.hpp"

#include <functional>
#include <string>

class InputDialogHandler final : public WindowDialogEventHandler {
private:
	WindowDialog* m_WindowDialog = nullptr;

	TextBox* m_TextBox = nullptr;
	Button* m_OkButton = nullptr;
	Button* m_CancelButton = nullptr;

	std::function<bool(WindowDialog&, const std::string&)> m_OnOkButtonClick;
	bool m_MultiLines;

public:
	InputDialogHandler(std::function<bool(WindowDialog&, const std::string&)> onOkButtonClick,
		bool multiLines = false) noexcept;
	InputDialogHandler(const InputDialogHandler&) = delete;
	virtual ~InputDialogHandler() override = default;

public:
	InputDialogHandler& operator=(const InputDialogHandler&) = delete;

public:
	virtual void OnCreate(WindowDialog& dialog) override;

	virtual void OnResize(WindowDialog& dialog) override;

private:
	void OnOkButtonClick();
	void OnCancelButtonClick();
};