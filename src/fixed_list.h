#pragma once

#include <array>
#include <cassert>
#include <cstddef>

template <typename T, std::size_t Capacity>
class FixedList {
public:
	void push_back(const T& item) {
		assert(size_ < Capacity);

		items_[size_++] = item;
	}

	std::size_t size() const { return size_; }
	bool empty() const { return size_ == 0; }

	T& operator[](std::size_t i) { return items_[i]; }
	const T& operator[](std::size_t i) const { return items_[i]; }

	T* begin() { return items_.data(); }
	T* end() { return items_.data() + size_; }
	const T* begin() const { return items_.data(); }
	const T* end() const { return items_.data() + size_; }

private:
	std::array<T, Capacity> items_;
	std::size_t size_ = 0;
};
