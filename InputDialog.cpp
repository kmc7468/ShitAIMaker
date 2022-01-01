#include "InputDialog.hpp"

#include <cassert>
#include <utility>

InputDialogHandler::InputDialogHandler(std::function<bool(WindowDialog&, const std::string&)> onOkButtonClick,
	bool multiLines) noexcept
	: m_OnOkButtonClick(std::move(onOkButtonClick)), m_MultiLines(multiLines) {
	assert(m_OnOkButtonClick != nullptr);
}

void InputDialogHandler::OnCreate(WindowDialog& dialog) {
	m_WindowDialog = &dialog;

	m_TextBox = &dynamic_cast<TextBox&>(dialog.AddChild(
		TextBoxRef(std::make_unique<TextBoxEventHandler>(), m_MultiLines)));

	m_TextBox->SetLocation(10, 10);
	m_TextBox->Show();

	class OkButtonHandler final : public ButtonEventHandler {
	private:
		WindowDialog& m_WindowDialog;

	public:
		OkButtonHandler(WindowDialog& windowDialog) noexcept
			: m_WindowDialog(windowDialog) {}
		OkButtonHandler(const OkButtonHandler&) = delete;
		virtual ~OkButtonHandler() override = default;

	public:
		OkButtonHandler& operator=(const OkButtonHandler&) = delete;

	public:
		virtual void OnClick(Control&) override {
			dynamic_cast<InputDialogHandler&>(m_WindowDialog.GetEventHandler()).OnOkButtonClick();
		}
	};

	m_OkButton = &dynamic_cast<Button&>(dialog.AddChild(
		ButtonRef(std::make_unique<OkButtonHandler>(*m_WindowDialog))));

	m_OkButton->SetText("확인");
	m_OkButton->Show();

	class CancelButtonHandler final : public ButtonEventHandler {
	private:
		WindowDialog& m_WindowDialog;

	public:
		CancelButtonHandler(WindowDialog& windowDialog) noexcept
			: m_WindowDialog(windowDialog) {}
		CancelButtonHandler(const CancelButtonHandler&) = delete;
		virtual ~CancelButtonHandler() override = default;

	public:
		CancelButtonHandler& operator=(const CancelButtonHandler&) = delete;

	public:
		virtual void OnClick(Control&) override {
			dynamic_cast<InputDialogHandler&>(m_WindowDialog.GetEventHandler()).OnCancelButtonClick();
		}
	};

	m_CancelButton = &dynamic_cast<Button&>(dialog.AddChild(
		ButtonRef(std::make_unique<CancelButtonHandler>(*m_WindowDialog))));

	m_CancelButton->SetText("취소");
	m_CancelButton->Show();

	if (m_MultiLines) {
		m_WindowDialog->SetMinimumSize(400, 200);
	} else {
		m_WindowDialog->SetMinimumSize(400, 130);
	}
}

void InputDialogHandler::OnResize(WindowDialog& dialog) {
	const auto& [clientWidth, clientHeight] = dialog.GetClientSize();

	if (m_TextBox) {
		if (m_MultiLines) {
			m_TextBox->SetSize(clientWidth - 20, clientHeight - (30 + 24));
		} else {
			m_TextBox->SetSize(clientWidth - 20, 24);
		}

		m_OkButton->SetLocation(clientWidth - (20 + 82 * 2), clientHeight - (10 + 24));
		m_OkButton->SetSize(82, 24);

		m_CancelButton->SetLocation(clientWidth - (10 + 82), clientHeight - (10 + 24));
		m_CancelButton->SetSize(82, 24);
	}
}

void InputDialogHandler::OnOkButtonClick() {
	if (m_OnOkButtonClick(*m_WindowDialog, m_TextBox->GetText())) {
		m_WindowDialog->Close(DialogResult::Ok);
	}
}
void InputDialogHandler::OnCancelButtonClick() {
	m_WindowDialog->Close(DialogResult::Cancel);
}