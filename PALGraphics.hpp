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
	void AddChild(ControlRef&& child);
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

class Window final : public ControlRef {
public:
	Window(std::unique_ptr<EventHandler>&& eventHandler);
	Window(Window&& other) noexcept = default;
	~Window() = default;

public:
	Window& operator=(Window&& other) noexcept = default;

private:
	static std::unique_ptr<Control> PALCreateWindow(std::unique_ptr<EventHandler>&& eventHandler);
};

int RunEventLoop();
int RunEventLoop(Window& mainWindow);

int PALRunEventLoop(Window* mainWindow);