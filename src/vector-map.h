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

#ifndef H_VECTOR_MAP_H
#define H_VECTOR_MAP_H

#include <algorithm>
#include <cstring>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "words.h"


struct VectorMap {
	VectorMap(const Words &words) : VectorMap(words.word_length(), words) {}
	VectorMap(size_t length, const Words::Map &words)
		: count(words.size()), length(length),
		  keys(new char[count * length]), values(new uint32_t[count]) {
		auto kk = keys.get();
		auto vv = values.get();
		for (auto &[k, v] : words) {
			std::memcpy(kk, k.data(), length);
			*vv = v;
			kk += length;
			++vv;
		}
	}

	template <class Fn>
	void for_each_pair(std::string_view prefix, const Fn &fn) const {
		auto [i, last] = range(prefix);
		for (; i < last; ++i) {
			const std::string_view key(
				keys.get() + i * length, length);
			fn(key, values.get()[i]);
		}
	}

	template <class Fn>
	size_t for_each_value(std::string_view prefix, const Fn &fn) const {
		auto [first, last] = range(prefix);
		const uint32_t *it = values.get() + first;
		const uint32_t *const end = values.get() + last;
		const size_t count = last - first;
		for (; it < end; ++it ){
			fn(*it);
		}
		return count;
	}

	std::pair<size_t, size_t> range(std::string_view prefix) const {
		const WordsIter beg(keys.get(), length);
		const auto [fst, lst] = std::equal_range(
			beg, beg + count, Prefix(prefix));
		return {fst - beg, lst - beg};
	}

	size_t size() const { return count; }
	size_t key_length() const { return length; }

	const char *key(size_t idx) const {
		return keys.get() + idx * key_length();
	}
	uint32_t value(size_t idx) const {
		return values[idx];
	}

	struct Iterator : std::iterator<std::random_access_iterator_tag,
	                                std::pair<std::string_view, uint32_t>> {
		Iterator() = delete;
		Iterator(WordsIter key, const uint32_t *value)
			: key(key), value(value) {}

		value_type operator*() const { return {*key, *value}; }
		value_type operator[](difference_type offset) const {
			return {key[offset], value[offset]};
		}

		Iterator &operator++() { ++key; ++value; return *this; }
		Iterator &operator--() { --key; --value; return *this; }
		Iterator operator++(int) { return {key++, value++}; }
		Iterator operator--(int) { return {key--, value--}; }

		Iterator operator+(difference_type offset) const {
			return {key + offset, value + offset};
		}
		Iterator operator-(difference_type offset) const {
			return {key - offset, value - offset};
		}

		bool operator==(Iterator rhs) const { return key == rhs.key; }
		bool operator!=(Iterator rhs) const { return key != rhs.key; }
		bool operator<=(Iterator rhs) const { return key <= rhs.key; }
		bool operator< (Iterator rhs) const { return key <  rhs.key; }
		bool operator>=(Iterator rhs) const { return key >= rhs.key; }
		bool operator> (Iterator rhs) const { return key >  rhs.key; }

		difference_type operator-(Iterator rhs) const {
			return value - rhs.value;
		}

	private:
		WordsIter key;
		const uint32_t *value;
	};

	Iterator begin() const {
		return {WordsIter(keys.get(), length), values.get()};
	}
	Iterator end() const { return begin() + count; }

private:
	const size_t count;
	const size_t length;
	const std::unique_ptr<char[]> keys;
	const std::unique_ptr<uint32_t[]> values;

	struct Prefix {
		explicit Prefix(std::string_view pre) : prefix(pre) {}

		bool lt(std::string_view str) const {
			return prefix < std::string_view(
				str.data(), prefix.size());
		}
		bool gt(std::string_view str) const {
			return prefix > std::string_view(
				str.data(), prefix.size());
		}

	private:
		const std::string_view prefix;
	};

	friend bool operator<(VectorMap::Prefix prefix, std::string_view str) {
		return prefix.lt(str);
	}
	friend bool operator<(std::string_view str, VectorMap::Prefix prefix) {
		return prefix.gt(str);
	}

	VectorMap(const VectorMap&) = delete;
};


#endif
