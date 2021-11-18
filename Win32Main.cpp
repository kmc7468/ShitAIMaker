#ifdef _WIN32
#	ifdef _MSC_VER
#		pragma comment(linker, "\"/manifestdependency:type='win32' \
			name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
			processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#	endif

#include <Windows.h>

int Main();

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	return Main();
}
#endif