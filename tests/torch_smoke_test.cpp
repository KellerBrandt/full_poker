#include <cstdio>

#include <torch/torch.h>

int main() {
	torch::manual_seed(0);

	torch::nn::Linear layer(4, 2);
	torch::Tensor input = torch::ones({1, 4});
	torch::Tensor output = layer->forward(input);

	std::printf("output shape [%lld, %lld]\n", output.size(0), output.size(1));
	std::printf("torch_smoke_test: OK\n");
}
