#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>

#include "reservoir_buffer.h"
#include "test_utils.h"

int main() {
	int capacity = 100;

	ReservoirBuffer<int> under_capacity(capacity, 0);

	for (int i = 0; i < capacity / 2; ++i) {
		under_capacity.add(i);
	}

	check(static_cast<int>(under_capacity.items().size()) == capacity / 2);

	for (int i = 0; i < capacity / 2; ++i) {
		check(under_capacity.items()[i] == i);
	}

	int num_items = 10000;
	ReservoirBuffer<int> over_capacity(capacity, 0);

	for (int i = 0; i < num_items; ++i) {
		over_capacity.add(i);
	}

	check(static_cast<int>(over_capacity.items().size()) == capacity);
	check(over_capacity.num_seen() == static_cast<uint64_t>(num_items));

	std::set<int> distinct_items(over_capacity.items().begin(), over_capacity.items().end());
	check(static_cast<int>(distinct_items.size()) == capacity);

	// a uniform sample of 0..9999 has mean near 5000, with standard deviation about 290
	double item_sum = 0;

	for (int item : over_capacity.items()) {
		item_sum += item;
	}

	double mean = item_sum / capacity;
	std::printf("kept item mean: %g\n", mean);

	check(std::abs(mean - (num_items - 1) / 2.0) < 1000);

	if (failures == 0) {
		std::printf("reservoir_buffer_test: OK\n");
	}

	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
