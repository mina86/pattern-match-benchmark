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

#ifndef H_VECTOR_MATCHER_H
#define H_VECTOR_MATCHER_H

#include <algorithm>
#include <cstring>
#include <exception>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "util.h"
#include "vector-map.h"
#include "words.h"


struct VectorMatcher {
	using Query = std::pair<std::string_view, std::string_view>;

	VectorMatcher(const Words &words) : words(words) {};

	size_t size() const { return words.size(); }
	size_t word_length() const { return words.key_length(); }
	template <class Callback>
	inline size_t query(std::string_view prefix,
	                    std::string_view suffix,
	                    Callback &cb) const HOT;

private:
	VectorMap words;

	VectorMatcher() = delete;
	VectorMatcher(const VectorMatcher&) = delete;
};


template <class Callback>
size_t VectorMatcher::query(std::string_view prefix,
                            std::string_view suffix,
                            Callback &cb) const {
	if (prefix.size() + suffix.size() == word_length() || suffix.empty()) {
		return words.for_each_value(std::string(prefix) += suffix, cb);
	}

	const size_t offset = word_length() - suffix.size();
	size_t cnt = 0;
	words.for_each_pair(
		prefix,
		[&cb, &cnt, suffix, offset](std::string_view key, uint32_t v) {
			key = std::string_view(
				key.data() + offset, suffix.size());
			if (key == suffix) {
				cb(v);
				++cnt;
			}
		});

	return cnt;
}


#endif
