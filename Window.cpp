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
	m_Project->SetName("���� ����");
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

	DropDownMenuItemRef file("����");

	file->AddSubItem(MenuItemRef("�� ������Ʈ", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));
	file->AddSubItem(MenuItemRef("������Ʈ ����", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));
	file->AddSubItem(MenuItemRef("������Ʈ ����", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			if (!m_IsSaved) {
				SaveProject();
			}
		})));
	file->AddSubItem(MenuItemRef("����", std::make_unique<FunctionalMenuItemEventHandler>(
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
	menu->AddItem(MenuItemRef("�н�", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));
	menu->AddItem(MenuItemRef("����", std::make_unique<FunctionalMenuItemEventHandler>(
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

	MessageDialogRef dialog(*m_Window, "ShitAIMaker", "������� ���� ���� ������ �ֽ��ϴ�",
		"������� ���� ���� ������ ��� �����˴ϴ�. ���� ������ �����ұ��?",
		MessageDialog::Warning, MessageDialog::Yes | MessageDialog::No | MessageDialog::Cancel);
	dialog->Show();

	return std::any_cast<MessageDialog::Button>(dialog->GetResult());
}
void MainWindowHandler::SaveProject() {
	// TODO

	m_IsSaved = true;

	UpdateText();
}