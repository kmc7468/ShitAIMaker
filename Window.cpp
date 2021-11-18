#include "Window.hpp"

#include <memory>
#include <utility>

FunctionalMenuItemEventHandler::FunctionalMenuItemEventHandler(std::function<void(MenuItem&)> onClick)
	: m_OnClick(std::move(onClick)) {}

void FunctionalMenuItemEventHandler::OnClick(MenuItem& menuItem) {
	if (m_OnClick) {
		m_OnClick(menuItem);
	}
}

void MainWindowHandler::OnCreate(Control& control) {
	m_Window = &dynamic_cast<Window&>(control);

	m_Window->SetMenu(CreateMenu());
}

MenuRef MainWindowHandler::CreateMenu() {
	MenuRef menu;

	DropDownMenuItemRef file("����");

	file->AddSubItem(MenuItemRef("�� ������Ʈ", std::make_unique<FunctionalMenuItemEventHandler>(
		[](MenuItem&) {
			// TODO
		})));
	file->AddSubItem(MenuItemRef("������Ʈ ����", std::make_unique<FunctionalMenuItemEventHandler>(
		[](MenuItem&) {
			// TODO
		})));
	file->AddSubItem(MenuItemRef("������Ʈ ����", std::make_unique<FunctionalMenuItemEventHandler>(
		[](MenuItem&) {
			// TODO
		})));
	file->AddSubItem(MenuItemRef("����", std::make_unique<FunctionalMenuItemEventHandler>(
		[](MenuItem&) {
			// TODO
		})));

	menu->AddItem(std::move(file));
	menu->AddItem(MenuItemRef("�н�", std::make_unique<FunctionalMenuItemEventHandler>(
		[](MenuItem&) {
			// TODO
		})));
	menu->AddItem(MenuItemRef("����", std::make_unique<FunctionalMenuItemEventHandler>(
		[](MenuItem&) {
			// TODO
		})));

	return menu;
}