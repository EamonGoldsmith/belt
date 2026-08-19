/*
	Belt (as in toolbelt) library

	Tool kit of things I like using and use frequently, available as
	a single stb-style header. The are no strict rules but I've tried to keep
	everything fairly platform agnostic and under 2000 lines. Most of this comes
	from the linux kernel or BSD kernel.

	Includes:
	- basic macro helpers
	- some shorter primitives
	- result type
	- arena allocator using malloc
	- reference counted pointers
	- vector style dynamic array
	- finite state machines
	- simple duff's device coroutines
	- string slices
	- json library
	- slab allocator
	- redblack trees
	- hash map
	- ring arena thing

	Eamon, 2026
*/

#ifndef BELT_H_
#define BELT_H_

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

#define MINIMUM(a, b) (a < b ? a : b)
#define CLAMP(a) (a < 0 ? 0 : a)
#define TOUCH(a) ((void)a)

/*
	this lambda trick is a bit unusual, so I'll provide an example:

	typedef int (*op_type)(int a, int b); // define a function type
	op_type add = lambda(int, (int a, int b) { return a + b; }); // create lambda
	int res = add(1, 2); // use lambda
	
	this makes things like predicates for slices and initialisers
	for slabs a bit easier.
*/
#define lambda(return_type, body) ({ return_type __fn__ body; __fn__; })

/* shorter type names, everyone knows what they mean */
typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;
typedef uint64_t u64;
typedef int64_t i64;
typedef float f32;
typedef double f64;

/* result type */
typedef struct {
	int ok;
	int status;
	const char *message;
} Result;

#define RESULT_OK() (Result){ 1, 0, "" }
#define RESULT_ERR(stat, msg) (Result){ 0, stat, msg }
#define RESULT_PASS_UP(result) if (!result.ok) { return result; }

/* finite state machines */
#define fsm if (1) {goto _fsm_state_START; _fsm_exit: ;} else 
#define fsm_state(s) \
for(;;) for(;;) if (1) goto _fsm_exit; else _fsm_state_ ## s:
#define fsm_goto(s) goto _fsm_state_ ## s
#define fsm_exit() goto _fsm_exit

/* duff's device coroutines */
#define coroutine_begin() static int __cr_state=0; switch(__cr_state) { case 0:
#define coroutine_yield(...) do { __cr_state=__LINE__; return __VA_ARGS__; case __LINE__:; } while (0)
#define coroutine_end(...) do { __cr_state=-1; default: return __VA_ARGS__; } while (0); }
#define coroutine_reset() __cr_state=0;

/* string slices/views */
typedef struct {
	const char *str;
	size_t len;
} Slice;

#define SLICE(s) slice_from_parts(s, sizeof(s) - 1)
#define STATIC_SLICE(s) Slice { s, sizeof(s) - 1 }
#define COPY_SLICE(s) slice_from_parts(s.str, s.len)
#define NULL_SLICE slice_from_parts(NULL, 0)
#define SLICE_FMT "%.*s"
#define SLICE_ARG(s) (int) (s.len), (s.str)

Slice slice_from_parts(const char *_str, size_t _len); // create slice
int slice_empty(Slice s); // true if input is NULL_SLICE
int slice_cmp(Slice _l, Slice _r); // strcmp slices (safe)
Slice slice_chop(Slice *input, Slice delim); // remove delim from input
Slice slice_split(Slice *input, Slice delim); // generator for split input

typedef int (*trim_func)(char);
Slice slice_trim_front(Slice input, trim_func predicate); // trim leading
Slice slice_trim_back(Slice input, trim_func predicate); // trim trailing
Slice slice_trim(Slice input, trim_func predicate); // trim both
int iswhitespace(char c);

#ifdef BELT_IMPLEMENTATION

Slice slice_from_parts(const char *_str, size_t _len)
{
	Slice s = { _str, _len };
	return s;
}

int slice_empty(Slice s)
{
	if (s.str == NULL || s.len == 0) return 1;
	return 0;
}

int slice_cmp(Slice _l, Slice _r)
{
	size_t n = MINIMUM(_l.len, _r.len);
	const char *l = _l.str, *r = _r.str;
	for (; *l && *r && n && *l == *r ; l++, r++, n--);
	return *l - *r;
}

Slice slice_chop(Slice *input, Slice delim)
{
	Slice window = slice_from_parts(input->str, delim.len);
	size_t i = 0;

	while (
		i + delim.len < input->len
	     && (slice_cmp(window, delim) > 0)
	) {
		i++;
		window.str++;
	}
	if (i == 0) return NULL_SLICE;

	Slice result = slice_from_parts(input->str, i);

	if (i + delim.len == input->len) {
		result.len += delim.len;
	} else {
		result.len -= delim.len;
	}

	input->str += i + delim.len;
	input->len -= i + delim.len;

	return result;
}

Slice slice_split(Slice *input, Slice delim)
{
	coroutine_begin();
	Slice result;
	for (;;) {
		result = slice_chop(input, delim);
		if (slice_empty(result)) return NULL_SLICE;
		coroutine_yield(result);
	}
	coroutine_end(NULL_SLICE);
}

Slice slice_trim_front(Slice input, trim_func predicate)
{
	if (slice_empty(input)) { return NULL_SLICE; }
	Slice trimmed = input;
	while (predicate(trimmed.str[0])) {
		trimmed.str++;
		trimmed.len--;
	}
	return trimmed;
}

Slice slice_trim_back(Slice input, trim_func predicate)
{
	if (slice_empty(input)) { return NULL_SLICE; }
	Slice trimmed = input;
	while (predicate(trimmed.str[trimmed.len - 1])) {
		trimmed.len--;
	}
	return trimmed;
}

Slice slice_trim(Slice input, trim_func predicate)
{
	Slice trimmed = slice_trim_front(input, predicate);
	trimmed = slice_trim_back(trimmed, predicate);
	return trimmed;
}

int iswhitespace(char c)
{
	if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v'
	|| c == '\f') { return 1; }
	return 0;
}

#endif // BELT_IMPLEMENTATION

/*
	arena allocator, backed by malloc
	don't free a child arena, just reset it.
	check an arena init failed by capacity != initial size

	init - create a new arena,
	scratch - create an arena contained by another,
	alloc - like malloc, but for arena
	bump - guarantees no syscall allocation,
	reset - move write back to beginning, leave capacity
	free - free underlying memory of arena
*/
typedef struct {
	void *start;
	void *ptr;
	size_t capacity;
} Arena;

#define IS_VALID_ARENA(arena) \
	(arena.start != NULL && arena.ptr == NULL)

Arena arena_init(size_t size);
void* arena_alloc(Arena *arena, size_t size);
void arena_reset(Arena *arena);
void arena_free(Arena *arena);
int arena_appendf(Arena *arena, char *fmt, ...);
int arena_append_null(Arena *arena);
int arena_append_slice(Arena *arena, Slice sl);
Slice arena_as_slice(Arena *arena, char *start);

#ifdef BELT_IMPLEMENTATION

Arena arena_init(size_t size)
{
	void *region = malloc(size);
	if (region == NULL) {
		return (Arena) {0};
	}
	return (Arena) {
		.start = region,
		.ptr = region,
		.capacity = size,
	};
}

void* arena_alloc(Arena *arena, size_t size)
{
	if (arena == NULL) { return NULL; }
	if (arena->start == NULL) { return NULL; }
	if (size == 0) { return arena->ptr; }

	if (size + (arena->ptr - arena->start) >= arena->capacity) {
		arena->start = realloc(
			arena->start, 
			arena->capacity = (2 * arena->capacity) + size
		);
	}

	void *region = arena->ptr;
	arena->ptr += size;
	return region;
}

void arena_reset(Arena *arena)
{
	arena->ptr = arena->start;
}

void arena_free(Arena *arena)
{
	free(arena->start);
	arena->start = NULL;
	arena->ptr = NULL;
	arena->capacity = 0;
}

int arena_appendf(Arena *arena, char *fmt, ...)
{
	va_list ap;
    va_start(ap, fmt);
	int length = vsnprintf(NULL, 0, fmt, ap);
    if (length > 0) {
		char *s = arena_alloc(arena, length);
		printf("%p\n", s);
		if (s == NULL) return -1;
        vsnprintf(s, length, fmt, ap);
    }
    va_end(ap);
	return length;
}

int arena_append_slice(Arena *arena, Slice sl)
{
	char *str = arena_alloc(arena, sl.len);
	if (str == NULL) return 0;
	for (int i = 0; i < sl.len; i++) {
		str[i] = sl.str[i];
	}
	return 1;
}

int arena_append_null(Arena *arena)
{
	char *c = arena_alloc(arena, 1);
	if (c == NULL) return 0;
	*c = (char)0;
	return 1;
}

Slice arena_as_slice(Arena *arena, char *start)
{
	if (arena == NULL) { return NULL_SLICE; }
	if (start == NULL) {
		// whole arena
		return slice_from_parts(arena->start, (arena->ptr - arena->start));
	}
	// check bounds
	if ((void*)start < arena->start || (void*)start > arena->ptr) {
		return NULL_SLICE;
	}
	// create slice
	return slice_from_parts(start, arena->ptr - arena->start - (size_t)start);
}

#endif // BELT_IMPLEMENTATION

/* dynamic arrays */
typedef struct {
	size_t size;
	size_t capacity;
} __da_header;

#define DYN_INITIAL_SIZE 1028

#define da_create(arr) \
do { \
	__da_header* header = malloc(arr, \
		(sizeof(*arr) * DYN_INITIAL_SIZE) + sizeof(__da_header)); \
	header->size = 0; \
	header->capacity = DYN_INITIAL_SIZE; \
	(arr) = (void*)(header + 1); \
} while (0);

#define __get_da_header(da) ((__da_header*)(da) - 1)

#define da_append(arr, el) \
do { \
	if (arr == NULL) { \
		da_create(arr, DYN_MALLOC); \
	} \
	__da_header* header = __get_da_header(arr); \
	if (header->size >= header->capacity) { \
		header->capacity *= 2; \
		header = realloc(header, sizeof(*arr)*header->capacity \
			+ sizeof(__da_header)); \
		(arr) = (void*)(header + 1); \
	} \
	(arr)[header->size++] = (el); \
} while (0);

#define da_destroy(arr) \
	free((__da_header*)(arr) - 1); arr = NULL

#define da_length(arr) \
	((__da_header*)(arr) - 1)->size

#define da_begin(arr) \
	arr

#define da_end(arr) \
	arr + da_length(arr)

#define da_foreach(type, it, arr) \
	for (type it = da_begin(arr); it != da_end(arr); it++)

#define da_rforeach(type, it, arr) \
	for (type it = da_end(arr) - 1; it != da_begin(arr) - 1; it--)

#define da_enumerate(en, type, it, arr) \
	size_t en = 0; \
	for (type it = da_begin(arr); it != da_end(arr); it++, en++)

#define da_renumerate(en, type, it, arr) \
	size_t en = da_length(arr); \
	for (type it = da_end(arr) - 1; it != da_begin(arr) - 1; it--, en--)

/* smart pointers and ref counting */
typedef struct {
	int count;
} __smart_header;

void cleanup_free(void **ptr);

#ifdef BELT_IMPLEMENTATION

void cleanup_free(void **ptr)
{
	free(*ptr);
	*ptr = NULL;
}

#endif // BELT_IMPLEMENTATION

/* json parser */
enum Json_Parser_Errors {
	JSON_ERR_NO_MEMORY = -1, /* not enough tokens provided or allocator OOM */
	JSON_ERR_INVALID = -2, /* invalid character in json */
	JSON_ERR_SHORT = -3 /* more bytes expected in input */ 
};

enum Json_Token_Type {
	JSON_UNDEFINED,
	JSON_OBJECT, /* like {"label": value} */
	JSON_ARRAY,	/* like ["first", "second", "third", 1, 2, 3] */
	JSON_STRING, /* like "string\uFF8A\n\t\v\/\\" */
	JSON_NUMBER, /* like 1, -2, 2e29, 3e-1, 3.1415 */
	JSON_PRIMITIVE /* like true, false, null */
};

typedef struct {
	enum Json_Token_Type type;
	Slice data; /* text behind the token */
	size_t parent; /* parent token index in parser's token array */
} Json_Token;

typedef struct {
	size_t index; /* current position in the input json string */
	size_t next; /* next token to allocate */
	size_t parent; /* index of parent in tokens array */
	Json_Token *tokens; /* tokens array */
} Json_Parser;

#define JSON_PARSER_INIT { \
	.index = 0, \
	.next = 0, \
	.parent = 0, \
	.tokens = NULL \
}

Result json_parse(Slice text, Json_Token *tokens, Json_Parser *parser);
Result json_parse_raw(const char *text, Json_Token *tokens, Json_Parser *parser);
Result json_dump(const char *path, Json_Token *tokens);

Arena* json_create(Arena *arena, enum Json_Token_Type type, Slice label);
int json_append(Arena *arena, enum Json_Token_Type type, Slice data);
int json_end(Arena *arena, enum Json_Token_Type type);

#ifdef BELT_IMPLEMENTATION

Result json_parse(Slice text, Json_Token *tokens, Json_Parser *parser)
{
	if (slice_empty(text)) {
		return RESULT_ERR(JSON_ERR_SHORT, "input text is empty");
	}

	// consume a character of unput
	#define nextc() ({ \
		(parser->index < text.len ? \
		(text.str[parser->index] != '\0' || text.str[parser->index] != EOF ? \
		text.str[parser->index++] : EOF) : EOF); \
	}) \

	fsm {
		fsm_state(START) {
			char c = nextc();
			if (c == '{') { fsm_goto(parse_object); }
			else if (c == '[') { fsm_goto(parse_array); }
			else if (c == '\"') { fsm_goto(parse_string); }
			else if (isdigit(c)) { fsm_goto(parse_number); }
			else if (isalpha(c)) { fsm_goto(parse_primitive); }
			else { fsm_goto(recover_error); }
		}

		fsm_state(parse_object) {
			// parse label
			// parse contents
		}

		fsm_state(parse_array) {
			// parse opening bracket
			// parse items
		}

		fsm_state(parse_string) {
			// parse characters and escape sequences
		}

		fsm_state(parse_number) {
			// parse number
		}

		fsm_state(parse_primitive) {
			// string compare to all primitives
		}

		fsm_state(recover_error) {
			return RESULT_ERR(JSON_ERR_INVALID, "please fix");
		}
	}

	#undef nextc
	return RESULT_OK();
}

Result json_parse_raw(const char *text, Json_Token *tokens, Json_Parser *parser)
{
	return json_parse(SLICE(text), tokens, parser);
}

Result json_dump(const char *path, Json_Token *tokens)
{
	return RESULT_OK();
}

Arena* json_create(Arena *arena, enum Json_Token_Type type, Slice label)
{
	return NULL;
}

int json_append(Arena *arena, enum Json_Token_Type type, Slice data)
{
	return 0;
}

int json_end(Arena *arena, enum Json_Token_Type type)
{
	return 0;
}

#endif // BELT_IMPLEMENTATION

/* hasing function (j-hash from the kernel) */
u32 jhash(const void *key, u32 length, u32 initval);

/* an arbitrary initial parameter */
#define JHASH_INITVAL 0xdeadbeef

/* rotate left 32-bit */
#define rol32(word, shift) \
	(word << (shift & 31)) | (word >> ((-shift) & 31))

/* __jhash_mix - mix 3 32-bit values reversibly. */
#define __jhash_mix(a, b, c)			\
{										\
	a -= c;  a ^= rol32(c, 4);  c += b;	\
	b -= a;  b ^= rol32(a, 6);  a += c;	\
	c -= b;  c ^= rol32(b, 8);  b += a;	\
	a -= c;  a ^= rol32(c, 16); c += b;	\
	b -= a;  b ^= rol32(a, 19); a += c;	\
	c -= b;  c ^= rol32(b, 4);  b += a;	\
}

/* __jhash_final - final mixing of 3 32-bit values (a,b,c) into c */
#define __jhash_final(a, b, c)		\
{									\
	c ^= b; c -= rol32(b, 14);		\
	a ^= c; a -= rol32(c, 11);		\
	b ^= a; b -= rol32(a, 25);		\
	c ^= b; c -= rol32(b, 16);		\
	a ^= c; a -= rol32(c, 4);		\
	b ^= a; b -= rol32(a, 14);		\
	c ^= b; c -= rol32(b, 24);		\
}

struct __una_u32 { u32 x; } __packed;
#define __get_unaligned_cpu32(p) ({								\
	const struct __una_u32 *ptr = (const struct __una_u32 *)p;	\
	ptr->x;														\
})

#ifdef BELT_IMPLEMENTATION

u32 jhash(const void *key, u32 length, u32 initval)
{
	u32 a, b, c;
	const u8 *k = key;

	// set up the internal state
	a = b = c = JHASH_INITVAL + length + initval;

	// all but the last block: affect some 32 bits of (a,b,c)
	while (length > 12) {
		a += __get_unaligned_cpu32(k);
		b += __get_unaligned_cpu32(k + 4);
		c += __get_unaligned_cpu32(k + 8);
		__jhash_mix(a, b, c);
		length -= 12;
		k += 12;
	}

	// last block: affect all 32 bits of (c)
	switch (length) 
	{
		case 12: c += (u32)k[11]<<24;
		case 11: c += (u32)k[10]<<16;
		case 10: c += (u32)k[9]<<8;
		case 9:  c += k[8];
		case 8:  b += (u32)k[7]<<24;
		case 7:  b += (u32)k[6]<<16;
		case 6:  b += (u32)k[5]<<8;
		case 5:  b += k[4];
		case 4:  a += (u32)k[3]<<24;
		case 3:  a += (u32)k[2]<<16;
		case 2:  a += (u32)k[1]<<8;
		case 1:  a += k[0];
			 __jhash_final(a, b, c);
			 break;
		case 0: // nothing left to add
			break;
	}

	return c;
}

#endif // BELT_IMPLEMENTATION

/* slab allocator */
typedef struct {
} Slab;

/* redblack trees */
#define RB_NODE(T) struct { \
	T id;
};

/* hash map */

/*
	ring buffers

	like the arena, but you allocate an initial block which persists and doesn't
	grow. Subsequent allocations loop back and overwrite old data.

	TODO: add usecase from message parser
*/
typedef struct {
	void *mem;
	void *start;
	void *end;
	size_t capacity;
} Ring;

Ring ring_init(size_t capacity);
void ring_clear(Ring *ring);
void ring_write(Ring *ring, void *el, size_t size);
void ring_write_slice(Ring *ring, Slice slice);
void* ring_read(Ring *ring, size_t size);
Slice ring_read_slice(Ring *ring, size_t size);

#ifndef BELT_IMPLEMENTATION

Ring ring_init(size_t capacity)
{
}

void ring_clear(Ring *ring)
{
}

void ring_write(Ring *ring, void *el, size_t size)
{
}

void ring_write_slice(Ring *ring, Slice slice)
{
}

void* ring_read(Ring *ring, size_t size)
{
}

Slice ring_read_slice(Ring *ring, size_t size)
{
}

#endif // BELT_IMPLEMENTATION

#endif // BELT_H_
