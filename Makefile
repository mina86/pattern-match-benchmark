LDFLAGS  := -flto -fwhole-program
CXXFLAGS := -std=c++20 -O3
CPPFLAGS := -Wall -Wextra -Werror

benches := dist/bench-bmap dist/bench-vec dist/bench-vec-range \
           dist/bench-trie-pool dist/bench-trie-alloc \
           dist/bench-mix-pool dist/bench-mix-alloc

all: test bench

test:: dist/test
	$^

benches: $(benches)

bench:: $(benches)
	for bench in $^; do "$$bench"; done


define compile
dist/$1: src/$2.cc $(wildcard src/*.h)
	@mkdir -p -- $$(dir $$@)
	$(CXX) $(LDFLAGS) $(CXXFLAGS) $(CPPFLAGS) $3 -o $$@ $$<

dist/$1.pg: src/$2.cc $(wildcard src/*.h)
	@mkdir -p -- $$(dir $$@)
	$(CXX) $(LDFLAGS) $(CXXFLAGS) $(CPPFLAGS) $3 -pg -g -o $$@ $$<

dist/$1.dbg: src/$2.cc $(wildcard src/*.h)
	@mkdir -p -- $$(dir $$@)
	$(CXX) $(LDFLAGS) $(CXXFLAGS) $(CPPFLAGS) $3 -ggdb -O0 -o $$@ $$<
endef

define bench
$(eval $(call compile,bench-$1,bench,-D "MATCHER=$2"))
endef

$(eval $(call compile,test,test))

$(eval $(call bench,bmap,BitmapMatcher))
$(eval $(call bench,vec,VectorMatcher))
$(eval $(call bench,vec-range,VectorRangeMatcher))
$(eval $(call bench,trie-pool,TrieMatcher<TriePoolStorage>))
$(eval $(call bench,trie-alloc,TrieMatcher<TrieAllocStorage>))
$(eval $(call bench,mix-pool,TrieMixMatcher<TriePoolStorage>))
$(eval $(call bench,mix-alloc,TrieMixMatcher<TrieAllocStorage>))


clean::
	rm -rf -- dist

distclean: clean
