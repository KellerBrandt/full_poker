#pragma once

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstdint>

#include "cards.h"
#include "fixed_list.h"
#include "game.h"
#include "hand_evaluator.h"

// player 0 is the button and small blind: first to act preflop, second after
struct LimitHoldem {
	static constexpr int kNumPlayers = 2;
	static constexpr int kMaxActions = 3;
	static constexpr int kNumStreets = 4;
	static constexpr int kNumBoardCards = 5;
	static constexpr int kSmallBlind = 1;
	static constexpr int kBigBlind = 2;
	static constexpr int kBetSize[kNumStreets] = {2, 2, 4, 4};
	static constexpr int kMaxBetsPerStreet = 4;
	static constexpr int kCardsDealtByStreet[kNumStreets] = {4, 7, 8, 9};

	static constexpr Action kFold = 0;
	static constexpr Action kCall = 1;
	static constexpr Action kRaise = 2;

	struct InfoSetKey {
		uint64_t cards;
		uint64_t history;

		bool operator==(const InfoSetKey&) const = default;
	};

	struct State {
		int8_t hole_cards[kNumPlayers][2] = {};
		int8_t board[kNumBoardCards] = {};
		CardSet dealt_cards = 0;
		int8_t num_dealt = 0;
		int8_t street = 0;
		int8_t num_bets = 1;
		int8_t folded_player = -1;
		int16_t contributions[kNumPlayers] = {kSmallBlind, kBigBlind};
		// 2 bits per action after a leading 1, one history per street
		uint16_t history[kNumStreets] = {1, 1, 1, 1};

		int num_actions_this_street() const {
			return (std::bit_width(history[street]) - 1) / 2;
		}

		bool street_over() const {
			Action last_action = history[street] & 0b11;

			return num_actions_this_street() >= 2 && last_action == kCall;
		}

		Player current_player() const {
			if (num_dealt < kCardsDealtByStreet[street]) {
				return kChancePlayer;
			}

			if (folded_player >= 0 || street_over()) {
				return kTerminalPlayer;
			}

			Player first_to_act = street == 0 ? 0 : 1;

			return (first_to_act + num_actions_this_street()) % 2;
		}
		FixedList<Action, kMaxActions> legal_actions() const {
			assert(current_player() >= 0);

			FixedList<Action, kMaxActions> actions;

			if (contributions[0] != contributions[1]) {
				actions.push_back(kFold);
			}

			actions.push_back(kCall);

			if (num_bets < kMaxBetsPerStreet) {
				actions.push_back(kRaise);
			}

			return actions;
		}
		FixedList<ChanceOutcome, kNumCards> chance_outcomes() const {
			assert(current_player() == kChancePlayer);

			FixedList<ChanceOutcome, kNumCards> outcomes;
			double prob = 1.0 / (kNumCards - num_dealt);

			for (Card card = 0; card < kNumCards; ++card) {
				if ((dealt_cards & card_bit(card)) == 0) {
					outcomes.push_back({card, prob});
				}
			}

			return outcomes;
		}
		void apply(Action action) {
			Player player = current_player();

			if (player == kChancePlayer) {
				if (num_dealt < kCardsDealtByStreet[0]) {
					hole_cards[num_dealt / 2][num_dealt % 2] = static_cast<int8_t>(action);
				} else {
					board[num_dealt - kCardsDealtByStreet[0]] = static_cast<int8_t>(action);
				}

				dealt_cards |= card_bit(action);
				++num_dealt;

				return;
			}

			assert(player >= 0);

			history[street] = static_cast<uint16_t>((history[street] << 2) | action);

			if (action == kFold) {
				folded_player = static_cast<int8_t>(player);

				return;
			}

			Player opponent = 1 - player;

			if (action == kCall) {
				contributions[player] = contributions[opponent];
			} else {
				contributions[player] = static_cast<int16_t>(contributions[opponent] + kBetSize[street]);
				++num_bets;
			}

			if (street < kNumStreets - 1 && street_over()) {
				++street;
				num_bets = 0;
			}
		}
		CardSet hole_card_set(Player player) const {
			return card_bit(hole_cards[player][0]) | card_bit(hole_cards[player][1]);
		}

		double utility(Player player) const {
			assert(current_player() == kTerminalPlayer);

			Player opponent = 1 - player;

			if (folded_player >= 0) {
				return folded_player == player ? -contributions[player] : contributions[opponent];
			}

			CardSet board_cards = dealt_cards & ~hole_card_set(0) & ~hole_card_set(1);
			HandStrength strength = evaluate_hand(board_cards | hole_card_set(player));
			HandStrength opponent_strength = evaluate_hand(board_cards | hole_card_set(opponent));

			if (strength == opponent_strength) {
				return 0;
			}

			return strength > opponent_strength ? contributions[opponent] : -contributions[player];
		}
		InfoSetKey infoset_key(Player player) const {
			constexpr uint64_t kNotDealt = 63;

			uint64_t card_slots[2 + kNumBoardCards];
			card_slots[0] = std::min(hole_cards[player][0], hole_cards[player][1]);
			card_slots[1] = std::max(hole_cards[player][0], hole_cards[player][1]);

			int num_board_dealt = num_dealt - kCardsDealtByStreet[0];

			for (int i = 0; i < kNumBoardCards; ++i) {
				card_slots[2 + i] = i < num_board_dealt ? board[i] : kNotDealt;
			}

			std::sort(card_slots + 2, card_slots + 5);

			InfoSetKey key = {0, 0};

			for (int i = 0; i < 2 + kNumBoardCards; ++i) {
				key.cards |= card_slots[i] << (6 * i);
			}

			for (int s = 0; s < kNumStreets; ++s) {
				key.history |= static_cast<uint64_t>(history[s]) << (13 * s);
			}

			return key;
		}
	};

	static State initial_state() { return {}; }
};

static_assert(Game<LimitHoldem>);
