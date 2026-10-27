#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

// fixed capacity, holds a uniform random subset of every item ever added
template <typename T>
class ReservoirBuffer {
public:
	ReservoirBuffer(std::size_t capacity, uint64_t seed) : capacity_(capacity), rng_(seed) {
		items_.reserve(capacity);
	}

	void add(const T& item) {
		++num_seen_;

		if (items_.size() < capacity_) {
			items_.push_back(item);

			return;
		}

		std::uniform_int_distribution<uint64_t> slot_distribution(0, num_seen_ - 1);
		uint64_t slot = slot_distribution(rng_);

		if (slot < capacity_) {
			items_[slot] = item;
		}
	}

	const std::vector<T>& items() const { return items_; }

	uint64_t num_seen() const { return num_seen_; }

private:
	std::size_t capacity_;
	std::vector<T> items_;
	uint64_t num_seen_ = 0;
	std::mt19937_64 rng_;
};
