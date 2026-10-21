#pragma once

#include <chrono>
#include <cstdio>

#include "average_policy.h"
#include "exploitability.h"
#include "game.h"

template <Game G, typename Solver>
void bench_solver(const char* name, int iterations) {
	Solver solver;

	auto start = std::chrono::steady_clock::now();

	for (int i = 0; i < iterations; ++i) {
		solver.iterate();
	}

	auto end = std::chrono::steady_clock::now();
	double seconds = std::chrono::duration<double>(end - start).count();
	double nodes_visited = static_cast<double>(solver.nodes_visited());

	std::printf("%s: %d iterations in %.3f s (%.0f iterations/s)\n", name, iterations, seconds, iterations / seconds);
	std::printf("%.0f nodes (%.0f nodes/s, %.2f ns/node)\n", nodes_visited, nodes_visited / seconds, seconds * 1e9 / nodes_visited);

	AveragePolicy policy(solver.infoset_table());
	double solver_exploitability = exploitability<G>(policy);

	std::printf("exploitability: %g\n\n", solver_exploitability);
}
