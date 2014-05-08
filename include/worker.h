/**
 * worker.h
 *
 * Toke Høiland-Jørgensen
 * 2014-05-08
 */

#ifndef WORKER_H
#define WORKER_H
#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <fcntl.h>              /* Obtain O_* constant definitions */
#include <unistd.h>
#include "options.h"

#define STATUS_READY 0
#define STATUS_WORKING 1

struct worker {
	struct worker *next;
	int status;
	int pid;
	int pipe_r;
	int pipe_w;
};

int start_worker(struct worker *w, struct options *opt);
int kill_worker(struct worker *w);

#endif
