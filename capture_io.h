/* Small testable output-I/O contract for Saleae strict capture. */
#ifndef SIGROK_CLI_CAPTURE_IO_H
#define SIGROK_CLI_CAPTURE_IO_H

#include <errno.h>
#include <stdint.h>
#include <stdio.h>

struct capture_output_result {
	uint64_t attempted, written, committed;
	unsigned int encode_errors, write_errors, flush_errors, close_errors;
	const char *first_stage;
	int first_code;
};

static inline void capture_output_error(struct capture_output_result *r,
		const char *stage, int code)
{
	if (!r->first_stage) {
		r->first_stage = stage;
		r->first_code = code;
	}
}

static inline int capture_output_write(struct capture_output_result *r,
		FILE *file, const void *data, size_t length)
{
	size_t written;
	r->attempted += length;
	written = fwrite(data, 1, length, file);
	r->written += written;
	if (written != length || ferror(file)) {
		r->write_errors++;
		capture_output_error(r, "fwrite", errno ? errno : EIO);
		return -1;
	}
	if (fflush(file) != 0) {
		r->flush_errors++;
		capture_output_error(r, "fflush", errno ? errno : EIO);
		return -1;
	}
	r->committed += written;
	return 0;
}

static inline int capture_output_close(struct capture_output_result *r, FILE *file)
{
	if (fclose(file) != 0) {
		r->close_errors++;
		capture_output_error(r, "fclose", errno ? errno : EIO);
		return -1;
	}
	return 0;
}

#endif
