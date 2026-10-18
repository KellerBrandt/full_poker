#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "average_policy.h"
#include "cfr.h"
#include "exploitability.h"
#include "leduc.h"
#include "policy.h"
#include "test_utils.h"

int main() {
	TreeStats<Leduc> stats;
	walk<Leduc>(Leduc::initial_state(), stats);

	check(stats.terminals == 1116);
	check(stats.infosets.size() == 288);

	// published uniform random NashConv is 4.747222, exploitability is half
	UniformPolicy<Leduc> uniform_policy;
	double uniform_exploitability = exploitability<Leduc>(uniform_policy);

	check(std::abs(uniform_exploitability - 2.373611) < 1e-5);

	CFR<Leduc> cfr;

	for (int i = 0; i < 100; ++i) {
		cfr.iterate();
	}

	AveragePolicy<Leduc> cfr_policy(cfr.table());
	double early_exploitability = exploitability<Leduc>(cfr_policy);

	for (int i = 0; i < 900; ++i) {
		cfr.iterate();
	}

	double late_exploitability = exploitability<Leduc>(cfr_policy);
	std::printf("cfr exploitability: %g after 100 iterations, %g after 1000\n", early_exploitability, late_exploitability);

	check(cfr.table().size() == stats.infosets.size());
	check(early_exploitability < uniform_exploitability);
	check(late_exploitability < early_exploitability);

	if (failures == 0) {
		std::printf("leduc_test: OK\n");
	}

	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
