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

#include <algorithm>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <utility>
#include <random>

#include "bitmap-matcher.h"
#include "trie-matcher.h"
#include "trie-mix-matcher.h"
#include "vector-matcher.h"
#include "vector-range-matcher.h"


static void print_header(const char *name, size_t count, size_t length) {
	fprintf(stderr, "\33[1;37m%s\33[0m %zu×%zu\n",
	        name, count, length);
}

static const char result_message[2][20] = {
	"\33[1;31mFAIL\33[0;32m",
	"\33[1;32mPASS\33[0;32m"
};

static void print_result(bool result, std::string_view prefix,
                         std::string_view suffix, size_t length) {
	fprintf(stderr, "  %s <%.*s %zd %.*s>\33[0m\n",
		result_message[result],
		static_cast<int>(prefix.size()), prefix.data(),
		length - prefix.size() - suffix.size(),
		static_cast<int>(suffix.size()), suffix.data());
}


static void print_array(const char *label, const uint32_t *it, size_t size) {
	const uint32_t *const end = it + size;
	fprintf(stderr, "    %-4s: [%2zu]", label, size);
	for (; it != end; ++it) {
		fprintf(stderr, " %" PRIu32, *it);
	}
	fputs("\n", stderr);
}


template <class Matcher>
static bool check(const Matcher &matcher,
                  std::string_view prefix, std::string_view suffix,
                  std::initializer_list<uint32_t> want,
                  std::vector<uint32_t> &got) {
	got.clear();
	const auto cb = [&got](uint32_t val) { got.push_back(val); };
	const size_t got_count = matcher.query(prefix, suffix, cb);
	std::sort(got.begin(), got.end());

	const bool ok = got_count == want.size() &&
		!memcmp(want.begin(), got.data(), want.size() * sizeof got[0]);
	print_result(ok, prefix, suffix, matcher.word_length());
	if (!ok) {
		print_array("want", want.begin(), want.size());
		print_array("got", got.data(), got.size());
	}
	return ok;
}


#define CHECK(prefix, suffix, ...) \
	(ok = check(matcher, prefix, suffix, { __VA_ARGS__ }, got) && ok)

template <class Matcher>
static bool run_small_tests(const char *name) {
	bool ok = true;

	for (size_t i = 3; i; --i) {
		print_header(name, 0, i);
		const Matcher matcher(Words("", 0, i));
		std::vector<uint32_t> got(0);
		CHECK("", "");
		if (i) {
			CHECK("x", "");
			CHECK("", "x");
		}
	}

	for (size_t i = 3; i; --i) {
		print_header(name, 1, i);
		const Matcher matcher(Words("foo", 1, i));
		std::vector<uint32_t> got(1);
		CHECK("", "", 0);
		if (i > 0) CHECK("f", "", 0);
		if (i > 1) CHECK("fo", "", 0);
		if (i > 2) CHECK("foo", "", 0);
		if (i == 1) CHECK("", "f", 0);
		if (i >  1) CHECK("f", "o", 0);
	}

	return ok;
}

static const Words words("foobarbazquxqax", 5, 3);

template <class Matcher>
static bool run_medium_tests(const char *name) {
	print_header(name, words.size(), words.word_length());
	const Matcher matcher(words);
	std::vector<uint32_t> got(words.size());

	bool ok = true;

	CHECK("", "", 0, 1, 2, 3, 4);
	CHECK("f", "", 0);
	CHECK("b", "", 1, 2);
	CHECK("q", "", 3, 4);
	CHECK("z", "");
	CHECK("bx", "");
	CHECK("", "r", 1);
	CHECK("", "x", 3, 4);
	CHECK("q", "x", 3, 4);

	for (const auto &pair : words) {
		const std::string_view word(pair.first);
		for (size_t n = 0; n <= 3; ++n) {
			CHECK(word.substr(0, n), word.substr(n),
			      static_cast<uint32_t>(pair.second));
		}
	}

	return ok;
}

#undef CHECK

static const char *random_buffer() {
	static std::vector<char> buffer;
	if (buffer.empty()) {
		buffer.resize(10'000'000);
		std::mt19937_64 gen(0x42);
		std::uniform_int_distribution<char> distrib('a', 'z');
		for (char &ch : buffer) {
			ch = distrib(gen);
		}
	}
	return buffer.data();
}

template <class Matcher>
static bool run_huge_tests(const char *name) {
	const Words words(random_buffer(), 10, 1'000'000);
	print_header(name, words.size(), words.word_length());
	const Matcher matcher(words);
	const size_t len = matcher.word_length();
	for (size_t i = 0; i < 7; ++i) {
		const size_t plen = !!(i & 4) * 50'000;
		const size_t slen = !!(i & 2) * 50'000;
		const char *const word = random_buffer() + (i & 1) * 50'000;
		const std::string_view prefix(word, plen);
		const std::string_view suffix(word + len - slen, slen);
		const auto ignore = [](uint32_t){};
		matcher.query(prefix, suffix, ignore);
		fprintf(stderr, "  %s <%zu %zu %zu> (deep recursion)\33[0m\n",
			result_message[true], plen, len - plen - slen, slen);
	}
	return true;
}

template <class Matcher>
static bool run_tests(const char *name) {
	bool ok = true;
	ok = run_small_tests<Matcher>(name) && ok;
	ok = run_medium_tests<Matcher>(name) && ok;
	ok = run_huge_tests<Matcher>(name) && ok;
	return ok;
}



int main(void) {
	bool ok = true;
#define RUN_TESTS(cls) (ok = run_tests<cls>(#cls) && ok)
	RUN_TESTS(BitmapMatcher);
	RUN_TESTS(TrieMatcher<TrieAllocStorage>);
	RUN_TESTS(TrieMatcher<TriePoolStorage>);
	RUN_TESTS(TrieMixMatcher<TrieAllocStorage>);
	RUN_TESTS(TrieMixMatcher<TriePoolStorage>);
	RUN_TESTS(VectorMatcher);
	RUN_TESTS(VectorRangeMatcher);
#undef RUN_TESTS
	return !ok;
}
