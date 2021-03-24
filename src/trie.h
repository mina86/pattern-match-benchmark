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

#ifndef H_TRIE_H
#define H_TRIE_H

#include <array>
#include <limits>
#include <string>
#include <string_view>
#include <vector>
#include <deque>

#include "util.h"

template <class Value, class Self>
struct TrieStorageBase {
	struct node_type;

	struct value_type {
		constexpr value_type() = default;
		value_type(const value_type &) = default;

		static constexpr value_type from_num(Value value) {
			return {Self::encode_value(value)};
		}
		constexpr Value as_num() const {
			return Self::decode_value(val);
		}

		constexpr explicit operator bool () const {
			return *this != Self::null();
		}
		constexpr bool     operator!     () const {
			return *this == Self::null();
		}

		constexpr bool operator==(value_type rhs) const {
			return val == rhs.val;
		}
		constexpr bool operator!=(value_type rhs) const {
			return val != rhs.val;
		}

	private:
		static constexpr value_type from_ptr(const void *ptr) {
			static_assert(sizeof(Value) >= sizeof ptr);
			return {reinterpret_cast<Value>(ptr)};
		}
		constexpr node_type *as_ptr() const {
			static_assert(sizeof(void*) <= sizeof(val));
			return reinterpret_cast<node_type*>(val);
		}

		static constexpr value_type from_raw(Value v) {
			return {v};
		}
		constexpr Value as_raw() const {
			return val;
		}

		constexpr value_type(Value v) : val(v) {}

		Value val;

		friend Self;
	};

	struct node_type : std::array<value_type, 26> {
		node_type() = default;
		node_type(value_type v) { this->fill(v); }
	};

	value_type follow(value_type node_pos, std::string_view str) const HOT {
		for (const char ch : str) {
			node_pos = node(node_pos)[ch - 'a'];
		}
		return node_pos;
	}

	value_type follow(value_type node_pos, char ch) const HOT {
		return node(node_pos)[ch - 'a'];
	}

	template <class Fn, class... Args>
	void deep_fan_out(value_type node_pos, size_t depth,
	                  const Fn &fn, Args&&... args) const HOT;

	template <class Fn, class... Args>
	void fan_out(value_type node_pos,
	             const Fn &fn, Args&&... args) const HOT;

	template <class It>
	void insert(value_type node_pos, It firstChar, It lastChar,
	            value_type data) COLD;

private:
	template <class Fn, class... Args>
	void do_fan_out(value_type node_pos, size_t depth,
	                const Fn &fn, Args&&... args) const HOT;

	value_type add_node() COLD {
		return static_cast<Self*>(this)->add_node();
	}

	constexpr node_type &node(value_type data) HOT {
		return static_cast<Self *>(this)->node(data);
	}
	constexpr const node_type &node(value_type data) const HOT {
		return static_cast<const Self *>(this)->node(data);
	}
};

template <class Value, class Self>
template <class It>
void TrieStorageBase<Value, Self>::insert(
	value_type node_pos, It firstChar, It lastChar, value_type data) {
	for (;;) {
		const char ch = *firstChar++ - 'a';
		if (firstChar == lastChar) {
			node(node_pos)[ch] = data;
			break;
		}

		value_type next = node(node_pos)[ch];
		if (!next) {
			next = add_node();
			node(node_pos)[ch] = next;
		}
		node_pos = next;
	}
}


template <class Value, class Self>
template <class Fn, class... Args>
void TrieStorageBase<Value, Self>::fan_out(
	value_type node_pos, const Fn &fn, Args&&... args) const {
	for (const value_type data : node(node_pos)) {
		if (data) {
			fn(std::forward<Args>(args)..., data);
		}
	}
}

template <class Value, class Self>
template <class Fn, class... Args>
void TrieStorageBase<Value, Self>::deep_fan_out(
	value_type node_pos, size_t depth, const Fn &fn, Args&&... args) const {
	if (depth) {
		do_fan_out(node_pos, depth,
			   fn, std::forward<Args>(args)...);
	} else if (node_pos) {
		fn(std::forward<Args>(args)..., node_pos);
	}
}


template <class Value, class Self>
template <class Fn, class... Args>
void TrieStorageBase<Value, Self>::do_fan_out(
	value_type node_pos, size_t depth, const Fn &fn, Args&&... args) const {

	using StackFrame = std::pair<const node_type*, size_t>;
	const bool use_alloca = depth <= 4096 / sizeof(StackFrame);
	StackFrame *const stack = reinterpret_cast<StackFrame*>(
		use_alloca ? alloca(depth * sizeof *stack)
		           : malloc(depth * sizeof *stack));
	StackFrame *sp = stack, *const last = stack + depth - 1;

	const node_type *n = sp->first = &node(node_pos);
	size_t ch_pos = sp->second = 0;

	for (;;) {
		node_pos = (*n)[ch_pos++];

		if (node_pos && sp != last) {
			sp->second = ch_pos;
			++sp;
			sp->first = n = &node(node_pos);
			sp->second = ch_pos = 0;
			continue;
		}

		if (node_pos) {
			fn(std::forward<Args>(args)..., node_pos);
		}

		if (ch_pos == 26) {
			do {
				if (sp == stack) {
					goto out;
				}
				--sp;
				ch_pos = sp->second;
			} while (ch_pos == 26);
			n = sp->first;
		}
	}

out:
	if (!use_alloca) {
		free(stack);
	}
}


struct TriePoolStorage : TrieStorageBase<uint32_t, TriePoolStorage> {
	TriePoolStorage() : nodes(1) {}

	static constexpr value_type null() {
		return value_type::from_raw(0u);
	}

	value_type add_node() COLD {
		nodes.emplace_back();
		return value_type::from_raw(nodes.size() - 1);
	}

	void free_trie(value_type, size_t) COLD {}

private:
	static constexpr uint32_t encode_value(uint32_t v) { return v + 1; }
	static constexpr uint32_t decode_value(uint32_t v) { return v - 1; }

	node_type &node(value_type data) {
		return nodes[data.as_raw()];
	}
	const node_type &node(value_type data) const {
		return nodes[data.as_raw()];
	}

	std::vector<node_type> nodes;

	friend class TrieStorageBase<uint32_t, TriePoolStorage>;
	friend class TrieStorageBase<uint32_t, TriePoolStorage>::value_type;
};


struct TrieAllocStorage : TrieStorageBase<uintptr_t, TrieAllocStorage> {
	static value_type null() {
		static const node_type null_node(
			value_type::from_ptr(&null_node));
		return value_type::from_ptr(&null_node);
	}

	value_type add_node() COLD {
		return value_type::from_ptr(new node_type(null()));
	}

	void free_trie(value_type root, size_t depth) COLD {
		std::deque<std::pair<node_type*, size_t>> nodes;
		nodes.emplace_back(root.as_ptr(), depth);
		while (!nodes.empty()) {
			node_type *const node = nodes.front().first;
			depth = nodes.front().second;
			nodes.pop_front();
			if (depth) {
				--depth;
				for (auto value : *node) {
					if (value) {
						nodes.emplace_back(
							value.as_ptr(), depth);
					}
				}
			}
			delete node;
		}
	}

private:
	static constexpr uintptr_t encode_value(uintptr_t v) { return v; }
	static constexpr uintptr_t decode_value(uintptr_t v) { return v; }

	constexpr node_type &node(value_type data) {
		return *data.as_ptr();
	}
	constexpr const node_type &node(value_type data) const {
		return *data.as_ptr();
	}

	friend class TrieStorageBase<uintptr_t, TrieAllocStorage>;
	friend class TrieStorageBase<uintptr_t, TrieAllocStorage>::value_type;
};


#endif
