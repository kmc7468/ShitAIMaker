#pragma once

#include "PALGraphics.hpp"

#include <functional>

class FunctionalMenuItemEventHandler final : public MenuItemEventHandler {
private:
	std::function<void(MenuItem&)> m_OnClick;

public:
	explicit FunctionalMenuItemEventHandler(std::function<void(MenuItem&)> onClick = nullptr);
	FunctionalMenuItemEventHandler(const FunctionalMenuItemEventHandler&) = delete;
	virtual ~FunctionalMenuItemEventHandler() override = default;

public:
	FunctionalMenuItemEventHandler& operator=(const FunctionalMenuItemEventHandler&) = delete;

public:
	virtual void OnClick(MenuItem& menuItem) override;
};

class MainWindowHandler final : public WindowEventHandler {
private:
	Window* m_Window = nullptr;

public:
	MainWindowHandler() noexcept = default;
	MainWindowHandler(const MainWindowHandler&) = delete;
	virtual ~MainWindowHandler() override = default;

public:
	virtual void OnCreate(Control& control) override;

private:
	MenuRef CreateMenu();
};