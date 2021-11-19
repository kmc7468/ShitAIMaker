#include "Window.hpp"

#include "Application.hpp"
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

	DropDownMenuItemRef optimizing("네트워크");

	optimizing->AddSubItem(MenuItemRef("빠른 실행", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));
	optimizing->AddSubItem(MenuItemRef("실행 시각화", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));

	optimizing->AddSubItem(MenuItemSeparatorRef());
	optimizing->AddSubItem(MenuItemRef("빠른 학습", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));
	optimizing->AddSubItem(MenuItemRef("학습 시각화", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));
	optimizing->AddSubItem(MenuItemRef("옵티마이저 설정", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));

	menu->AddItem(std::move(optimizing));

	DropDownMenuItemRef help("도움말");

	help->AddSubItem(MenuItemRef("도움말 보기", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));
	help->AddSubItem(MenuItemRef("피드백 보내기", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));

	help->AddSubItem(MenuItemSeparatorRef());
	help->AddSubItem(MenuItemRef("업데이트 확인", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));
	help->AddSubItem(MenuItemRef("ShitAIMaker 정보", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));

	menu->AddItem(std::move(help));

	return menu;
}
void MainWindowHandler::UpdateText() {
	const std::string newText = std::string(m_Project->GetName()) + " - " SAM_APPTITLE;

	m_Window->SetText(m_IsSaved ? newText : '*' + newText);
}

MessageDialog::Button MainWindowHandler::AskDiscardChanges() {
	if (m_IsSaved) return MessageDialog::No;

	MessageDialogRef dialog(*m_Window, SAM_APPNAME, "저장되지 않은 변경 사항이 있습니다",
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