#pragma once

#include <any>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
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

class Control;
class ControlRef;

enum class MouseButton {
	Left,
};

enum class MouseWheel {
	Forward,
	Backward,
};

enum class Key {
	None,

	Enter,
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

	virtual void OnResize(Control& control);

	virtual void OnMouseDown(Control& control, int x, int y, MouseButton mouseButton);
	virtual void OnMouseMove(Control& control, int x, int y);
	virtual void OnMouseUp(Control& control, int x, int y, MouseButton mouseButton);
	virtual void OnMouseWheel(Control& control, int x, int y, MouseWheel mouseWheel);

	virtual void OnKeyDown(Control& control, Key key);
	virtual void OnKeyUp(Control& control, Key key);

	virtual void OnReceiveMessage(Control& control, std::size_t messageId, std::optional<std::any> message);
};

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
	std::size_t GetChildCount() const noexcept;
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
	bool GetEnabled() const;
	void SetEnabled(bool newEnabled);

	void Show();
	void Hide();

	void Invalidate();
	void SendMessage(std::size_t messageId, std::optional<std::any> message = std::nullopt);

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
	virtual bool PALGetEnabled() const = 0;
	virtual void PALSetEnabled(bool newEnabled) = 0;

	virtual void PALInvalidate() = 0;
	virtual void PALSendMessage(std::size_t messageId, std::optional<std::any> message) = 0;
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
	std::size_t GetItemCount() const noexcept;
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

class DropDownMenuItem;

class MenuItemEventHandler {
public:
	MenuItemEventHandler() noexcept = default;
	MenuItemEventHandler(const MenuItemEventHandler&) = delete;
	virtual ~MenuItemEventHandler() = default;

public:
	MenuItemEventHandler& operator=(const MenuItemEventHandler&) = delete;

public:
	virtual void OnClick(MenuItem& menuItem);
};

class MenuItem {
	friend class DropDownMenuItem;
	friend class Menu;

private:
	std::variant<std::monostate, Menu*, MenuItem*> m_Parent;
	std::unique_ptr<MenuItemEventHandler> m_EventHandler;

public:
	explicit MenuItem(std::unique_ptr<MenuItemEventHandler>&& eventHandler) noexcept;
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
	MenuItemEventHandler& GetEventHandler() noexcept;

protected:
	virtual void* PALGetHandle() noexcept = 0;
};

class MenuItemRef final : public UniqueRef<MenuItem> {
public:
	using UniqueRef::UniqueRef;

	explicit MenuItemRef(std::string string, std::unique_ptr<MenuItemEventHandler>&& eventHandler);

private:
	static std::unique_ptr<MenuItem> CreateMenuItem(std::string string, std::unique_ptr<MenuItemEventHandler>&& eventHandler);
	static std::unique_ptr<MenuItem> PALCreateMenuItem(std::string string, std::unique_ptr<MenuItemEventHandler>&& eventHandler);
};

class DropDownMenuItem : public virtual MenuItem {
private:
	std::vector<MenuItemRef> m_SubItems;

public:
	DropDownMenuItem() noexcept;
	DropDownMenuItem(const DropDownMenuItem&) = delete;
	virtual ~DropDownMenuItem() override = default;

public:
	DropDownMenuItem& operator=(const DropDownMenuItem&) = delete;

public:
	const MenuItem& GetSubItem(std::size_t index) const noexcept;
	MenuItem& GetSubItem(std::size_t index) noexcept;
	std::size_t GetSubItemCount() const noexcept;
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

class MenuItemSeparator : public virtual MenuItem {
public:
	MenuItemSeparator() noexcept;
	MenuItemSeparator(const MenuItemSeparator&) = delete;
	virtual ~MenuItemSeparator() override = default;

public:
	MenuItemSeparator& operator=(const MenuItemSeparator&) = delete;
};

class MenuItemSeparatorRef final : public UniqueRef<MenuItemSeparator> {
public:
	using UniqueRef::UniqueRef;

	MenuItemSeparatorRef();

private:
	static std::unique_ptr<MenuItemSeparator> PALCreateMenuItemSeparator();
};

class Graphics;

class PaintableEventHandler : public virtual EventHandler {
public:
	PaintableEventHandler() noexcept;
	PaintableEventHandler(const PaintableEventHandler&) = delete;
	virtual ~PaintableEventHandler() override = default;

public:
	PaintableEventHandler& operator=(const PaintableEventHandler&) = delete;

public:
	virtual void OnPaint(Control& control, Graphics& graphics);
};

class WindowEventHandler : public virtual PaintableEventHandler {
public:
	WindowEventHandler() noexcept;
	WindowEventHandler(const WindowEventHandler&) = delete;
	virtual ~WindowEventHandler() override = default;

public:
	WindowEventHandler& operator=(const WindowEventHandler&) = delete;

public:
	virtual void OnClose(Window& window, bool& cancel);
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
	std::pair<int, int> GetMinimumSize() const;
	void SetMinimumSize(int newMinimumWidth, int newMinimumHeight);
	void SetMinimumSize(const std::pair<int, int>& newMinimumSize);
	bool HasMenu() const noexcept;
	const Menu& GetMenu() const noexcept;
	Menu& GetMenu() noexcept;
	Menu& SetMenu(MenuRef&& menu);

	void Close();

protected:
	virtual std::pair<int, int> PALGetMinimumSize() const = 0;
	virtual void PALSetMinimumSize(int newMinimumWidth, int newMinimumHeight) = 0;
	virtual void PALSetMenu(Menu& menu) = 0;

	virtual void PALClose() = 0;
};

class WindowRef final : public UniqueRef<Window> {
public:
	using UniqueRef::UniqueRef;

	explicit WindowRef(std::unique_ptr<WindowEventHandler>&& eventHandler);

private:
	static std::unique_ptr<Window> CreateWindow(std::unique_ptr<WindowEventHandler>&& eventHandler);
	static std::unique_ptr<Window> PALCreateWindow(std::unique_ptr<WindowEventHandler>&& eventHandler);
};

int RunEventLoop();
int RunEventLoop(WindowRef& mainWindow);

int PALRunEventLoop(WindowRef* mainWindow);

class ClickableEventHandler : public virtual EventHandler {
public:
	ClickableEventHandler() noexcept;
	ClickableEventHandler(const ClickableEventHandler&) = delete;
	virtual ~ClickableEventHandler() override = default;

public:
	ClickableEventHandler& operator=(const ClickableEventHandler&) = delete;

public:
	virtual void OnClick(Control& control);
};

using ButtonEventHandler = ClickableEventHandler;

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

	explicit ButtonRef(std::unique_ptr<ButtonEventHandler>&& eventHandler);

private:
	static std::unique_ptr<Button> CreateButton(std::unique_ptr<ButtonEventHandler>&& eventHandler);
	static std::unique_ptr<Button> PALCreateButton(std::unique_ptr<ButtonEventHandler>&& eventHandler);
};

using PanelEventHandler = PaintableEventHandler;

class Panel : public virtual Control {
public:
	Panel() noexcept;
	Panel(const Panel&) = delete;
	virtual ~Panel() override = default;

public:
	Panel& operator=(const Panel&) = delete;
};

class PanelRef final : public UniqueRef<Panel> {
public:
	using UniqueRef::UniqueRef;

	explicit PanelRef(std::unique_ptr<PanelEventHandler>&& eventHandler);

private:
	static std::unique_ptr<Panel> CreatePanel(std::unique_ptr<PanelEventHandler>&& eventHandler);
	static std::unique_ptr<Panel> PALCreatePanel(std::unique_ptr<PanelEventHandler>&& eventHandler);
};

using TextBoxEventHandler = EventHandler;

class TextBox : public virtual Control {
private:
	bool m_MultiLines = false;

public:
	explicit TextBox(bool multiLines) noexcept;
	TextBox(const TextBox&) = delete;
	virtual ~TextBox() override = default;

public:
	TextBox& operator=(const TextBox&) = delete;

public:
	bool GetMultiLines() const noexcept;
};

class TextBoxRef final : public UniqueRef<TextBox> {
public:
	using UniqueRef::UniqueRef;

	explicit TextBoxRef(std::unique_ptr<TextBoxEventHandler>&& eventHandler, bool multiLines = false);

private:
	static std::unique_ptr<TextBox> CreateTextBox(std::unique_ptr<TextBoxEventHandler>&& eventHandler,
		bool multiLines);
	static std::unique_ptr<TextBox> PALCreateTextBox(std::unique_ptr<TextBoxEventHandler>&& eventHandler,
		bool multiLines);
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
	RenderingContext2D(Graphics& graphics, PenRef defaultPen, BrushRef defaultBrush);
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
	void DrawEllipse(int x, int y, int width, int height);
	void DrawEllipse(const std::pair<int, int>& location, const std::pair<int, int>& size);
	void DrawLine(int x1, int y1, int x2, int y2);
	void DrawLine(const std::pair<int, int>& from, const std::pair<int, int>& to);

	void FillRectangle(int x, int y, int width, int height);
	void FillRectangle(const std::pair<int, int>& location, const std::pair<int, int>& size);
	void FillEllipse(int x, int y, int width, int height);
	void FillEllipse(const std::pair<int, int>& location, const std::pair<int, int>& size);

protected:
	virtual void PALSetPen(Pen& newPen) = 0;
	virtual void PALSetBrush(Brush& newBrush) = 0;

	virtual void PALDrawRectangle(int x, int y, int width, int height) = 0;
	virtual void PALDrawEllipse(int x, int y, int width, int height) = 0;
	virtual void PALDrawLine(int x, int y, int width, int height) = 0;

	virtual void PALFillRectangle(int x, int y, int width, int height) = 0;
	virtual void PALFillEllipse(int x, int y, int width, int height) = 0;
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

enum class DialogResult {
	Ok,
	Yes,
	No,
	Cancel,
	Retry,
};

class Dialog {
private:
	Window* m_Owner = nullptr;
	std::string m_DialogTitle;

public:
	explicit Dialog(std::string dialogTitle) noexcept;
	Dialog(Window& owner, std::string dialogTitle) noexcept;
	Dialog(const Dialog&) = delete;
	virtual ~Dialog() = default;

public:
	Dialog& operator=(const Dialog&) = delete;

public:
	bool HasOwner() const noexcept;
	const Window& GetOwner() const noexcept;
	Window& GetOwner() noexcept;
	std::string_view GetDialogTitle() const noexcept;

public:
	virtual DialogResult Show() = 0;
};

class MessageDialog : public virtual Dialog {
public:
	enum Icon {
		None = 0,
		Information,
		Warning,
		Error,
	};

	enum Button {
		Ok = 1 << 0,
		Yes = 1 << 1,
		No = 1 << 2,
		Cancel = 1 << 3,
		Retry = 1 << 4,
		Close = 1 << 5,
	};

private:
	std::string m_Title, m_Message;
	Icon m_Icon;
	Button m_Buttons;

public:
	MessageDialog(std::string title, std::string message, Icon icon = None, Button buttons = Ok) noexcept;
	MessageDialog(const MessageDialog&) = delete;
	virtual ~MessageDialog() override = default;

public:
	MessageDialog& operator=(const MessageDialog&) = delete;

public:
	std::string_view GetTitle() const noexcept;
	std::string_view GetMessage() const noexcept;
	Icon GetIcon() const noexcept;
	Button GetButtons() const noexcept;

public:
	virtual DialogResult Show() override;

protected:
	virtual DialogResult PALShow() = 0;
};

MessageDialog::Button operator|(MessageDialog::Button lhs, MessageDialog::Button rhs) noexcept;

class MessageDialogRef final : public UniqueRef<MessageDialog> {
public:
	using UniqueRef::UniqueRef;

	MessageDialogRef(Window& owner, std::string dialogTitle, std::string title, std::string message,
		MessageDialog::Icon icon = MessageDialog::None, MessageDialog::Button buttons = MessageDialog::Ok);

private:
	std::unique_ptr<MessageDialog> PALCreateMessageDialog(Window& owner, std::string dialogTitle,
		std::string title, std::string message, MessageDialog::Icon icon, MessageDialog::Button buttons);
};

class FileDialog : public virtual Dialog {
private:
	std::vector<std::pair<std::string, std::string>> m_Filters;
	std::filesystem::path m_Path;

public:
	FileDialog() noexcept;
	FileDialog(const MessageDialog&) = delete;
	virtual ~FileDialog() override = default;

public:
	FileDialog& operator=(const FileDialog&) = delete;

public:
	const std::pair<std::string, std::string>& GetFilter(std::size_t index) const noexcept;
	std::vector<std::pair<std::string, std::string>> GetAllFilters() const;
	std::size_t GetFilterCount() const noexcept;
	void AddFilter(std::string description, std::string pattern);
	const std::filesystem::path& GetPath() const noexcept;

protected:
	void SetPath(std::filesystem::path newPath) noexcept;

public:
	virtual DialogResult Show() override;

protected:
	virtual DialogResult PALShow() = 0;
};

class OpenFileDialog : public virtual FileDialog {
private:
	bool m_FileMustExist = true;

public:
	OpenFileDialog() noexcept;
	OpenFileDialog(const OpenFileDialog&) = delete;
	virtual ~OpenFileDialog() override = default;

public:
	OpenFileDialog& operator=(const OpenFileDialog&) = delete;

public:
	bool GetFileMustExist() const noexcept;
	void SetFileMustExist(bool newFileMustExist) noexcept;
};

class OpenFileDialogRef final : public UniqueRef<OpenFileDialog> {
public:
	using UniqueRef::UniqueRef;

	OpenFileDialogRef(Window& owner, std::string dialogTitle);

private:
	std::unique_ptr<OpenFileDialog> PALCreateOpenFileDialog(Window& owner, std::string dialogTitle);
};

class SaveFileDialog : public virtual FileDialog {
private:
	bool m_OverWritePrompt = true;

public:
	SaveFileDialog() noexcept;
	SaveFileDialog(const SaveFileDialog&) = delete;
	virtual ~SaveFileDialog() override = default;

public:
	SaveFileDialog& operator=(const SaveFileDialog&) = delete;

public:
	bool GetOverWritePrompt() const noexcept;
	void SetOverWritePrompt(bool newOverWritePrompt) noexcept;
};

class SaveFileDialogRef final : public UniqueRef<SaveFileDialog> {
public:
	using UniqueRef::UniqueRef;

	SaveFileDialogRef(Window& owner, std::string dialogTitle);

private:
	std::unique_ptr<SaveFileDialog> PALCreateSaveFileDialog(Window& owner, std::string dialogTitle);
};

class WindowDialog;

class WindowDialogEventHandler {
public:
	WindowDialogEventHandler() noexcept = default;
	WindowDialogEventHandler(const WindowDialogEventHandler&) = delete;
	virtual ~WindowDialogEventHandler() = default;

public:
	WindowDialogEventHandler& operator=(const WindowDialogEventHandler&) = delete;

public:
	virtual void OnCreate(WindowDialog& dialog);
	virtual void OnDestroy(WindowDialog& dialog);

	virtual void OnResize(WindowDialog& dialog);

	virtual void OnPaint(WindowDialog& dialog, Graphics& graphics);
};

class WindowDialogWindowEventHandler;

class WindowDialog final : public virtual Dialog {
	friend class WindowDialogWindowEventHandler;

private:
	WindowRef m_Window;
	std::unique_ptr<WindowDialogEventHandler> m_EventHandler;

	bool m_IsRunning = false;
	DialogResult m_Result;

public:
	WindowDialog(Window& owner, std::string dialogTitle,
		std::unique_ptr<WindowDialogEventHandler>&& eventHandler);
	WindowDialog(const WindowDialog&) = delete;
	virtual ~WindowDialog() override = default;

public:
	WindowDialog& operator=(const WindowDialog&) = delete;

public:
	const Window& GetWindow() const noexcept;
	Window& GetWindow() noexcept;

public:
	virtual DialogResult Show() override;

private:
	void PALShow(bool& isRunning);

public:
	const Control& GetChild(std::size_t index) const noexcept;
	Control& GetChild(std::size_t index) noexcept;
	std::size_t GetChildCount() const noexcept;
	Control& AddChild(ControlRef&& child);
	void* GetHandle() noexcept;
	WindowDialogEventHandler& GetEventHandler() noexcept;

	std::pair<int, int> GetSize() const;
	void SetSize(int newWidth, int newHeight);
	void SetSize(const std::pair<int, int>& newSize);
	int GetWidth() const;
	void SetWidth(int newWidth);
	int GetHeight() const;
	void SetHeight(int newHeight);
	std::pair<int, int> GetClientSize() const;

	void Invalidate();

	std::pair<int, int> GetMinimumSize() const;
	void SetMinimumSize(int newMinimumWidth, int newMinimumHeight);
	void SetMinimumSize(const std::pair<int, int>& newMinimumSize);

	void Close(DialogResult dialogResult);
};

class WindowDialogRef final : public UniqueRef<WindowDialog> {
public:
	using UniqueRef::UniqueRef;

	WindowDialogRef(Window& owner, std::string dialogTitle,
		std::unique_ptr<WindowDialogEventHandler>&& eventHandler);
};