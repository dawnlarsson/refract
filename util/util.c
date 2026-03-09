// single file micro utilities for Optical

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

typedef struct optical_str
{
	const char *data;
	size_t length;
} optical_str;

typedef struct optical_config_error
{
	int line;
	const char *message;
} optical_config_error;

typedef int (*optical_config_visit_fn)(
	void *user,
	optical_str section,
	optical_str key,
	optical_str value,
	int line);

typedef struct optical_arena_block
{
	struct optical_arena_block *next;
	size_t capacity;
	size_t used;
	char data[];
} optical_arena_block;

typedef struct optical_arena
{
	optical_arena_block *head;
	size_t block_size;
} optical_arena;

static void optical_mem_copy(void *dst_ptr, const void *src_ptr, size_t size)
{
	char *dst = dst_ptr;
	const char *src = src_ptr;
	size_t index;

	for (index = 0; index < size; index++)
		dst[index] = src[index];
}

static size_t optical_str_length(const char *text)
{
	size_t length = 0;

	if (!text)
		return 0;

	while (text[length])
		length++;

	return length;
}

static optical_str optical_str_from_parts(const char *data, size_t length)
{
	optical_str value;

	value.data = data;
	value.length = length;
	return value;
}

static optical_str optical_str_from_cstr(const char *text)
{
	return optical_str_from_parts(text, optical_str_length(text));
}

static int optical_is_space(char ch)
{
	return ch == ' ' || ch == '\t' || ch == '\r';
}

static const char *optical_trim_left(const char *cursor, const char *end)
{
	while (cursor < end && optical_is_space(*cursor))
		cursor++;

	return cursor;
}

static const char *optical_trim_right(const char *begin, const char *end)
{
	while (end > begin && optical_is_space(end[-1]))
		end--;

	return end;
}

static int optical_config_fail(
	optical_config_error *error,
	int line,
	const char *message)
{
	if (error)
	{
		error->line = line;
		error->message = message;
	}

	return 0;
}

int optical_config_parse_n(
	const char *text,
	size_t length,
	optical_config_visit_fn visit,
	void *user,
	optical_config_error *error)
{
	const char *cursor = text orelse return optical_config_fail(error, 0, "config text is null");
	const char *end = text + length;
	optical_str current_section;
	int line = 1;

	visit orelse return optical_config_fail(error, 0, "config visitor is null");

	while (cursor < end)
	{
		const char *line_begin = cursor;
		const char *line_end = cursor;
		const char *content_begin;
		const char *content_end;

		while (line_end < end && *line_end != '\n')
			line_end++;

		content_begin = optical_trim_left(line_begin, line_end);
		content_end = optical_trim_right(content_begin, line_end);

		if (content_begin < content_end && *content_begin != '#')
		{
			if (*content_begin == '-')
			{
				const char *section_begin = optical_trim_left(content_begin + 1, content_end);
				const char *section_end = optical_trim_right(section_begin, content_end);

				if (section_begin == section_end)
					return optical_config_fail(error, line, "section name is missing");

				current_section = optical_str_from_parts(section_begin, (size_t)(section_end - section_begin));
			}
			else
			{
				const char *key_begin = content_begin;
				const char *key_end = content_begin;
				const char *value_begin;
				const char *value_end;

				while (key_end < content_end && !optical_is_space(*key_end))
					key_end++;

				value_begin = optical_trim_left(key_end, content_end);
				value_end = optical_trim_right(value_begin, content_end);

				if (key_begin == key_end)
					return optical_config_fail(error, line, "key is missing");

				if (value_begin == value_end)
					return optical_config_fail(error, line, "value is missing");

				visit(
					user,
					current_section,
					optical_str_from_parts(key_begin, (size_t)(key_end - key_begin)),
					optical_str_from_parts(value_begin, (size_t)(value_end - value_begin)),
					line) orelse return 0;
			}
		}

		cursor = line_end;
		if (cursor < end && *cursor == '\n')
			cursor++;

		line++;
	}

	return 1;
}

int optical_config_parse(
	const char *text,
	optical_config_visit_fn visit,
	void *user,
	optical_config_error *error)
{
	const char *source = text orelse return optical_config_fail(error, 0, "config text is null");
	return optical_config_parse_n(source, optical_str_length(source), visit, user, error);
}

static size_t optical_align_up(size_t value, size_t alignment)
{
	size_t mask = alignment - 1;
	return (value + mask) & ~mask;
}

static optical_arena_block *optical_arena_block_new(size_t capacity)
{
	size_t total = sizeof(optical_arena_block) + capacity;
	optical_arena_block *block = mmap(NULL, total, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (block == MAP_FAILED)
		return NULL;

	block->next = NULL;
	block->capacity = capacity;
	block->used = 0;
	return block;
}

void optical_arena_init(optical_arena *arena, size_t block_size)
{
	arena->head = NULL;
	arena->block_size = block_size ? block_size : 4096;
}

void optical_arena_free(optical_arena *arena)
{
	optical_arena_block *block = arena->head;

	while (block)
	{
		optical_arena_block *next = block->next;
		munmap(block, sizeof(*block) + block->capacity);
		block = next;
	}

	arena->head = NULL;
}

void *optical_arena_alloc(optical_arena *arena, size_t size, size_t alignment)
{
	optical_arena_block *block = arena->head;
	void *memory;
	size_t start;

	if (alignment < sizeof(void *))
		alignment = sizeof(void *);

	if (!block)
	{
		size_t capacity = arena->block_size;
		if (capacity < size + alignment)
			capacity = size + alignment;

		block = optical_arena_block_new(capacity) orelse return NULL;
		block->next = arena->head;
		arena->head = block;
	}

	start = optical_align_up(block->used, alignment);
	if (start + size > block->capacity)
	{
		size_t capacity = arena->block_size;
		if (capacity < size + alignment)
			capacity = size + alignment;

		block = optical_arena_block_new(capacity) orelse return NULL;
		block->next = arena->head;
		arena->head = block;
		start = optical_align_up(block->used, alignment);
	}

	memory = block->data + start;
	block->used = start + size;
	return memory;
}

char *optical_arena_copy_cstr(optical_arena *arena, const char *text)
{
	size_t length = optical_str_length(text);
	char *copy = optical_arena_alloc(arena, length + 1, 1) orelse return NULL;

	optical_mem_copy(copy, text, length);
	copy[length] = '\0';
	return copy;
}

char *optical_arena_join_path(optical_arena *arena, const char *left, const char *right)
{
	size_t left_length = optical_str_length(left);
	size_t right_length = optical_str_length(right);
	int needs_slash = left_length > 0 && left[left_length - 1] != '/';
	char *path = optical_arena_alloc(arena, left_length + (size_t)needs_slash + right_length + 1, 1) orelse return NULL;

	optical_mem_copy(path, left, left_length);
	if (needs_slash)
		path[left_length++] = '/';

	optical_mem_copy(path + left_length, right, right_length);
	path[left_length + right_length] = '\0';
	return path;
}

char *optical_file_read_all(
	optical_arena *arena,
	const char *path,
	size_t *length_out,
	int *error_out)
{
	int fd = open(path, O_RDONLY) orelse {
		if (error_out)
			*error_out = errno;
		return NULL;
	};
	struct stat st;
	char *text;
	size_t total = 0;

	defer close(fd);

	if (fstat(fd, &st) != 0 || st.st_size < 0)
	{
		if (error_out)
			*error_out = errno ? errno : EIO;
		return NULL;
	}

	text = optical_arena_alloc(arena, (size_t)st.st_size + 1, 1) orelse {
		if (error_out)
			*error_out = ENOMEM;
		return NULL;
	};

	while (total < (size_t)st.st_size)
	{
		ssize_t got = read(fd, text + total, (size_t)st.st_size - total);

		if (got < 0)
		{
			if (errno == EINTR)
				continue;

			if (error_out)
				*error_out = errno;
			return NULL;
		}

		if (got == 0)
			break;

		total += (size_t)got;
	}

	text[total] = '\0';
	if (length_out)
		*length_out = total;
	if (error_out)
		*error_out = 0;
	return text;
}

const char *optical_errno_message(int code)
{
	switch (code)
	{
		case 0: return "ok";
		case ENOENT: return "not found";
		case ENOTDIR: return "not a directory";
		case EISDIR: return "is a directory";
		case EACCES: return "permission denied";
		case ENOMEM: return "out of memory";
		case EIO: return "i/o error";
		default: return "system error";
	}
}

int optical_fd_write_all(int fd, const void *data_ptr, size_t length)
{
	const char *data = data_ptr;

	while (length > 0)
	{
		ssize_t wrote = write(fd, data, length);

		if (wrote < 0)
		{
			if (errno == EINTR)
				continue;

			return 0;
		}

		data += wrote;
		length -= (size_t)wrote;
	}

	return 1;
}

int optical_fd_write_cstr(int fd, const char *text)
{
	return optical_fd_write_all(fd, text, optical_str_length(text));
}

int optical_fd_write_str(int fd, optical_str text)
{
	return optical_fd_write_all(fd, text.data, text.length);
}

int optical_fd_write_uint(int fd, unsigned long long value)
{
	char buffer[32];
	size_t length = 0;
 	size_t index;

	if (value == 0)
		return optical_fd_write_all(fd, "0", 1);

	while (value > 0)
	{
		buffer[length++] = (char)('0' + (value % 10));
		value /= 10;
	}

	for (index = 0; index < length / 2; index++)
	{
		char tmp = buffer[index];
		buffer[index] = buffer[length - 1 - index];
		buffer[length - 1 - index] = tmp;
	}

	return optical_fd_write_all(fd, buffer, length);
}

int optical_fd_write_int(int fd, long long value)
{
	unsigned long long magnitude = (unsigned long long)value;

	if (value < 0)
	{
		if (!optical_fd_write_all(fd, "-", 1))
			return 0;
		magnitude = 0 - magnitude;
	}

	return optical_fd_write_uint(fd, magnitude);
}