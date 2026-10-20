#pragma once

#include <bit>
#include <cassert>
#include <cstdint>

#include "fixed_list.h"
#include "game.h"

struct Leduc {
	static constexpr int kNumPlayers = 2;
	static constexpr int kMaxActions = 3;
	static constexpr int kNumRanks = 3;
	static constexpr int kNumSuits = 2;
	static constexpr int kAnte = 1;
	static constexpr int kMaxRaisesPerRound = 2;
	static constexpr int kRaiseSize[2] = {2, 4};

	static constexpr Action kFold = 0;
	static constexpr Action kCall = 1;
	static constexpr Action kRaise = 2;

	using InfoSetKey = uint64_t;

	struct State {
		int8_t private_cards[2] = {};
		int8_t public_card = -1;
		int8_t num_dealt = 0;
		int8_t round = 0;
		int8_t num_raises = 0;
		int8_t folded_player = -1;
		int16_t contributions[2] = {kAnte, kAnte};
		// 2 bits per action after a leading 1, one history per round
		uint16_t history[2] = {1, 1};

		int num_actions_this_round() const {
			return (std::bit_width(history[round]) - 1) / 2;
		}

		bool round_over() const {
			Action last_action = history[round] & 0b11;

			return num_actions_this_round() >= 2 && last_action == kCall;
		}

		Player current_player() const {
			if (num_dealt < 2 || (round == 1 && public_card < 0)) {
				return kChancePlayer;
			}

			if (folded_player >= 0 || round_over()) {
				return kTerminalPlayer;
			}

			return num_actions_this_round() % 2;
		}

		FixedList<Action, kMaxActions> legal_actions() const {
			assert(current_player() >= 0);

			FixedList<Action, kMaxActions> actions;

			if (contributions[0] != contributions[1]) {
				actions.push_back(kFold);
			}

			actions.push_back(kCall);

			if (num_raises < kMaxRaisesPerRound) {
				actions.push_back(kRaise);
			}

			return actions;
		}

		FixedList<ChanceOutcome, kNumRanks> chance_outcomes() const {
			assert(current_player() == kChancePlayer);

			int copies_left[kNumRanks];

			for (int rank = 0; rank < kNumRanks; ++rank) {
				copies_left[rank] = kNumSuits;
			}

			// only private cards have been dealt at any chance node
			for (int i = 0; i < num_dealt; ++i) {
				--copies_left[private_cards[i]];
			}

			double cards_left = kNumRanks * kNumSuits - num_dealt;
			FixedList<ChanceOutcome, kNumRanks> outcomes;

			for (int rank = 0; rank < kNumRanks; ++rank) {
				if (copies_left[rank] > 0) {
					outcomes.push_back({rank, copies_left[rank] / cards_left});
				}
			}

			return outcomes;
		}

		void apply(Action action) {
			Player player = current_player();

			if (player == kChancePlayer) {
				if (num_dealt < 2) {
					private_cards[num_dealt] = static_cast<int8_t>(action);
				} else {
					public_card = static_cast<int8_t>(action);
				}

				++num_dealt;

				return;
			}

			assert(player >= 0);

			history[round] = static_cast<uint16_t>((history[round] << 2) | action);

			if (action == kFold) {
				folded_player = static_cast<int8_t>(player);

				return;
			}

			Player opponent = 1 - player;

			if (action == kCall) {
				contributions[player] = contributions[opponent];
			} else {
				contributions[player] = static_cast<int16_t>(contributions[opponent] + kRaiseSize[round]);
				++num_raises;
			}

			if (round == 0 && round_over()) {
				round = 1;
				num_raises = 0;
			}
		}

		// any pair beats any high card
		int hand_strength(Player player) const {
			int rank = private_cards[player];

			return rank == public_card ? kNumRanks + rank : rank;
		}

		double utility(Player player) const {
			assert(current_player() == kTerminalPlayer);

			Player opponent = 1 - player;

			if (folded_player >= 0) {
				return folded_player == player ? -contributions[player] : contributions[opponent];
			}

			int strength = hand_strength(player);
			int opponent_strength = hand_strength(opponent);

			if (strength == opponent_strength) {
				return 0;
			}

			return strength > opponent_strength ? contributions[opponent] : -contributions[player];
		}

		// bits 0-1: private card, 2-3: public card + 1 (0 = not dealt)
		// bits 4-12: round 0 history, 13-21: round 1 history
		InfoSetKey infoset_key(Player player) const {
			InfoSetKey key = static_cast<InfoSetKey>(private_cards[player]);
			key |= static_cast<InfoSetKey>(public_card + 1) << 2;
			key |= static_cast<InfoSetKey>(history[0]) << 4;
			key |= static_cast<InfoSetKey>(history[1]) << 13;

			return key;
		}
	};

	static State initial_state() { return {}; }
};

static_assert(Game<Leduc>);
