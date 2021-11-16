#include "PALGraphics.hpp"

#include <cassert>

void InitializeGraphics() {
	PALInitializeGraphics();
}
void FinalizeGraphics() noexcept {
	PALFinalizeGraphics();
}

Control::Control(std::unique_ptr<EventHandler>&& eventHandler) noexcept
	: m_EventHandler(std::move(eventHandler)) {}

bool Control::HasParent() const noexcept {
	return m_Parent != nullptr;
}
const Control& Control::GetParent() const noexcept {
	return *m_Parent;
}
Control& Control::GetParent() noexcept {
	return *m_Parent;
}
const Control& Control::GetChild(std::size_t index) const noexcept {
	return *m_Children[index];
}
Control& Control::GetChild(std::size_t index) noexcept {
	return *m_Children[index];
}
std::size_t Control::GetChildrenCount() const noexcept {
	return m_Children.size();
}
Control& Control::AddChild(ControlRef&& child) {
	assert(!child.IsEmpty());

	m_Children.push_back(std::move(child));

	Control& control = *m_Children.back();

	control.m_Parent = this;
	PALAddChild(control);

	return control;
}
EventHandler& Control::GetEventHandler() noexcept {
	return *m_EventHandler;
}
void* Control::GetHandle() noexcept {
	return PALGetHandle();
}

std::pair<int, int> Control::GetSize() const {
	return PALGetSize();
}
void Control::SetSize(int newWidth, int newHeight) {
	assert(newWidth >= 0);
	assert(newHeight >= 0);

	PALSetSize(newWidth, newHeight);
}
void Control::SetSize(const std::pair<int, int>& newSize) {
	SetSize(newSize.first, newSize.second);
}
int Control::GetWidth() const {
	return GetSize().first;
}
void Control::SetWidth(int newWidth) {
	SetSize(newWidth, GetHeight());
}
int Control::GetHeight() const {
	return GetSize().second;
}
void Control::SetHeight(int newHeight) {
	SetSize(GetWidth(), newHeight);
}
std::pair<int, int> Control::GetClientSize() const {
	return PALGetClientSize();
}
std::pair<int, int> Control::GetLocation() const {
	return PALGetLocation();
}
void Control::SetLocation(int newX, int newY) {
	PALSetLocation(newX, newY);
}
void Control::SetLocation(const std::pair<int, int>& newLocation) {
	SetLocation(newLocation.first, newLocation.second);
}
int Control::GetX() const {
	return GetLocation().first;
}
void Control::SetX(int newX) {
	SetLocation(newX, GetY());
}
int Control::GetY() const {
	return GetLocation().second;
}
void Control::SetY(int newY) {
	SetLocation(GetX(), newY);
}
bool Control::GetVisibility() const {
	return PALGetVisibility();
}
void Control::SetVisibility(bool newVisibility) {
	PALSetVisibility(newVisibility);
}

void Control::Show() {
	SetVisibility(true);
}
void Control::Hide() {
	SetVisibility(false);
}

void EventHandler::OnCreate(Control&) {}
void EventHandler::OnDestroy(Control&) {}

bool Menu::HasParent() const noexcept {
	return m_Parent != nullptr;
}
const Window& Menu::GetParent() const noexcept {
	return *m_Parent;
}
Window& Menu::GetParent() noexcept {
	return *m_Parent;
}
const MenuItem& Menu::GetItem(std::size_t index) const noexcept {
	return m_Items[index].Get();
}
MenuItem& Menu::GetItem(std::size_t index) noexcept {
	return m_Items[index].Get();
}
std::size_t Menu::GetItemsCount() const noexcept {
	return m_Items.size();
}
MenuItem& Menu::AddItem(MenuItemRef&& item) {
	assert(!item.IsEmpty());

	m_Items.push_back(std::move(item));

	MenuItem& menuItem = *m_Items.back();

	menuItem.m_Parent = this;
	PALAddItem(menuItem);

	return menuItem;
}
void* Menu::GetHandle() noexcept {
	return PALGetHandle();
}

MenuRef::MenuRef()
	: GraphicsObjectRef(PALCreateMenu()) {}

void ClickableEventHandler::OnClick(Control&) {}

MenuItem::MenuItem(std::unique_ptr<ClickableEventHandler>&& eventHandler) noexcept
	: m_EventHandler(std::move(eventHandler)) {
	assert(m_EventHandler != nullptr);
}

bool MenuItem::HasParent() const noexcept {
	return m_Parent.index() != 0;
}
bool MenuItem::IsRootItem() const noexcept {
	return m_Parent.index() == 1;
}
bool MenuItem::IsSubItem() const noexcept {
	return m_Parent.index() == 2;
}
const Menu& MenuItem::GetParentMenu() const noexcept {
	return *std::get<1>(m_Parent);
}
Menu& MenuItem::GetParentMenu() noexcept {
	return *std::get<1>(m_Parent);
}
const MenuItem& MenuItem::GetParentItem() const noexcept {
	return *std::get<2>(m_Parent);
}
MenuItem& MenuItem::GetParentItem() noexcept {
	return *std::get<2>(m_Parent);
}
void* MenuItem::GetHandle() noexcept {
	return PALGetHandle();
}
ClickableEventHandler& MenuItem::GetEventHandler() noexcept {
	return *m_EventHandler.get();
}

MenuItemRef::MenuItemRef(std::string string, std::unique_ptr<ClickableEventHandler>&& eventHandler)
	: GraphicsObjectRef(CreateMenuItem(std::move(string), std::move(eventHandler))) {}

std::unique_ptr<MenuItem> MenuItemRef::CreateMenuItem(std::string string, std::unique_ptr<ClickableEventHandler>&& eventHandler) {
	assert(eventHandler != nullptr);

	return PALCreateMenuItem(std::move(string), std::move(eventHandler));
}

DropDownMenuItem::DropDownMenuItem() noexcept {}

const MenuItem& DropDownMenuItem::GetSubItem(std::size_t index) const noexcept {
	return m_SubItems[index].Get();
}
MenuItem& DropDownMenuItem::GetSubItem(std::size_t index) noexcept {
	return m_SubItems[index].Get();
}
std::size_t DropDownMenuItem::GetSubItemsCount() const noexcept {
	return m_SubItems.size();
}
MenuItem& DropDownMenuItem::AddSubItem(MenuItemRef&& subItem) {
	assert(!subItem.IsEmpty());

	m_SubItems.push_back(std::move(subItem));

	MenuItem& menuItem = *m_SubItems.back();

	menuItem.m_Parent = this;
	PALAddSubItem(menuItem);

	return menuItem;
}

DropDownMenuItemRef::DropDownMenuItemRef(std::string string)
	: GraphicsObjectRef(PALCreateDropDownMenuItem(std::move(string))) {}

Window::Window() noexcept {}

bool Window::HasMenu() const noexcept {
	return m_Menu.has_value();
}
const Menu& Window::GetMenu() const noexcept {
	return m_Menu->Get();
}
Menu& Window::GetMenu() noexcept {
	return m_Menu->Get();
}
Menu& Window::SetMenu(MenuRef&& menu) {
	assert(!menu.IsEmpty());

	Menu& movedMenu = m_Menu.emplace(std::move(menu)).Get();

	movedMenu.m_Parent = this;
	PALSetMenu(movedMenu);

	return movedMenu;
}

WindowRef::WindowRef(std::unique_ptr<EventHandler>&& eventHandler)
	: GraphicsObjectRef(PALCreateWindow(std::move(eventHandler))) {}

std::unique_ptr<Window> WindowRef::CreateWindow(std::unique_ptr<EventHandler>&& eventHandler) {
	assert(eventHandler != nullptr);

	return PALCreateWindow(std::move(eventHandler));
}

int RunEventLoop() {
	return PALRunEventLoop(nullptr);
}
int RunEventLoop(WindowRef& mainWindow) {
	assert(!mainWindow.IsEmpty());

	return PALRunEventLoop(&mainWindow);
}

Button::Button() noexcept {}

ButtonRef::ButtonRef(std::unique_ptr<ClickableEventHandler>&& eventHandler)
	: GraphicsObjectRef(PALCreateButton(std::move(eventHandler))) {}

std::unique_ptr<Button> ButtonRef::CreateButton(std::unique_ptr<ClickableEventHandler>&& eventHandler) {
	assert(eventHandler != nullptr);

	return PALCreateButton(std::move(eventHandler));
}