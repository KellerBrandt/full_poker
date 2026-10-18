#pragma once

#include <chrono>
#include <cstdio>

#include "average_policy.h"
#include "cfr.h"
#include "exploitability.h"
#include "game.h"

template <Game G>
void bench_cfr(int iterations) {
	CFR<G> cfr;

	auto start = std::chrono::steady_clock::now();

	for (int i = 0; i < iterations; ++i) {
		cfr.iterate();
	}

	auto end = std::chrono::steady_clock::now();
	double seconds = std::chrono::duration<double>(end - start).count();
	double nodes = static_cast<double>(cfr.nodes_visited());

	std::printf("cfr: %d iterations in %.3f s (%.0f iterations/s)\n", iterations, seconds, iterations / seconds);
	std::printf("%.0f nodes (%.0f nodes/s, %.2f ns/node)\n", nodes, nodes / seconds, seconds * 1e9 / nodes);

	AveragePolicy<G> policy(cfr.table());
	double cfr_exploitability = exploitability<G>(policy);

	std::printf("exploitability: %g\n", cfr_exploitability);
}
