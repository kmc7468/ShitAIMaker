#include "Optimizer.hpp"
#include "PALGraphics.hpp"
#include "Window.hpp"

#include <memory>

int Main() {
	InitializeGraphics();

	WindowRef mainWindow(std::make_unique<MainWindowHandler>());

	mainWindow->SetSize(640, 480);
	mainWindow->SetMinimumSize(640, 480);

	mainWindow->Show();

	const int result = RunEventLoop(mainWindow);

	FinalizeGraphics();

	return result;
}