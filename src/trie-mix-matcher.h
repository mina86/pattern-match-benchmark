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

#ifndef H_TRIE_MIX_MATCHER_H
#define H_TRIE_MIX_MATCHER_H

#include <cstring>
#include <utility>
#include <memory>

#include "trie.h"
#include "util.h"
#include "words.h"


template <class Trie>
struct TrieMixMatcher {
	using Query = std::pair<std::string_view, std::string_view>;

	TrieMixMatcher(const Words &words) COLD;
	~TrieMixMatcher() COLD;

	constexpr size_t size() const { return count; }
	constexpr size_t word_length() const { return length; }
	template <class Callback>
	size_t query(std::string_view prefix,
	             std::string_view suffix,
	             Callback &cb) const HOT;

private:
	Trie nodes;
	const typename Trie::value_type trie_root;
	const size_t count;
	const size_t length;

	TrieMixMatcher() = delete;
	TrieMixMatcher(const TrieMixMatcher&) = delete;
};


inline char *mix(char *dst, const char *src, size_t len) HOT;
inline char *mix(char *dst, const char *src, size_t len) {
	const char *end = src + len;
	char *out = dst;
	while (src < end) {
		*out++ = *src++;
		if (src < end) {
			*out++ = *--end;
		}
	}
	return out;
}


template <class Trie>
TrieMixMatcher<Trie>::TrieMixMatcher(const Words &words)
	: trie_root(words.word_length() ? nodes.add_node() : Trie::null()),
	  count(words.size()), length(words.word_length()) {
	auto buf = std::make_unique<char[]>(length);
	for (const auto &pair : words) {
		nodes.insert(trie_root, buf.get(),
		             mix(buf.get(), pair.first.data(), length),
		             Trie::value_type::from_num(pair.second));
	}
}


template <class Trie>
TrieMixMatcher<Trie>::~TrieMixMatcher() {
	if (length) {
		nodes.free_trie(trie_root, length - 1);
	}
}


template <class Trie, class Callback>
struct Crawler {
	Crawler(const Trie &n, const char *e, Callback &cb) HOT;

	void operator()(const char *key,
	                typename Trie::value_type value) const HOT;

	constexpr size_t count() const { return n; }

private:
	const Trie &nodes;
	const char *const key_end;
	Callback &cb;
	mutable size_t n = 0;
};


template <class Trie, class Callback>
Crawler<Trie, Callback>::Crawler(const Trie &n, const char *e, Callback &cb)
	: nodes(n), key_end(e), cb(cb) {}


template <class Trie, class Callback>
void Crawler<Trie, Callback>::operator()(
	const char *key, typename Trie::value_type value) const {
	while (key != key_end) {
		const char *ptr = key;
		while (key != key_end && *key == '\0') {
			++key;
		}
		const size_t count = key - ptr;
		switch (count) {
		case 0:
			value = nodes.follow(value, *key++);
			if (!value) {
				return;
			}
			break;
		case 1:
			nodes.fan_out(value, *this, key);
			return;
		default:
			nodes.deep_fan_out(value, count, *this, key);
			return;
		}
	}
	cb(value.as_num());
	++n;
}


template <class Trie>
template <class Callback>
size_t TrieMixMatcher<Trie>::query(std::string_view prefix,
                                   std::string_view suffix,
                                   Callback &cb) const {
	/* No, we're not pretending to be thread-safe. */
	static std::vector<char> buffer;

	if (__builtin_expect(buffer.size() < length * 2, 0)) {
		buffer.resize(length * 2);
	}
	char *const buf = buffer.data();
	std::memcpy(buf, prefix.data(), prefix.size());
	std::memset(buf + prefix.size(), '\0',
	            length - prefix.size() - suffix.size());
	std::memcpy(buf + length - suffix.size(),
	            suffix.data(), suffix.size());

	Crawler crawler(nodes, mix(buf + length, buf, length), cb);
	crawler(buf + length, trie_root);
	return crawler.count();
}


#endif
