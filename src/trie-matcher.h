/* Pattern Matching problem benchmark
   Copyright © 2021 Michał Nazarewicz <mina86@mina86.com>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef H_TRIE_MATCHER_H
#define H_TRIE_MATCHER_H

#include <utility>
#include <algorithm>

#include "trie.h"
#include "util.h"
#include "words.h"


template <class Trie>
struct TrieMatcher {
	TrieMatcher(const Words &words) COLD;
	~TrieMatcher() COLD;

	constexpr size_t size() const { return count; }
	constexpr size_t word_length() const { return length; }
	template <class Callback>
	size_t query(std::string_view prefix,
	             std::string_view suffix,
	             Callback &cb) const HOT;

private:
	Trie nodes;
	const typename Trie::value_type fwd_trie_root;
	const typename Trie::value_type rev_trie_root;
	const size_t count;
	const size_t length;

	TrieMatcher() = delete;
	TrieMatcher(const TrieMatcher&) = delete;
};


template <class Trie>
TrieMatcher<Trie>::TrieMatcher(const Words &words)
	: fwd_trie_root(words.word_length() ? nodes.add_node() : Trie::null()),
	  rev_trie_root(words.word_length() ? nodes.add_node() : Trie::null()),
	  count(words.size()), length(words.word_length()) {
	for (const auto &pair : words) {
		const std::string &word = pair.first;
		nodes.insert(fwd_trie_root, word.begin(), word.end(),
		             Trie::value_type::from_num(pair.second));
		nodes.insert(rev_trie_root, word.rbegin(), word.rend(),
		             Trie::value_type::from_num(pair.second));
	}
}


template <class Trie>
TrieMatcher<Trie>::~TrieMatcher() {
	if (length) {
		nodes.free_trie(fwd_trie_root, length - 1);
		nodes.free_trie(rev_trie_root, length - 1);
	}
}


template <class Trie>
template <class Callback>
size_t TrieMatcher<Trie>::query(std::string_view prefix,
                                std::string_view suffix,
                                Callback &cb) const {
	/* No, we're not pretending to be thread-safe. */
	static std::vector<char> buffer;

	typename Trie::value_type node;
	if (__builtin_expect(prefix.size() >= suffix.size(), 1)) {
		node = fwd_trie_root;
	} else {
		node = rev_trie_root;

		if (buffer.size() < word_length()) {
			buffer.resize(word_length());
		}

		/* Reverse prefix and suffix */
		auto buf = buffer.data();
		auto ptr = std::copy(prefix.rbegin(), prefix.rend(), buf);
		std::copy(suffix.rbegin(), suffix.rend(), ptr);

		/* Swap prefix and suffix */
		const size_t prefix_size = prefix.size();
		const size_t suffix_size = suffix.size();
		suffix = std::string_view(buf, prefix_size);
		prefix = std::string_view(ptr, suffix_size);
	}

	if (!prefix.empty()) {
		node = nodes.follow(node, prefix);
		if (!node) {
			return 0;
		}
	}
	size_t count = 0;
	nodes.deep_fan_out(
		node, length - prefix.size() - suffix.size(),
		[n = &nodes, &count, &cb, suffix](auto pos) {
			pos = n->follow(pos, suffix);
			if (pos) {
				cb(pos.as_num());
				++count;
			}
		});
	return count;
}


#endif
