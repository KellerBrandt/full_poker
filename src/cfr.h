#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include "fixed_list.h"
#include "game.h"
#include "infoset_table.h"

// vanillaCFR
template <Game G>
class CFR {
public:
	void iterate() {
		Values reach;
		reach.fill(1.0);

		traverse(G::initial_state(), reach, 1.0);

		for (auto& [key, entry] : table_) {
			for (int i = 0; i < G::kMaxActions; ++i) {
				entry.regret_sum[i] += entry.iteration_regret[i];
				entry.iteration_regret[i] = 0;
			}
		}
	}

	const InfoSetTable<G>& table() const { return table_; }

	uint64_t nodes_visited() const { return nodes_visited_; }

private:
	using State = typename G::State;
	using Entry = typename InfoSetTable<G>::Entry;
	using Values = std::array<double, G::kNumPlayers>;
	using Strategy = FixedList<double, G::kMaxActions>;

	static Strategy regret_matching(const Entry& entry, std::size_t num_actions) {
		double positive_sum = 0;

		for (std::size_t i = 0; i < num_actions; ++i) {
			positive_sum += std::max(entry.regret_sum[i], 0.0);
		}

		Strategy strategy;

		for (std::size_t i = 0; i < num_actions; ++i) {
			double positive_regret = std::max(entry.regret_sum[i], 0.0);
			strategy.push_back(positive_sum > 0 ? positive_regret / positive_sum : 1.0 / num_actions);
		}

		return strategy;
	}

	Values traverse(const State& state, const Values& reach, double chance_reach) {
		if (std::none_of(reach.begin(), reach.end(), [](double r) { return r > 0; })) {
			return {};
		}

		++nodes_visited_;

		Player player = state.current_player();

		if (player == kTerminalPlayer) {
			Values values;

			for (Player p = 0; p < G::kNumPlayers; ++p) {
				values[p] = state.utility(p);
			}

			return values;
		}

		if (player == kChancePlayer) {
			Values values = {};

			for (ChanceOutcome outcome : state.chance_outcomes()) {
				State child = state;
				child.apply(outcome.action);

				Values child_values = traverse(child, reach, chance_reach * outcome.prob);

				for (Player p = 0; p < G::kNumPlayers; ++p) {
					values[p] += outcome.prob * child_values[p];
				}
			}

			return values;
		}

		auto actions = state.legal_actions();
		Entry& entry = table_[state.infoset_key(player)];
		Strategy strategy = regret_matching(entry, actions.size());

		Values values = {};
		std::array<Values, G::kMaxActions> action_values;

		for (std::size_t i = 0; i < actions.size(); ++i) {
			State child = state;
			child.apply(actions[i]);

			Values child_reach = reach;
			child_reach[player] *= strategy[i];
			action_values[i] = traverse(child, child_reach, chance_reach);

			for (Player p = 0; p < G::kNumPlayers; ++p) {
				values[p] += strategy[i] * action_values[i][p];
			}
		}

		double counterfactual_reach = chance_reach;

		for (Player p = 0; p < G::kNumPlayers; ++p) {
			if (p != player) {
				counterfactual_reach *= reach[p];
			}
		}

		for (std::size_t i = 0; i < actions.size(); ++i) {
			double regret = action_values[i][player] - values[player];
			entry.iteration_regret[i] += counterfactual_reach * regret;
			entry.strategy_sum[i] += reach[player] * strategy[i];
		}

		return values;
	}

	InfoSetTable<G> table_;
	uint64_t nodes_visited_ = 0;
};
