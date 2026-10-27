#include <cstdio>
#include <cstdlib>

#include "average_policy.h"
#include "deep_cfr.h"
#include "exploitability.h"
#include "leduc.h"
#include "policy.h"
#include "test_utils.h"

int main() {
	DeepCFROptions options;
	options.traversals_per_iteration = 300;
	options.buffer_capacity = 100000;
	options.train_steps = 200;
	options.batch_size = 256;

	int early_iterations = 5;
	int iterations = 20;
	DeepCFR<Leduc> deep_cfr(options);

	for (int i = 0; i < early_iterations; ++i) {
		deep_cfr.iterate();
	}

	auto early_table = deep_cfr.average_strategy_table();
	AveragePolicy early_policy(early_table);
	double early_exploitability = exploitability<Leduc>(early_policy);

	for (int i = early_iterations; i < iterations; ++i) {
		deep_cfr.iterate();
	}

	auto table = deep_cfr.average_strategy_table();
	AveragePolicy policy(table);
	double deep_cfr_exploitability = exploitability<Leduc>(policy);

	std::printf("deep cfr exploitability: %g after %d iterations, %g after %d\n", early_exploitability, early_iterations, deep_cfr_exploitability, iterations);

	UniformPolicy<Leduc> uniform_policy;
	double uniform_exploitability = exploitability<Leduc>(uniform_policy);

	check(early_exploitability < uniform_exploitability);
	check(deep_cfr_exploitability < early_exploitability);

	if (failures == 0) {
		std::printf("deep_cfr_test: OK\n");
	}

	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
