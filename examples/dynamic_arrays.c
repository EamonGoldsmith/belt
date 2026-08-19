#include <stdio.h>
#include <stdlib.h>

#define BELT_IMPLEMENTATION
#include "../belt.h"

int main()
{
	Arena str = arena_init(1028);

	arena_appendf(&str, "hello");
	arena_appendf(&str, ",");
	arena_appendf(&str, "world\n");
	arena_append_null(&str);

	printf("arena: %s\n", (char*)str.start);

	Slice words = arena_as_slice(&str, NULL);
	printf(SLICE_FMT"\n", SLICE_ARG(words));

	return 0;
}
