#include "NetworkViewer.hpp"

void NetworkViewerHandler::OnCreate(Control& control) {
	m_Panel = &dynamic_cast<Panel&>(control);
}

void NetworkViewerHandler::OnPaint(Control&, Graphics& graphics) {
	// TODO
}