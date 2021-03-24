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

#ifndef H_VECTOR_RANGE_MATCHER_H
#define H_VECTOR_RANGE_MATCHER_H

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "util.h"
#include "vector-map.h"
#include "words.h"


struct VectorRangeMatcher {
	VectorRangeMatcher(const Words &words)
		: fwd(words), rev(make_rev(fwd)) {}


	size_t size() const { return fwd.size(); }
	size_t word_length() const { return fwd.key_length(); }
	template <class Callback>
	inline size_t query(std::string_view prefix,
	                    std::string_view suffix,
	                    Callback &cb) const HOT;

private:
	static VectorMap make_rev(const VectorMap &fwd) {
		std::string word(fwd.key_length(), '\0');
		Words::Map reversed;
		for (const auto &pair : fwd) {
			std::string_view src = pair.first;
			std::copy(src.rbegin(), src.rend(), word.data());
			reversed.emplace(word, pair.second);
		}
		return VectorMap(fwd.key_length(), reversed);
	}

	VectorMap fwd, rev;

	VectorRangeMatcher() = delete;
	VectorRangeMatcher(const VectorRangeMatcher&) = delete;
};

template <class Callback>
size_t VectorRangeMatcher::query(std::string_view prefix,
                                 std::string_view suffix,
                                 Callback &cb) const {
	if (prefix.size() + suffix.size() == word_length() ||
	    prefix.empty() || suffix.empty()) {
		const bool isSuffix = prefix.empty() &&
			suffix.size() != word_length();
		std::string ix = std::string(prefix) += suffix;
		if (isSuffix) {
			std::reverse(ix.begin(), ix.end());
		}
		return (isSuffix ? rev : fwd).for_each_value(ix, cb);
	}

	const auto [lo, hi] = fwd.range(prefix);
	std::string suffix_str(suffix.rbegin(), suffix.rend());
	size_t cnt = 0;
	rev.for_each_value(suffix_str, [&cnt, &cb, lo, hi, this](uint32_t v) {
		if (lo <= v && v < hi) {
			cb(fwd.value(v));
			++cnt;
		}
	});
	return cnt;
}


#endif
