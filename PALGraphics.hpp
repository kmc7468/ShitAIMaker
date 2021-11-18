#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

template<typename T>
class UniqueRef {
	template<typename U>
	friend class UniqueRef;

private:
	std::unique_ptr<T> m_Object;

public:
	UniqueRef(std::unique_ptr<T>&& object) noexcept
		: m_Object(std::move(object)) {}
	template<typename U> requires(std::is_base_of_v<T, U>)
	UniqueRef(UniqueRef<U>&& other) noexcept
		: m_Object(std::move(other.m_Object)) {}
	~UniqueRef() = default;

public:
	template<typename U> requires(std::is_base_of_v<T, U>)
	UniqueRef& operator=(UniqueRef<U>&& other) noexcept {
		m_Object = std::move(other.m_Object);

		return *this;
	}
	bool operator==(std::nullptr_t) const noexcept {
		return m_Object == nullptr;
	}
	T* operator->() const noexcept {
		return m_Object.get();
	}
	T& operator*() const noexcept {
		return *m_Object.get();
	}
	explicit operator bool() const noexcept {
		return m_Object != nullptr;
	}

public:
	bool IsEmpty() const noexcept {
		return m_Object == nullptr;
	}
	T& Get() const noexcept {
		return *m_Object.get();
	}
};

template<typename T>
class SharedRef {
	template<typename U>
	friend class SharedRef;

private:
	std::shared_ptr<T> m_Object;

public:
	SharedRef(std::shared_ptr<T> object) noexcept
		: m_Object(std::move(object)) {}
	template<typename U> requires(std::is_base_of_v<T, U>)
	SharedRef(const SharedRef<U>& other) noexcept
		: m_Object(other.m_Object) {}
	template<typename U> requires(std::is_base_of_v<T, U>)
	SharedRef(SharedRef<U>&& other) noexcept
		: m_Object(std::move(other.m_Object)) {}
	~SharedRef() = default;

public:
	template<typename U> requires(std::is_base_of_v<T, U>)
	SharedRef& operator=(const SharedRef<U>& other) noexcept {
		m_Object = other.m_Object;

		return *this;
	}
	template<typename U> requires(std::is_base_of_v<T, U>)
	SharedRef& operator=(SharedRef<U>&& other) noexcept {
		m_Object = std::move(other.m_Object);

		return *this;
	}
	bool operator==(std::nullptr_t) const noexcept {
		return m_Object == nullptr;
	}
	T* operator->() const noexcept {
		return m_Object.get();
	}
	T& operator*() const noexcept {
		return *m_Object.get();
	}
	explicit operator bool() const noexcept {
		return m_Object != nullptr;
	}

public:
	bool IsEmpty() const noexcept {
		return m_Object == nullptr;
	}
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
	std::string GetText() const;
	void SetText(const std::string& newText);

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
	virtual std::string PALGetText() const = 0;
	virtual void PALSetText(const std::string& newText) = 0;
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

class ControlRef final : public UniqueRef<Control> {
public:
	using UniqueRef::UniqueRef;
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

class MenuRef final : public UniqueRef<Menu> {
public:
	using UniqueRef::UniqueRef;

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

class MenuItemRef final : public UniqueRef<MenuItem> {
public:
	using UniqueRef::UniqueRef;

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

class DropDownMenuItemRef final : public UniqueRef<DropDownMenuItem> {
public:
	using UniqueRef::UniqueRef;

	explicit DropDownMenuItemRef(std::string string);

private:
	static std::unique_ptr<DropDownMenuItem> PALCreateDropDownMenuItem(std::string string);
};

class Graphics;

class PaintableEventHandler : public virtual EventHandler {
public:
	PaintableEventHandler() noexcept = default;
	PaintableEventHandler(const ClickableEventHandler&) = delete;
	virtual ~PaintableEventHandler() override = default;

public:
	PaintableEventHandler& operator=(const PaintableEventHandler&) = delete;

public:
	virtual void OnPaint(Control& control, Graphics& graphics);
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

class WindowRef final : public UniqueRef<Window> {
public:
	using UniqueRef::UniqueRef;

	explicit WindowRef(std::unique_ptr<PaintableEventHandler>&& eventHandler);

private:
	static std::unique_ptr<Window> CreateWindow(std::unique_ptr<PaintableEventHandler>&& eventHandler);
	static std::unique_ptr<Window> PALCreateWindow(std::unique_ptr<PaintableEventHandler>&& eventHandler);
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

class ButtonRef final : public UniqueRef<Button> {
public:
	using UniqueRef::UniqueRef;

	explicit ButtonRef(std::unique_ptr<ClickableEventHandler>&& eventHandler);

private:
	static std::unique_ptr<Button> CreateButton(std::unique_ptr<ClickableEventHandler>&& eventHandler);
	static std::unique_ptr<Button> PALCreateButton(std::unique_ptr<ClickableEventHandler>&& eventHandler);
};

class Color final {
public:
	static const Color Black;
	static const Color Red;
	static const Color Green;
	static const Color Blue;
	static const Color Yellow;
	static const Color Cyan;
	static const Color Magenta;
	static const Color White;

public:
	std::uint8_t R = 0, G = 0, B = 0, A = 255;

public:
	Color() noexcept = default;
	Color(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) noexcept;
	Color(const Color& other) noexcept = default;
	~Color() = default;

public:
	Color& operator=(const Color& other) noexcept = default;
};

class Pen {
private:
	float m_Width;

public:
	explicit Pen(float width) noexcept;
	Pen(const Pen&) = delete;
	virtual ~Pen() = default;

public:
	Pen& operator=(const Pen&) = delete;

public:
	float GetWidth() const noexcept;
};

class PenRef final : public SharedRef<Pen> {
public:
	using SharedRef::SharedRef;
};

class SolidPen : public virtual Pen {
private:
	Color m_Color;

public:
	SolidPen(Color color) noexcept;
	SolidPen(const SolidPen&) = delete;
	virtual ~SolidPen() override = 0;

public:
	SolidPen& operator=(const SolidPen&) = delete;

public:
	Color GetColor() const noexcept;
};

class SolidPenRef final : public SharedRef<SolidPen> {
public:
	using SharedRef::SharedRef;

	SolidPenRef(Color color, float width);

private:
	static std::shared_ptr<SolidPen> PALCreateSolidPen(Color color, float width);
};

class Brush {
public:
	Brush() noexcept = default;
	Brush(const Brush&) = delete;
	virtual ~Brush() = default;

public:
	Brush& operator=(const Brush&) = delete;
};

class BrushRef final : public SharedRef<Brush> {
public:
	using SharedRef::SharedRef;
};

class SolidBrush : public virtual Brush {
private:
	Color m_Color;

public:
	SolidBrush(Color color) noexcept;
	SolidBrush(const SolidBrush&) = delete;
	virtual ~SolidBrush() override = default;

public:
	SolidBrush& operator=(const SolidBrush&) = delete;

public:
	Color GetColor() const noexcept;
};

class SolidBrushRef final : public SharedRef<SolidBrush> {
public:
	using SharedRef::SharedRef;

	explicit SolidBrushRef(Color color);

private:
	static std::shared_ptr<SolidBrush> PALCreateSolidBrush(Color color);
};

class Graphics;

class RenderingContext {
private:
	Graphics* m_Graphics = nullptr;

public:
	explicit RenderingContext(Graphics& graphics) noexcept;
	RenderingContext(const RenderingContext&) = delete;
	virtual ~RenderingContext() = default;

public:
	RenderingContext& operator=(const RenderingContext&) = delete;

public:
	const Graphics& GetGraphics() const noexcept;
	Graphics& GetGraphics() noexcept;
};

class RenderingContext2D : public RenderingContext {
private:
	PenRef m_Pen;
	BrushRef m_Brush;

public:
	explicit RenderingContext2D(Graphics& graphics);
	RenderingContext2D(const RenderingContext2D&) = delete;
	virtual ~RenderingContext2D() override = default;

public:
	RenderingContext2D& operator=(const RenderingContext2D&) = delete;

public:
	const Pen& GetPen() const noexcept;
	PenRef SetPen(PenRef newPen);
	const Brush& GetBrush() const noexcept;
	BrushRef SetBrush(BrushRef newBrush);

	void DrawRectangle(int x, int y, int width, int height);
	void DrawRectangle(const std::pair<int, int>& location, const std::pair<int, int>& size);

	void FillRectangle(int x, int y, int width, int height);
	void FillRectangle(const std::pair<int, int>& location, const std::pair<int, int>& size);

protected:
	virtual void PALSetPen(Pen& newPen) = 0;
	virtual void PALSetBrush(Brush& newBrush) = 0;

	virtual void PALDrawRectangle(int x, int y, int width, int height) = 0;

	virtual void PALFillRectangle(int x, int y, int width, int height) = 0;
};

class RenderingContext2DRef final : public UniqueRef<RenderingContext2D> {
public:
	using UniqueRef::UniqueRef;
};

class Graphics {
private:
	Control* m_Device;

public:
	explicit Graphics(Control& control) noexcept;
	Graphics(const Graphics&) = delete;
	virtual ~Graphics() = default;

public:
	Graphics& operator=(const Graphics&) = delete;

public:
	std::pair<int, int> GetSize() const;
	int GetWidth() const;
	int GetHeight() const;

	RenderingContext2DRef GetContext2D();

protected:
	virtual RenderingContext2DRef PALGetContext2D(Control& control) = 0;
};

class GraphicsRef final : public UniqueRef<Graphics> {
public:
	using UniqueRef::UniqueRef;
};