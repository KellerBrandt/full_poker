#pragma once

#include <cmath>
#include <cstdio>
#include <set>
#include <source_location>
#include <utility>

#include "game.h"

inline int failures = 0;

inline void check(bool cond, std::source_location loc = std::source_location::current()) {
	if (!cond) {
		std::fprintf(stderr, "%s:%u: check failed\n", loc.file_name(), loc.line());
		++failures;
	}
}

template <Game G>
struct TreeStats {
	int terminals = 0;
	std::set<std::pair<Player, typename G::InfoSetKey>> infosets;
};

template <Game G>
void walk_tree(const typename G::State& state, TreeStats<G>& stats) {
	Player player = state.current_player();

	if (player == kTerminalPlayer) {
		++stats.terminals;

		double utility_sum = 0;

		for (Player p = 0; p < G::kNumPlayers; ++p) {
			utility_sum += state.utility(p);
		}

		check(std::abs(utility_sum) < 1e-12);

		return;
	}

	if (player == kChancePlayer) {
		double prob_sum = 0;

		for (ChanceOutcome outcome : state.chance_outcomes()) {
			prob_sum += outcome.prob;

			typename G::State child = state;
			child.apply(outcome.action);
			walk_tree<G>(child, stats);
		}

		check(std::abs(prob_sum - 1.0) < 1e-12);

		return;
	}

	stats.infosets.insert({player, state.infoset_key(player)});

	for (Action action : state.legal_actions()) {
		typename G::State child = state;
		child.apply(action);
		walk_tree<G>(child, stats);
	}
}
