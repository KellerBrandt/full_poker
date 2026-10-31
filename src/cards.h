#pragma once

#include <cstdint>

// card = suit * 13 + rank, so each suit is 13 consecutive bits of a CardSet
using Card = int;
using CardSet = uint64_t;

constexpr int kNumRanks = 13;
constexpr int kNumSuits = 4;
constexpr int kNumCards = kNumRanks * kNumSuits;
constexpr uint64_t kRankMask = (uint64_t{1} << kNumRanks) - 1;

constexpr int card_rank(Card card) { return card % kNumRanks; }
constexpr int card_suit(Card card) { return card / kNumRanks; }
constexpr Card make_card(int rank, int suit) { return suit * kNumRanks + rank; }
constexpr CardSet card_bit(Card card) { return CardSet{1} << card; }

constexpr uint32_t suit_ranks(CardSet cards, int suit) {
	return static_cast<uint32_t>((cards >> (suit * kNumRanks)) & kRankMask);
}
