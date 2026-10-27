#pragma once

#include <cstdint>

#include <torch/torch.h>

struct AdvantageNetwork : torch::nn::Module {
	AdvantageNetwork(int64_t input_size, int64_t num_actions, int64_t hidden_size = 64) {
		input_layer = register_module("input_layer", torch::nn::Linear(input_size, hidden_size));
		hidden_layer = register_module("hidden_layer", torch::nn::Linear(hidden_size, hidden_size));
		output_layer = register_module("output_layer", torch::nn::Linear(hidden_size, num_actions));
	}

	torch::Tensor forward(torch::Tensor input) {
		torch::Tensor hidden = torch::relu(input_layer->forward(input));
		hidden = torch::relu(hidden_layer->forward(hidden));

		return output_layer->forward(hidden);
	}

	torch::nn::Linear input_layer = nullptr;
	torch::nn::Linear hidden_layer = nullptr;
	torch::nn::Linear output_layer = nullptr;
};
