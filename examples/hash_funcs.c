#include <stdio.h>
#include <string.h>

#define BELT_IMPLEMENTATION
#include "../belt.h"

int main(int argc, char **argv)
{
	const char *string;

	if (argc > 1) {
		string = argv[1];
	} else {
		string = "Hello, world";
	}
	
	if (strlen(string) % 4 != 0) {
		printf("length must be multiple of 4!\n");
	}

	u32 hash = jhash(string, strlen(string), 0);

	printf("hash of %s is %u\n", string, hash);

	return 0;
}
