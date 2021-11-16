#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

void InitializeGraphics();
void FinalizeGraphics() noexcept;

void PALInitializeGraphics();
void PALFinalizeGraphics() noexcept;

class ControlRef;
class EventHandler;

class Control {
private:
	Control* m_Parent = nullptr;
	std::vector<ControlRef> m_Children;
	std::unique_ptr<EventHandler> m_EventHandler;

public:
	Control(std::unique_ptr<EventHandler>&& eventHandler) noexcept;
	Control(const Control&) = delete;
	virtual ~Control() = default;

public:
	Control& operator=(const Control&) = delete;

public:
	bool HasParent() const noexcept;
	const Control& GetParent() const noexcept;
	Control& GetParent() noexcept;
	const Control& GetChild(std::size_t index) const noexcept;
	Control& GetChild(std::size_t index) noexcept;
	std::size_t GetChildrenCount() const noexcept;
	Control& AddChild(ControlRef&& child);
	void* GetHandle() noexcept;
	EventHandler& GetEventHandler() noexcept;

	std::pair<int, int> GetSize() const;
	void SetSize(int newWidth, int newHeight);
	void SetSize(const std::pair<int, int>& newSize);
	int GetWidth() const;
	void SetWidth(int newWidth);
	int GetHeight() const;
	void SetHeight(int newHeight);
	std::pair<int, int> GetClientSize() const;
	std::pair<int, int> GetLocation() const;
	void SetLocation(int newX, int newY);
	void SetLocation(const std::pair<int, int>& newLocation);
	int GetX() const;
	void SetX(int newX);
	int GetY() const;
	void SetY(int newY);
	bool GetVisibility() const;
	void SetVisibility(bool newVisibility);

	void Show();
	void Hide();

protected:
	virtual void PALAddChild(Control& child) = 0;
	virtual void* PALGetHandle() noexcept = 0;

	virtual std::pair<int, int> PALGetSize() const = 0;
	virtual void PALSetSize(int newWidth, int newHeight) = 0;
	virtual std::pair<int, int> PALGetClientSize() const = 0;
	virtual std::pair<int, int> PALGetLocation() const = 0;
	virtual void PALSetLocation(int newX, int newY) = 0;
	virtual bool PALGetVisibility() const = 0;
	virtual void PALSetVisibility(bool newVisibility) = 0;
};

class EventHandler {
public:
	EventHandler() noexcept = default;
	EventHandler(const EventHandler&) = delete;
	virtual ~EventHandler() = default;

public:
	EventHandler& operator=(const EventHandler&) = delete;

public:
	virtual void OnCreate(Control& control);
	virtual void OnDestroy(Control& control);
};

class ControlRef {
private:
	std::unique_ptr<Control> m_Control;

public:
	ControlRef(std::unique_ptr<Control>&& control) noexcept;
	ControlRef(ControlRef&& other) noexcept = default;
	~ControlRef() = default;

public:
	ControlRef& operator=(ControlRef&& other) noexcept = default;
	Control* operator->() const noexcept;
	Control& operator*() const noexcept;

public:
	Control& GetControl() const noexcept;
};

class Window : public virtual Control {
public:
	Window() noexcept;
	Window(const Window&) = delete;
	virtual ~Window() override = default;

public:
	Window& operator=(const Window&) = delete;
};

class WindowRef final : public ControlRef {
public:
	WindowRef(std::unique_ptr<EventHandler>&& eventHandler);
	WindowRef(WindowRef&& other) noexcept = default;
	~WindowRef() = default;

public:
	WindowRef& operator=(WindowRef&& other) noexcept = default;

private:
	static std::unique_ptr<Control> PALCreateWindow(std::unique_ptr<EventHandler>&& eventHandler);
};

int RunEventLoop();
int RunEventLoop(WindowRef& mainWindow);

int PALRunEventLoop(WindowRef* mainWindow);

class Button : public virtual Control {
public:
	Button() noexcept;
	Button(const Button&) = delete;
	virtual ~Button() override = default;

public:
	Button& operator=(const Button&) = delete;
};

class ClickableEventHandler : public virtual EventHandler {
public:
	ClickableEventHandler() noexcept = default;
	ClickableEventHandler(const ClickableEventHandler&) = delete;
	virtual ~ClickableEventHandler() override = default;

public:
	ClickableEventHandler& operator=(const ClickableEventHandler&) = delete;

public:
	virtual void OnClick(Control& control);
};

class ButtonRef final : public ControlRef {
public:
	ButtonRef(std::unique_ptr<ClickableEventHandler>&& eventHandler);
	ButtonRef(ButtonRef&& other) noexcept = default;
	~ButtonRef() = default;

public:
	ButtonRef& operator=(ButtonRef&& other) noexcept = default;

private:
	static std::unique_ptr<Control> PALCreateButton(std::unique_ptr<ClickableEventHandler>&& eventHandler);
};