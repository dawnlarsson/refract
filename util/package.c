// package manager for Optical

#include <stddef.h>
#include <unistd.h>

#include "util.c"

typedef struct optical_package_entry
{
	optical_str section;
	optical_str key;
	optical_str value;
	int line;
} optical_package_entry;

typedef struct optical_package_manifest
{
	optical_arena arena;
	optical_package_entry *entries;
	size_t count;
	size_t capacity;
	optical_str config_path;
	optical_str text;
} optical_package_manifest;

typedef struct optical_package_error
{
	optical_str path;
	int line;
	const char *message;
	int code;
} optical_package_error;

static void optical_package_manifest_reset(optical_package_manifest *manifest)
{
	optical_arena_free(&manifest->arena);
	optical_arena_init(&manifest->arena, 4096);
	manifest->entries = NULL;
	manifest->count = 0;
	manifest->capacity = 0;
	manifest->config_path = optical_str_from_parts(NULL, 0);
	manifest->text = optical_str_from_parts(NULL, 0);
}

static int optical_package_set_error(
	optical_package_error *error,
	optical_str path,
	int line,
	const char *message,
	int code)
{
	if (error)
	{
		error->path = path;
		error->line = line;
		error->message = message;
		error->code = code;
	}

	return 0;
}

static int optical_package_push_entry(
	optical_package_manifest *manifest,
	optical_str section,
	optical_str key,
	optical_str value,
	int line)
{
	if (manifest->count == manifest->capacity)
	{
		size_t new_capacity = manifest->capacity ? manifest->capacity * 2 : 16;
		optical_package_entry *entries = optical_arena_alloc(
			&manifest->arena,
			new_capacity * sizeof(*entries),
			sizeof(void *)) orelse return 0;

		if (manifest->entries)
			optical_mem_copy(entries, manifest->entries, manifest->count * sizeof(*entries));

		manifest->entries = entries;
		manifest->capacity = new_capacity;
	}

	manifest->entries[manifest->count].section = section;
	manifest->entries[manifest->count].key = key;
	manifest->entries[manifest->count].value = value;
	manifest->entries[manifest->count].line = line;
	manifest->count++;
	return 1;
}

static int optical_package_visit(
	void *user,
	optical_str section,
	optical_str key,
	optical_str value,
	int line)
{
	return optical_package_push_entry(user, section, key, value, line);
}

static int optical_package_manifest_load_dir(
	optical_package_manifest *manifest,
	const char *dir_path,
	optical_package_error *error)
{
	const char *path = dir_path orelse ".";
	optical_config_error config_error;
	char *config_path;
	char *text;
	size_t length;
	int io_error;

	manifest orelse return optical_package_set_error(
		error,
		optical_str_from_parts(NULL, 0),
		0,
		"manifest is null",
		0);

	optical_package_manifest_reset(manifest);

	config_path = optical_arena_join_path(&manifest->arena, path, "package.config") orelse return optical_package_set_error(
		error,
		optical_str_from_cstr(path),
		0,
		"out of memory",
		ENOMEM);

	manifest->config_path = optical_str_from_cstr(config_path);
	text = optical_file_read_all(&manifest->arena, config_path, &length, &io_error) orelse return optical_package_set_error(
		error,
		manifest->config_path,
		0,
		optical_errno_message(io_error),
		io_error);

	manifest->text = optical_str_from_parts(text, length);
	if (!optical_config_parse_n(text, length, optical_package_visit, manifest, &config_error))
	{
		const char *message = config_error.message;
		if (!message)
			message = "out of memory";

		return optical_package_set_error(error, manifest->config_path, config_error.line, message, 0);
	}

	if (error)
	{
		error->path = manifest->config_path;
		error->line = 0;
		error->message = NULL;
		error->code = 0;
	}

	return 1;
}

static int optical_package_write_entry(int fd, const optical_package_entry *entry)
{
	if (!optical_fd_write_int(fd, entry->line))
		return 0;
	if (!optical_fd_write_cstr(fd, " ["))
		return 0;
	if (entry->section.length)
	{
		if (!optical_fd_write_str(fd, entry->section))
			return 0;
	}
	else
	{
		if (!optical_fd_write_cstr(fd, "root"))
			return 0;
	}
	if (!optical_fd_write_cstr(fd, "] "))
		return 0;
	if (!optical_fd_write_str(fd, entry->key))
		return 0;
	if (!optical_fd_write_cstr(fd, " = "))
		return 0;
	if (!optical_fd_write_str(fd, entry->value))
		return 0;
	return optical_fd_write_cstr(fd, "\n");
}

static int optical_package_manifest_write(int fd, const optical_package_manifest *manifest)
{
	size_t index;

	if (!optical_fd_write_str(fd, manifest->config_path))
		return 0;
	if (!optical_fd_write_cstr(fd, "\nentries: "))
		return 0;
	if (!optical_fd_write_int(fd, (long long)manifest->count))
		return 0;
	if (!optical_fd_write_cstr(fd, "\n"))
		return 0;

	for (index = 0; index < manifest->count; index++)
	{
		if (!optical_package_write_entry(fd, &manifest->entries[index]))
			return 0;
	}

	return 1;
}

static int optical_package_write_error(const optical_package_error *error, const char *fallback_path)
{
	if (!optical_fd_write_cstr(STDERR_FILENO, "package: "))
		return 0;

	if (error->path.data)
	{
		if (!optical_fd_write_str(STDERR_FILENO, error->path))
			return 0;
	}
	else if (fallback_path)
	{
		if (!optical_fd_write_cstr(STDERR_FILENO, fallback_path))
			return 0;
	}

	if (error->line > 0)
	{
		if (!optical_fd_write_cstr(STDERR_FILENO, ":"))
			return 0;
		if (!optical_fd_write_int(STDERR_FILENO, error->line))
			return 0;
	}

	if (!optical_fd_write_cstr(STDERR_FILENO, ": "))
		return 0;
	if (!optical_fd_write_cstr(STDERR_FILENO, error->message ? error->message : "error"))
		return 0;
	return optical_fd_write_cstr(STDERR_FILENO, "\n");
}

int main(int argc, char **argv)
{
	const char *dir_path = argc > 1 ? argv[1] : ".";
	optical_package_manifest manifest;
	optical_package_error error;

	optical_arena_init(&manifest.arena, 4096);
	if (!optical_package_manifest_load_dir(&manifest, dir_path, &error))
	{
		optical_package_write_error(&error, dir_path);
		optical_arena_free(&manifest.arena);
		return 1;
	}

	if (!optical_package_manifest_write(STDOUT_FILENO, &manifest))
	{
		optical_fd_write_cstr(STDERR_FILENO, "package: stdout write failed\n");
		optical_arena_free(&manifest.arena);
		return 1;
	}

	optical_arena_free(&manifest.arena);
	return 0;
}
