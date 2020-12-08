/**
 * worker.c
 *
 * Toke Høiland-Jørgensen
 * 2014-05-08
 */

#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <curl/curl.h>
#include "worker.h"
#include "util.h"

struct memory_chunk {
	char *memory;
	size_t size;
	int enabled;
};

struct worker_data {
	int debug;
	int timeout;
	char *dns_servers;
	int ai_family;
	struct memory_chunk chunk;
	CURL *curl;
	CURLcode res;
	int pipe_r;
	int pipe_w;
};


static size_t memory_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct memory_chunk *chunk = userp;
	if(!chunk->enabled) return realsize;

	chunk->memory = realloc(chunk->memory, chunk->size + realsize + 1);
	if(chunk->memory == NULL) {
		perror("realloc");
		return 0;
	}

	memcpy(&(chunk->memory[chunk->size]), contents, realsize);
	chunk->size += realsize;
	chunk->memory[chunk->size] = 0;
	return realsize;
}

static int init_worker(struct worker_data *data)
{
	int res = 0;
	data->curl = curl_easy_init();
	if(!data->curl)
		return -1;
	if(data->debug > 1 && (res = curl_easy_setopt(data->curl, CURLOPT_VERBOSE, 1L)) != CURLE_OK) {
		fprintf(stderr, "cURL VERBOSE option error: %s\n", curl_easy_strerror(res));
		goto out;
	}

	if((res = curl_easy_setopt(data->curl, CURLOPT_FOLLOWLOCATION, 1L)) != CURLE_OK) {
		fprintf(stderr, "cURL FOLLOWLOCATION option error: %s\n", curl_easy_strerror(res));
		goto out;
	}
	if(data->timeout && (res = curl_easy_setopt(data->curl, CURLOPT_TIMEOUT_MS, data->timeout)) != CURLE_OK) {
		fprintf(stderr, "cURL TIMEOUT_MS option error: %s\n", curl_easy_strerror(res));
		goto out;
	}
	if((res = curl_easy_setopt(data->curl, CURLOPT_TCP_NODELAY, 1L)) != CURLE_OK) {
		fprintf(stderr, "cURL TCP_NODELAY option error: %s\n", curl_easy_strerror(res));
		goto out;
	}

	if(data->dns_servers && (res = curl_easy_setopt(data->curl, CURLOPT_DNS_SERVERS, data->dns_servers)) != CURLE_OK) {
		fprintf(stderr, "cURL DNS_SERVERS option error: %s\n", curl_easy_strerror(res));
		goto out;
	}

	if(data->ai_family == AF_INET && (res = curl_easy_setopt(data->curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4)) != CURLE_OK) {
		fprintf(stderr, "cURL IPRESOLVE (v4) option error: %s\n", curl_easy_strerror(res));
		goto out;
	}
	if(data->ai_family == AF_INET6 && (res = curl_easy_setopt(data->curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V6)) != CURLE_OK) {
		fprintf(stderr, "cURL IPRESOLVE (v6) option error: %s\n", curl_easy_strerror(res));
		goto out;
	}

	/* send all data to this function  */
	if((res = curl_easy_setopt(data->curl, CURLOPT_WRITEFUNCTION, memory_callback)) != CURLE_OK) {
		fprintf(stderr, "cURL WRITEFUNCTION option error: %s\n", curl_easy_strerror(res));
		goto out;
	}


	/* we pass our 'chunk' struct to the callback function */
	if((res = curl_easy_setopt(data->curl, CURLOPT_WRITEDATA, (void *)&data->chunk)) != CURLE_OK) {
		fprintf(stderr, "cURL WRITEDATA option error: %s\n", curl_easy_strerror(res));
		goto out;
	}


	/* some servers don't like requests that are made without a user-agent
	   field, so we provide one */
	if((res = curl_easy_setopt(data->curl, CURLOPT_USERAGENT, "http-getter/0.1")) != CURLE_OK) {
		fprintf(stderr, "cURL USERAGENT option error: %s\n", curl_easy_strerror(res));
		goto out;
	}


	data->chunk.memory = NULL;
	data->chunk.size = 0;
	data->chunk.enabled = 0;

out:
	return res;
}

static size_t parse_urls(char *buf, size_t buflen, char **urls, size_t urls_l)
{
	char *p, *lstart = buf;
	int discard = 0, cp_len;
	size_t urls_c = 0;
	for(p = buf; p - buf <= buflen; p++) {
		if(lstart == p && *p == '#') discard = 1;
		if(*p == '\n' || p - buf == buflen) {
			if(!discard && p > lstart) {
				cp_len = min(1+p-lstart, PIPE_BUF);
				urls[urls_c] = malloc(cp_len);
				memcpy(urls[urls_c], lstart, cp_len);
				urls[urls_c][cp_len-1] = '\0';
				if(++urls_c >= urls_l) return urls_c;
			}
			lstart = p+1;
			discard = 0;
		}
	}
	return urls_c;
}

static int destroy_worker(struct worker_data *data)
{
	curl_easy_cleanup(data->curl);
	free(data->chunk.memory);
	return 0;
}

static int reset_worker(struct worker_data *data)
{
	destroy_worker(data);
	return init_worker(data);
}

static int run_worker(struct worker_data *data)
{
	char buf[PIPE_BUF+1] = {0};
	char outbuf[PIPE_BUF+1] = {0};
	char *urls[MAX_URLS];
	size_t urls_c;
	char *p;
	int res, i;
	ssize_t len;
	double bytes;
	long header_bytes;
	if(init_worker(data)) return -1;

	while(1)
	{
		if((len = msg_read(data->pipe_r, buf, sizeof(buf))) == -1) {
			return EXIT_FAILURE;
		}
		buf[len] = '\0';
		if(strncmp(buf, "STOP", 4) == 0 || len == 0) return destroy_worker(data);
		if(strncmp(buf, "RESET", 5) == 0) {
			if((res = reset_worker(data)) != 0) {
				len = sprintf(outbuf, "ERR %d", res);
				msg_write(data->pipe_w, outbuf, len);
			} else {
				msg_write(data->pipe_w, "OK", sizeof("OK"));
			}
			continue;
		}
		if(strncmp(buf, "URLLIST ", 8) == 0) {
			p = buf + 8;
			data->chunk.enabled = 1;
		} else if(strncmp(buf, "URL ", 4) == 0) {
			p = buf + 4;
		} else {
			fprintf(stderr, "Unrecognised command '%s'!\n", buf);
			break;
		}

		if(data->debug) {
			fprintf(stderr, "Getting URL '%s'.\n", p);
		}

		curl_easy_setopt(data->curl, CURLOPT_URL, p);
		data->chunk.size = 0;
		if((res = curl_easy_perform(data->curl)) != CURLE_OK) {
			len = sprintf(outbuf, "ERR %d", res);
			msg_write(data->pipe_w, outbuf, len);
		} else {
			if((res = curl_easy_getinfo(data->curl, CURLINFO_SIZE_DOWNLOAD, &bytes)) != CURLE_OK ||
				(res = curl_easy_getinfo(data->curl, CURLINFO_HEADER_SIZE, &header_bytes)) != CURLE_OK ) {
				fprintf(stderr, "cURL error: %s\n", curl_easy_strerror(res));
			}
			if(data->chunk.enabled == 0) {
				len = sprintf(outbuf, "OK %lu bytes", (long)bytes + header_bytes);
				msg_write(data->pipe_w, outbuf, len);
			} else {
				urls_c = parse_urls(data->chunk.memory, data->chunk.size, urls, MAX_URLS);
				len = sprintf(outbuf, "OK %lu bytes %lu urls", (long)bytes + header_bytes, (long)urls_c);
				msg_write(data->pipe_w, outbuf, len);
				for(i = 0; i < urls_c; i++) {
					len = sprintf(outbuf, "%s", urls[i]);
					msg_write(data->pipe_w, outbuf, len);
					free(urls[i]);
				}
				data->chunk.enabled = 0;
			}
		}
	}
	return 0;
}

static struct sigaction sigign = {
	.sa_handler = SIG_IGN,
};

static struct sigaction sigdfl = {
	.sa_handler = SIG_DFL,
};


int start_worker(struct worker *w, struct options *opt)
{
	int fds_r[2];
	int fds_w[2];
	int cpid;

	w->next = NULL;
	w->status = STATUS_READY;

	if(pipe(fds_r) || pipe(fds_w)) {
		perror("pipe");
		return EXIT_FAILURE;
	}

	cpid = fork();
	if(cpid == -1) {
		perror("fork");
		return EXIT_FAILURE;
	}
	if(cpid == 0) {
		struct worker_data wd = {0};
		wd.pipe_r = fds_w[0];
		wd.pipe_w = fds_r[1];
		wd.debug = opt->debug;
		wd.timeout = opt->timeout;
		wd.dns_servers = opt->dns_servers;
		wd.ai_family = opt->ai_family;
		close(fds_r[0]);
		close(fds_w[1]);
		sigaction(SIGINT, &sigign, NULL);
		sigaction(SIGTERM, &sigdfl, NULL);
		_exit(run_worker(&wd));
	} else {
		w->pipe_r = fds_r[0];
		w->pipe_w = fds_w[1];
		w->url = NULL;
		close(fds_r[1]);
		close(fds_w[0]);
		w->pid = cpid;
		return 0;
	}
	return 0;
}

int kill_worker(struct worker *w)
{
	msg_write(w->pipe_w, "STOP", sizeof("STOP"));
	waitpid(w->pid, NULL, 0);
	return 0;
}
