#pragma once

#include "Network.hpp"

class Optimizer {
private:
	Network* m_TargetNetwork = nullptr;

public:
	Optimizer() noexcept = default;
	Optimizer(const Optimizer&) = delete;
	virtual ~Optimizer() = default;

public:
	Optimizer& operator=(const Optimizer&) = delete;

public:
	void Attach(Network* network) noexcept;
	Network* GetTargetNetwork() noexcept;

	virtual void Optimize() = 0;
};