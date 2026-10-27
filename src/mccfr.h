#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>

#include "fixed_list.h"
#include "game.h"
#include "infoset_table.h"
#include "regret_matching.h"
#include "sampling.h"

// external sampling
template <Game G>
class MCCFR {
public:
	struct Entry {
		std::array<double, G::kMaxActions> regret_sum = {};
		std::array<double, G::kMaxActions> strategy_sum = {};
	};

	explicit MCCFR(uint64_t seed = 0) : rng_(seed) {}

	void iterate() {
		for (Player p = 0; p < G::kNumPlayers; ++p) {
			traverse(G::initial_state(), p);
		}
	}

	const InfoSetTable<G, Entry>& infoset_table() const { return infoset_table_; }

	uint64_t nodes_visited() const { return nodes_visited_; }

private:
	using State = typename G::State;
	using Strategy = FixedList<double, G::kMaxActions>;

	double traverse(const State& state, Player updating_player) {
		++nodes_visited_;

		Player player = state.current_player();

		if (player == kTerminalPlayer) {
			return state.utility(updating_player);
		}

		if (player == kChancePlayer) {
			auto outcomes = state.chance_outcomes();
			std::size_t i = sample_index(outcomes, [](const ChanceOutcome& outcome) { return outcome.prob; }, rng_);

			State child = state;
			child.apply(outcomes[i].action);

			return traverse(child, updating_player);
		}

		auto actions = state.legal_actions();
		Entry& entry = infoset_table_[state.infoset_key(player)];
		Strategy strategy = regret_matching(entry.regret_sum, actions.size());

		if (player != updating_player) {
			for (std::size_t i = 0; i < actions.size(); ++i) {
				entry.strategy_sum[i] += strategy[i];
			}

			std::size_t i = sample_index(strategy, [](double prob) { return prob; }, rng_);

			State child = state;
			child.apply(actions[i]);

			return traverse(child, updating_player);
		}

		double utility = 0;
		std::array<double, G::kMaxActions> action_utilities;

		for (std::size_t i = 0; i < actions.size(); ++i) {
			State child = state;
			child.apply(actions[i]);

			action_utilities[i] = traverse(child, updating_player);
			utility += strategy[i] * action_utilities[i];
		}

		for (std::size_t i = 0; i < actions.size(); ++i) {
			entry.regret_sum[i] += action_utilities[i] - utility;
		}

		return utility;
	}

	InfoSetTable<G, Entry> infoset_table_;
	std::mt19937_64 rng_;
	uint64_t nodes_visited_ = 0;
};
