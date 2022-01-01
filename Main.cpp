#include "MainWindow.hpp"
#include "Optimizer.hpp"
#include "PALGraphics.hpp"

#include <memory>

static int Run() {
	WindowRef mainWindow(std::make_unique<MainWindowHandler>());

	mainWindow->SetSize(640, 480);
	mainWindow->SetMinimumSize(640, 480);

	mainWindow->Show();

	return RunEventLoop(mainWindow);
}

int Main() {
	InitializeGraphics();

	const int result = Run();

	FinalizeGraphics();

	return result;
}