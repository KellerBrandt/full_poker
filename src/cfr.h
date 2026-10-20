#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include "fixed_list.h"
#include "game.h"
#include "infoset_table.h"

enum class Averaging { Uniform, Linear };

struct CFROptions {
	bool regret_matching_plus = false;
	Averaging averaging = Averaging::Uniform;
	bool alternating_updates = false;
};

// vanilla CFR with default options
template <Game G, CFROptions Options = CFROptions{}>
class CFR {
public:
	void iterate() {
		++iteration_;

		Reach root_reach;
		root_reach.fill(1.0);

		if constexpr (Options.alternating_updates) {
			for (Player p = 0; p < G::kNumPlayers; ++p) {
				updating_player_ = p;
				traverse(G::initial_state(), root_reach, 1.0);
				apply_iteration_regret();
			}
		} else {
			traverse(G::initial_state(), root_reach, 1.0);
			apply_iteration_regret();
		}
	}

	const InfoSetTable<G>& table() const { return table_; }

	uint64_t nodes_visited() const { return nodes_visited_; }

private:
	using State = typename G::State;
	using Entry = typename InfoSetTable<G>::Entry;
	using Reach = std::array<double, G::kNumPlayers>;
	using Utilities = std::array<double, G::kNumPlayers>;
	using Strategy = FixedList<double, G::kMaxActions>;

	void apply_iteration_regret() {
		for (auto& [key, entry] : table_) {
			for (int i = 0; i < G::kMaxActions; ++i) {
				entry.regret_sum[i] += entry.iteration_regret[i];
				entry.iteration_regret[i] = 0;

				if constexpr (Options.regret_matching_plus) {
					entry.regret_sum[i] = std::max(entry.regret_sum[i], 0.0);
				}
			}
		}
	}

	static Strategy regret_matching(const Entry& entry, std::size_t num_actions) {
		double positive_regret_sum = 0;

		for (std::size_t i = 0; i < num_actions; ++i) {
			positive_regret_sum += std::max(entry.regret_sum[i], 0.0);
		}

		Strategy strategy;

		for (std::size_t i = 0; i < num_actions; ++i) {
			double positive_regret = std::max(entry.regret_sum[i], 0.0);
			strategy.push_back(positive_regret_sum > 0 ? positive_regret / positive_regret_sum : 1.0 / num_actions);
		}

		return strategy;
	}

	Utilities traverse(const State& state, const Reach& reach, double chance_reach) {
		if (std::none_of(reach.begin(), reach.end(), [](double r) { return r > 0; })) {
			return {};
		}

		++nodes_visited_;

		Player player = state.current_player();

		if (player == kTerminalPlayer) {
			Utilities utilities;

			for (Player p = 0; p < G::kNumPlayers; ++p) {
				utilities[p] = state.utility(p);
			}

			return utilities;
		}

		if (player == kChancePlayer) {
			Utilities utilities = {};

			for (ChanceOutcome outcome : state.chance_outcomes()) {
				State child = state;
				child.apply(outcome.action);

				Utilities child_utilities = traverse(child, reach, chance_reach * outcome.prob);

				for (Player p = 0; p < G::kNumPlayers; ++p) {
					utilities[p] += outcome.prob * child_utilities[p];
				}
			}

			return utilities;
		}

		auto actions = state.legal_actions();
		Entry& entry = table_[state.infoset_key(player)];
		Strategy strategy = regret_matching(entry, actions.size());

		Utilities utilities = {};
		std::array<Utilities, G::kMaxActions> action_utilities;

		for (std::size_t i = 0; i < actions.size(); ++i) {
			State child = state;
			child.apply(actions[i]);

			Reach child_reach = reach;
			child_reach[player] *= strategy[i];
			action_utilities[i] = traverse(child, child_reach, chance_reach);

			for (Player p = 0; p < G::kNumPlayers; ++p) {
				utilities[p] += strategy[i] * action_utilities[i][p];
			}
		}

		if constexpr (Options.alternating_updates) {
			if (player != updating_player_) {
				return utilities;
			}
		}

		double counterfactual_reach = chance_reach;

		for (Player p = 0; p < G::kNumPlayers; ++p) {
			if (p != player) {
				counterfactual_reach *= reach[p];
			}
		}

		double strategy_weight = Options.averaging == Averaging::Linear ? static_cast<double>(iteration_) : 1.0;

		for (std::size_t i = 0; i < actions.size(); ++i) {
			double regret = action_utilities[i][player] - utilities[player];
			entry.iteration_regret[i] += counterfactual_reach * regret;
			entry.strategy_sum[i] += strategy_weight * reach[player] * strategy[i];
		}

		return utilities;
	}

	InfoSetTable<G> table_;
	uint64_t nodes_visited_ = 0;
	uint64_t iteration_ = 0;
	Player updating_player_ = 0;
};

template <Game G>
using CFRPlus = CFR<G, CFROptions{
	.regret_matching_plus = true,
	.averaging = Averaging::Linear,
	.alternating_updates = true,
}>;
