#pragma once

#include <cstddef>
#include <unordered_map>

#include "game.h"

// data indexed in legal_actions() order
template <Game G, typename Entry>
class InfoSetTable {
public:
	Entry& operator[](const typename G::InfoSetKey& key) { return entries_[key]; }

	const Entry* find(const typename G::InfoSetKey& key) const {
		auto it = entries_.find(key);

		return it == entries_.end() ? nullptr : &it->second;
	}

	std::size_t size() const { return entries_.size(); }

	auto begin() { return entries_.begin(); }
	auto end() { return entries_.end(); }

private:
	std::unordered_map<typename G::InfoSetKey, Entry> entries_;
};
