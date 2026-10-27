#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "average_policy.h"
#include "cfr.h"
#include "exploitability.h"
#include "kuhn.h"
#include "mccfr.h"
#include "policy.h"
#include "test_utils.h"

int main() {
	TreeStats<Kuhn> stats;
	walk_tree<Kuhn>(Kuhn::initial_state(), stats);

	check(stats.terminals == 30);
	check(stats.infosets.size() == 12);

	// known value for uniform random Kuhn: 11/24
	UniformPolicy<Kuhn> uniform_policy;
	double uniform_exploitability = exploitability<Kuhn>(uniform_policy);

	check(std::abs(uniform_exploitability - 11.0 / 24.0) < 1e-9);

	int cfr_iterations = 100000;
	CFR<Kuhn> cfr;

	for (int i = 0; i < cfr_iterations; ++i) {
		cfr.iterate();
	}

	AveragePolicy cfr_policy(cfr.infoset_table());
	double cfr_exploitability = exploitability<Kuhn>(cfr_policy);
	std::printf("cfr exploitability after %d iterations: %g\n", cfr_iterations, cfr_exploitability);

	check(cfr.infoset_table().size() == 12);
	check(cfr_exploitability < 0.01);

	CFRPlus<Kuhn> cfr_plus;

	for (int i = 0; i < cfr_iterations; ++i) {
		cfr_plus.iterate();
	}

	AveragePolicy cfr_plus_policy(cfr_plus.infoset_table());
	double cfr_plus_exploitability = exploitability<Kuhn>(cfr_plus_policy);
	std::printf("cfr+ exploitability after %d iterations: %g\n", cfr_iterations, cfr_plus_exploitability);

	check(cfr_plus_exploitability < cfr_exploitability);

	int mccfr_early_iterations = 10000;
	int mccfr_iterations = 100000;
	MCCFR<Kuhn> mccfr;

	for (int i = 0; i < mccfr_early_iterations; ++i) {
		mccfr.iterate();
	}

	AveragePolicy mccfr_policy(mccfr.infoset_table());
	double mccfr_early_exploitability = exploitability<Kuhn>(mccfr_policy);

	for (int i = mccfr_early_iterations; i < mccfr_iterations; ++i) {
		mccfr.iterate();
	}

	double mccfr_exploitability = exploitability<Kuhn>(mccfr_policy);
	std::printf("mccfr exploitability: %g after %d iterations, %g after %d\n", mccfr_early_exploitability, mccfr_early_iterations, mccfr_exploitability, mccfr_iterations);

	check(mccfr_early_exploitability < uniform_exploitability);
	check(mccfr_exploitability < mccfr_early_exploitability);

	MCCFR<Kuhn> mccfr_same_seed;

	for (int i = 0; i < mccfr_iterations; ++i) {
		mccfr_same_seed.iterate();
	}

	AveragePolicy mccfr_same_seed_policy(mccfr_same_seed.infoset_table());
	double mccfr_same_seed_exploitability = exploitability<Kuhn>(mccfr_same_seed_policy);

	check(mccfr_same_seed_exploitability == mccfr_exploitability);

	if (failures == 0) {
		std::printf("kuhn_test: OK\n");
	}

	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
