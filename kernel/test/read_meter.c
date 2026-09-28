/*
 * read_meter.c - minimal userspace client for /dev/pulsemeter.
 *
 * Demonstrates the Linux System Programming half of this module: talking to
 * a character device using raw POSIX syscalls (open/read/close), the same
 * pattern used to talk to any real Linux device node.
 *
 * Build:  gcc -Wall -Wextra -O2 -o read_meter read_meter.c
 * Run:    ./read_meter                (reads /dev/pulsemeter once and exits)
 *         ./read_meter -w 5           (reads it every second, 5 times)
 *
 * Requires pulsemeter_driver.ko to be loaded first (sudo insmod ...) on a
 * real Linux machine or WSL2 -- see ../README.md. Running this without the
 * module loaded is still meaningful: it demonstrates correct, non-crashing
 * error handling on a failed open() (ENOENT), which is exercised below.
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define DEVICE_PATH "/dev/pulsemeter"

static int read_once(void) {
    int fd = open(DEVICE_PATH, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open(%s) failed: %s\n", DEVICE_PATH, strerror(errno));
        fprintf(stderr, "(Is the module loaded? Try: sudo insmod pulsemeter_driver.ko)\n");
        return -1;
    }

    char buf[32];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n < 0) {
        fprintf(stderr, "read(%s) failed: %s\n", DEVICE_PATH, strerror(errno));
        close(fd);
        return -1;
    }

    buf[n] = '\0';
    printf("Pulses since last read: %s", buf); /* driver's string already ends in \n */

    close(fd);
    return 0;
}

int main(int argc, char **argv) {
    int repeats = 1;
    int interval_seconds = 1;

    if (argc == 3 && strcmp(argv[1], "-w") == 0) {
        repeats = atoi(argv[2]);
        if (repeats <= 0) {
            fprintf(stderr, "Usage: %s [-w COUNT]  (COUNT must be a positive integer)\n", argv[0]);
            return 1;
        }
    } else if (argc != 1) {
        fprintf(stderr, "Usage: %s [-w COUNT]\n", argv[0]);
        return 1;
    }

    for (int i = 0; i < repeats; ++i) {
        if (read_once() != 0) {
            return 1;
        }
        if (i + 1 < repeats) {
            sleep((unsigned int)interval_seconds);
        }
    }

    return 0;
}
