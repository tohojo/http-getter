/**
 * worker.c
 *
 * Toke Høiland-Jørgensen
 * 2014-05-08
 */

#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <curl/curl.h>
#include "worker.h"

struct memory_chunk {
	char *memory;
	size_t size;
};

struct worker_data {
	int timeout;
	size_t bytes;
	CURL *curl;
	CURLcode res;
	int pipe_r;
	int pipe_w;
};


static size_t discard_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	size_t *bytes = (size_t *)userp;

	*bytes += realsize;
	return realsize;
}

static int init_worker(struct worker_data *data)
{
	data->curl = curl_easy_init();
	if(!data->curl)
		return -1;
	curl_easy_setopt(data->curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(data->curl, CURLOPT_HEADER, 1L);
	curl_easy_setopt(data->curl, CURLOPT_TIMEOUT_MS, data->timeout);
//	curl_easy_setopt(data->curl, CURLOPT_VERBOSE, 1L);

	/* send all data to this function  */
	curl_easy_setopt(data->curl, CURLOPT_WRITEFUNCTION, discard_callback);

	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(data->curl, CURLOPT_WRITEDATA, (void *)&data->bytes);

	/* some servers don't like requests that are made without a user-agent
	   field, so we provide one */
	curl_easy_setopt(data->curl, CURLOPT_USERAGENT, "http-getter/0.1");

	data->bytes = 0;

	return 0;
}

static int destroy_worker(struct worker_data *data)
{
	curl_easy_cleanup(data->curl);
	return 0;
}

static int reset_worker(struct worker_data *data)
{
	destroy_worker(data);
	init_worker(data);
	return 0;
}

static int run_worker(struct worker_data *data)
{
	char buf[PIPE_BUF] = {};
	char outbuf[PIPE_BUF] = {};
	int res;
	ssize_t len;
	if(init_worker(data)) return -1;

	while(1)
	{
		if((len = read(data->pipe_r, buf, sizeof(buf))) == -1) {
			perror("read");
			return EXIT_FAILURE;
		}
		buf[len] = '\0';
		if(strncmp(buf, "STOP", 4) == 0) return destroy_worker(data);
		if(strncmp(buf, "RESET", 5) == 0) {
			if((res = reset_worker(data)) != 0) {
				len = sprintf(outbuf, "ERR %d", res);
				write(data->pipe_w, outbuf, len);
			} else {
				write(data->pipe_w, "OK", sizeof("OK"));
			}
			continue;
		}
		if(strncmp(buf, "URL ", 4) != 0) {
			fprintf(stderr, "Unrecognised command '%s'!\n", buf);
			break;
		}

		curl_easy_setopt(data->curl, CURLOPT_URL, buf+4);
		data->bytes = 0;
		res = curl_easy_perform(data->curl);
		if(res != CURLE_OK) {
			fprintf(stderr, "cURL error: %s.\n", curl_easy_strerror(res));
			len = sprintf(outbuf, "ERR %d", res);
			write(data->pipe_w, outbuf, len);
		} else {
			len = sprintf(outbuf, "OK %lu bytes", (long)data->bytes);
			write(data->pipe_w, outbuf, len);
		}
	}
	return 0;
}

int start_worker(struct worker *w, struct options *opt)
{
	int fds_r[2];
	int fds_w[2];
	int cpid;

	w->next = NULL;
	w->status = STATUS_READY;

	if(pipe2(fds_r, O_DIRECT) || pipe2(fds_w, O_DIRECT)) {
		perror("pipe");
		return EXIT_FAILURE;
	}

	cpid = fork();
	if(cpid == -1) {
		perror("fork");
		return EXIT_FAILURE;
	}
	if(cpid == 0) {
		struct worker_data wd = {};
		wd.pipe_r = fds_w[0];
		wd.pipe_w = fds_r[1];
		wd.timeout = opt->timeout;
		close(fds_r[0]);
		close(fds_w[1]);
		_exit(run_worker(&wd));
	} else {
		w->pipe_r = fds_r[0];
		w->pipe_w = fds_w[1];
		close(fds_r[1]);
		close(fds_w[0]);
		w->pid = cpid;
		return 0;
	}
	return 0;
}

int kill_worker(struct worker *w)
{
	write(w->pipe_w, "STOP", sizeof("STOP"));
	waitpid(w->pid, NULL, 0);
	return 0;
}
