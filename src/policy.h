#pragma once

#include <concepts>
#include <cstddef>

#include "fixed_list.h"
#include "game.h"

// probs in same order as state.legal_actions()
template <typename P, typename G>
concept Policy = Game<G> && requires(const P& policy, const typename G::State& state) {
	{ policy.action_probs(state) } -> std::same_as<FixedList<double, G::kMaxActions>>;
};

template <Game G>
struct UniformPolicy {
	FixedList<double, G::kMaxActions> action_probs(const typename G::State& state) const {
		std::size_t num_actions = state.legal_actions().size();
		double prob = 1.0 / num_actions;

		FixedList<double, G::kMaxActions> probs;

		for (std::size_t i = 0; i < num_actions; ++i) {
			probs.push_back(prob);
		}

		return probs;
	}
};
