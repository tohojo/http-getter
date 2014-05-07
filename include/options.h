/**
 * options.h
 *
 * Toke Høiland-Jørgensen
 * 2014-05-07
 */

#ifndef OPTIONS_H
#define OPTIONS_H

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#define MAX_URLS 1024


struct options {
	char initialised;
	int run_length;
	int interval;
	int ai_family;
	int threads;
	struct timeval start_time;
	FILE *output;
	char *urls[MAX_URLS];
	size_t urls_l;
};

int initialise_options(struct options *opt, int argc, char **argv);
void destroy_options(struct options *opt);

#endif
