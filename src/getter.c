/**
 * getter.c
 *
 * Toke Høiland-Jørgensen
 * 2014-05-07
 */

#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include "getter.h"
#include "worker.h"
#include "util.h"

int get_once(struct worker *workers, char **urls, size_t urls_l)
{
	struct worker *w;
	int cururl = 0, total_bytes = 0, bytes, nfds, retval, len;
	fd_set rfds;
	char buf[PIPE_BUF] = {}, outbuf[PIPE_BUF] = {};
	for(w = workers; w; w = w->next) {
		write(w->pipe_w, "RESET", sizeof("RESET"));
		w->status = STATUS_WORKING;
	}

	do {
		FD_ZERO(&rfds);
		nfds = -1;
		for(w = workers; w; w = w->next) {
			if(w->status == STATUS_READY && cururl < urls_l) {
				len = sprintf(outbuf, "URL %s", urls[cururl++]);
				write(w->pipe_w, outbuf, len);
				w->status = STATUS_WORKING;
			}
			if(w->status == STATUS_WORKING) {
				FD_SET(w->pipe_r, &rfds);
				nfds = max(nfds, w->pipe_r);
			}
		}
		if(nfds == -1) break;
		nfds++;
		retval = select(nfds, &rfds, NULL, NULL, NULL);
		if(retval == -1) {
			perror("select()");
			return -1;
		}
		for(w = workers; w; w = w->next) {
			if(FD_ISSET(w->pipe_r, &rfds)) {
				if((len = read(w->pipe_r, buf, sizeof(buf))) == -1) {
					perror("read");
					continue;
				}
				buf[len] = '\0';
				if(sscanf(buf, "OK %d bytes", &bytes) == 1)
					total_bytes += bytes;
				w->status = STATUS_READY;
			}
		}
	} while(1);
	return total_bytes;
}

static void schedule_next(int interval, struct timeval *now, struct timeval *next)
{
	int delay = interval * 1000;
	next->tv_sec = now->tv_sec;
	next->tv_usec = now->tv_usec + delay;
	if(next->tv_usec > 1000000) {
		next->tv_sec += next->tv_usec / 1000000;
		next->tv_usec %= 1000000;
	}
}


static struct worker *workers = NULL;

void kill_workers()
{
	struct worker *w;
	for(w = workers; w; w = w->next) kill_worker(w);
}


void get_loop(struct options *opt)
{
	struct timeval start, end, stop, next;
	double time;
	struct worker *w;
	int i, bytes;
	int count = 0;
	for(i = 0; i < opt->workers; i++) {
		w = malloc(sizeof(*w));
		if(w == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}
		if(start_worker(w, opt) != 0) exit(EXIT_FAILURE);
		w->next = workers;
		workers = w;
	}
	gettimeofday(&stop, NULL);
	start.tv_sec = next.tv_sec = stop.tv_sec;
	start.tv_usec = next.tv_usec = stop.tv_usec;
	stop.tv_sec += opt->run_length;

	do {
		while(start.tv_sec < next.tv_sec || start.tv_usec < next.tv_usec) {
			if(next.tv_usec - start.tv_usec > USLEEP_THRESHOLD)
				usleep(USLEEP_THRESHOLD);
			gettimeofday(&start, NULL);
		}
		schedule_next(opt->interval, &start, &next);
		if((bytes = get_once(workers, opt->urls, opt->urls_l)) < 0) exit(-bytes);
		gettimeofday(&end, NULL);
		time = end.tv_sec - start.tv_sec;
		time += (double)(end.tv_usec - start.tv_usec) / 1000000;
		printf("[%lu.%06lu] Received %lu bytes in %f seconds.\n", (long)end.tv_sec, (long)end.tv_usec, (long)bytes, time);
		count++;
	} while((opt->count == 0 || count < opt->count) &&
		(end.tv_sec < stop.tv_sec || end.tv_usec < stop.tv_usec || opt->run_length == 0));
	kill_workers();
}
