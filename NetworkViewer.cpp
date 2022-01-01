#include "NetworkViewer.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <tuple>
#include <vector>

#define SAM_UNITSIZE 50
#define SAM_UNITINTERVAL 10
#define SAM_UNITMARGIN 5
#define SAM_LAYERINTERVAL 100
#define SAM_LINEMINWIDTH 1
#define SAM_LINEMAXWIDTH 5

#define SAM_UNITX(index) (SAM_UNITMARGIN)
#define SAM_UNITY(index) (SAM_UNITMARGIN + index * SAM_UNITINTERVAL + index * SAM_UNITSIZE)
#define SAM_LAYERHEIGHT(unitCount)																\
	((unitCount) * SAM_UNITSIZE + ((unitCount) - 1) * SAM_UNITINTERVAL + 2 * SAM_UNITMARGIN)
#define SAM_LAYERWIDTH(unitCount) (SAM_UNITSIZE + 2 * SAM_UNITMARGIN)

#define SAM_ZOOM (std::powf(1.25f, static_cast<float>(m_ZoomLevel)))

#define SAM_ROUND(x) static_cast<int>(std::roundf(x))
#define SAM_MAGNIFY(x) SAM_ROUND((x) * SAM_ZOOM)

void NetworkViewerHandler::OnCreate(Control& control) {
	m_Panel = &dynamic_cast<Panel&>(control);

	m_BlackBrush = SolidBrushRef({ 0, 0, 0 });
	m_CloudBrush = SolidBrushRef({ 236, 240, 241 });
	m_BelizeHoleBrush = SolidBrushRef({ 41, 128, 185 });
}
void NetworkViewerHandler::OnPaint(Control&, Graphics& graphics) {
	if (!m_TargetNetworkDump) return;

	const auto ctx = graphics.GetContext2D();

	const auto& layers = m_TargetNetworkDump->GetLayers();
	const std::size_t layerCount = layers.size();
	std::vector<std::tuple<int, int, int, int>> layerRectangles;

	int x = m_MovedX;
	int maxHeight = 0;

	for (std::size_t i = 0; i < layerCount; ++i) {
		const auto& layer = layers[i];

		const int unitCount = static_cast<int>(layer.GetDrawnUnits().size());
		const int width = SAM_LAYERWIDTH(unitCount);
		const int height = SAM_LAYERHEIGHT(unitCount);

		layerRectangles.push_back({ x + SAM_LAYERINTERVAL, 0, width, height });

		x += width + SAM_LAYERINTERVAL;
		maxHeight = std::max(maxHeight, height);
	}

	ctx->SetFont(FontRef("¸¼Àº °íµñ", 11 * SAM_ZOOM));

	for (std::size_t i = 0; i < layerCount; ++i) {
		const auto& layer = layers[i];
		auto& [x, y, width, height] = layerRectangles[i];

		const auto& units = layer.GetDrawnUnits();
		const int unitCount = static_cast<int>(units.size());

		y = SAM_ROUND(m_MovedY + (maxHeight - height) / 2.f);

		ctx->SetBrush(m_CloudBrush);
		ctx->FillRectangle(SAM_MAGNIFY(x), SAM_MAGNIFY(y), SAM_MAGNIFY(width), SAM_MAGNIFY(height));

		ctx->SetBrush(m_BelizeHoleBrush);
		for (int j = 0; j < unitCount; ++j) {
			ctx->FillEllipse(SAM_MAGNIFY(x + SAM_UNITX(j)), SAM_MAGNIFY(y + SAM_UNITY(j)),
				SAM_MAGNIFY(SAM_UNITSIZE), SAM_MAGNIFY(SAM_UNITSIZE));
		}

		ctx->SetBrush(m_BlackBrush);
		for (int j = 0; j < unitCount; ++j) {
			ctx->DrawString('#' + std::to_string(units[j].first),
				SAM_MAGNIFY(x + SAM_UNITX(j)), SAM_MAGNIFY(y + SAM_UNITY(j)));
		}

		ctx->DrawString(std::string(layer.GetName()), SAM_MAGNIFY(x), SAM_MAGNIFY(y + height + SAM_UNITMARGIN));
	}

	for (std::size_t i = 1; i < layerCount; ++i) {
		const auto& layer = layers[i];

		const auto& units = layer.GetDrawnUnits();
		const std::size_t unitCount = units.size();
		const std::size_t prevUnitCount = layers[i - 1].GetDrawnUnits().size();

		for (std::size_t j = 0; j < unitCount; ++j) {
			for (std::size_t k = 0; k < prevUnitCount; ++k) {
				const float width = units[j].second[k];

				if (width == 0) continue;

				ctx->SetPen(SolidPenRef(Color::Black,
					(SAM_LINEMINWIDTH + width * (SAM_LINEMAXWIDTH - SAM_LINEMINWIDTH)) * SAM_ZOOM));
				ctx->DrawLine(
					SAM_MAGNIFY(std::get<0>(layerRectangles[i - 1]) + SAM_UNITX(static_cast<int>(k)) + SAM_UNITSIZE),
					SAM_MAGNIFY(std::get<1>(layerRectangles[i - 1]) + SAM_UNITY(static_cast<int>(k)) + SAM_UNITSIZE / 2),
					SAM_MAGNIFY(std::get<0>(layerRectangles[i]) + SAM_UNITX(static_cast<int>(j))),
					SAM_MAGNIFY(std::get<1>(layerRectangles[i]) + SAM_UNITY(static_cast<int>(j)) + SAM_UNITSIZE / 2));
			}
		}
	}
}
void NetworkViewerHandler::OnMouseDown(Control&, int x, int y, MouseButton mouseButton) {
	if (mouseButton == MouseButton::Left) {
		m_IsMouseDown = true;
		m_MouseX = x;
		m_MouseY = y;
	}
}
void NetworkViewerHandler::OnMouseMove(Control&, int x, int y) {
	if (m_IsMouseDown) {
		m_MovedX += SAM_ROUND((x - m_MouseX) / SAM_ZOOM);
		m_MovedY += SAM_ROUND((y - m_MouseY) / SAM_ZOOM);

		m_MouseX = x;
		m_MouseY = y;

		m_Panel->Invalidate();
	}
}
void NetworkViewerHandler::OnMouseUp(Control&, int, int, MouseButton mouseButton) {
	if (mouseButton == MouseButton::Left) {
		m_IsMouseDown = false;
	}
}
void NetworkViewerHandler::OnMouseWheel(Control&, int, int, MouseWheel mouseWheel) {
	const int oldZoomLevel = m_ZoomLevel;

	if (mouseWheel == MouseWheel::Forward) {
		m_ZoomLevel = std::min(2, m_ZoomLevel + 1);
	} else {
		m_ZoomLevel = std::max(-2, m_ZoomLevel - 1);
	}

	if (oldZoomLevel != m_ZoomLevel) {
		m_Panel->Invalidate();
	}
}

void NetworkViewerHandler::UpdateTargetNetworkDump(const Network& targetNetwork) {
	if (targetNetwork.GetLayerCount() > 0 && targetNetwork.GetInputSize() > 0) {
		m_TargetNetworkDump = targetNetwork.GetDump();
	} else {
		m_TargetNetworkDump = std::nullopt;
	}

	m_Panel->Invalidate();
}