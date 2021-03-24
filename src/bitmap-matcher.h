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

#ifndef H_BITMAP_MATCHER_H
#define H_BITMAP_MATCHER_H

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "util.h"
#include "vector-map.h"
#include "words.h"


struct BitmapMatcher {
	BitmapMatcher(const Words &words)
		: fwd(words), rev(words.reverse()),
		  bitmap(new uint64_t[sizeof_bitmap(words.size())]) {}


	size_t size() const { return fwd.size(); }
	size_t word_length() const { return fwd.key_length(); }
	template <class Callback>
	inline size_t query(std::string_view prefix,
	                    std::string_view suffix,
	                    Callback &cb) const HOT;

private:
	static constexpr uint64_t sizeof_bitmap(size_t count) {
		return (count + 63) / 64;
	}

	VectorMap fwd, rev;
	mutable std::unique_ptr<uint64_t[]> bitmap;

	BitmapMatcher() = delete;
	BitmapMatcher(const BitmapMatcher&) = delete;
};

template <class Callback>
size_t BitmapMatcher::query(std::string_view prefix,
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

	uint64_t *const bm = bitmap.get();
	std::memset(bitmap.get(), 0, sizeof_bitmap(size()));

	fwd.for_each_value(prefix, [bm](uint32_t v) {
		bm[v / 64] |= UINT64_C(1) << (v % 64);
	});

	std::string suffix_str(suffix.rbegin(), suffix.rend());
	size_t cnt = 0;
	rev.for_each_value(suffix_str, [&cnt, &cb, bm](uint32_t v) {
		if (bm[v / 64] & (UINT64_C(1) << (v % 64))) {
			cb(v);
			++cnt;
		}
	});
	return cnt;
}


#endif
