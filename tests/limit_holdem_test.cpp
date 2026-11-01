#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <random>

#include "cards.h"
#include "limit_holdem.h"
#include "sampling.h"
#include "test_utils.h"

using State = LimitHoldem::State;

static constexpr int kTwo = 0;
static constexpr int kThree = 1;
static constexpr int kSeven = 5;
static constexpr int kNine = 7;
static constexpr int kTen = 8;
static constexpr int kJack = 9;
static constexpr int kQueen = 10;
static constexpr int kKing = 11;
static constexpr int kAce = 12;

static State deal(std::initializer_list<Card> cards) {
	State state = LimitHoldem::initial_state();

	for (Card card : cards) {
		state.apply(card);
	}

	return state;
}

static void check_down(State& state, std::initializer_list<Card> board) {
	const Card* next_board_card = board.begin();

	while (state.current_player() != kTerminalPlayer) {
		if (state.current_player() == kChancePlayer) {
			state.apply(*next_board_card);
			++next_board_card;
		} else {
			state.apply(LimitHoldem::kCall);
		}
	}
}

static void test_preflop_fold() {
	State state = deal({make_card(kAce, 0), make_card(kAce, 1), make_card(kKing, 0), make_card(kKing, 1)});

	check(state.current_player() == 0);

	state.apply(LimitHoldem::kFold);

	check(state.current_player() == kTerminalPlayer);
	check(state.utility(0) == -LimitHoldem::kSmallBlind);
	check(state.utility(1) == LimitHoldem::kSmallBlind);
}

static void test_preflop_cap() {
	State state = deal({make_card(kAce, 0), make_card(kAce, 1), make_card(kKing, 0), make_card(kKing, 1)});

	state.apply(LimitHoldem::kRaise);
	state.apply(LimitHoldem::kRaise);
	state.apply(LimitHoldem::kRaise);

	check(state.contributions[0] == 8);
	check(state.contributions[1] == 6);
	check(state.legal_actions().size() == 2);
}

static void test_postflop_order() {
	State state = deal({make_card(kAce, 0), make_card(kAce, 1), make_card(kKing, 0), make_card(kKing, 1)});

	state.apply(LimitHoldem::kCall);
	state.apply(LimitHoldem::kCall);

	check(state.current_player() == kChancePlayer);

	state.apply(make_card(kTwo, 2));
	state.apply(make_card(kSeven, 3));
	state.apply(make_card(kNine, 2));

	check(state.current_player() == 1);
}

static void test_showdown_winner() {
	State state = deal({make_card(kAce, 0), make_card(kAce, 1), make_card(kKing, 2), make_card(kKing, 3)});

	check_down(state, {make_card(kTwo, 2), make_card(kSeven, 3), make_card(kNine, 1), make_card(kJack, 0), make_card(kThree, 2)});

	check(state.utility(0) == LimitHoldem::kBigBlind);
	check(state.utility(1) == -LimitHoldem::kBigBlind);
}

static void test_split_pot() {
	State state = deal({make_card(kTwo, 0), make_card(kThree, 1), make_card(kTwo, 2), make_card(kThree, 3)});

	check_down(state, {make_card(kAce, 0), make_card(kKing, 0), make_card(kQueen, 0), make_card(kJack, 0), make_card(kTen, 0)});

	check(state.utility(0) == 0);
	check(state.utility(1) == 0);
}

static void test_key_ignores_card_order() {
	State first = deal({
		make_card(kAce, 0), make_card(kKing, 1), make_card(kTwo, 2), make_card(kThree, 3),
	});
	State second = deal({
		make_card(kKing, 1), make_card(kAce, 0), make_card(kSeven, 2), make_card(kNine, 3),
	});

	for (State* state : {&first, &second}) {
		state->apply(LimitHoldem::kCall);
		state->apply(LimitHoldem::kCall);
	}

	first.apply(make_card(kTwo, 0));
	first.apply(make_card(kSeven, 0));
	first.apply(make_card(kNine, 0));
	second.apply(make_card(kNine, 0));
	second.apply(make_card(kTwo, 0));
	second.apply(make_card(kSeven, 0));

	check(first.infoset_key(0) == second.infoset_key(0));
	check(!(first.infoset_key(1) == second.infoset_key(1)));
}

static void test_random_playouts(int num_playouts) {
	std::mt19937_64 rng(0);
	int max_payoff = 0;

	for (int playout = 0; playout < num_playouts; ++playout) {
		State state = LimitHoldem::initial_state();

		while (state.current_player() != kTerminalPlayer) {
			if (state.current_player() == kChancePlayer) {
				auto outcomes = state.chance_outcomes();
				double prob_sum = 0;

				for (ChanceOutcome outcome : outcomes) {
					prob_sum += outcome.prob;
				}

				check(std::abs(prob_sum - 1.0) < 1e-12);
				check(static_cast<int>(outcomes.size()) == kNumCards - state.num_dealt);

				std::size_t i = sample_index(outcomes, [](const ChanceOutcome& outcome) { return outcome.prob; }, rng);
				state.apply(outcomes[i].action);
			} else {
				auto actions = state.legal_actions();

				check(!actions.empty());

				std::uniform_int_distribution<std::size_t> action_distribution(0, actions.size() - 1);
				state.apply(actions[action_distribution(rng)]);
			}
		}

		double payoff = state.utility(0);

		check(std::abs(payoff + state.utility(1)) < 1e-12);
		check(std::abs(payoff) <= 48);

		max_payoff = std::max(max_payoff, static_cast<int>(std::abs(payoff)));
	}

	std::printf("%d random playouts, largest payoff %d\n", num_playouts, max_payoff);
}

int main() {
	test_preflop_fold();
	test_preflop_cap();
	test_postflop_order();
	test_showdown_winner();
	test_split_pot();
	test_key_ignores_card_order();
	test_random_playouts(100000);

	if (failures == 0) {
		std::printf("limit_holdem_test: OK\n");
	}

	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
