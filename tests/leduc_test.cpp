#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <vector>

#include "average_policy.h"
#include "cfr.h"
#include "exploitability.h"
#include "leduc.h"
#include "mccfr.h"
#include "policy.h"
#include "test_utils.h"

int main() {
	TreeStats<Leduc> stats;
	walk_tree<Leduc>(Leduc::initial_state(), stats);

	check(stats.terminals == 1116);
	check(stats.infosets.size() == 288);

	std::set<std::vector<float>> distinct_tensors;

	for (const auto& [infoset, tensor] : stats.infoset_tensors) {
		distinct_tensors.insert(tensor);
	}

	check(distinct_tensors.size() == 288);

	// published uniform random NashConv is 4.747222, exploitability is half
	UniformPolicy<Leduc> uniform_policy;
	double uniform_exploitability = exploitability<Leduc>(uniform_policy);

	check(std::abs(uniform_exploitability - 2.373611) < 1e-5);

	int cfr_early_iterations = 100;
	int cfr_iterations = 1000;
	CFR<Leduc> cfr;

	for (int i = 0; i < cfr_early_iterations; ++i) {
		cfr.iterate();
	}

	AveragePolicy cfr_policy(cfr.infoset_table());
	double cfr_early_exploitability = exploitability<Leduc>(cfr_policy);

	for (int i = cfr_early_iterations; i < cfr_iterations; ++i) {
		cfr.iterate();
	}

	double cfr_exploitability = exploitability<Leduc>(cfr_policy);
	std::printf("cfr exploitability: %g after %d iterations, %g after %d\n", cfr_early_exploitability, cfr_early_iterations, cfr_exploitability, cfr_iterations);

	check(cfr.infoset_table().size() == stats.infosets.size());
	check(cfr_early_exploitability < uniform_exploitability);
	check(cfr_exploitability < cfr_early_exploitability);

	CFRPlus<Leduc> cfr_plus;

	for (int i = 0; i < cfr_iterations; ++i) {
		cfr_plus.iterate();
	}

	AveragePolicy cfr_plus_policy(cfr_plus.infoset_table());
	double cfr_plus_exploitability = exploitability<Leduc>(cfr_plus_policy);
	std::printf("cfr+ exploitability after %d iterations: %g\n", cfr_iterations, cfr_plus_exploitability);

	check(cfr_plus_exploitability < cfr_exploitability);

	int mccfr_early_iterations = 1000;
	int mccfr_iterations = 10000;
	MCCFR<Leduc> mccfr;

	for (int i = 0; i < mccfr_early_iterations; ++i) {
		mccfr.iterate();
	}

	AveragePolicy mccfr_policy(mccfr.infoset_table());
	double mccfr_early_exploitability = exploitability<Leduc>(mccfr_policy);

	for (int i = mccfr_early_iterations; i < mccfr_iterations; ++i) {
		mccfr.iterate();
	}

	double mccfr_exploitability = exploitability<Leduc>(mccfr_policy);
	std::printf("mccfr exploitability: %g after %d iterations, %g after %d\n", mccfr_early_exploitability, mccfr_early_iterations, mccfr_exploitability, mccfr_iterations);

	check(mccfr_early_exploitability < uniform_exploitability);
	check(mccfr_exploitability < mccfr_early_exploitability);

	if (failures == 0) {
		std::printf("leduc_test: OK\n");
	}

	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
