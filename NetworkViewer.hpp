#pragma once

#include "Network.hpp"
#include "PALGraphics.hpp"

#include <optional>

class NetworkViewerHandler final : public PanelEventHandler {
private:
	Panel* m_Panel = nullptr;

	BrushRef m_BlackBrush = std::shared_ptr<Brush>();
	BrushRef m_CloudBrush = std::shared_ptr<Brush>();
	BrushRef m_BelizeHoleBrush = std::shared_ptr<Brush>();

	std::optional<NetworkDump> m_TargetNetworkDump;
	int m_ZoomLevel = 0;
	int m_MovedX = 0, m_MovedY = 0;

	bool m_IsMouseDown = false;
	int m_MouseX = 0, m_MouseY = 0;

public:
	NetworkViewerHandler() noexcept = default;
	NetworkViewerHandler(const NetworkViewerHandler&) = delete;
	virtual ~NetworkViewerHandler() override = default;

public:
	virtual void OnCreate(Control& control) override;
	virtual void OnPaint(Control& control, Graphics& graphics) override;
	virtual void OnMouseDown(Control& control, int x, int y, MouseButton mouseButton) override;
	virtual void OnMouseMove(Control& control, int x, int y) override;
	virtual void OnMouseUp(Control& control, int x, int y, MouseButton mouseButton) override;
	virtual void OnMouseWheel(Control& control, int x, int y, MouseWheel mouseWheel) override;

public:
	void UpdateTargetNetworkDump(const Network& targetNetwork);
};