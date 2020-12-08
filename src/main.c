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

static struct sigaction sigdfl = {
	.sa_handler = SIG_DFL,
};

static void sig_exit(int signal)
{
	kill_workers();
	if(signal == SIGINT) print_stats(opt.output);
	destroy_options(&opt);
	if(signal == SIGINT) {
		sigaction(SIGINT, &sigdfl, NULL);
		kill(getpid(), SIGINT);
	}
	exit(signal);
}

static struct sigaction sigact = {
	.sa_handler = sig_exit,
};


int main(int argc, char **argv)
{
	int ret;
	if(sigaction(SIGINT, &sigact, NULL) < 0 ||
		sigaction(SIGTERM, &sigact, NULL) < 0) {
		perror("Error installing signal handler");
		return 1;
	}

	if(initialise_options(&opt, argc, argv) < 0)
		return 1;

	ret = get_loop(&opt);

	destroy_options(&opt);
	return ret;

}
