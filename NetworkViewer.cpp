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

NetworkViewerHandler::NetworkViewerHandler(const Network& targetNetwork) noexcept
	: m_TargetNetwork(&targetNetwork) {}

void NetworkViewerHandler::OnCreate(Control& control) {
	m_Panel = &dynamic_cast<Panel&>(control);

	m_CloudBrush = SolidBrushRef({ 236, 240, 241 });
	m_BelizeHoleBrush = SolidBrushRef({ 41, 128, 185 });
}
void NetworkViewerHandler::OnPaint(Control&, Graphics& graphics) {
	const auto ctx = graphics.GetContext2D();

	const std::size_t layerCount = m_TargetNetwork->GetLayerCount();

	if (layerCount == 0) return;

	const int networkInputSize = static_cast<int>(m_TargetNetwork->GetInputSize());

	if (layerCount == 0) return;

	std::vector<std::tuple<int, int, int, int, int>> layers;
	std::vector<std::vector<std::vector<float>>> lines;

	layers.push_back({ m_MovedX, 0,
		SAM_LAYERWIDTH(networkInputSize), SAM_LAYERHEIGHT(networkInputSize), networkInputSize });

	int x = std::get<0>(layers[0]) + std::get<2>(layers[0]);
	int maxHeight = std::get<3>(layers[0]);

	for (std::size_t i = 0; i < layerCount; ++i) {
		const Layer& layer = m_TargetNetwork->GetLayer(i);
		const std::string_view layerName = layer.GetName();

		const int unitCount = static_cast<int>(m_TargetNetwork->GetOutputSize(i));
		const int width = SAM_LAYERWIDTH(unitCount);
		const int height = SAM_LAYERHEIGHT(unitCount);

		const std::size_t prevUnitCount = std::get<4>(layers.back());
		auto& layerLines = lines.emplace_back();

		if (layerName == "FCLayer") {
			const FCLayer& fcLayer = static_cast<const FCLayer&>(layer);
			float maxWeight = 0;

			for (std::size_t j = 0; j < prevUnitCount; ++j) {
				auto& unitLines = layerLines.emplace_back();
				const ReadonlyParameter weights = fcLayer.GetParameterTable().GetParameter("Weights");
				const Matrix& weightsMatrix = weights.GetValue();

				for (std::size_t k = 0; k < unitCount; ++k) {
					const float weight = weightsMatrix(k, j);

					unitLines.push_back(weight);

					maxWeight = std::max(maxWeight, std::fabsf(weight));
				}
			}

			for (auto& unit : layerLines) {
				for (auto& weight : unit) {
					weight = std::fabsf(weight) / maxWeight;
				}
			}
		} else if (layerName == "ALayer") {
			for (std::size_t j = 0; j < prevUnitCount; ++j) {
				layerLines.push_back(std::vector(unitCount, 0.f));
				layerLines.back()[j] = 1;
			}
		}

		layers.push_back({ x + SAM_LAYERINTERVAL, 0, width, height, unitCount });

		x += width + SAM_LAYERINTERVAL;
		maxHeight = std::max(maxHeight, height);
	}

	for (auto& [x, y, width, height, unitCount] : layers) {
		y = SAM_ROUND(m_MovedY + (maxHeight - height) / 2.f);

		ctx->SetBrush(m_CloudBrush);
		ctx->FillRectangle(SAM_MAGNIFY(x), SAM_MAGNIFY(y), SAM_MAGNIFY(width), SAM_MAGNIFY(height));

		ctx->SetBrush(m_BelizeHoleBrush);
		for (int i = 0; i < unitCount; ++i) {
			ctx->FillEllipse(SAM_MAGNIFY(x + SAM_UNITX(i)), SAM_MAGNIFY(y + SAM_UNITY(i)),
				SAM_MAGNIFY(SAM_UNITSIZE), SAM_MAGNIFY(SAM_UNITSIZE));
		}
	}

	for (std::size_t i = 0; i < lines.size(); ++i) {
		const auto& layerLines = lines[i];

		for (std::size_t j = 0; j < layerLines.size(); ++j) {
			const auto& unitLines = layerLines[j];

			for (std::size_t k = 0; k < unitLines.size(); ++k) {
				const float width = unitLines[k];

				if (width == 0) continue;

				ctx->SetPen(SolidPenRef(Color::Black,
					(SAM_LINEMINWIDTH + width * (SAM_LINEMAXWIDTH - SAM_LINEMINWIDTH)) * SAM_ZOOM));
				ctx->DrawLine(SAM_MAGNIFY(std::get<0>(layers[i]) + SAM_UNITX(static_cast<int>(j)) + SAM_UNITSIZE),
					SAM_MAGNIFY(std::get<1>(layers[i]) + SAM_UNITY(static_cast<int>(j)) + SAM_UNITSIZE / 2),
					SAM_MAGNIFY(std::get<0>(layers[i + 1]) + SAM_UNITX(static_cast<int>(k))),
					SAM_MAGNIFY(std::get<1>(layers[i + 1]) + SAM_UNITY(static_cast<int>(k)) + SAM_UNITSIZE / 2));
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

void NetworkViewerHandler::SetTargetNetwork(const Network& newTargetNetwork) {
	m_TargetNetwork = &newTargetNetwork;

	m_Panel->Invalidate();
}