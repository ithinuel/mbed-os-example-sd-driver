#include "mbed.h"
#include "FATFileSystem.h"
#include "SDBlockDevice.h"
#include <stdio.h>
#include <errno.h>
/* mbed_retarget.h is included after errno.h so symbols are mapped to
 * consistent values for all toolchains */
#include "platform/mbed_retarget.h"


SDBlockDevice sd1(MBED_CONF_APP_SPI_MOSI, MBED_CONF_APP_SPI_MISO, MBED_CONF_APP_SPI_CLK, MBED_CONF_APP_SPI_CS);
FATFileSystem fs1("sd1", &sd1);
SDBlockDevice sd2(MBED_CONF_APP_SPI_MOSI2, MBED_CONF_APP_SPI_MISO2, MBED_CONF_APP_SPI_CLK2, MBED_CONF_APP_SPI_CS2);
FATFileSystem fs2("sd2", &sd2);

Semaphore join(0, 2);
Mutex print;

void my_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    print.lock();
    vprintf(fmt, args);
    print.unlock();
    va_end(args);
}

void return_error(int ret_val){
  if (ret_val)
    my_printf("Failure. %d\n", ret_val);
  else
    my_printf("done.\n");
}

bool errno_error(void* ret_val){
  if (ret_val == NULL) {
    my_printf(" Failure. %d \n", errno);
  } else
    my_printf(" done.\n");
  return (ret_val != NULL);
}

void run_test(void *arg) {
	int error = 0;
	const char *id = (const char *)arg;

    char filename[20+strlen(id)] = {0};
    sprintf(filename, "/sd%s/numbers.txt", id);
    my_printf("%s: Opening a new file, %s.", id, filename);
	FILE* fd = fopen(filename, "w+");
    if (!errno_error(fd)) {
        join.release();
        return;
    }

	for (int i = 0; i < 20; i++){
		my_printf("%s: Writing decimal numbers to a file (%d/20)\r", id, i);
		fprintf(fd, "%02d\n", i);
        Thread::yield();
	}
	my_printf("%s: Writing decimal numbers to a file (20/20) done.\n", id);

	my_printf("%s: Closing file.\n", id);
	fclose(fd);
	my_printf("%s: File Closed.\n", id);

	my_printf("%s: Re-opening file read-only.", id);
	fd = fopen(filename, "r");
    if (!errno_error(fd)) {
        join.release();
        return;
    }

	my_printf("%s: Dumping file to screen.\n", id);
	char buff[16] = {0};
	while (!feof(fd)){
		int size = fread(buff, 1, 3, fd);
        my_printf("%s: %s", id, buff);
        Thread::yield();
	}
	my_printf("%s: EOF.\n", id);

	my_printf("%s: Closing file.\n", id);
	fclose(fd);
	my_printf("%s: File closed.\n", id);

	my_printf("%s: Opening root directory. /sd%s", id, id);
    sprintf(filename, "/sd%s/", id);
	DIR* dir = opendir(filename);
    if (!errno_error(fd)) {
        join.release();
        return;
    }

	struct dirent* de;
	my_printf("%s: Printing all filenames:\n", id);
	while((de = readdir(dir)) != NULL){
		my_printf("%s:  %s\n", id, &(de->d_name)[0]);
	}

	my_printf("%s: Closeing root directory.\n", id);
	error = closedir(dir);
	return_error(error);
	my_printf("%s: Filesystem Demo complete.\n", id);

    join.release();
}

int main()
{
	my_printf("Welcome to the filesystem example.\n");

    // get time
    Timer t;
    Thread t1;
    Thread t2;
    t.start();
    t1.start(callback(run_test, (void*)"1"));
    t2.start(callback(run_test, (void*)"2"));

    uint32_t cnt = 0;
    uint64_t activity = 0;
    while (cnt < 2) {
        cnt += join.wait(0);
        activity += 1;
    }
    t.stop();

    my_printf("Done: %dms activity: %llu\n", t.read_ms(), activity);
	while (true) {}
}

