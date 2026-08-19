#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define BELT_IMPLEMENTATION
#include "../belt.h"

Slice open_file(const char *path)
{
	int fd = open(path, O_RDWR);
	if (fd == -1) {
		perror("error opening file");
		return NULL_SLICE;
	}

	struct stat sb;

	if (fstat(fd, &sb) == -1) {
		perror("error getting file size");
		close(fd);
		return NULL_SLICE;
	}

	char *file_memory = mmap(
		NULL, /* start address, NULL for any */
		sb.st_size, /* file size in bytes */
		PROT_READ | PROT_WRITE, /* mem region protection flags */
		MAP_SHARED, /* mem region type flags */
		fd, /* descriptor to map */
		0 /* offset to start mapping */
	);

	if (file_memory == MAP_FAILED) {
		perror("error mapping file");
		close(fd);
		return NULL_SLICE;
	}

	close(fd);

	return slice_from_parts(file_memory, sb.st_size);
}

void close_file(Slice *slice)
{
	if (munmap((char*)slice->str, slice->len) == -1) {
		perror("unmapping?");
	}
}

int main(int argc, char **argv)
{
	// open file
	Slice file = open_file(argv[1]);
	if (slice_empty(file)) {
		printf("file is empty\n");
		return 1;
	}
	printf("data:\n" SLICE_FMT, SLICE_ARG(file));

	Arena arena = arena_init(file.len);
	Json_Token *tokens = arena_alloc(&arena, file.len);
	Json_Parser parser = JSON_PARSER_INIT;

	// parse
	Result r = json_parse(file, tokens, &parser);
	if (!r.ok) {
		printf("json_parse failed with: %i,%s\n",
			r.status, r.message);
		return 1;
	}

	// close
	close_file(&file);

	return 0;
}
