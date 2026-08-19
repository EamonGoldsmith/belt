#include <stdio.h>

#define BELT_IMPLEMENTATION
#include "../belt.h"

int main()
{
	// basic slice functionality
	Slice hello = SLICE("hello, world!");
	printf(SLICE_FMT"\n", SLICE_ARG(hello));

	// slice list iterator
	Slice groceries = SLICE("milk, eggs, flour, sugar");
	
	for (;;) {
		Slice item = slice_split(&groceries, SLICE(","));
		if (slice_empty(item)) break;
		printf(SLICE_FMT"\n", SLICE_ARG(item));
	}

	// slice trimming 
	Slice padded = SLICE("   \n\n Hello, World\n \v\f");
	Slice trimmed = slice_trim(padded, iswhitespace);
	printf(SLICE_FMT"\n", SLICE_ARG(trimmed)); // output: "Hello, World"

	// with lambdas
	Slice dirty = SLICE("aaaaaaaaaaWHATaaaaaaaaaaa");
	trim_func remove_a = lambda(int, (char c) {
		if (c == 'a') { return 1; }
		return 0;
	});
	Slice clean = slice_trim(dirty, remove_a);
	printf(SLICE_FMT"\n", SLICE_ARG(clean)); // output: "WHAT"

	// with lambdas
	Slice withnumbers = SLICE("2132193021FUCKYOU231321");
	Slice withoutnumbers = slice_trim(withnumbers, (trim_func)isdigit);
	printf(SLICE_FMT"\n", SLICE_ARG(withoutnumbers)); // output: "WHAT"

	return 0;
}
