#include "Optimizer.hpp"

#include <cassert>

void Optimizer::Attach(Network* network) noexcept {
	assert(m_TargetNetwork == nullptr);

	m_TargetNetwork = network;
}
Network* Optimizer::GetTargetNetwork() noexcept {
	return m_TargetNetwork;
}