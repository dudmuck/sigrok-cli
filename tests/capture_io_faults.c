/* Offline deterministic FILE* failures for the strict capture output seam. */
#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "../capture_io.h"

struct fault_cookie {
	int fail_write;
	int fail_close;
	int calls;
};

static ssize_t fault_write(void *opaque, const char *buf, size_t size)
{
	struct fault_cookie *fault = opaque;
	(void)buf;
	fault->calls++;
	if (fault->fail_write) {
		errno = EIO;
		return -1;
	}
	return size;
}

static int fault_close(void *opaque)
{
	struct fault_cookie *fault = opaque;
	if (fault->fail_close) {
		errno = EIO;
		return -1;
	}
	return 0;
}

static FILE *stream(struct fault_cookie *fault)
{
	cookie_io_functions_t io = {0};
	io.write = fault_write;
	io.close = fault_close;
	return fopencookie(fault, "w", io);
}

int main(void)
{
	struct capture_output_result r = {0};
	struct fault_cookie fault = {0};
	FILE *file = tmpfile();
	assert(file);
	assert(capture_output_write(&r, file, "abcd", 4) == 0);
	assert(r.attempted == 4 && r.written == 4 && r.committed == 4);
	assert(capture_output_close(&r, file) == 0);

	memset(&r, 0, sizeof(r));
	fault.fail_write = 1;
	file = stream(&fault);
	assert(file);
	setvbuf(file, NULL, _IONBF, 0);
	assert(capture_output_write(&r, file, "abcd", 4) != 0);
	assert(r.write_errors == 1 && r.committed == 0);
	assert(!strcmp(r.first_stage, "fwrite"));
	(void)fclose(file);

	memset(&r, 0, sizeof(r));
	memset(&fault, 0, sizeof(fault));
	fault.fail_write = 1;
	file = stream(&fault);
	assert(file);
	/* Buffered fwrite succeeds; the injected sink fails at fflush. */
	assert(capture_output_write(&r, file, "abcd", 4) != 0);
	assert(r.flush_errors == 1 && r.written == 4 && r.committed == 0);
	assert(!strcmp(r.first_stage, "fflush"));
	(void)fclose(file);

	memset(&r, 0, sizeof(r));
	memset(&fault, 0, sizeof(fault));
	fault.fail_close = 1;
	file = stream(&fault);
	assert(file);
	assert(capture_output_close(&r, file) != 0);
	assert(r.close_errors == 1 && !strcmp(r.first_stage, "fclose"));
	memset(&r, 0, sizeof(r));
	capture_output_error(&r, "primary_encode", -1);
	capture_output_error(&r, "analog_encode", -2);
	assert(!strcmp(r.first_stage, "primary_encode") && r.first_code == -1);
	return 0;
}
