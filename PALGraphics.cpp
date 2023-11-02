#include "PALGraphics.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <stdexcept>
#include <utility>

void InitializeGraphics() {
	PALInitializeGraphics();
}
void FinalizeGraphics() noexcept {
	PALFinalizeGraphics();
}

Font::Font(std::string fontFamily, float size, Unit sizeUnit) noexcept
	: m_FontFamily(std::move(fontFamily)), m_Size(size), m_SizeUnit(sizeUnit) {
	assert(!m_FontFamily.empty());
	assert(size > 0);
}
Font::~Font() {}

std::string_view Font::GetFontFamily() const noexcept {
	return m_FontFamily;
}
float Font::GetSize() const noexcept {
	return m_Size;
}
Font::Unit Font::GetSizeUnit() const noexcept {
	return m_SizeUnit;
}

FontRef::FontRef(std::string fontFamily, float size, Font::Unit sizeUnit)
	: SharedRef(PALCreateFont(std::move(fontFamily), size, sizeUnit)) {}

void EventHandler::OnCreate(Control&) {}
void EventHandler::OnDestroy(Control&) {}

void EventHandler::OnResize(Control&) {}

void EventHandler::OnMouseDown(Control&, int, int, MouseButton) {}
void EventHandler::OnMouseMove(Control&, int, int) {}
void EventHandler::OnMouseUp(Control&, int, int, MouseButton) {}
void EventHandler::OnMouseWheel(Control&, int, int, MouseWheel) {}

void EventHandler::OnKeyDown(Control&, Key) {}
void EventHandler::OnKeyUp(Control&, Key) {}

void EventHandler::OnReceiveMessage(Control&, std::size_t, std::optional<std::any>) {}

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
std::size_t Control::GetChildCount() const noexcept {
	return m_Children.size();
}
Control& Control::AddChild(ControlRef&& child) {
	assert(!child.IsEmpty());

	m_Children.push_back(std::move(child));

	Control& control = *m_Children.back();

	control.m_Parent = this;
	PALAddChild(control);

	if (m_Font != nullptr && control.m_Font == nullptr) {
		control.SetFont(m_Font);
	}

	return control;
}
void* Control::GetHandle() noexcept {
	return PALGetHandle();
}
EventHandler& Control::GetEventHandler() noexcept {
	return *m_EventHandler;
}

FontRef Control::GetFont() const noexcept {
	return m_Font;
}
void Control::SetFont(FontRef newFont) noexcept {
	assert(!newFont.IsEmpty());

	m_Font = std::move(newFont);

	PALSetFont(*m_Font);
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
bool Control::GetEnabled() const {
	return PALGetEnabled();
}
void Control::SetEnabled(bool newEnabled) {
	PALSetEnabled(newEnabled);
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
void Control::SendMessage(std::size_t messageId, std::optional<std::any> message) {
	PALSendMessage(messageId, std::move(message));
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

bool MenuItem::GetEnabled() const {
	return PALGetEnabled();
}
void MenuItem::SetEnabled(bool newEnabled) {
	PALSetEnabled(newEnabled);
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

TextBoxEventHandler::TextBoxEventHandler() noexcept {}

void TextBoxEventHandler::OnTextChanged(TextBox&) {}

TextBox::TextBox(bool multiLines) noexcept
	: m_MultiLines(multiLines) {}

bool TextBox::GetMultiLines() const noexcept {
	return m_MultiLines;
}

TextBoxRef::TextBoxRef(std::unique_ptr<TextBoxEventHandler>&& eventHandler, bool multiLines)
	: UniqueRef(CreateTextBox(std::move(eventHandler), multiLines)) {}

std::unique_ptr<TextBox> TextBoxRef::CreateTextBox(std::unique_ptr<TextBoxEventHandler>&& eventHandler,
	bool multiLines) {
	assert(eventHandler != nullptr);

	return PALCreateTextBox(std::move(eventHandler), multiLines);
}

ComboBoxEventHandler::ComboBoxEventHandler() noexcept {}

void ComboBoxEventHandler::OnItemSelected(ComboBox&, std::size_t) {}

ComboBox::ComboBox() noexcept {}

std::string_view ComboBox::GetItem(std::size_t index) const noexcept {
	return m_Items[index];
}
std::size_t ComboBox::GetItemCount() const noexcept {
	return m_Items.size();
}
std::size_t ComboBox::GetSelectedItemIndex() const {
	return PALGetSelectedItemIndex();
}
void ComboBox::SetSelectedItemIndex(std::size_t index) {
	assert(index < m_Items.size());

	PALSetSelectedItemIndex(index);
}
void ComboBox::AddItem(std::string newItem) {
	m_Items.push_back(std::move(newItem));

	PALAddItem(m_Items.back());
}

ComboBoxRef::ComboBoxRef(std::unique_ptr<ComboBoxEventHandler>&& eventHandler)
	: UniqueRef(CreateComboBox(std::move(eventHandler))) {}

std::unique_ptr<ComboBox> ComboBoxRef::CreateComboBox(std::unique_ptr<ComboBoxEventHandler>&& eventHandler) {
	assert(eventHandler != nullptr);

	return PALCreateComboBox(std::move(eventHandler));
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

RenderingContext2D::RenderingContext2D(Graphics& graphics, PenRef defaultPen, BrushRef defaultBrush,
	FontRef defaultFont)
	: RenderingContext(graphics), m_Pen(std::move(defaultPen)), m_Brush(std::move(defaultBrush)),
	m_Font(std::move(defaultFont)) {}

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
const Font& RenderingContext2D::GetFont() const noexcept {
	return *m_Font;
}
FontRef RenderingContext2D::SetFont(FontRef newFont) {
	assert(!newFont.IsEmpty());

	PALSetFont(newFont.Get());
	std::swap(m_Font, newFont);

	return newFont;
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
void RenderingContext2D::DrawString(const std::string& string, int x, int y) {
	if (string.empty()) return;

	PALDrawString(string, x, y);
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
Dialog::Dialog(Window& owner, std::string dialogTitle) noexcept
	: m_Owner(&owner), m_DialogTitle(std::move(dialogTitle)) {}

bool Dialog::HasOwner() const noexcept {
	return m_Owner != nullptr;
}
const Window& Dialog::GetOwner() const noexcept {
	return *m_Owner;
}
Window& Dialog::GetOwner() noexcept {
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
DialogResult MessageDialog::Show(Window& owner, std::string dialogTitle, std::string title,
	std::string message, Icon icon, Button buttons) {
	MessageDialogRef messageDialog(owner, std::move(dialogTitle), std::move(title),
		std::move(message), icon, buttons);

	return messageDialog->Show();
}

MessageDialog::Button operator|(MessageDialog::Button lhs, MessageDialog::Button rhs) noexcept {
	return static_cast<MessageDialog::Button>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

MessageDialogRef::MessageDialogRef(Window& owner, std::string dialogTitle, std::string title,
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

OpenFileDialogRef::OpenFileDialogRef(Window& owner, std::string dialogTitle)
	: UniqueRef(PALCreateOpenFileDialog(owner, std::move(dialogTitle))) {}

SaveFileDialog::SaveFileDialog() noexcept {}

bool SaveFileDialog::GetOverWritePrompt() const noexcept {
	return m_OverWritePrompt;
}
void SaveFileDialog::SetOverWritePrompt(bool newOverWritePrompt) noexcept {
	m_OverWritePrompt = newOverWritePrompt;
}

SaveFileDialogRef::SaveFileDialogRef(Window& owner, std::string dialogTitle)
	: UniqueRef(PALCreateSaveFileDialog(owner, std::move(dialogTitle))) {}

void WindowDialogEventHandler::OnCreate(WindowDialog&) {}
void WindowDialogEventHandler::OnDestroy(WindowDialog&) {}

void WindowDialogEventHandler::OnResize(WindowDialog&) {}

void WindowDialogEventHandler::OnPaint(WindowDialog&, Graphics&) {}

class WindowDialogWindowEventHandler final : public virtual WindowEventHandler {
private:
	WindowDialog& m_WindowDialog;

public:
	WindowDialogWindowEventHandler(WindowDialog& windowDialog) noexcept
		: m_WindowDialog(windowDialog) {}
	WindowDialogWindowEventHandler(const WindowDialogWindowEventHandler&) = delete;
	virtual ~WindowDialogWindowEventHandler() override = default;

public:
	WindowDialogWindowEventHandler& operator=(const WindowDialogWindowEventHandler&) = delete;

public:
	virtual void OnCreate(Control&) override {
		m_WindowDialog.GetEventHandler().OnCreate(m_WindowDialog);
	}
	virtual void OnClose(Window&, bool&) override {
		if (m_WindowDialog.m_IsRunning) {
			m_WindowDialog.m_IsRunning = false;
			m_WindowDialog.m_Result = DialogResult::Cancel;
		}

		m_WindowDialog.GetOwner().SetEnabled(true);
	}
	virtual void OnDestroy(Control&) override {
		m_WindowDialog.GetEventHandler().OnDestroy(m_WindowDialog);
	}

	virtual void OnResize(Control&) override {
		m_WindowDialog.GetEventHandler().OnResize(m_WindowDialog);
	}

	virtual void OnPaint(Control&, Graphics& graphics) override {
		m_WindowDialog.GetEventHandler().OnPaint(m_WindowDialog, graphics);
	}
};

WindowDialog::WindowDialog(Window& owner, std::string dialogTitle,
	std::unique_ptr<WindowDialogEventHandler>&& eventHandler)
	: Dialog(owner, std::move(dialogTitle)), m_Window(std::make_unique<WindowDialogWindowEventHandler>(*this)),
	m_EventHandler(std::move(eventHandler)) {
	assert(m_EventHandler != nullptr);

	m_Window->SetText(std::string(GetDialogTitle()));
}

const Window& WindowDialog::GetWindow() const noexcept {
	return m_Window.Get();
}
Window& WindowDialog::GetWindow() noexcept {
	return m_Window.Get();
}

DialogResult WindowDialog::Show() {
	m_IsRunning = true;

	m_Window->Show();
	GetOwner().SetEnabled(false);

	PALShow(m_IsRunning);

	return m_Result;
}

const Control& WindowDialog::GetChild(std::size_t index) const noexcept {
	return m_Window->GetChild(index);
}
Control& WindowDialog::GetChild(std::size_t index) noexcept {
	return m_Window->GetChild(index);
}
std::size_t WindowDialog::GetChildCount() const noexcept {
	return m_Window->GetChildCount();
}
Control& WindowDialog::AddChild(ControlRef&& child) {
	return m_Window->AddChild(std::move(child));
}
void* WindowDialog::GetHandle() noexcept {
	return m_Window->GetHandle();
}

FontRef WindowDialog::GetFont() const noexcept {
	return m_Window->GetFont();
}
void WindowDialog::SetFont(FontRef newFont) noexcept {
	m_Window->SetFont(newFont);
}

std::pair<int, int> WindowDialog::GetSize() const {
	return m_Window->GetSize();
}
void WindowDialog::SetSize(int newWidth, int newHeight) {
	m_Window->SetSize(newWidth, newHeight);
}
void WindowDialog::SetSize(const std::pair<int, int>& newSize) {
	m_Window->SetSize(newSize);
}
int WindowDialog::GetWidth() const {
	return m_Window->GetWidth();
}
void WindowDialog::SetWidth(int newWidth) {
	m_Window->SetWidth(newWidth);
}
int WindowDialog::GetHeight() const {
	return m_Window->GetHeight();
}
void WindowDialog::SetHeight(int newHeight) {
	m_Window->SetHeight(newHeight);
}
std::pair<int, int> WindowDialog::GetClientSize() const {
	return m_Window->GetClientSize();
}
WindowDialogEventHandler& WindowDialog::GetEventHandler() noexcept {
	return *m_EventHandler;
}

void WindowDialog::Invalidate() {
	m_Window->Invalidate();
}

void WindowDialog::Close(DialogResult dialogResult) {
	m_IsRunning = false;
	m_Result = dialogResult;

	m_Window->Close();
}

std::pair<int, int> WindowDialog::GetMinimumSize() const {
	return m_Window->GetMinimumSize();
}
void WindowDialog::SetMinimumSize(int newMinimumWidth, int newMinimumHeight) {
	m_Window->SetMinimumSize(newMinimumWidth, newMinimumHeight);
}
void WindowDialog::SetMinimumSize(const std::pair<int, int>& newMinimumSize) {
	m_Window->SetMinimumSize(newMinimumSize);
}

WindowDialogRef::WindowDialogRef(Window& owner, std::string dialogTitle,
	std::unique_ptr<WindowDialogEventHandler>&& eventHandler)
	: UniqueRef(std::make_unique<WindowDialog>(owner, std::move(dialogTitle), std::move(eventHandler))) {}