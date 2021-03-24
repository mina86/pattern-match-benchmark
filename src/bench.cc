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


#ifndef MATCHER
#  error "Expected MATCHER to be defined"
#endif


using Query = std::pair<std::string_view, std::string_view>;


static char buffer[10'000'000];

static constexpr size_t max_word_length = 1'000'000;
static constexpr size_t max_count       = 1'000'000;
static constexpr size_t max_reps        = 100'000'000;
static constexpr double min_time_ns     = 1'000'000'000;


static constexpr size_t max_length(size_t count) {
	if (sizeof buffer < count) {
		return 0;
	}
	const size_t max = sizeof buffer / count;
	size_t ret = 1;
	while (ret * 10 <= max) {
		ret *= 10;
	}
	return std::min(max_word_length, ret);
}


static void generate_rand_data() COLD;
static void generate_rand_data() {
	std::mt19937_64 gen(42);
	std::uniform_int_distribution<char> distrib('a', 'z');
	for (char &ch : buffer) {
		ch = distrib(gen);
	}
}


static constexpr int64_t nsec(timespec spec) {
	const int64_t sec = spec.tv_sec, nsec = spec.tv_nsec;
	return sec * INT64_C(1'000'000'000) + nsec;
}


template <class Matcher>
static size_t run_bench(const Matcher &matcher,
                        size_t plen, size_t slen, size_t reps) {
	static_assert(max_word_length * 10 <= sizeof buffer);

	const size_t len = matcher.word_length();
	size_t sum;
	do {
		sum = 0;
		for (size_t i = 0; i < 20; ++i) {
			const char *const word = buffer +
				i * std::max<size_t>(len / 5, 1);
			const std::string_view prefix(word, plen);
			const std::string_view suffix(word + len - slen, slen);
			const auto ignore = [](uint32_t){};
			sum += matcher.query(prefix, suffix, ignore);
		}
	} while (--reps);
	return sum;
}


template <class Matcher>
static void run_bench(const char *name, const Matcher &matcher,
                      size_t plen, size_t slen) {
	printf("%-14s %8zu×%-8zu %8zu…%-8zu",
	       name, matcher.size(), matcher.word_length(), plen, slen);
	fflush(stdout);

	struct timespec start, end;
	for (size_t reps = 1;; reps = std::min<size_t>(reps, max_reps)) {
		clock_gettime(CLOCK_MONOTONIC_RAW, &start);

		const size_t sum = run_bench(matcher, plen, slen, reps);

		clock_gettime(CLOCK_MONOTONIC_RAW, &end);
		const int64_t ns = nsec(end) - nsec(start);

		if ((reps > 1 && ns > min_time_ns) || reps >=  max_reps) {
			const double us = ns / static_cast<double>(1000 * reps);
			printf(" %12.3f µs (%zu %zu)\n", us, sum, reps);
			fflush(stdout);
			break;
		}
		reps = std::max<size_t>(
			reps * 2, reps * 110 * min_time_ns / ns / 100);
	}
}


template <class Matcher>
static void run_bench(const char *name, const Matcher &matcher) {
	const size_t len = matcher.word_length();
	for (size_t plen = len;; plen /= 10) {
		for (size_t slen = std::min(plen, len - plen);; slen /= 10) {
			run_bench(name, matcher, plen, slen);
			if (!slen) {
				break;
			}
		}
		if (!plen) {
			break;
		}
	}
}


template <class Matcher>
static void run_bench(const char *name) {
	for (size_t count = max_count; count; count /= 10) {
		for (size_t len = max_length(count); len; len /= 10) {
			const Matcher matcher(Words(buffer, count, len));
			run_bench(name, matcher);
		}
	}
}


static void fix_name(std::string &name) {
	if (size_t pos = name.find("Matcher"); pos != std::string::npos) {
		name.erase(pos, 7);
	}
	if (size_t pos = name.find("<Trie"); pos != std::string::npos) {
		name.erase(pos + 1, 4);
	}
	if (size_t pos = name.find("Storage>"); pos != std::string::npos) {
		name.erase(pos, 7);
	}
}


#define STRINGIFY(s) STRINGIFY_INTERNAL(s)
#define STRINGIFY_INTERNAL(s) #s

int main() {
	generate_rand_data();
	std::string name(STRINGIFY(MATCHER));
	fix_name(name);
	run_bench<MATCHER>(name.c_str());
	return 0;
}
