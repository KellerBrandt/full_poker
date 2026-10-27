#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include <torch/torch.h>

#include "advantage_network.h"
#include "leduc.h"
#include "test_utils.h"

int main() {
	torch::manual_seed(0);

	int deals[3][2] = {{0, 1}, {1, 2}, {2, 0}};
	int64_t num_samples = 3;
	std::vector<float> inputs;

	for (const auto& deal : deals) {
		Leduc::State state = Leduc::initial_state();
		state.apply(deal[0]);
		state.apply(deal[1]);

		auto tensor = state.infoset_tensor(0);
		inputs.insert(inputs.end(), tensor.begin(), tensor.end());
	}

	torch::Tensor input = torch::from_blob(inputs.data(), {num_samples, Leduc::kInfosetTensorSize}).clone();
	torch::Tensor target = torch::tensor({{1.0f, -1.0f, 0.5f}, {-0.5f, 2.0f, 0.0f}, {0.0f, 0.0f, -2.0f}});

	AdvantageNetwork network(Leduc::kInfosetTensorSize, Leduc::kMaxActions);
	torch::optim::Adam optimizer(network.parameters(), torch::optim::AdamOptions(1e-2));

	torch::Tensor output = network.forward(input);

	check(output.size(0) == num_samples);
	check(output.size(1) == Leduc::kMaxActions);

	double initial_loss = torch::mse_loss(output, target).item<double>();
	double loss = initial_loss;
	int num_steps = 500;

	for (int step = 0; step < num_steps; ++step) {
		optimizer.zero_grad();

		torch::Tensor step_loss = torch::mse_loss(network.forward(input), target);
		step_loss.backward();
		optimizer.step();

		loss = step_loss.item<double>();
	}

	std::printf("loss: %g initially, %g after %d steps\n", initial_loss, loss, num_steps);

	check(loss < 1e-3);

	if (failures == 0) {
		std::printf("advantage_network_test: OK\n");
	}

	return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
