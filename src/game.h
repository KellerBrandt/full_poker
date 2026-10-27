#pragma once

#include <array>
#include <concepts>
#include <type_traits>

using Action = int;
using Player = int;

constexpr Player kChancePlayer = -1;
constexpr Player kTerminalPlayer = -2;

struct ChanceOutcome {
	Action action;
	double prob;
};

template <typename G>
concept Game = requires(const typename G::State& state, typename G::State& mutable_state, Action action, Player player) {
	{ G::kNumPlayers } -> std::convertible_to<int>;
	{ G::kMaxActions } -> std::convertible_to<int>;
	{ G::initial_state() } -> std::same_as<typename G::State>;

	{ state.current_player() } -> std::same_as<Player>;
	{ state.legal_actions() };
	{ state.chance_outcomes() };
	{ mutable_state.apply(action) } -> std::same_as<void>;
	{ state.utility(player) } -> std::same_as<double>;
	{ state.infoset_key(player) } -> std::same_as<typename G::InfoSetKey>;
} && std::is_trivially_copyable_v<typename G::State>;

template <typename G>
concept TensorGame = Game<G> && requires(const typename G::State& state, Player player) {
	{ G::kInfosetTensorSize } -> std::convertible_to<int>;
	{ state.infoset_tensor(player) } -> std::same_as<std::array<float, G::kInfosetTensorSize>>;
};
