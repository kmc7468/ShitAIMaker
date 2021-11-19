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

void MainWindowHandler::OnCreate(Control& control) {
	CreateNewProject();

	m_Window = &dynamic_cast<Window&>(control);

	m_Window->SetMenu(CreateMenu());
	UpdateText();
}
void MainWindowHandler::OnClose(Window&, bool& cancel) {
	switch (AskDiscardChanges()) {
	case DialogResult::Yes:
		SaveProject();
		break;

	case DialogResult::No:
		break;

	case DialogResult::Cancel:
		cancel = true;
		break;
	}
}

MenuRef MainWindowHandler::CreateMenu() {
	MenuRef menu;

	DropDownMenuItemRef file("����");

	file->AddSubItem(MenuItemRef("�� ������Ʈ", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			switch (AskDiscardChanges()) {
			case DialogResult::Yes:
				SaveProject();
				break;

			case DialogResult::No:
				break;

			case DialogResult::Cancel:
				return;
			}

			CreateNewProject();
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
			case DialogResult::Yes:
				SaveProject();
				break;

			case DialogResult::No:
				break;

			case DialogResult::Cancel:
				return;
			}

			m_Window->Close();
		})));

	menu->AddItem(std::move(file));

	DropDownMenuItemRef optimizing("��Ʈ��ũ");

	optimizing->AddSubItem(MenuItemRef("���� ����", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));
	optimizing->AddSubItem(MenuItemRef("���� �ð�ȭ", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));

	optimizing->AddSubItem(MenuItemSeparatorRef());
	optimizing->AddSubItem(MenuItemRef("���� �н�", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));
	optimizing->AddSubItem(MenuItemRef("�н� �ð�ȭ", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));
	optimizing->AddSubItem(MenuItemRef("��Ƽ������ ����", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));

	menu->AddItem(std::move(optimizing));

	DropDownMenuItemRef help("����");

	help->AddSubItem(MenuItemRef("���� ����", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));
	help->AddSubItem(MenuItemRef("�ǵ�� ������", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));

	help->AddSubItem(MenuItemSeparatorRef());
	help->AddSubItem(MenuItemRef("������Ʈ Ȯ��", std::make_unique<FunctionalMenuItemEventHandler>(
		[&](MenuItem&) {
			// TODO
		})));
	help->AddSubItem(MenuItemRef("ShitAIMaker ����", std::make_unique<FunctionalMenuItemEventHandler>(
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

DialogResult MainWindowHandler::AskDiscardChanges() {
	if (m_IsSaved) return DialogResult::No;

	MessageDialogRef dialog(*m_Window, SAM_APPNAME, "������� ���� ���� ������ �ֽ��ϴ�",
		"������� ���� ���� ������ ��� �����˴ϴ�. ���� ������ �����ұ��?",
		MessageDialog::Warning, MessageDialog::Yes | MessageDialog::No | MessageDialog::Cancel);

	return dialog->Show();
}
void MainWindowHandler::CreateNewProject() {
	m_Project = std::make_unique<Project>();
	m_Project->SetName("���� ����");

	m_IsSaved = true;
}
void MainWindowHandler::SaveProject() {
	// TODO

	m_IsSaved = true;

	UpdateText();
}