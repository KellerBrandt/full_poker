#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <vector>

#include <torch/torch.h>

#include "advantage_network.h"
#include "fixed_list.h"
#include "game.h"
#include "infoset_table.h"
#include "reservoir_buffer.h"
#include "sampling.h"

struct DeepCFROptions {
	int traversals_per_iteration = 1000;
	std::size_t buffer_capacity = 1000000;
	int train_steps = 1000;
	int batch_size = 1024;
	double learning_rate = 1e-3;
	int64_t hidden_size = 64;
	bool warm_start = true;
	uint64_t seed = 0;
};

// single deep CFR: the average strategy is the mixture of every iteration's networks
template <TensorGame G>
class DeepCFR {
public:
	struct Sample {
		std::array<float, G::kInfosetTensorSize> infoset_tensor;
		std::array<float, G::kMaxActions> advantages;
		int iteration;
	};

	explicit DeepCFR(DeepCFROptions options = {}) : options_(options), rng_(options.seed) {
		torch::manual_seed(options.seed);

		for (Player p = 0; p < G::kNumPlayers; ++p) {
			networks_[p] = make_network();
			buffers_.emplace_back(options.buffer_capacity, options.seed + 1 + p);
		}
	}

	void iterate() {
		++iteration_;

		for (Player p = 0; p < G::kNumPlayers; ++p) {
			for (int traversal = 0; traversal < options_.traversals_per_iteration; ++traversal) {
				traverse(G::initial_state(), p);
			}

			train(p);
			network_history_[p].push_back(copy_network(networks_[p]));
		}
	}

	uint64_t nodes_visited() const { return nodes_visited_; }

	struct AverageStrategyEntry {
		std::array<double, G::kMaxActions> strategy_sum = {};
	};

	InfoSetTable<G, AverageStrategyEntry> average_strategy_table() {
		InfoSetTable<G, AverageStrategyEntry> table;
		NetworkReach reach;

		for (Player p = 0; p < G::kNumPlayers; ++p) {
			reach[p].assign(network_history_[p].size(), 1.0);
		}

		accumulate_average_strategy(G::initial_state(), reach, table);

		return table;
	}

private:
	using State = typename G::State;
	using Strategy = FixedList<double, G::kMaxActions>;
	using Network = std::shared_ptr<AdvantageNetwork>;
	using NetworkReach = std::array<std::vector<double>, G::kNumPlayers>;

	Strategy network_strategy(const Network& network, const State& state, Player player) {
		torch::NoGradGuard no_grad;

		auto infoset_tensor = state.infoset_tensor(player);
		torch::Tensor input = torch::from_blob(infoset_tensor.data(), {1, G::kInfosetTensorSize});
		torch::Tensor output = network->forward(input);
		auto advantages = output.accessor<float, 2>();

		auto actions = state.legal_actions();
		double positive_advantage_sum = 0;

		for (Action action : actions) {
			positive_advantage_sum += std::max(advantages[0][action], 0.0f);
		}

		Strategy strategy;

		if (positive_advantage_sum > 0) {
			for (Action action : actions) {
				strategy.push_back(std::max(advantages[0][action], 0.0f) / positive_advantage_sum);
			}

			return strategy;
		}

		std::size_t max_index = 0;

		for (std::size_t i = 1; i < actions.size(); ++i) {
			if (advantages[0][actions[i]] > advantages[0][actions[max_index]]) {
				max_index = i;
			}
		}

		for (std::size_t i = 0; i < actions.size(); ++i) {
			strategy.push_back(i == max_index ? 1.0 : 0.0);
		}

		return strategy;
	}

	double traverse(const State& state, Player updating_player) {
		++nodes_visited_;

		Player player = state.current_player();

		if (player == kTerminalPlayer) {
			return state.utility(updating_player);
		}

		if (player == kChancePlayer) {
			auto outcomes = state.chance_outcomes();
			std::size_t i = sample_index(outcomes, [](const ChanceOutcome& outcome) { return outcome.prob; }, rng_);

			State child = state;
			child.apply(outcomes[i].action);

			return traverse(child, updating_player);
		}

		auto actions = state.legal_actions();
		Strategy strategy = network_strategy(networks_[player], state, player);

		if (player != updating_player) {
			std::size_t i = sample_index(strategy, [](double prob) { return prob; }, rng_);

			State child = state;
			child.apply(actions[i]);

			return traverse(child, updating_player);
		}

		double utility = 0;
		std::array<double, G::kMaxActions> action_utilities;

		for (std::size_t i = 0; i < actions.size(); ++i) {
			State child = state;
			child.apply(actions[i]);

			action_utilities[i] = traverse(child, updating_player);
			utility += strategy[i] * action_utilities[i];
		}

		Sample sample;
		sample.infoset_tensor = state.infoset_tensor(player);
		sample.advantages = {};
		sample.iteration = iteration_;

		for (std::size_t i = 0; i < actions.size(); ++i) {
			sample.advantages[actions[i]] = static_cast<float>(action_utilities[i] - utility);
		}

		buffers_[player].add(sample);

		return utility;
	}

	void train(Player player) {
		const std::vector<Sample>& samples = buffers_[player].items();

		if (samples.empty()) {
			return;
		}

		if (!options_.warm_start) {
			networks_[player] = make_network();
		}

		Network& network = networks_[player];
		torch::optim::Adam optimizer(network->parameters(), torch::optim::AdamOptions(options_.learning_rate));
		std::uniform_int_distribution<std::size_t> sample_distribution(0, samples.size() - 1);

		std::vector<float> inputs(options_.batch_size * G::kInfosetTensorSize);
		std::vector<float> targets(options_.batch_size * G::kMaxActions);
		std::vector<float> weights(options_.batch_size);

		for (int step = 0; step < options_.train_steps; ++step) {
			for (int row = 0; row < options_.batch_size; ++row) {
				const Sample& sample = samples[sample_distribution(rng_)];

				std::copy(sample.infoset_tensor.begin(), sample.infoset_tensor.end(), inputs.begin() + row * G::kInfosetTensorSize);
				std::copy(sample.advantages.begin(), sample.advantages.end(), targets.begin() + row * G::kMaxActions);
				weights[row] = static_cast<float>(sample.iteration);
			}

			torch::Tensor input = torch::from_blob(inputs.data(), {options_.batch_size, G::kInfosetTensorSize});
			torch::Tensor target = torch::from_blob(targets.data(), {options_.batch_size, G::kMaxActions});
			torch::Tensor weight = torch::from_blob(weights.data(), {options_.batch_size, 1});

			torch::Tensor squared_error = (network->forward(input) - target).pow(2);
			torch::Tensor loss = (weight * squared_error).mean() / weight.mean();

			optimizer.zero_grad();
			loss.backward();
			optimizer.step();
		}
	}

	void accumulate_average_strategy(const State& state, const NetworkReach& reach, InfoSetTable<G, AverageStrategyEntry>& table) {
		Player player = state.current_player();

		if (player == kTerminalPlayer) {
			return;
		}

		if (player == kChancePlayer) {
			for (ChanceOutcome outcome : state.chance_outcomes()) {
				State child = state;
				child.apply(outcome.action);
				accumulate_average_strategy(child, reach, table);
			}

			return;
		}

		auto actions = state.legal_actions();
		AverageStrategyEntry& entry = table[state.infoset_key(player)];
		const std::vector<Network>& networks = network_history_[player];
		std::vector<Strategy> strategies;

		for (std::size_t t = 0; t < networks.size(); ++t) {
			strategies.push_back(network_strategy(networks[t], state, player));
			double weight = (t + 1) * reach[player][t];

			for (std::size_t i = 0; i < actions.size(); ++i) {
				entry.strategy_sum[i] += weight * strategies[t][i];
			}
		}

		for (std::size_t i = 0; i < actions.size(); ++i) {
			NetworkReach child_reach = reach;

			for (std::size_t t = 0; t < networks.size(); ++t) {
				child_reach[player][t] *= strategies[t][i];
			}

			State child = state;
			child.apply(actions[i]);
			accumulate_average_strategy(child, child_reach, table);
		}
	}

	Network make_network() const {
		return std::make_shared<AdvantageNetwork>(G::kInfosetTensorSize, G::kMaxActions, options_.hidden_size);
	}

	Network copy_network(const Network& source) const {
		torch::NoGradGuard no_grad;

		Network copy = make_network();
		std::vector<torch::Tensor> source_parameters = source->parameters();
		std::vector<torch::Tensor> copy_parameters = copy->parameters();

		for (std::size_t i = 0; i < source_parameters.size(); ++i) {
			copy_parameters[i].copy_(source_parameters[i]);
		}

		return copy;
	}

	DeepCFROptions options_;
	std::array<Network, G::kNumPlayers> networks_;
	std::vector<ReservoirBuffer<Sample>> buffers_;
	std::array<std::vector<Network>, G::kNumPlayers> network_history_;
	std::mt19937_64 rng_;
	int iteration_ = 0;
	uint64_t nodes_visited_ = 0;
};
