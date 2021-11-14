#ifdef _WIN32
#include "PALGraphics.hpp"

#include <cassert>
#include <cstdint>
#include <string>
#include <Windows.h>

#define WM_REFLECT WM_USER + 0x1C00

namespace {
	HINSTANCE g_Instance;

	void RegisterWindowClass();
}

void PALInitializeGraphics() {
	g_Instance = GetModuleHandle(nullptr);

	RegisterWindowClass();
}
void PALFinalizeGraphics() noexcept {}

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

class Win32Control : public Control {
public:
	HWND Handle = nullptr;

private:
	CreateParams m_CreateParams;
	WNDPROC m_OldCallback = nullptr;

public:
	Win32Control(std::unique_ptr<EventHandler>&& eventHandler, std::string className, DWORD style) noexcept
		: Control(std::move(eventHandler)) {
		assert(!className.empty());

		m_CreateParams.ClassName = std::move(className);
		m_CreateParams.Style = style;
	}
	Win32Control(const Win32Control&) = delete;
	virtual ~Win32Control() override {
		DestroyWindow(Handle);
	}

public:
	Win32Control& operator=(const Win32Control&) = delete;

protected:
	virtual void PALAddChild(Control& child) {
		Win32Control& childControl = static_cast<Win32Control&>(child);
		assert(childControl.m_CreateParams.Style & WS_CHILD);

		childControl.m_CreateParams.Menu = reinterpret_cast<HMENU>(GetChildrenCount() - 1);

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
		} else return m_CreateParams.Size;
	}
	virtual void PALSetSize(int newWidth, int newHeight) override {
		if (Handle) {
			SetWindowPos(Handle, nullptr, 0, 0, newWidth, newHeight, SWP_NOZORDER | SWP_NOMOVE);
		} else {
			m_CreateParams.Size = { newWidth, newHeight };
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
		} else return m_CreateParams.Location;
	}
	virtual void PALSetLocation(int newX, int newY) override {
		if (Handle) {
			SetWindowPos(Handle, nullptr, newX, newY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		} else {
			m_CreateParams.Location = { newX, newY };
		}
	}
	virtual bool PALGetVisibility() const override {
		if (Handle) return IsWindowVisible(Handle);
		else return m_CreateParams.Style & WS_VISIBLE;
	}
	virtual void PALSetVisibility(bool newVisibility) override {
		if (Handle) {
			ShowWindow(Handle, newVisibility ? SW_SHOW : SW_HIDE);
		} else {
			if (newVisibility) {
				m_CreateParams.Style |= WS_VISIBLE;

				if (!(m_CreateParams.Style & WS_CHILD)) {
					CreateHandle();
				}
			} else {
				m_CreateParams.Style &= ~WS_VISIBLE;
			}
		}
	}

private:
	void CreateHandle() {
		const std::size_t childrenCount = GetChildrenCount();

		Handle = CreateWindowA(m_CreateParams.ClassName.data(), m_CreateParams.WindowName.data(),
			m_CreateParams.Style, m_CreateParams.Location.first, m_CreateParams.Location.second,
			m_CreateParams.Size.first, m_CreateParams.Size.second, HasParent() ? GetParentHandle() : nullptr,
			m_CreateParams.Menu, g_Instance, m_CreateParams.Param);
		assert(Handle != nullptr);

		SetWindowLongPtr(Handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		m_OldCallback = reinterpret_cast<WNDPROC>(
			SetWindowLongPtr(Handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));

		for (std::size_t i = 0; i < childrenCount; ++i) {
			static_cast<Win32Control&>(GetChild(i)).CreateHandle();
		}

		GetEventHandler().OnCreate(*this);
	}
	HWND GetParentHandle() {
		return static_cast<Win32Control&>(GetParent()).Handle;
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

		RegisterClassA(&wndClass);
	}
}

class Win32Window final : public Win32Control {
public:
	bool IsMainWindow = false;

public:
	Win32Window(std::unique_ptr<EventHandler>&& eventHandler) noexcept
		: Win32Control(std::move(eventHandler), "Window", WS_OVERLAPPEDWINDOW) {}
	Win32Window(const Win32Control&) = delete;
	virtual ~Win32Window() override = default;

public:
	Win32Window& operator=(const Win32Window&) = delete;

protected:
	virtual LRESULT Callback(UINT message, WPARAM wParam, LPARAM lParam) override {
		switch (message) {
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
};

std::unique_ptr<Control> Window::PALCreateWindow(std::unique_ptr<EventHandler>&& eventHandler) {
	return std::make_unique<Win32Window>(std::move(eventHandler));
}

int PALRunEventLoop(Window* mainWindow) {
	if (mainWindow) {
		static_cast<Win32Window&>(mainWindow->GetControl()).IsMainWindow = true;
	}

	MSG message;

	while (GetMessage(&message, nullptr, 0, 0)) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	return static_cast<int>(message.wParam);
}

class Win32Button final : public Win32Control {
public:
	Win32Button(std::unique_ptr<ClickableEventHandler>&& eventHandler) noexcept
		: Win32Control(std::move(eventHandler), "Button", WS_CHILD | BS_PUSHBUTTON) {}
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

std::unique_ptr<Control> Button::PALCreateButton(std::unique_ptr<ClickableEventHandler>&& eventHandler) {
	return std::make_unique<Win32Button>(std::move(eventHandler));
}
#endif