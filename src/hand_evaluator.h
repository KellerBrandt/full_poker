#pragma once

#include <bit>
#include <cstdint>

#include "cards.h"

// category << 26 | primary ranks << 13 | kicker ranks, as rank bitmasks; higher wins
using HandStrength = uint32_t;

enum HandCategory : uint32_t {
	kHighCard,
	kPair,
	kTwoPair,
	kTrips,
	kStraight,
	kFlush,
	kFullHouse,
	kQuads,
	kStraightFlush,
};

constexpr HandStrength make_strength(HandCategory category, uint32_t primary_ranks, uint32_t kicker_ranks) {
	return category << 26 | primary_ranks << 13 | kicker_ranks;
}

constexpr uint32_t keep_highest(uint32_t ranks, int count) {
	while (std::popcount(ranks) > count) {
		ranks &= ranks - 1;
	}

	return ranks;
}

// bit of the highest straight's top rank, or 0. the ace also plays low
constexpr uint32_t straight_top(uint32_t ranks) {
	uint32_t with_low_ace = ranks << 1 | ranks >> (kNumRanks - 1);
	uint32_t runs = with_low_ace & with_low_ace >> 1 & with_low_ace >> 2 & with_low_ace >> 3 & with_low_ace >> 4;

	return runs == 0 ? 0 : std::bit_floor(runs) << 3;
}

constexpr HandStrength evaluate_hand(CardSet cards) {
	uint32_t suits[kNumSuits];
	uint32_t flush_ranks = 0;

	for (int suit = 0; suit < kNumSuits; ++suit) {
		suits[suit] = suit_ranks(cards, suit);

		if (std::popcount(suits[suit]) >= 5) {
			flush_ranks = suits[suit];
		}
	}

	uint32_t ranks = suits[0] | suits[1] | suits[2] | suits[3];
	uint32_t at_least_two = (suits[0] & suits[1]) | (suits[0] & suits[2]) | (suits[0] & suits[3])
		| (suits[1] & suits[2]) | (suits[1] & suits[3]) | (suits[2] & suits[3]);
	uint32_t at_least_three = (suits[0] & suits[1] & suits[2]) | (suits[0] & suits[1] & suits[3])
		| (suits[0] & suits[2] & suits[3]) | (suits[1] & suits[2] & suits[3]);
	uint32_t four = suits[0] & suits[1] & suits[2] & suits[3];

	if (flush_ranks != 0) {
		uint32_t top = straight_top(flush_ranks);

		if (top != 0) {
			return make_strength(kStraightFlush, top, 0);
		}
	}

	if (four != 0) {
		return make_strength(kQuads, four, keep_highest(ranks & ~four, 1));
	}

	if (at_least_three != 0) {
		uint32_t trips = std::bit_floor(at_least_three);
		uint32_t pair_ranks = at_least_two & ~trips;

		if (pair_ranks != 0) {
			return make_strength(kFullHouse, trips, std::bit_floor(pair_ranks));
		}
	}

	if (flush_ranks != 0) {
		return make_strength(kFlush, 0, keep_highest(flush_ranks, 5));
	}

	uint32_t top = straight_top(ranks);

	if (top != 0) {
		return make_strength(kStraight, top, 0);
	}

	if (at_least_three != 0) {
		return make_strength(kTrips, at_least_three, keep_highest(ranks & ~at_least_three, 2));
	}

	if (std::popcount(at_least_two) >= 2) {
		uint32_t pairs = keep_highest(at_least_two, 2);

		return make_strength(kTwoPair, pairs, keep_highest(ranks & ~pairs, 1));
	}

	if (at_least_two != 0) {
		return make_strength(kPair, at_least_two, keep_highest(ranks & ~at_least_two, 3));
	}

	return make_strength(kHighCard, 0, keep_highest(ranks, 5));
}
