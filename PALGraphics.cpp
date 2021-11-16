#include "PALGraphics.hpp"

#include <cassert>

void InitializeGraphics() {
	PALInitializeGraphics();
}
void FinalizeGraphics() noexcept {
	PALFinalizeGraphics();
}

Control::Control(std::unique_ptr<EventHandler>&& eventHandler) noexcept
	: m_EventHandler(std::move(eventHandler)) {
	assert(m_EventHandler != nullptr);
}

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
	m_Children.push_back(std::move(child));

	Control& childControl = *m_Children.back();

	childControl.m_Parent = this;
	PALAddChild(*m_Children.back());

	return childControl;
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

ControlRef::ControlRef(std::unique_ptr<Control>&& control) noexcept
	: m_Control(std::move(control)) {
	assert(m_Control != nullptr);
}

Control* ControlRef::operator->() const noexcept {
	return m_Control.get();
}
Control& ControlRef::operator*() const noexcept {
	return *m_Control.get();
}

Control& ControlRef::GetControl() const noexcept {
	return *m_Control.get();
}

Window::Window() noexcept {}

WindowRef::WindowRef(std::unique_ptr<EventHandler>&& eventHandler)
	: ControlRef(PALCreateWindow(std::move(eventHandler))) {}

int RunEventLoop() {
	return PALRunEventLoop(nullptr);
}
int RunEventLoop(WindowRef& mainWindow) {
	return PALRunEventLoop(&mainWindow);
}

Button::Button() noexcept {}

void ClickableEventHandler::OnClick(Control&) {}

ButtonRef::ButtonRef(std::unique_ptr<ClickableEventHandler>&& eventHandler)
	: ControlRef(PALCreateButton(std::move(eventHandler))) {}