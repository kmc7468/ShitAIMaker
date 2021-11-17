#ifdef _WIN32
#include "PALGraphics.hpp"

#include <cassert>
#include <stdexcept>
#include <Windows.h>

#include <gdiplus.h>

#define WM_REFLECT WM_USER + 0x1C00

#ifdef _MSC_VER
#	pragma warning(disable: 4250)
#endif

namespace {
	HINSTANCE g_Instance;

	void RegisterWindowClass();
}

namespace {
	ULONG_PTR g_GdiplusToken;

	void InitializeGdiplus() {
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;

		if (Gdiplus::GdiplusStartup(&g_GdiplusToken, &gdiplusStartupInput, nullptr) != Gdiplus::Ok)
			throw std::runtime_error("Failed to start up GDI+");
	}
	void FinalizeGdiplus() {
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

		childControl.CreateParams.Menu = reinterpret_cast<HMENU>(GetChildrenCount() - 1);

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
			GetWindowRect(Handle, &rect);

			return { static_cast<int>(rect.right - rect.left), static_cast<int>(rect.bottom - rect.top) };
		} else return CreateParams.Size;
	}
	virtual void PALSetSize(int newWidth, int newHeight) override {
		if (Handle) {
			SetWindowPos(Handle, nullptr, 0, 0, newWidth, newHeight, SWP_NOZORDER | SWP_NOMOVE);
		} else {
			CreateParams.Size = { newWidth, newHeight };
		}
	}
	virtual std::pair<int, int> PALGetClientSize() const override {
		assert(Handle != nullptr);

		RECT clientRect;
		GetClientRect(Handle, &clientRect);

		return { static_cast<int>(clientRect.right), static_cast<int>(clientRect.bottom) };
	}
	virtual std::pair<int, int> PALGetLocation() const override {
		if (Handle) {
			RECT rect;
			GetWindowRect(Handle, &rect);

			return { static_cast<int>(rect.left), static_cast<int>(rect.top) };
		} else return CreateParams.Location;
	}
	virtual void PALSetLocation(int newX, int newY) override {
		if (Handle) {
			SetWindowPos(Handle, nullptr, newX, newY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
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

private:
	void CreateHandle() {
		const std::size_t childrenCount = GetChildrenCount();

		Handle = CreateWindowA(CreateParams.ClassName.data(), CreateParams.WindowName.data(),
			CreateParams.Style, CreateParams.Location.first, CreateParams.Location.second,
			CreateParams.Size.first, CreateParams.Size.second, HasParent() ? GetParentHandle() : nullptr,
			CreateParams.Menu, g_Instance, CreateParams.Param);
		assert(Handle != nullptr);

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
		case WM_COMMAND:
			if (lParam != 0)
				return SendMessage(reinterpret_cast<HWND>(lParam), WM_REFLECT + message, wParam, lParam);

			break;

		case WM_DESTROY:
			GetEventHandler().OnCreate(*this);

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

class Win32Menu : public Menu {
public:
	HMENU Handle = nullptr;

public:
	Win32Menu() {
		Handle = CreateMenu();
		assert(Handle != nullptr);
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
};

std::unique_ptr<Menu> MenuRef::PALCreateMenu() {
	return std::make_unique<Win32Menu>();
}

class Win32MenuItem : public DropDownMenuItem {
private:
	static inline UINT_PTR m_IdCount = 0;

public:
	std::optional<std::string> String;
	std::optional<HMENU> PopupMenu;

private:
	std::size_t m_Index = 0;
	UINT_PTR m_Id = 0;

public:
	Win32MenuItem(std::string string, std::unique_ptr<ClickableEventHandler>&& eventHandler) noexcept
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
			assert(*PopupMenu != nullptr);

			if (HasParent()) {
				AddItemToParent(static_cast<HMENU>(
					IsRootItem() ? GetParentMenu().GetHandle() : GetParentItem().GetHandle()), m_Index, true);
			}
		}

		dynamic_cast<Win32MenuItem&>(subItem).AddItemToParent(*PopupMenu, GetSubItemsCount() - 1);
	}

public:
	void AddItemToParent(HMENU parent, std::size_t index, bool isModify = false) {
		const UINT flag = (String ? MF_STRING : 0) | (PopupMenu ? MF_POPUP : 0);
		const UINT_PTR itemId = PopupMenu ?
			reinterpret_cast<UINT_PTR>(*PopupMenu) : (isModify ? m_Id : (m_Id = m_IdCount++));
		const LPCSTR item = String ? String->data() : nullptr;

		if (isModify) {
			ModifyMenuA(parent, static_cast<UINT>(index), flag | MF_BYPOSITION, itemId, item);
			DrawMenuBar(GetParentWindow());
		} else {
			AppendMenuA(parent, flag, itemId, item);

			m_Index = index;
		}
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

std::unique_ptr<MenuItem> MenuItemRef::PALCreateMenuItem(std::string string, std::unique_ptr<ClickableEventHandler>&& eventHandler) {
	return std::make_unique<Win32MenuItem>(std::move(string), std::move(eventHandler));
}

std::unique_ptr<DropDownMenuItem> DropDownMenuItemRef::PALCreateDropDownMenuItem(std::string string) {
	return std::make_unique<Win32MenuItem>(std::move(string), std::make_unique<ClickableEventHandler>());
}

void Win32Menu::PALAddItem(MenuItem& item) {
	dynamic_cast<Win32MenuItem&>(item).AddItemToParent(Handle, GetItemsCount() - 1);
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
			throw std::runtime_error("Failed to register window class");
	}
}

class Win32Window final : public Window, public Win32Control {
public:
	bool IsMainWindow = false;

public:
	Win32Window(std::unique_ptr<PaintableEventHandler>&& eventHandler) noexcept
		: Control(std::move(eventHandler)), Win32Control("Window", WS_OVERLAPPEDWINDOW) {}
	Win32Window(const Win32Control&) = delete;
	virtual ~Win32Window() override = default;

public:
	Win32Window& operator=(const Win32Window&) = delete;

protected:
	virtual void PALSetMenu(Menu& menu) override {
		const HMENU menuHandle = static_cast<Win32Menu&>(menu).Handle;

		if (Handle) {
			::SetMenu(Handle, menuHandle);
		} else {
			CreateParams.Menu = menuHandle;
		}
	}

protected:
	virtual LRESULT Callback(UINT message, WPARAM wParam, LPARAM lParam) override {
		switch (message) {
		case WM_PAINT:
			WmPaint();

			return 0;

		case WM_CLOSE:
			DestroyWindow(Handle);

			return 0;

		case WM_NCDESTROY:
			if (IsMainWindow) {
				PostQuitMessage(0);
			}

			break;
		}

		return Win32Control::Callback(message, wParam, lParam);
	}

private:
	void WmPaint();
};

std::unique_ptr<Window> WindowRef::PALCreateWindow(std::unique_ptr<PaintableEventHandler>&& eventHandler) {
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
	Win32Button(std::unique_ptr<ClickableEventHandler>&& eventHandler) noexcept
		: Control(std::move(eventHandler)), Win32Control("Button", WS_CHILD | BS_PUSHBUTTON) {}
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
				dynamic_cast<ClickableEventHandler&>(GetEventHandler()).OnClick(*this);

				break;
			}

			return 0;
		}

		return Win32Control::Callback(message, wParam, lParam);
	}
};

std::unique_ptr<Button> ButtonRef::PALCreateButton(std::unique_ptr<ClickableEventHandler>&& eventHandler) {
	return std::make_unique<Win32Button>(std::move(eventHandler));
}

class Win32Pen final : public SolidPen {
public:
	Gdiplus::Pen Object;

public:
	Win32Pen(Color color, float width)
		: Pen(width), SolidPen(color), Object(Gdiplus::Color(color.R, color.G, color.B), width) {}
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
		: SolidBrush(color), Win32Brush(m_SolidBrush), m_SolidBrush(Gdiplus::Color(color.R, color.G, color.B)) {}
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

	virtual void PALFillRectangle(int x, int y, int width, int height) override {
		m_Graphics.FillRectangle(&dynamic_cast<const Win32Brush&>(GetBrush()).Object, x, y, width, height);
	}
};

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
	virtual RenderingContext2DRef PALGetContext2D(Control& control) override {
		return { std::make_unique<Win32RenderingContext2D>(*this, DeviceContext) };
	}
};

void Win32Window::WmPaint() {
	PAINTSTRUCT ps;
	const HDC dc = BeginPaint(Handle, &ps);

	Win32Graphics graphics(*this, dc);

	dynamic_cast<PaintableEventHandler&>(GetEventHandler()).OnPaint(*this, graphics);
	EndPaint(Handle, &ps);
}

Win32RenderingContext2D::Win32RenderingContext2D(Graphics& graphics, HDC deviceContext)
	: RenderingContext2D(graphics), m_Graphics(deviceContext) {}
#endif