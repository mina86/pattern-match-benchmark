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

#ifndef H_WORDS_H
#define H_WORDS_H

#include <iterator>
#include <string_view>


struct WordsIter : std::iterator<std::random_access_iterator_tag,
                                 std::string_view> {
	WordsIter(const char *p, size_t length) : ptr(p), len(length) {}

	value_type operator*() const { return {ptr, len}; }
	value_type operator[](difference_type idx) const {
		return {ptr + idx, len};
	}

	WordsIter &operator++() {
		ptr += len;
		return *this;
	}
	WordsIter operator++(int) {
		const char *tmp = ptr;
		ptr += len;
		return {tmp, len};
	}
	WordsIter &operator--() {
		ptr -= len;
		return *this;
	}
	WordsIter operator--(int) {
		const char *tmp = ptr;
		ptr -= len;
		return {tmp, len};
	}

	WordsIter operator+(difference_type offset) const {
		return {ptr + offset * len, len};
	}
	WordsIter operator-(difference_type offset) const {
		return {ptr - offset * len, len};
	}
	WordsIter &operator+=(difference_type offset) {
		ptr += offset * len;
		return *this;
	}
	WordsIter &operator-=(difference_type offset) {
		ptr -= offset * len;
		return *this;
	}

	bool operator==(WordsIter rhs) const { return ptr == rhs.ptr; }
	bool operator!=(WordsIter rhs) const { return ptr != rhs.ptr; }
	bool operator< (WordsIter rhs) const { return ptr <  rhs.ptr; }
	bool operator<=(WordsIter rhs) const { return ptr <= rhs.ptr; }
	bool operator> (WordsIter rhs) const { return ptr >  rhs.ptr; }
	bool operator>=(WordsIter rhs) const { return ptr >= rhs.ptr; }

	difference_type operator-(WordsIter rhs) const {
		return (ptr - rhs.ptr) / len;
	}

private:
	const char *ptr;
	size_t len;
};


struct Words {
	using Map = std::map<std::string, size_t>;

	Words(const char *data, size_t count, size_t length)
		: words(make_map(data, count, length)), length(length) {}

	Words reverse() const {
		std::string word(word_length(), '\0');
		Map reversed;
		for (const auto &pair : words) {
			const std::string &src = pair.first;
			std::copy(src.rbegin(), src.rend(), word.data());
			reversed.emplace(word, pair.second);
		}
		return {reversed, length};
	}

	bool empty() const { return words.empty(); }
	size_t size() const { return words.size(); }
	size_t word_length() const { return length; }

	operator const Map &() const { return words; }
	auto begin() const { return words.begin(); }
	auto end() const { return words.end(); }

private:
	static Map make_map(
		const char *data, size_t count, size_t length) {
		Map words;
		for (size_t i = 0; i < count; ++i, data += length) {
			words.emplace(std::string(data, length), i);
		}
		return words;
	}

	Words(Map words, size_t length)
		: words(std::move(words)), length(length) {}

	const Map words;
	const size_t length;
};


#endif
