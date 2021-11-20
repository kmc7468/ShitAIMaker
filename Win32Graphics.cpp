#ifdef _WIN32
#include "PALGraphics.hpp"

#include "Win32String.hpp"

#include <cassert>
#include <exception>
#include <stdexcept>
#include <Windows.h>
#include <windowsx.h>

#include <CommCtrl.h>
#include <gdiplus.h>

#define WM_REFLECT WM_USER + 0x1C00
#define WM_RECEIVEMESSAGE WM_USER + 0x2000

#ifdef _MSC_VER
#	pragma warning(disable: 4250)
#endif

namespace {
	HINSTANCE g_Instance;

	void RegisterWindowClass();
}

namespace {
	ULONG_PTR g_GdiplusToken;

	PenRef g_DefaultPen(nullptr);
	BrushRef g_DefaultBrush(nullptr);

	void InitializeGdiplus() {
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;

		if (Gdiplus::GdiplusStartup(&g_GdiplusToken, &gdiplusStartupInput, nullptr) != Gdiplus::Ok)
			throw std::runtime_error("Failed to start up GDI+");

		g_DefaultPen = SolidPenRef(Color::Black, 1);
		g_DefaultBrush = SolidBrushRef(Color::Black);
	}
	void FinalizeGdiplus() {
		g_DefaultPen = std::shared_ptr<Pen>();
		g_DefaultBrush = std::shared_ptr<Brush>();

		Gdiplus::GdiplusShutdown(g_GdiplusToken);
	}
}

void PALInitializeGraphics() {
	g_Instance = GetModuleHandle(nullptr);

	RegisterWindowClass();
	InitializeGdiplus();
}
void PALFinalizeGraphics() noexcept {
	FinalizeGdiplus();
}

class Win32Control;

struct CreateParams {
	std::string ClassName;
	std::string WindowName;
	DWORD Style = 0;
	std::pair<int, int> Location = { CW_USEDEFAULT, CW_USEDEFAULT };
	std::pair<int, int> Size = { 100, 50 };
	HMENU Menu = nullptr;
	void* Param = nullptr;
};

class Win32Control : public virtual Control {
public:
	HWND Handle = nullptr;
	CreateParams CreateParams;

private:
	WNDPROC m_OldCallback = nullptr;

public:
	Win32Control(std::string className, DWORD style) noexcept {
		assert(!className.empty());

		CreateParams.ClassName = std::move(className);
		CreateParams.Style = style;
	}
	Win32Control(const Win32Control&) = delete;
	virtual ~Win32Control() override = 0;

public:
	Win32Control& operator=(const Win32Control&) = delete;

protected:
	virtual void PALAddChild(Control& child) {
		Win32Control& childControl = dynamic_cast<Win32Control&>(child);
		assert(childControl.CreateParams.Style & WS_CHILD);

		childControl.CreateParams.Menu = reinterpret_cast<HMENU>(GetChildCount() - 1);

		if (Handle) {
			childControl.CreateHandle();
		}
	}
	virtual void* PALGetHandle() noexcept override {
		return Handle;
	}

	virtual std::pair<int, int> PALGetSize() const override {
		if (Handle) {
			RECT rect;
			const BOOL result = GetWindowRect(Handle, &rect);
			if (!result) throw std::runtime_error("Failed to get the size of a control");

			return { static_cast<int>(rect.right - rect.left), static_cast<int>(rect.bottom - rect.top) };
		} else return CreateParams.Size;
	}
	virtual void PALSetSize(int newWidth, int newHeight) override {
		if (Handle) {
			const BOOL result = SetWindowPos(Handle, nullptr, 0, 0, newWidth, newHeight, SWP_NOZORDER | SWP_NOMOVE);
			if (!result) throw std::runtime_error("Failed to set the size of a control");
		} else {
			CreateParams.Size = { newWidth, newHeight };
		}
	}
	virtual std::pair<int, int> PALGetClientSize() const override {
		assert(Handle != nullptr);

		RECT clientRect;
		const BOOL result = GetClientRect(Handle, &clientRect);
		if (!result) throw std::runtime_error("Failed to get the client size of a control");

		return { static_cast<int>(clientRect.right), static_cast<int>(clientRect.bottom) };
	}
	virtual std::pair<int, int> PALGetLocation() const override {
		if (Handle) {
			RECT rect;
			const BOOL result = GetWindowRect(Handle, &rect);
			if (!result) throw std::runtime_error("Failed to get the location of a control");

			return { static_cast<int>(rect.left), static_cast<int>(rect.top) };
		} else return CreateParams.Location;
	}
	virtual void PALSetLocation(int newX, int newY) override {
		if (Handle) {
			const BOOL result = SetWindowPos(Handle, nullptr, newX, newY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
			if (!result) throw std::runtime_error("Failed to set the location of a control");
		} else {
			CreateParams.Location = { newX, newY };
		}
	}
	virtual bool PALGetVisibility() const override {
		if (Handle) return IsWindowVisible(Handle);
		else return CreateParams.Style & WS_VISIBLE;
	}
	virtual void PALSetVisibility(bool newVisibility) override {
		if (Handle) {
			ShowWindow(Handle, newVisibility ? SW_SHOW : SW_HIDE);
		} else {
			if (newVisibility) {
				CreateParams.Style |= WS_VISIBLE;

				if (!(CreateParams.Style & WS_CHILD)) {
					CreateHandle();
				}
			} else {
				CreateParams.Style &= ~WS_VISIBLE;
			}
		}
	}
	virtual std::string PALGetText() const override {
		if (Handle) {
			SetLastError(0);

			const int length = GetWindowTextLengthA(Handle);

			if (length == 0) {
				if (GetLastError() == 0) return {};
				else throw std::runtime_error("Failed to get the text of a control");
			}

			std::string text(length + 1, 0);

			const int readLength = GetWindowTextA(Handle, text.data(), length + 1);
			if (readLength == 0) throw std::runtime_error("Failed to get the text of a control");

			const std::size_t nullBegin = text.find_first_of('\0', 0);
			if (nullBegin != std::string::npos) {
				text.erase(text.begin() + nullBegin, text.end());
			}

			return text;
		} else return CreateParams.WindowName;
	}
	virtual void PALSetText(const std::string& newText) override {
		if (Handle) {
			const BOOL result = SetWindowTextA(Handle, newText.data());
			if (!result) throw std::runtime_error("Failed to set the text of a control");
		} else {
			CreateParams.WindowName = newText;
		}
	}
	virtual bool PALGetEnabled() const override {
		assert(Handle != nullptr);

		return IsWindowEnabled(Handle);
	}
	virtual void PALSetEnabled(bool newEnabled) override {
		assert(Handle != nullptr);

		EnableWindow(Handle, newEnabled ? TRUE : FALSE);
	}

	virtual void PALInvalidate() override {
		assert(Handle != nullptr);

		InvalidateRect(Handle, nullptr, TRUE);
		UpdateWindow(Handle);
	}
	virtual void PALSendMessage(std::size_t messageId, std::optional<std::any> message) override {
		assert(Handle != nullptr);

		LPARAM lParam = 0;

		if (message) {
			std::any* heap = new std::any(std::move(*message));

			lParam = reinterpret_cast<LPARAM>(heap);
		}

		SendMessage(Handle, WM_RECEIVEMESSAGE, static_cast<WPARAM>(messageId), lParam);
	}

private:
	void CreateHandle() {
		const std::size_t childrenCount = GetChildCount();

		Handle = CreateWindowA(CreateParams.ClassName.data(), CreateParams.WindowName.data(),
			CreateParams.Style, CreateParams.Location.first, CreateParams.Location.second,
			CreateParams.Size.first, CreateParams.Size.second, HasParent() ? GetParentHandle() : nullptr,
			CreateParams.Menu, g_Instance, CreateParams.Param);
		if (Handle == nullptr) throw std::runtime_error("Failed to create the handle of a control");

		SetWindowLongPtr(Handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		m_OldCallback = reinterpret_cast<WNDPROC>(
			SetWindowLongPtr(Handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));

		for (std::size_t i = 0; i < childrenCount; ++i) {
			dynamic_cast<Win32Control&>(GetChild(i)).CreateHandle();
		}

		GetEventHandler().OnCreate(*this);
		InvalidateRect(Handle, nullptr, TRUE);
	}
	HWND GetParentHandle() {
		return dynamic_cast<Win32Control&>(GetParent()).Handle;
	}
	static LRESULT CALLBACK WndProc(HWND handle, UINT message, WPARAM wParam, LPARAM lParam) {
		return reinterpret_cast<Win32Control*>(GetWindowLongPtr(handle, GWLP_USERDATA))->Callback(message, wParam, lParam);
	}

protected:
	virtual LRESULT Callback(UINT message, WPARAM wParam, LPARAM lParam) {
		switch (message) {
		case WM_SIZE:
			GetEventHandler().OnResize(*this);

			break;

		case WM_LBUTTONDOWN:
			GetEventHandler().OnMouseDown(*this, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), MouseButton::Left);

			break;

		case WM_MOUSEMOVE:
			GetEventHandler().OnMouseMove(*this, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

			break;

		case WM_LBUTTONUP:
			GetEventHandler().OnMouseUp(*this, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), MouseButton::Left);

			break;

		case WM_MOUSEWHEEL:
			GetEventHandler().OnMouseWheel(*this, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam),
				GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? MouseWheel::Forward : MouseWheel::Backward);

			break;

		case WM_KEYDOWN: {
			Key key = Key::None;

			switch (wParam) {
			case VK_RETURN: key = Key::Enter; break;
			}

			if (key != Key::None) {
				GetEventHandler().OnKeyDown(*this, key);
			}

			break;
		}

		case WM_KEYUP: {
			Key key = Key::None;

			switch (wParam) {
			case VK_RETURN: key = Key::Enter; break;
			}

			if (key != Key::None) {
				GetEventHandler().OnKeyUp(*this, key);
			}

			break;
		}

		case WM_RECEIVEMESSAGE: {
			std::optional<std::any> argument;

			if (lParam) {
				std::any* const heap = reinterpret_cast<std::any*>(lParam);

				argument.emplace(std::move(*heap));

				delete heap;
			}

			GetEventHandler().OnReceiveMessage(*this, static_cast<std::size_t>(wParam), std::move(argument));

			return 0;
		}

		case WM_DESTROY:
			GetEventHandler().OnDestroy(*this);

			break;

		case WM_NCDESTROY:
			SetWindowLongPtr(Handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_OldCallback));

			Handle = nullptr;
			m_OldCallback = nullptr;

			break;
		}

		return DefaultCallback(message, wParam, lParam);
	}
	LRESULT DefaultCallback(UINT message, WPARAM wParam, LPARAM lParam) {
		return CallWindowProc(m_OldCallback, Handle, message, wParam, lParam);
	}
};

Win32Control::~Win32Control() {
	DestroyWindow(Handle);
}

class Win32MenuItem;

class Win32Menu : public Menu {
public:
	HMENU Handle = nullptr;

public:
	Win32Menu() {
		Handle = CreateMenu();
		if (Handle == nullptr) throw std::runtime_error("Failed to create the handle of a menu");
	}
	Win32Menu(const Win32Menu&) = delete;
	virtual ~Win32Menu() override {
		DestroyMenu(Handle);
	}

public:
	Win32Menu& operator=(const Win32Menu&) = delete;

protected:
	virtual void PALAddItem(MenuItem& item) override;
	virtual void* PALGetHandle() noexcept override {
		return Handle;
	}

public:
	Win32MenuItem& FindMenuItem(UINT_PTR menuItemId);
};

std::unique_ptr<Menu> MenuRef::PALCreateMenu() {
	return std::make_unique<Win32Menu>();
}

class Win32MenuItem : public DropDownMenuItem {
private:
	static inline UINT_PTR m_IdCount = 0;

public:
	UINT_PTR Id = 0;

	std::optional<std::string> String;
	std::optional<HMENU> PopupMenu;

private:
	std::size_t m_Index = 0;

public:
	Win32MenuItem(std::string string, std::unique_ptr<MenuItemEventHandler>&& eventHandler) noexcept
		: MenuItem(std::move(eventHandler)), String(std::move(string)) {}
	Win32MenuItem(const Win32MenuItem&) = delete;
	virtual ~Win32MenuItem() override {
		if (PopupMenu) {
			DestroyMenu(*PopupMenu);
		}
	}

public:
	Win32Menu& operator=(const Win32Menu&) = delete;

protected:
	virtual void* PALGetHandle() noexcept override {
		return PopupMenu ? *PopupMenu : nullptr;
	}
	virtual void PALAddSubItem(MenuItem& subItem) override {
		if (!PopupMenu) {
			PopupMenu = CreatePopupMenu();
			if (PopupMenu == nullptr) throw std::runtime_error("Failed to create the handle of a menu item");

			if (HasParent()) {
				AddItemToParent(static_cast<HMENU>(
					IsRootItem() ? GetParentMenu().GetHandle() : GetParentItem().GetHandle()), m_Index, true);
			}
		}

		dynamic_cast<Win32MenuItem&>(subItem).AddItemToParent(*PopupMenu, GetSubItemCount() - 1);
	}

public:
	void AddItemToParent(HMENU parent, std::size_t index, bool isModify = false) {
		const UINT flag = GetMenuFlag();
		UINT_PTR itemId = GetMenuItemId();
		const LPCSTR item = GetMenuItem();

		if (itemId == 0) {
			itemId = Id = ++m_IdCount;
		}

		if (isModify) {
			BOOL result = ModifyMenuA(parent, static_cast<UINT>(index), flag | MF_BYPOSITION, itemId, item);
			if (!result) throw std::runtime_error("Failed to add a menu item to its parent");

			result = DrawMenuBar(GetParentWindow());
			if (!result) throw std::runtime_error("Failed to add a menu item to its parent");
		} else {
			const BOOL result = AppendMenuA(parent, flag, itemId, item);
			if (!result) throw std::runtime_error("Failed to add a menu item to its parent");

			m_Index = index;
		}
	}
	Win32MenuItem* FindMenuItem(UINT_PTR menuItemId) noexcept {
		if (menuItemId == Id) return this;

		const std::size_t subItemCount = GetSubItemCount();

		for (std::size_t i = 0; i < subItemCount; ++i) {
			Win32MenuItem* const result = dynamic_cast<Win32MenuItem&>(GetSubItem(i)).FindMenuItem(menuItemId);

			if (result) return result;
		}

		return nullptr;
	}

protected:
	virtual UINT GetMenuFlag() noexcept {
		return (String ? MF_STRING : 0) | (PopupMenu ? MF_POPUP : 0);
	}
	virtual UINT_PTR GetMenuItemId() noexcept {
		return PopupMenu ? reinterpret_cast<UINT_PTR>(*PopupMenu) : 0;
	}
	virtual LPCSTR GetMenuItem() noexcept {
		return String ? String->data() : nullptr;
	}

private:
	HWND GetParentWindow() const noexcept {
		const MenuItem* menuItem = this;

		while (menuItem->IsSubItem()) {
			menuItem = &menuItem->GetParentItem();
		}

		if (!menuItem->HasParent()) return nullptr;

		const Menu& menu = menuItem->GetParentMenu();

		return menu.HasParent() ? dynamic_cast<const Win32Control&>(menu.GetParent()).Handle : nullptr;
	}
};

std::unique_ptr<MenuItem> MenuItemRef::PALCreateMenuItem(std::string string,
	std::unique_ptr<MenuItemEventHandler>&& eventHandler) {
	return std::make_unique<Win32MenuItem>(std::move(string), std::move(eventHandler));
}

std::unique_ptr<DropDownMenuItem> DropDownMenuItemRef::PALCreateDropDownMenuItem(std::string string) {
	return std::make_unique<Win32MenuItem>(std::move(string), std::make_unique<MenuItemEventHandler>());
}

void Win32Menu::PALAddItem(MenuItem& item) {
	dynamic_cast<Win32MenuItem&>(item).AddItemToParent(Handle, GetItemCount() - 1);
}

Win32MenuItem& Win32Menu::FindMenuItem(UINT_PTR menuItemId) {
	const std::size_t itemCount = GetItemCount();

	for (std::size_t i = 0; i < itemCount; ++i) {
		Win32MenuItem* const result = dynamic_cast<Win32MenuItem&>(GetItem(i)).FindMenuItem(menuItemId);

		if (result) return *result;
	}

	throw std::runtime_error("Failed to find the menu item");
}

class Win32MenuItemSeparator final : public MenuItemSeparator, public Win32MenuItem {
public:
	Win32MenuItemSeparator()
		: MenuItem(std::make_unique<MenuItemEventHandler>()), Win32MenuItem("", nullptr) {}
	Win32MenuItemSeparator(const Win32MenuItemSeparator&) = delete;
	virtual ~Win32MenuItemSeparator() override = default;

public:
	Win32MenuItemSeparator& operator=(const Win32MenuItemSeparator&) = delete;

protected:
	virtual UINT GetMenuFlag() noexcept override {
		return MF_SEPARATOR;
	}
};

std::unique_ptr<MenuItemSeparator> MenuItemSeparatorRef::PALCreateMenuItemSeparator() {
	return std::make_unique<Win32MenuItemSeparator>();
}

namespace {
	void RegisterWindowClass() {
		WNDCLASSA wndClass{};
		wndClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
		wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wndClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
		wndClass.hInstance = g_Instance;
		wndClass.lpfnWndProc = DefWindowProcA;
		wndClass.lpszClassName = "Window";
		wndClass.lpszMenuName = nullptr;
		wndClass.style = CS_VREDRAW | CS_HREDRAW;

		if (RegisterClassA(&wndClass) == 0)
			throw std::runtime_error("Failed to register a window class");

		InitCommonControls();
	}
}

class Win32Window final : public Window, public Win32Control {
public:
	bool IsMainWindow = false;

private:
	std::pair<int, int> m_MinimumSize{ 0, 0 };

public:
	Win32Window(std::unique_ptr<WindowEventHandler>&& eventHandler) noexcept
		: Control(std::move(eventHandler)), Win32Control("Window", WS_OVERLAPPEDWINDOW) {}
	Win32Window(const Win32Control&) = delete;
	virtual ~Win32Window() override = default;

public:
	Win32Window& operator=(const Win32Window&) = delete;

protected:
	virtual std::pair<int, int> PALGetMinimumSize() const override {
		return m_MinimumSize;
	}
	virtual void PALSetMinimumSize(int newMinimumWidth, int newMinimumHeight) override {
		m_MinimumSize.first = newMinimumWidth;
		m_MinimumSize.second = newMinimumHeight;
	}
	virtual void PALSetMenu(Menu& menu) override {
		const HMENU menuHandle = static_cast<Win32Menu&>(menu).Handle;

		if (Handle) {
			const BOOL result = ::SetMenu(Handle, menuHandle);
			if (!result) throw std::runtime_error("Failed to add a menu item to its parent");
		} else {
			CreateParams.Menu = menuHandle;
		}
	}

	virtual void PALInvalidate() override {
		assert(Handle != nullptr);

		InvalidateRect(Handle, nullptr, FALSE);
		UpdateWindow(Handle);
	}

	virtual void PALClose() override {
		if (Handle) {
			CreateParams.Location = PALGetLocation();
			CreateParams.Size = PALGetSize();
			CreateParams.WindowName = PALGetText();

			if (HasMenu()) {
				CreateParams.Menu = dynamic_cast<Win32Menu&>(GetMenu()).Handle;
			}

			SendMessage(Handle, WM_CLOSE, 0, 0);
		}
	}

protected:
	virtual LRESULT Callback(UINT message, WPARAM wParam, LPARAM lParam) override {
		switch (message) {
		case WM_COMMAND:
			if (lParam == 0 && HIWORD(wParam) == 0) {
				Win32Menu& menu = dynamic_cast<Win32Menu&>(GetMenu());
				Win32MenuItem& menuItem = menu.FindMenuItem(LOWORD(wParam));

				menuItem.GetEventHandler().OnClick(menuItem);
			} else if (lParam != 0)
				return SendMessage(reinterpret_cast<HWND>(lParam), WM_REFLECT + message, wParam, lParam);

			break;

		case WM_PAINT:
			WmPaint();

			return 0;

		case WM_GETMINMAXINFO: {
			const LPMINMAXINFO sizeInfo = reinterpret_cast<LPMINMAXINFO>(lParam);

			sizeInfo->ptMinTrackSize.x = m_MinimumSize.first;
			sizeInfo->ptMinTrackSize.y = m_MinimumSize.second;

			return 0;
		}

		case WM_CLOSE: {
			bool cancel = false;

			dynamic_cast<WindowEventHandler&>(GetEventHandler()).OnClose(*this, cancel);

			if (!cancel) {
				DestroyWindow(Handle);
			}

			return 0;
		}

		case WM_NCDESTROY:
			if (IsMainWindow) {
				PostQuitMessage(0);
			}

			return 0;
		}

		return Win32Control::Callback(message, wParam, lParam);
	}

private:
	void WmPaint();
};

std::unique_ptr<Window> WindowRef::PALCreateWindow(std::unique_ptr<WindowEventHandler>&& eventHandler) {
	return std::make_unique<Win32Window>(std::move(eventHandler));
}

int PALRunEventLoop(WindowRef* mainWindow) {
	if (mainWindow && !mainWindow->IsEmpty()) {
		dynamic_cast<Win32Window&>(**mainWindow).IsMainWindow = true;
	}

	MSG message;

	while (GetMessage(&message, nullptr, 0, 0)) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	return static_cast<int>(message.wParam);
}

class Win32Button final : public Button, public Win32Control {
public:
	Win32Button(std::unique_ptr<ButtonEventHandler>&& eventHandler) noexcept
		: Control(std::move(eventHandler)), Win32Control(WC_BUTTONA, WS_CHILD | BS_PUSHBUTTON) {}
	Win32Button(const Win32Control&) = delete;
	virtual ~Win32Button() override = default;

public:
	Win32Button& operator=(const Win32Button&) = delete;

protected:
	virtual LRESULT Callback(UINT message, WPARAM wParam, LPARAM lParam) override {
		switch (message) {
		case WM_REFLECT + WM_COMMAND:
			switch (HIWORD(wParam)) {
			case BN_CLICKED:
				dynamic_cast<ButtonEventHandler&>(GetEventHandler()).OnClick(*this);

				break;
			}

			return 0;
		}

		return Win32Control::Callback(message, wParam, lParam);
	}
};

std::unique_ptr<Button> ButtonRef::PALCreateButton(std::unique_ptr<ButtonEventHandler>&& eventHandler) {
	return std::make_unique<Win32Button>(std::move(eventHandler));
}

class Win32Panel final : public Panel, public Win32Control {
public:
	Win32Panel(std::unique_ptr<PanelEventHandler>&& eventHandler) noexcept
		: Control(std::move(eventHandler)), Win32Control("Window", WS_CHILD | BS_PUSHBUTTON) {}
	Win32Panel(const Win32Panel&) = delete;
	virtual ~Win32Panel() override = default;

public:
	Win32Panel& operator=(const Win32Panel&) = delete;

protected:
	virtual void PALInvalidate() override {
		assert(Handle != nullptr);

		InvalidateRect(Handle, nullptr, FALSE);
		UpdateWindow(Handle);
	}

protected:
	virtual LRESULT Callback(UINT message, WPARAM wParam, LPARAM lParam) override {
		switch (message) {
		case WM_PAINT:
			WmPaint();

			return 0;
		}

		return Win32Control::Callback(message, wParam, lParam);
	}

private:
	void WmPaint();
};

std::unique_ptr<Panel> PanelRef::PALCreatePanel(std::unique_ptr<PanelEventHandler>&& eventHandler) {
	return std::make_unique<Win32Panel>(std::move(eventHandler));
}

class Win32TextBox final : public TextBox, public Win32Control {
public:
	Win32TextBox(std::unique_ptr<TextBoxEventHandler>&& eventHandler, bool multiLines) noexcept
		: Control(std::move(eventHandler)), TextBox(multiLines), Win32Control(WC_EDITA,
			WS_CHILD | WS_BORDER | (multiLines ? ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL : 0)) {}
	Win32TextBox(const Win32TextBox&) = delete;
	virtual ~Win32TextBox() override = default;

public:
	Win32TextBox& operator=(const Win32TextBox&) = delete;
};

std::unique_ptr<TextBox> TextBoxRef::PALCreateTextBox(std::unique_ptr<TextBoxEventHandler>&& eventHandler,
	bool multiLines) {
	return std::make_unique<Win32TextBox>(std::move(eventHandler), multiLines);
}

class Win32Pen final : public SolidPen {
public:
	Gdiplus::Pen Object;

public:
	Win32Pen(Color color, float width)
		: Pen(width), SolidPen(color), Object(Gdiplus::Color(color.A, color.R, color.G, color.B), width) {}
	Win32Pen(const Win32Pen&) = delete;
	virtual ~Win32Pen() override = default;

public:
	Win32Pen& operator=(const Win32Pen&) = delete;
};

std::shared_ptr<SolidPen> SolidPenRef::PALCreateSolidPen(Color color, float width) {
	return std::make_shared<Win32Pen>(color, width);
}

class Win32Brush : public virtual Brush {
public:
	Gdiplus::Brush& Object;

public:
	explicit Win32Brush(Gdiplus::Brush& brush) noexcept
		: Object(brush) {}
	Win32Brush(const Win32Brush&) = delete;
	virtual ~Win32Brush() override = default;

public:
	Win32Brush& operator=(const Win32Brush&) = delete;
};

class Win32SolidBrush final : public SolidBrush, public Win32Brush {
private:
	Gdiplus::SolidBrush m_SolidBrush;

public:
	explicit Win32SolidBrush(Color color)
		: SolidBrush(color), Win32Brush(m_SolidBrush), m_SolidBrush(Gdiplus::Color(color.A, color.R, color.G, color.B)) {}
	Win32SolidBrush(const Win32SolidBrush&) = delete;
	virtual ~Win32SolidBrush() override = default;

public:
	Win32SolidBrush& operator=(const Win32SolidBrush&) = delete;
};

std::shared_ptr<SolidBrush> SolidBrushRef::PALCreateSolidBrush(Color color) {
	return std::make_shared<Win32SolidBrush>(color);
}

class Win32RenderingContext2D : public RenderingContext2D {
private:
	Gdiplus::Graphics m_Graphics;

public:
	Win32RenderingContext2D(Graphics& graphics, HDC deviceContext);
	Win32RenderingContext2D(const Win32RenderingContext2D&) = delete;
	virtual ~Win32RenderingContext2D() override = default;

public:
	Win32RenderingContext2D& operator=(const Win32RenderingContext2D&) = delete;

protected:
	virtual void PALSetPen(Pen&) override {}
	virtual void PALSetBrush(Brush&) override {}

	virtual void PALDrawRectangle(int x, int y, int width, int height) override {
		m_Graphics.DrawRectangle(&dynamic_cast<const Win32Pen&>(GetPen()).Object, x, y, width, height);
	}
	virtual void PALDrawEllipse(int x, int y, int width, int height) override {
		m_Graphics.DrawEllipse(&dynamic_cast<const Win32Pen&>(GetPen()).Object, x, y, width, height);
	}
	virtual void PALDrawLine(int x1, int y1, int x2, int y2) override {
		m_Graphics.DrawLine(&dynamic_cast<const Win32Pen&>(GetPen()).Object, x1, y1, x2, y2);
	}

	virtual void PALFillRectangle(int x, int y, int width, int height) override {
		m_Graphics.FillRectangle(&dynamic_cast<const Win32Brush&>(GetBrush()).Object, x, y, width, height);
	}
	virtual void PALFillEllipse(int x, int y, int width, int height) override {
		m_Graphics.FillEllipse(&dynamic_cast<const Win32Brush&>(GetBrush()).Object, x, y, width, height);
	}
};

namespace {
	struct DoubleBufferingContext final {
		HDC OriginalDC;
		HDC BufferDC;
		HBITMAP BufferBitmap;
		HBITMAP OriginalBitmap;

		RECT ClientRectangle;
	};

	void CreateBufferDC(DoubleBufferingContext& doubleBufferingContext, HDC originalDC,
		int clientWidth, int clientHeight) {
		doubleBufferingContext.OriginalDC = originalDC;
		doubleBufferingContext.BufferDC = CreateCompatibleDC(originalDC);
		doubleBufferingContext.BufferBitmap =
			CreateCompatibleBitmap(doubleBufferingContext.OriginalDC, clientWidth, clientHeight);
		doubleBufferingContext.OriginalBitmap = static_cast<HBITMAP>(
			SelectObject(doubleBufferingContext.BufferDC, doubleBufferingContext.BufferBitmap));

		doubleBufferingContext.ClientRectangle.right = clientWidth;
		doubleBufferingContext.ClientRectangle.bottom = clientHeight;
	}
	void DeleteBufferDC(DoubleBufferingContext& doubleBufferingContext) {
		BitBlt(doubleBufferingContext.OriginalDC, 0, 0,
			doubleBufferingContext.ClientRectangle.right, doubleBufferingContext.ClientRectangle.bottom,
			doubleBufferingContext.BufferDC, 0, 0, SRCCOPY);
		SelectObject(doubleBufferingContext.BufferDC, doubleBufferingContext.OriginalBitmap);

		DeleteObject(doubleBufferingContext.BufferBitmap);
		DeleteDC(doubleBufferingContext.BufferDC);
	}
}

class Win32Graphics final : public Graphics {
public:
	HDC DeviceContext;

public:
	Win32Graphics(Control& control, HDC deviceContext) noexcept
		: Graphics(control), DeviceContext(deviceContext) {
		assert(deviceContext != nullptr);
	}
	Win32Graphics(const Win32Graphics&) = delete;
	virtual ~Win32Graphics() override = default;

public:
	Win32Graphics& operator=(const Win32Graphics&) = delete;

protected:
	virtual RenderingContext2DRef PALGetContext2D(Control&) override {
		return { std::make_unique<Win32RenderingContext2D>(*this, DeviceContext) };
	}
};

void Win32Window::WmPaint() {
	PAINTSTRUCT ps;
	const HDC dc = BeginPaint(Handle, &ps);

	const auto& [clientWidth, clientHeight] = GetClientSize();
	DoubleBufferingContext ctx{};

	CreateBufferDC(ctx, dc, clientWidth, clientHeight);
	FillRect(ctx.BufferDC, &ctx.ClientRectangle, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));

	Win32Graphics graphics(*this, ctx.BufferDC);

	try {
		dynamic_cast<WindowEventHandler&>(GetEventHandler()).OnPaint(*this, graphics);
	} catch (const std::exception&) {
		DeleteBufferDC(ctx);
		EndPaint(Handle, &ps);

		throw;
	}

	DeleteBufferDC(ctx);
	EndPaint(Handle, &ps);
}

void Win32Panel::WmPaint() {
	PAINTSTRUCT ps;
	const HDC dc = BeginPaint(Handle, &ps);

	const auto& [clientWidth, clientHeight] = GetClientSize();
	DoubleBufferingContext ctx{};

	CreateBufferDC(ctx, dc, clientWidth, clientHeight);
	FillRect(ctx.BufferDC, &ctx.ClientRectangle, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));

	Win32Graphics graphics(*this, ctx.BufferDC);

	try {
		dynamic_cast<PanelEventHandler&>(GetEventHandler()).OnPaint(*this, graphics);
	} catch (const std::exception&) {
		DeleteBufferDC(ctx);
		EndPaint(Handle, &ps);

		throw;
	}

	DeleteBufferDC(ctx);
	EndPaint(Handle, &ps);
}

Win32RenderingContext2D::Win32RenderingContext2D(Graphics& graphics, HDC deviceContext)
	: RenderingContext2D(graphics, g_DefaultPen, g_DefaultBrush), m_Graphics(deviceContext) {
	m_Graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
}

class Win32MessageDialog final : public MessageDialog {
public:
	Win32MessageDialog(Window& owner, std::string dialogTitle, std::string title, std::string message,
		Icon icon, Button buttons)
		: Dialog(owner, std::move(dialogTitle)), MessageDialog(std::move(title), std::move(message), icon, buttons) {}
	Win32MessageDialog(const Win32MessageDialog&) = delete;
	virtual ~Win32MessageDialog() override = default;

public:
	Win32MessageDialog& operator=(const Win32MessageDialog&) = delete;

public:
	virtual DialogResult PALShow() override;
};

std::unique_ptr<MessageDialog> MessageDialogRef::PALCreateMessageDialog(Window& owner, std::string dialogTitle,
	std::string title, std::string message, MessageDialog::Icon icon, MessageDialog::Button buttons) {
	return std::make_unique<Win32MessageDialog>(owner, std::move(dialogTitle), std::move(title), std::move(message),
		icon, buttons);
}

#undef GetOpenFileName

class Win32FileDialog : public virtual FileDialog {
public:
	Win32FileDialog() noexcept {}
	Win32FileDialog(const Win32FileDialog&) = delete;
	virtual ~Win32FileDialog() override = default;

public:
	Win32MessageDialog& operator=(const Win32MessageDialog&) = delete;

protected:
	OPENFILENAMEA GetOpenFileName() const {
		OPENFILENAMEA openFileName{};

		openFileName.lStructSize = sizeof(openFileName);
		openFileName.lpstrTitle = GetDialogTitle().data();
		openFileName.hwndOwner = dynamic_cast<const Win32Window&>(GetOwner()).Handle;
		openFileName.Flags = OFN_EXPLORER;

		thread_local char path[MAX_PATH + 1]{};

		openFileName.lpstrFile = path;
		openFileName.nMaxFile = ARRAYSIZE(path);

		thread_local std::string filter;
		filter = GetWin32Filter();

		openFileName.lpstrFilter = filter.data();
		openFileName.lpstrDefExt = GetFilter(0).second.data();

		return openFileName;
	}

private:
	std::string GetWin32Filter() const {
		std::string result;

		for (const auto& [description, pattern] : GetAllFilters()) {
			result += description + '\0' + pattern + '\0';
		}

		return result;
	}
};

class Win32OpenFileDialog final : public OpenFileDialog, public Win32FileDialog {
public:
	Win32OpenFileDialog(Window& owner, std::string dialogTitle) noexcept
		: Dialog(owner, std::move(dialogTitle)) {}
	Win32OpenFileDialog(const Win32OpenFileDialog&) = delete;
	virtual ~Win32OpenFileDialog() override = default;

public:
	Win32OpenFileDialog& operator=(const Win32OpenFileDialog&) = delete;

protected:
	virtual DialogResult PALShow() override {
		OPENFILENAMEA openFileName = GetOpenFileName();

		openFileName.Flags |= GetFileMustExist() ? OFN_FILEMUSTEXIST : 0;

		if (GetOpenFileNameA(&openFileName) != 0) {
			SetPath(openFileName.lpstrFile);

			return DialogResult::Ok;
		} else return DialogResult::Cancel;
	}
};

std::unique_ptr<OpenFileDialog> OpenFileDialogRef::PALCreateOpenFileDialog(Window& owner, std::string dialogTitle) {
	return std::make_unique<Win32OpenFileDialog>(owner, std::move(dialogTitle));
}

class Win32SaveFileDialog final : public SaveFileDialog, public Win32FileDialog {
public:
	Win32SaveFileDialog(Window& owner, std::string dialogTitle) noexcept
		: Dialog(owner, std::move(dialogTitle)) {}
	Win32SaveFileDialog(const Win32SaveFileDialog&) = delete;
	virtual ~Win32SaveFileDialog() override = default;

public:
	Win32SaveFileDialog& operator=(const Win32SaveFileDialog&) = delete;

protected:
	virtual DialogResult PALShow() override {
		OPENFILENAMEA openFileName = GetOpenFileName();

		openFileName.Flags |= GetOverWritePrompt() ? OFN_OVERWRITEPROMPT : 0;

		if (GetSaveFileNameA(&openFileName) != 0) {
			SetPath(openFileName.lpstrFile);

			return DialogResult::Ok;
		} else return DialogResult::Cancel;
	}
};

std::unique_ptr<SaveFileDialog> SaveFileDialogRef::PALCreateSaveFileDialog(Window& owner, std::string dialogTitle) {
	return std::make_unique<Win32SaveFileDialog>(owner, std::move(dialogTitle));
}

void WindowDialog::PALShow(bool& isRunning) {
	const HWND dialogHandle = dynamic_cast<Win32Window&>(m_Window.Get()).Handle;

	UINT style = static_cast<UINT>(GetWindowLongPtr(dialogHandle, GWL_STYLE));

	style &= ~WS_MINIMIZEBOX;
	style &= ~WS_MAXIMIZEBOX;

	SetWindowLongPtr(dialogHandle, GWL_STYLE, static_cast<LONG_PTR>(style));
	SetWindowLongPtr(dialogHandle, GWLP_HWNDPARENT,
		reinterpret_cast<LONG_PTR>(dynamic_cast<Win32Window&>(GetOwner()).Handle));

	while (isRunning) {
		MSG message;

		if (GetMessage(&message, nullptr, 0, 0)) {
			TranslateMessage(&message);
			DispatchMessage(&message);
		} else {
			isRunning = false;

			PostQuitMessage(static_cast<int>(message.wParam));
		}
	}
}

#undef GetMessage

DialogResult Win32MessageDialog::PALShow() {
	const Button buttons = GetButtons();
	TASKDIALOG_COMMON_BUTTON_FLAGS buttonsForApi = 0;

	if (buttons & Button::Ok) {
		buttonsForApi |= TDCBF_OK_BUTTON;
	}
	if (buttons & Button::Yes) {
		buttonsForApi |= TDCBF_YES_BUTTON;
	}
	if (buttons & Button::No) {
		buttonsForApi |= TDCBF_NO_BUTTON;
	}
	if (buttons & Button::Cancel) {
		buttonsForApi |= TDCBF_CANCEL_BUTTON;
	}
	if (buttons & Button::Retry) {
		buttonsForApi |= TDCBF_RETRY_BUTTON;
	}
	if (buttons & Button::Close) {
		buttonsForApi |= TDCBF_CLOSE_BUTTON;
	}

	const Icon icon = GetIcon();
	PCWSTR iconForApi = nullptr;

	switch (icon) {
	case Icon::None: break;

	case Icon::Information:
		iconForApi = TD_INFORMATION_ICON;
		break;

	case Icon::Warning:
		iconForApi = TD_WARNING_ICON;
		break;

	case Icon::Error:
		iconForApi = TD_ERROR_ICON;
		break;

	default:
		assert(false);
		break;
	}

	int button;
	const HRESULT result = TaskDialog(dynamic_cast<const Win32Window&>(GetOwner()).Handle, g_Instance,
		EncodeToWideCharString(std::string(GetDialogTitle()), CP_ACP).data(),
		EncodeToWideCharString(std::string(GetTitle()), CP_ACP).data(),
		EncodeToWideCharString(std::string(GetMessage()), CP_ACP).data(), buttonsForApi, iconForApi, &button);
	if (result != S_OK) throw std::runtime_error("Failed to show a message dialog");

	switch (button) {
	case IDOK: return DialogResult::Ok;
	case IDYES: return DialogResult::Yes;
	case IDNO: return DialogResult::No;
	case IDCANCEL: return DialogResult::Cancel;
	case IDRETRY: return DialogResult::Retry;
	default: return DialogResult::Cancel;
	}
}
#endif