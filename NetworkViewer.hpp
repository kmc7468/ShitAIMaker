#pragma once

#include "PALGraphics.hpp"

class NetworkViewerHandler final : public PanelEventHandler {
private:
	Panel* m_Panel = nullptr;

public:
	NetworkViewerHandler() noexcept = default;
	NetworkViewerHandler(const NetworkViewerHandler&) = delete;
	virtual ~NetworkViewerHandler() override = default;

public:
	virtual void OnCreate(Control& control) override;
	virtual void OnPaint(Control& control, Graphics& graphics) override;
};