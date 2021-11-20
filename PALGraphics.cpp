#include "PALGraphics.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <stdexcept>

void InitializeGraphics() {
	PALInitializeGraphics();
}
void FinalizeGraphics() noexcept {
	PALFinalizeGraphics();
}

void EventHandler::OnCreate(Control&) {}
void EventHandler::OnDestroy(Control&) {}

void EventHandler::OnResize(Control&) {}

void EventHandler::OnMouseDown(Control&, int, int, MouseButton) {}
void EventHandler::OnMouseMove(Control&, int, int) {}
void EventHandler::OnMouseUp(Control&, int, int, MouseButton) {}
void EventHandler::OnMouseWheel(Control&, int, int, MouseWheel) {}

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
std::string Control::GetText() const {
	return PALGetText();
}
void Control::SetText(const std::string& newText) {
	PALSetText(newText);
}

void Control::Show() {
	SetVisibility(true);
}
void Control::Hide() {
	SetVisibility(false);
}

void Control::Invalidate() {
	PALInvalidate();
}

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
std::size_t Menu::GetItemCount() const noexcept {
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
	: UniqueRef(PALCreateMenu()) {}

void MenuItemEventHandler::OnClick(MenuItem&) {}

MenuItem::MenuItem(std::unique_ptr<MenuItemEventHandler>&& eventHandler) noexcept
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
MenuItemEventHandler& MenuItem::GetEventHandler() noexcept {
	return *m_EventHandler.get();
}

MenuItemRef::MenuItemRef(std::string string, std::unique_ptr<MenuItemEventHandler>&& eventHandler)
	: UniqueRef(CreateMenuItem(std::move(string), std::move(eventHandler))) {}

std::unique_ptr<MenuItem> MenuItemRef::CreateMenuItem(std::string string, std::unique_ptr<MenuItemEventHandler>&& eventHandler) {
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
std::size_t DropDownMenuItem::GetSubItemCount() const noexcept {
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
	: UniqueRef(PALCreateDropDownMenuItem(std::move(string))) {}

MenuItemSeparator::MenuItemSeparator() noexcept {}

MenuItemSeparatorRef::MenuItemSeparatorRef()
	: UniqueRef(PALCreateMenuItemSeparator()) {}

PaintableEventHandler::PaintableEventHandler() noexcept {}

void PaintableEventHandler::OnPaint(Control&, Graphics&) {}

WindowEventHandler::WindowEventHandler() noexcept {}

void WindowEventHandler::OnClose(Window&, bool&) {}

Window::Window() noexcept {}

std::pair<int, int> Window::GetMinimumSize() const {
	return PALGetMinimumSize();
}
void Window::SetMinimumSize(int newMinimumWidth, int newMinimumHeight) {
	assert(newMinimumWidth >= 0);
	assert(newMinimumHeight >= 0);

	PALSetMinimumSize(newMinimumWidth, newMinimumHeight);

	auto size = GetSize();

	size.first = std::max(newMinimumWidth, size.first);
	size.second = std::max(newMinimumHeight, size.second);

	SetSize(size);
}
void Window::SetMinimumSize(const std::pair<int, int>& newMinimumSize) {
	SetMinimumSize(newMinimumSize.first, newMinimumSize.second);
}
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

void Window::Close() {
	PALClose();
}

WindowRef::WindowRef(std::unique_ptr<WindowEventHandler>&& eventHandler)
	: UniqueRef(PALCreateWindow(std::move(eventHandler))) {}

std::unique_ptr<Window> WindowRef::CreateWindow(std::unique_ptr<WindowEventHandler>&& eventHandler) {
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

ClickableEventHandler::ClickableEventHandler() noexcept {}

void ClickableEventHandler::OnClick(Control&) {}

Button::Button() noexcept {}

ButtonRef::ButtonRef(std::unique_ptr<ButtonEventHandler>&& eventHandler)
	: UniqueRef(PALCreateButton(std::move(eventHandler))) {}

std::unique_ptr<Button> ButtonRef::CreateButton(std::unique_ptr<ButtonEventHandler>&& eventHandler) {
	assert(eventHandler != nullptr);

	return PALCreateButton(std::move(eventHandler));
}

Panel::Panel() noexcept {}

PanelRef::PanelRef(std::unique_ptr<PanelEventHandler>&& eventHandler)
	: UniqueRef(CreatePanel(std::move(eventHandler))) {}

std::unique_ptr<Panel> PanelRef::CreatePanel(std::unique_ptr<PanelEventHandler>&& eventHandler) {
	assert(eventHandler != nullptr);

	return PALCreatePanel(std::move(eventHandler));
}

const Color Color::Black(0, 0, 0);
const Color Color::Red(255, 0, 0);
const Color Color::Green(0, 255, 0);
const Color Color::Blue(0, 0, 255);
const Color Color::Yellow(255, 255, 0);
const Color Color::Cyan(0, 255, 255);
const Color Color::Magenta(255, 0, 255);
const Color Color::White(255, 255, 255);

Color::Color(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) noexcept
	: R(r), G(g), B(b), A(a) {}

Pen::Pen(float width) noexcept
	: m_Width(width) {
	assert(std::isfinite(width));
	assert(width > 0);
}

float Pen::GetWidth() const noexcept {
	return m_Width;
}

SolidPen::SolidPen(Color color) noexcept
	: m_Color(color) {}

SolidPen::~SolidPen() {}

Color SolidPen::GetColor() const noexcept {
	return m_Color;
}

SolidPenRef::SolidPenRef(Color color, float width)
	: SharedRef(PALCreateSolidPen(color, width)) {}

SolidBrush::SolidBrush(Color color) noexcept
	: m_Color(color) {}

Color SolidBrush::GetColor() const noexcept {
	return m_Color;
}

SolidBrushRef::SolidBrushRef(Color color)
	: SharedRef(PALCreateSolidBrush(color)) {}

RenderingContext::RenderingContext(Graphics& graphics) noexcept
	: m_Graphics(&graphics) {}

const Graphics& RenderingContext::GetGraphics() const noexcept {
	return *m_Graphics;
}
Graphics& RenderingContext::GetGraphics() noexcept {
	return *m_Graphics;
}

RenderingContext2D::RenderingContext2D(Graphics& graphics, PenRef defaultPen, BrushRef defaultBrush)
	: RenderingContext(graphics), m_Pen(std::move(defaultPen)), m_Brush(std::move(defaultBrush)) {}

const Pen& RenderingContext2D::GetPen() const noexcept {
	return m_Pen.Get();
}
PenRef RenderingContext2D::SetPen(PenRef newPen) {
	assert(!newPen.IsEmpty());

	PALSetPen(newPen.Get());
	std::swap(m_Pen, newPen);

	return newPen;
}
const Brush& RenderingContext2D::GetBrush() const noexcept {
	return m_Brush.Get();
}
BrushRef RenderingContext2D::SetBrush(BrushRef newBrush) {
	assert(!newBrush.IsEmpty());

	PALSetBrush(newBrush.Get());
	std::swap(m_Brush, newBrush);

	return newBrush;
}

void RenderingContext2D::DrawRectangle(int x, int y, int width, int height) {
	PALDrawRectangle(x, y, width, height);
}
void RenderingContext2D::DrawRectangle(const std::pair<int, int>& location, const std::pair<int, int>& size) {
	DrawRectangle(location.first, location.second, size.first, size.second);
}
void RenderingContext2D::DrawEllipse(int x, int y, int width, int height) {
	PALDrawEllipse(x, y, width, height);
}
void RenderingContext2D::DrawEllipse(const std::pair<int, int>& location, const std::pair<int, int>& size) {
	DrawEllipse(location.first, location.second, size.first, size.second);
}
void RenderingContext2D::DrawLine(int x1, int y1, int x2, int y2) {
	PALDrawLine(x1, y1, x2, y2);
}
void RenderingContext2D::DrawLine(const std::pair<int, int>& from, const std::pair<int, int>& to) {
	DrawLine(from.first, from.second, to.first, to.second);
}

void RenderingContext2D::FillRectangle(int x, int y, int width, int height) {
	PALFillRectangle(x, y, width, height);
}
void RenderingContext2D::FillRectangle(const std::pair<int, int>& location, const std::pair<int, int>& size) {
	FillRectangle(location.first, location.second, size.first, size.second);
}
void RenderingContext2D::FillEllipse(int x, int y, int width, int height) {
	PALFillEllipse(x, y, width, height);
}
void RenderingContext2D::FillEllipse(const std::pair<int, int>& location, const std::pair<int, int>& size) {
	FillEllipse(location.first, location.second, size.first, size.second);
}

Graphics::Graphics(Control& control) noexcept
	: m_Device(&control) {}

std::pair<int, int> Graphics::GetSize() const {
	return m_Device->GetClientSize();
}
int Graphics::GetWidth() const {
	return GetSize().first;
}
int Graphics::GetHeight() const {
	return GetSize().second;
}

RenderingContext2DRef Graphics::GetContext2D() {
	return PALGetContext2D(*m_Device);
}

Dialog::Dialog(std::string dialogTitle) noexcept
	: m_DialogTitle(std::move(dialogTitle)) {}
Dialog::Dialog(const Window& owner, std::string dialogTitle) noexcept
	: m_Owner(&owner), m_DialogTitle(std::move(dialogTitle)) {}

bool Dialog::HasOwner() const noexcept {
	return m_Owner != nullptr;
}
const Window& Dialog::GetOwner() const noexcept {
	return *m_Owner;
}
std::string_view Dialog::GetDialogTitle() const noexcept {
	return m_DialogTitle;
}

MessageDialog::MessageDialog(std::string title, std::string message, Icon icon, Button buttons) noexcept
	: m_Title(std::move(title)), m_Message(std::move(message)), m_Icon(icon), m_Buttons(buttons) {}

std::string_view MessageDialog::GetTitle() const noexcept {
	return m_Title;
}
std::string_view MessageDialog::GetMessage() const noexcept {
	return m_Message;
}
MessageDialog::Icon MessageDialog::GetIcon() const noexcept {
	return m_Icon;
}
MessageDialog::Button MessageDialog::GetButtons() const noexcept {
	return m_Buttons;
}

DialogResult MessageDialog::Show() {
	return PALShow();
}

MessageDialog::Button operator|(MessageDialog::Button lhs, MessageDialog::Button rhs) noexcept {
	return static_cast<MessageDialog::Button>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

MessageDialogRef::MessageDialogRef(const Window& owner, std::string dialogTitle, std::string title,
	std::string message, MessageDialog::Icon icon, MessageDialog::Button buttons)
	: UniqueRef(PALCreateMessageDialog(owner, std::move(dialogTitle), std::move(title),
		std::move(message), icon, buttons)) {}

FileDialog::FileDialog() noexcept {}

const std::pair<std::string, std::string>& FileDialog::GetFilter(std::size_t index) const noexcept {
	return m_Filters[index];
}
std::vector<std::pair<std::string, std::string>> FileDialog::GetAllFilters() const {
	return m_Filters;
}
std::size_t FileDialog::GetFilterCount() const noexcept {
	return m_Filters.size();
}
void FileDialog::AddFilter(std::string description, std::string pattern) {
	const auto duplicated = std::find_if(m_Filters.begin(), m_Filters.end(),
		[&](const auto& filter) {
			return filter.first == description;
		});
	assert(duplicated == m_Filters.end());

	m_Filters.push_back(std::make_pair(std::move(description), std::move(pattern)));
}
const std::filesystem::path& FileDialog::GetPath() const noexcept {
	return m_Path;
}

void FileDialog::SetPath(std::filesystem::path newPath) noexcept {
	m_Path = std::move(newPath);
}

DialogResult FileDialog::Show() {
	assert(!m_Filters.empty());

	return PALShow();
}

OpenFileDialog::OpenFileDialog() noexcept {}

bool OpenFileDialog::GetFileMustExist() const noexcept {
	return m_FileMustExist;
}
void OpenFileDialog::SetFileMustExist(bool newFileMustExist) noexcept {
	m_FileMustExist = newFileMustExist;
}

OpenFileDialogRef::OpenFileDialogRef(const Window& owner, std::string dialogTitle)
	: UniqueRef(PALCreateOpenFileDialog(owner, std::move(dialogTitle))) {}

SaveFileDialog::SaveFileDialog() noexcept {}

bool SaveFileDialog::GetOverWritePrompt() const noexcept {
	return m_OverWritePrompt;
}
void SaveFileDialog::SetOverWritePrompt(bool newOverWritePrompt) noexcept {
	m_OverWritePrompt = newOverWritePrompt;
}

SaveFileDialogRef::SaveFileDialogRef(const Window& owner, std::string dialogTitle)
	: UniqueRef(PALCreateSaveFileDialog(owner, std::move(dialogTitle))) {}