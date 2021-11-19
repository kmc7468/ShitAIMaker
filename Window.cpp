#include "Window.hpp"

#include "Optimizer.hpp"

#include <string>
#include <utility>

FunctionalMenuItemEventHandler::FunctionalMenuItemEventHandler(std::function<void(MenuItem&)> onClick)
	: m_OnClick(std::move(onClick)) {}

void FunctionalMenuItemEventHandler::OnClick(MenuItem& menuItem) {
	if (m_OnClick) {
		m_OnClick(menuItem);
	}
}

MainWindowHandler::MainWindowHandler() {
	m_Project->SetName("제목 없음");
}

void MainWindowHandler::OnCreate(Control& control) {
	m_Window = &dynamic_cast<Window&>(control);

	m_Window->SetMenu(CreateMenu());
	UpdateText();
}
void MainWindowHandler::OnClose(Window&, bool& cancel) {
	switch (AskDiscardChanges()) {
	case MessageDialog::Yes:
		SaveProject();
		break;

	case MessageDialog::No:
		break;

	case MessageDialog::Cancel:
		cancel = true;
		break;
	}
}

MenuRef MainWindowHandler::CreateMenu() {
	MenuRef menu;

	DropDownMenuItemRef file("파일");

	file->AddSubItem(MenuItemRef("새 프로젝트", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));
	file->AddSubItem(MenuItemRef("프로젝트 열기", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));
	file->AddSubItem(MenuItemRef("프로젝트 저장", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			if (!m_IsSaved) {
				SaveProject();
			}
		})));
	file->AddSubItem(MenuItemRef("종료", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			switch (AskDiscardChanges()) {
			case MessageDialog::Yes:
				SaveProject();
				break;

			case MessageDialog::No:
				break;

			case MessageDialog::Cancel:
				return;
			}

			m_Window->Close();
		})));

	menu->AddItem(std::move(file));
	menu->AddItem(MenuItemRef("학습", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));
	menu->AddItem(MenuItemRef("실행", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));

	return menu;
}
void MainWindowHandler::UpdateText() {
	const std::string newText = std::string(m_Project->GetName()) + " - ShitAIMaker";

	m_Window->SetText(m_IsSaved ? newText : '*' + newText);
}

MessageDialog::Button MainWindowHandler::AskDiscardChanges() {
	if (m_IsSaved) return MessageDialog::No;

	MessageDialogRef dialog(*m_Window, "ShitAIMaker", "저장되지 않은 변경 사항이 있습니다",
		"저장되지 않은 변경 사항은 모두 삭제됩니다. 변경 사항을 저장할까요?",
		MessageDialog::Warning, MessageDialog::Yes | MessageDialog::No | MessageDialog::Cancel);
	dialog->Show();

	return std::any_cast<MessageDialog::Button>(dialog->GetResult());
}
void MainWindowHandler::SaveProject() {
	// TODO

	m_IsSaved = true;

	UpdateText();
}