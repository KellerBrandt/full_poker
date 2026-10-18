#pragma once

#include <bit>
#include <cassert>
#include <cstdint>

#include "fixed_list.h"
#include "game.h"

struct Kuhn {
	static constexpr int kNumPlayers = 2;
	static constexpr int kMaxActions = 2;
	static constexpr int kNumCards = 3;
	static constexpr Action kPass = 0;
	static constexpr Action kBet = 1;

	using InfoSetKey = uint64_t;

	struct State {
		int8_t cards[2] = {};
		int8_t num_dealt = 0;
		// one bit per action after a leading 1: "" = 0b1, "pb" = 0b101
		uint8_t history = 0b1;

		bool is_terminal() const {
			return history == 0b100 || history == 0b110 || history == 0b111 || history >= 0b1000;
		}

		Player current_player() const {
			if (num_dealt < 2) {
				return kChancePlayer;
			}

			if (is_terminal()) {
				return kTerminalPlayer;
			}

			return static_cast<Player>((std::bit_width(history) - 1) % 2);
		}

		FixedList<Action, kMaxActions> legal_actions() const {
			assert(current_player() >= 0);

			FixedList<Action, kMaxActions> actions;

			actions.push_back(kPass);
			actions.push_back(kBet);

			return actions;
		}

		FixedList<ChanceOutcome, kNumCards> chance_outcomes() const {
			assert(current_player() == kChancePlayer);

			FixedList<ChanceOutcome, kNumCards> outcomes;
			double prob = 1.0 / (kNumCards - num_dealt);

			for (int card = 0; card < kNumCards; ++card) {
				if (num_dealt == 0 || card != cards[0]) {
					outcomes.push_back({card, prob});
				}
			}

			return outcomes;
		}

		void apply(Action action) {
			if (num_dealt < 2) {
				cards[num_dealt++] = static_cast<int8_t>(action);
			} else {
				assert(!is_terminal());

				history = static_cast<uint8_t>((history << 1) | action);
			}
		}

		double utility(Player player) const {
			assert(current_player() == kTerminalPlayer);

			double p0;

			if (history == 0b110) {
				// bp: p1 folds
				p0 = 1;
			} else if (history == 0b1010) {
				// pbp: p0 folds
				p0 = -1;
			} else {
				double stake = history == 0b100 ? 1 : 2;
				p0 = cards[0] > cards[1] ? stake : -stake;
			}

			return player == 0 ? p0 : -p0;
		}

		InfoSetKey infoset_key(Player player) const {
			return static_cast<InfoSetKey>(cards[player]) * 16 + history;
		}
	};

	static State initial_state() { return {}; }
};

static_assert(Game<Kuhn>);
