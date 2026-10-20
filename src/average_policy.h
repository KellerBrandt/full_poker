#pragma once

#include <cstddef>

#include "fixed_list.h"
#include "game.h"
#include "infoset_table.h"

template <Game G>
class AveragePolicy {
public:
	explicit AveragePolicy(const InfoSetTable<G>& table) : table_(table) {}

	FixedList<double, G::kMaxActions> action_probs(const typename G::State& state) const {
		std::size_t num_actions = state.legal_actions().size();
		Player player = state.current_player();
		const Entry* entry = table_.find(state.infoset_key(player));

		double strategy_sum_total = 0;

		if (entry != nullptr) {
			for (std::size_t i = 0; i < num_actions; ++i) {
				strategy_sum_total += entry->strategy_sum[i];
			}
		}

		FixedList<double, G::kMaxActions> probs;

		for (std::size_t i = 0; i < num_actions; ++i) {
			probs.push_back(strategy_sum_total > 0 ? entry->strategy_sum[i] / strategy_sum_total : 1.0 / num_actions);
		}

		return probs;
	}

private:
	using Entry = typename InfoSetTable<G>::Entry;

	const InfoSetTable<G>& table_;
};
