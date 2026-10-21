#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

#include "fixed_list.h"

template <std::size_t N>
FixedList<double, N> regret_matching(const std::array<double, N>& regret_sum, std::size_t num_actions) {
	double positive_regret_sum = 0;

	for (std::size_t i = 0; i < num_actions; ++i) {
		positive_regret_sum += std::max(regret_sum[i], 0.0);
	}

	FixedList<double, N> strategy;

	for (std::size_t i = 0; i < num_actions; ++i) {
		double positive_regret = std::max(regret_sum[i], 0.0);
		strategy.push_back(positive_regret_sum > 0 ? positive_regret / positive_regret_sum : 1.0 / num_actions);
	}

	return strategy;
}
