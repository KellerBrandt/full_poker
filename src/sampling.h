#pragma once

#include <cstddef>
#include <random>

template <typename List, typename Prob, typename Rng>
std::size_t sample_index(const List& items, Prob prob, Rng& rng) {
	std::uniform_real_distribution<double> uniform(0.0, 1.0);
	double r = uniform(rng);

	for (std::size_t i = 0; i + 1 < items.size(); ++i) {
		r -= prob(items[i]);

		if (r < 0) {
			return i;
		}
	}

	return items.size() - 1;
}
