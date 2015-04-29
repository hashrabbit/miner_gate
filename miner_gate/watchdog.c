#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <syslog.h>
#include <systemd/sd-daemon.h>
#include <unistd.h>

#include "watchdog.h"


typedef struct {
	bool notify;
	timeval interval;
	timeval next;
} Watchdog;

static Watchdog watchdog = {
	.notify   = false,
	.interval = { .tv_sec = 0, .tv_usec = 0 },
	.next     = { .tv_sec = 0, .tv_usec = 0 }
};

static void watchdog_reset(struct timeval tv) {
	timeradd(&tv, &watchdog.interval, &watchdog.next);
}

void watchdog_init() {
	uint64_t usec;
	struct timeval tv;

	if (!sd_watchdog_enabled(0, &usec))
		return;

	watchdog.notify = true;

	// Notify every half WatchdogUsec
	watchdog.interval = {
		.tv_sec  = usec / 2000000,    // seconds
		.tv_usec = usec / 2 % 1000000 // microseconds
	};

	gettimeofday(&tv, NULL);
	watchdog_reset(tv);

	sd_notifyf(0, "READY=1\n"
		"STATUS=Listening for connections...\n"
		"MAINPID=%lu",
		(unsigned long) getpid());
}

void watchdog_heartbeat(struct timeval tv) {
	if (!watchdog.notify)
		return;

	if (timercmp(&tv, &watchdog.next, <))
		return;

	sd_notify(0, "WATCHDOG=1");
	watchdog_reset(tv);
}
