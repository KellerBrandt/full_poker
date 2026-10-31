#include <cstdio>
#include <vector>

#include "average_policy.h"
#include "deep_cfr.h"
#include "exploitability.h"
#include "leduc.h"

int main() {
	DeepCFROptions options;
	options.traversals_per_iteration = 1000;
	options.train_steps = 500;
	options.batch_size = 1024;

	int iterations = 200;
	int eval_interval = 25;
	int num_seeds = 3;
	int num_checkpoints = iterations / eval_interval;
	std::vector<double> exploitability_sums(num_checkpoints, 0.0);
	double training_seconds_sum = 0;

	std::printf("exploitability at iterations");

	for (int checkpoint = 1; checkpoint <= num_checkpoints; ++checkpoint) {
		std::printf(" %d", checkpoint * eval_interval);
	}

	std::printf("\n");

	for (int seed = 0; seed < num_seeds; ++seed) {
		options.seed = seed;
		DeepCFR<Leduc> deep_cfr(options);

		std::printf("seed %d:", seed);

		for (int i = 1; i <= iterations; ++i) {
			deep_cfr.iterate();

			if (i % eval_interval == 0) {
				auto table = deep_cfr.average_strategy_table();
				AveragePolicy policy(table);
				double deep_cfr_exploitability = exploitability<Leduc>(policy);

				exploitability_sums[i / eval_interval - 1] += deep_cfr_exploitability;
				std::printf(" %.3f", deep_cfr_exploitability);
				std::fflush(stdout);
			}
		}

		std::printf(" (training %.1f s)\n", deep_cfr.training_seconds());
		training_seconds_sum += deep_cfr.training_seconds();
	}

	std::printf("mean:  ");

	for (double exploitability_sum : exploitability_sums) {
		std::printf(" %.3f", exploitability_sum / num_seeds);
	}

	std::printf(" (training %.1f s)\n", training_seconds_sum / num_seeds);
}
