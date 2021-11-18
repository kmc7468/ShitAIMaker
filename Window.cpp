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

	DropDownMenuItemRef file("파일");

	file->AddSubItem(MenuItemRef("새 프로젝트", std::make_unique<FunctionalMenuItemEventHandler>(
		[](MenuItem&) {
			// TODO
		})));
	file->AddSubItem(MenuItemRef("프로젝트 열기", std::make_unique<FunctionalMenuItemEventHandler>(
		[](MenuItem&) {
			// TODO
		})));
	file->AddSubItem(MenuItemRef("프로젝트 저장", std::make_unique<FunctionalMenuItemEventHandler>(
		[](MenuItem&) {
			// TODO
		})));
	file->AddSubItem(MenuItemRef("종료", std::make_unique<FunctionalMenuItemEventHandler>(
		[](MenuItem&) {
			// TODO
		})));

	menu->AddItem(std::move(file));
	menu->AddItem(MenuItemRef("학습", std::make_unique<FunctionalMenuItemEventHandler>(
		[](MenuItem&) {
			// TODO
		})));
	menu->AddItem(MenuItemRef("실행", std::make_unique<FunctionalMenuItemEventHandler>(
		[](MenuItem&) {
			// TODO
		})));

	return menu;
}