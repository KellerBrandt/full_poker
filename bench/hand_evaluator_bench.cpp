#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <unordered_set>

#include "../tests/test_utils.h"
#include "cards.h"
#include "hand_evaluator.h"

template <typename Callback>
void for_each_seven_card_hand(Callback callback) {
	for (Card c1 = 0; c1 < kNumCards; ++c1) {
		CardSet cards1 = card_bit(c1);

		for (Card c2 = c1 + 1; c2 < kNumCards; ++c2) {
			CardSet cards2 = cards1 | card_bit(c2);

			for (Card c3 = c2 + 1; c3 < kNumCards; ++c3) {
				CardSet cards3 = cards2 | card_bit(c3);

				for (Card c4 = c3 + 1; c4 < kNumCards; ++c4) {
					CardSet cards4 = cards3 | card_bit(c4);

					for (Card c5 = c4 + 1; c5 < kNumCards; ++c5) {
						CardSet cards5 = cards4 | card_bit(c5);

						for (Card c6 = c5 + 1; c6 < kNumCards; ++c6) {
							CardSet cards6 = cards5 | card_bit(c6);

							for (Card c7 = c6 + 1; c7 < kNumCards; ++c7) {
								callback(cards6 | card_bit(c7));
							}
						}
					}
				}
			}
		}
	}
}

int main() {
	uint64_t num_hands = 0;
	uint64_t checksum = 0;

	auto start = std::chrono::steady_clock::now();

	for_each_seven_card_hand([&](CardSet hand) {
		checksum += evaluate_hand(hand);
		++num_hands;
	});

	auto end = std::chrono::steady_clock::now();
	double seconds = std::chrono::duration<double>(end - start).count();

	std::printf("%llu hands in %.2f s (%.1f ns/hand, checksum %llu)\n", num_hands, seconds, seconds * 1e9 / num_hands, checksum);

	std::array<uint64_t, kStraightFlush + 1> category_counts = {};
	std::unordered_set<HandStrength> distinct_strengths;

	for_each_seven_card_hand([&](CardSet hand) {
		HandStrength strength = evaluate_hand(hand);

		++category_counts[strength >> 26];
		distinct_strengths.insert(strength);
	});

	// published 7-card category counts, high card through straight flush
	std::array<uint64_t, kStraightFlush + 1> expected_counts = {
		23294460, 58627800, 31433400, 6461620, 6180020, 4047644, 3473184, 224848, 41584,
	};

	for (uint32_t category = 0; category <= kStraightFlush; ++category) {
		std::printf("category %d: %llu (expected %llu)\n", category, category_counts[category], expected_counts[category]);
		check(category_counts[category] == expected_counts[category]);
	}

	std::printf("distinct strengths: %zu (expected 4824)\n", distinct_strengths.size());

	check(num_hands == 133784560);
	check(distinct_strengths.size() == 4824);

	if (failures == 0) {
		std::printf("hand_evaluator_bench: OK\n");
	}

	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
