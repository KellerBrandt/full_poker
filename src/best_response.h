#pragma once

#include <cstddef>
#include <limits>
#include <unordered_map>
#include <vector>

#include "game.h"
#include "policy.h"

// full tree traversal, small games only for now
template <Game G, Policy<G> P>
class BestResponse {
public:
	BestResponse(const P& policy, Player br_player) : policy_(policy), br_player_(br_player) {
		collect_infoset_states(G::initial_state(), 1.0);
	}

	double value() { return state_value(G::initial_state()); }

private:
	using State = typename G::State;
	using InfoSetKey = typename G::InfoSetKey;

	struct WeightedState {
		State state;
		// chance and opponent probability of reaching state. br_player's own choices excluded
		double reach;
	};

	void collect_infoset_states(const State& state, double reach) {
		Player player = state.current_player();

		if (player == kTerminalPlayer) {
			return;
		}

		if (player == kChancePlayer) {
			for (ChanceOutcome outcome : state.chance_outcomes()) {
				State child = state;
				child.apply(outcome.action);
				collect_infoset_states(child, reach * outcome.prob);
			}

			return;
		}

		if (player == br_player_) {
			infoset_states_[state.infoset_key(player)].push_back({state, reach});

			for (Action action : state.legal_actions()) {
				State child = state;
				child.apply(action);
				collect_infoset_states(child, reach);
			}

			return;
		}

		auto actions = state.legal_actions();
		auto probs = policy_.action_probs(state);

		for (std::size_t i = 0; i < actions.size(); ++i) {
			State child = state;
			child.apply(actions[i]);
			collect_infoset_states(child, reach * probs[i]);
		}
	}

	double state_value(const State& state) {
		Player player = state.current_player();

		if (player == kTerminalPlayer) {
			return state.utility(br_player_);
		}

		if (player == kChancePlayer) {
			double total = 0;

			for (ChanceOutcome outcome : state.chance_outcomes()) {
				State child = state;
				child.apply(outcome.action);
				total += outcome.prob * state_value(child);
			}

			return total;
		}

		if (player == br_player_) {
			Action action = best_action(state.infoset_key(player));
			State child = state;
			child.apply(action);

			return state_value(child);
		}

		auto actions = state.legal_actions();
		auto probs = policy_.action_probs(state);
		double total = 0;

		for (std::size_t i = 0; i < actions.size(); ++i) {
			State child = state;
			child.apply(actions[i]);
			total += probs[i] * state_value(child);
		}

		return total;
	}

	// action maximizing reach-weighted value summed over every state in the infoset
	Action best_action(InfoSetKey key) {
		auto it = best_actions_.find(key);

		if (it != best_actions_.end()) {
			return it->second;
		}

		const std::vector<WeightedState>& states = infoset_states_.at(key);
		Action best = 0;
		double best_ev = -std::numeric_limits<double>::infinity();

		for (Action action : states[0].state.legal_actions()) {
			double ev = 0;

			for (const WeightedState& weighted_state : states) {
				State child = weighted_state.state;
				child.apply(action);
				ev += weighted_state.reach * state_value(child);
			}

			if (ev > best_ev) {
				best_ev = ev;
				best = action;
			}
		}

		best_actions_[key] = best;

		return best;
	}

	const P& policy_;
	Player br_player_;
	std::unordered_map<InfoSetKey, std::vector<WeightedState>> infoset_states_;
	std::unordered_map<InfoSetKey, Action> best_actions_;
};
