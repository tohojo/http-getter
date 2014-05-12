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
	int debug;
	int run_length;
	int interval;
	int count;
	int timeout;
	char *dns_servers;
	int ai_family;
	int workers;
	struct timeval start_time;
	FILE *output;
	char *urls[MAX_URLS];
	size_t urls_l;
	char *urls_loc;
};

int initialise_options(struct options *opt, int argc, char **argv);
void destroy_options(struct options *opt);

#endif
