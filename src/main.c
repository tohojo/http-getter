/**
 * main.c
 *
 * Toke Høiland-Jørgensen
 * 2014-05-07
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "options.h"
#include "getter.h"

static struct options opt;

struct sigaction sigdfl = {
	.sa_handler = SIG_DFL,
	.sa_mask = {},
	.sa_flags = 0,
};

static void sig_exit(int signal)
{
	destroy_options(&opt);
	kill_workers();
	if(signal == SIGINT) {
		sigaction(SIGINT, &sigdfl, NULL);
		kill(getpid(), SIGINT);
	}
	exit(signal);
}

struct sigaction sigact = {
	.sa_handler = sig_exit,
	.sa_mask = {},
	.sa_flags = 0,
};


int main(int argc, char **argv)
{
	if(sigaction(SIGINT, &sigact, NULL) < 0 ||
		sigaction(SIGTERM, &sigact, NULL) < 0) {
		perror("Error installing signal handler");
		return 1;
	}

	if(initialise_options(&opt, argc, argv) < 0)
		return 1;

	get_loop(&opt);

	destroy_options(&opt);
	return 0;

}
