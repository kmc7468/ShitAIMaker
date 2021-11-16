#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

template<typename T>
class GraphicsObjectRef {
	template<typename U>
	friend class GraphicsObjectRef;

private:
	std::unique_ptr<T> m_Object;

public:
	GraphicsObjectRef(std::unique_ptr<T>&& object) noexcept
		: m_Object(std::move(object)) {
		assert(m_Object != nullptr);
	}
	template<typename U> requires(std::is_base_of_v<T, U>)
	GraphicsObjectRef(GraphicsObjectRef<U>&& other) noexcept
		: m_Object(std::move(other.m_Object)) {}
	~GraphicsObjectRef() = default;

public:
	template<typename U> requires(std::is_base_of_v<T, U>)
	GraphicsObjectRef& operator=(GraphicsObjectRef<U>&& other) noexcept {
		m_Object = std::move(other.m_Object);

		return *this;
	}
	T* operator->() const noexcept {
		return m_Object.get();
	}
	T& operator*() const noexcept {
		return *m_Object.get();
	}

public:
	T& Get() const noexcept {
		return *m_Object.get();
	}
};

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
	explicit Control(std::unique_ptr<EventHandler>&& eventHandler) noexcept;
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

class ControlRef final : public GraphicsObjectRef<Control> {
public:
	using GraphicsObjectRef::GraphicsObjectRef;
};

class MenuItem;
class MenuItemRef;
class Window;

class Menu {
	friend class Window;

private:
	Window* m_Parent = nullptr;
	std::vector<MenuItemRef> m_Items;

public:
	Menu() noexcept = default;
	Menu(const Menu&) = delete;
	virtual ~Menu() = default;

public:
	Menu& operator=(const Menu&) = delete;

public:
	bool HasParent() const noexcept;
	const Window& GetParent() const noexcept;
	Window& GetParent() noexcept;
	const MenuItem& GetItem(std::size_t index) const noexcept;
	MenuItem& GetItem(std::size_t index) noexcept;
	std::size_t GetItemsCount() const noexcept;
	MenuItem& AddItem(MenuItemRef&& item);
	void* GetHandle() noexcept;

protected:
	virtual void PALAddItem(MenuItem& item) = 0;
	virtual void* PALGetHandle() noexcept = 0;
};

class MenuRef final : public GraphicsObjectRef<Menu> {
public:
	using GraphicsObjectRef::GraphicsObjectRef;

	MenuRef();

private:
	static std::unique_ptr<Menu> PALCreateMenu();
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

class DropDownMenuItem;

class MenuItem {
	friend class DropDownMenuItem;
	friend class Menu;

private:
	std::variant<std::monostate, Menu*, MenuItem*> m_Parent;
	std::unique_ptr<ClickableEventHandler> m_EventHandler;

public:
	explicit MenuItem(std::unique_ptr<ClickableEventHandler>&& eventHandler) noexcept;
	MenuItem(const MenuItem&) = delete;
	virtual ~MenuItem() = default;

public:
	MenuItem& operator=(const MenuItem&) = delete;

public:
	bool HasParent() const noexcept;
	bool IsRootItem() const noexcept;
	bool IsSubItem() const noexcept;
	const Menu& GetParentMenu() const noexcept;
	Menu& GetParentMenu() noexcept;
	const MenuItem& GetParentItem() const noexcept;
	MenuItem& GetParentItem() noexcept;
	void* GetHandle() noexcept;
	ClickableEventHandler& GetEventHandler() noexcept;

protected:
	virtual void* PALGetHandle() noexcept = 0;
};

class MenuItemRef final : public GraphicsObjectRef<MenuItem> {
public:
	using GraphicsObjectRef::GraphicsObjectRef;

	explicit MenuItemRef(std::string string, std::unique_ptr<ClickableEventHandler>&& eventHandler);

private:
	static std::unique_ptr<MenuItem> CreateMenuItem(std::string string, std::unique_ptr<ClickableEventHandler>&& eventHandler);
	static std::unique_ptr<MenuItem> PALCreateMenuItem(std::string string, std::unique_ptr<ClickableEventHandler>&& eventHandler);
};

class DropDownMenuItem : public virtual MenuItem {
private:
	std::vector<MenuItemRef> m_SubItems;

public:
	DropDownMenuItem() noexcept;
	DropDownMenuItem(const DropDownMenuItem&) = delete;
	virtual ~DropDownMenuItem() override = default;

public:
	MenuItem& operator=(const MenuItem&) = delete;

public:
	const MenuItem& GetSubItem(std::size_t index) const noexcept;
	MenuItem& GetSubItem(std::size_t index) noexcept;
	std::size_t GetSubItemsCount() const noexcept;
	MenuItem& AddSubItem(MenuItemRef&& subItem);

protected:
	virtual void PALAddSubItem(MenuItem& subItem) = 0;
};

class DropDownMenuItemRef final : public GraphicsObjectRef<DropDownMenuItem> {
public:
	using GraphicsObjectRef::GraphicsObjectRef;

	explicit DropDownMenuItemRef(std::string string);

private:
	static std::unique_ptr<DropDownMenuItem> PALCreateDropDownMenuItem(std::string string);
};

class Window : public virtual Control {
private:
	std::optional<MenuRef> m_Menu;

public:
	Window() noexcept;
	Window(const Window&) = delete;
	virtual ~Window() override = default;

public:
	Window& operator=(const Window&) = delete;

public:
	bool HasMenu() const noexcept;
	const Menu& GetMenu() const noexcept;
	Menu& GetMenu() noexcept;
	Menu& SetMenu(MenuRef&& menu);

protected:
	virtual void PALSetMenu(Menu& menu) = 0;
};

class WindowRef final : public GraphicsObjectRef<Window> {
public:
	using GraphicsObjectRef::GraphicsObjectRef;

	explicit WindowRef(std::unique_ptr<EventHandler>&& eventHandler);

private:
	static std::unique_ptr<Window> CreateWindow(std::unique_ptr<EventHandler>&& eventHandler);
	static std::unique_ptr<Window> PALCreateWindow(std::unique_ptr<EventHandler>&& eventHandler);
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

class ButtonRef final : public GraphicsObjectRef<Button> {
public:
	explicit ButtonRef(std::unique_ptr<ClickableEventHandler>&& eventHandler);

private:
	static std::unique_ptr<Button> CreateButton(std::unique_ptr<ClickableEventHandler>&& eventHandler);
	static std::unique_ptr<Button> PALCreateButton(std::unique_ptr<ClickableEventHandler>&& eventHandler);
};