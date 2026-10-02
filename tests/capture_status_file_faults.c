/* Offline status-file integration seam: no device is scanned or opened. */
#include <config.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "../session.c"

gchar *opt_capture_status_file;

int main(void)
{
	char path[] = "/tmp/sigrok-status-test-XXXXXX";
	char line[2048];
	FILE *file;
	int fd = g_mkstemp(path);
	assert(fd >= 0);
	assert(close(fd) == 0);
	assert(unlink(path) == 0);
	opt_capture_status_file = path;
	capture_io.saleae_requested = 1;
	capture_io.started = 1;
	capture_io.ended = 1;
	capture_io.driver_known = 1;
	capture_io.output_closed = 1;
	capture_io.driver.stop_requested = 1;
	capture_io.driver.stop_completed = 1;
	assert(capture_status_report() == 0);
	file = fopen(path, "r");
	assert(file);
	assert(fgets(line, sizeof(line), file));
	assert(strstr(line, "SIGROK_CAPTURE_STATUS schema=1 outcome=OK ") == line);
	assert(strchr(line, '\n'));
	assert(fgetc(file) == EOF);
	assert(fclose(file) == 0);
	capture_io.reported = 0;
	assert(capture_status_report() == 2); /* O_EXCL collision never overwrites */
	assert(unlink(path) == 0);
	return 0;
}
