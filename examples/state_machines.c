#include <stdio.h>

#define BELT_IMPLEMENTATION
#include "../belt.h"

int get_next_char(FILE *fp)
{
	#define nextc(c) if ((c = fgetc(fp)) == EOF) fsm_exit()

	coroutine_begin();

	fsm {
		int c;

		fsm_state(START) {
			fsm_goto(code);
		}

		fsm_state(code) {
			nextc(c);
			if (c == '*') fsm_goto(comment);
			coroutine_yield(c);
			fsm_goto(code);
		}

		fsm_state(comment) {
			nextc(c);
			if (c == '*') fsm_goto(code);
			fsm_goto(comment);
		}
	}

	coroutine_end(EOF);

	#undef nextc
}

int main(int argc, char **argv)
{
	FILE *fp = fopen(argv[1], "r");

	printf("stripped:\n");

	int c;
    while ((c = get_next_char(fp)) != EOF) {
		putchar(c);
		putchar('\n');
	}

	fclose(fp);

	return 0;
}
