/**
 * getter.c
 *
 * Toke Høiland-Jørgensen
 * 2014-05-07
 */

#include <stdlib.h>
#include <string.h>
#include "getter.h"

struct MemoryStruct {
	char *memory;
	size_t size;
};


static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) {
		/* out of memory! */
		fprintf(stderr, "not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

void get_loop(struct options *opt)
{
	struct timeval start, end;
	double time;
	CURL *curl;
	CURLcode res;
	struct MemoryStruct chunk;
	chunk.memory = malloc(1);
	chunk.size = 0;

	curl_global_init(CURL_GLOBAL_ALL);

	curl = curl_easy_init();
	if(curl) {
		int i;
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

		/* send all data to this function  */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

		/* we pass our 'chunk' struct to the callback function */
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

		/* some servers don't like requests that are made without a user-agent
		   field, so we provide one */
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "http-getter/0.1");

		for(i = 0; i < opt->urls_l; i++) {
			gettimeofday(&start, NULL);
			curl_easy_setopt(curl, CURLOPT_URL, opt->urls[i]);
			res = curl_easy_perform(curl);
			if(res != CURLE_OK)
				fprintf(stderr, "curl_easy_perform() failed: %s\n",
					curl_easy_strerror(res));
			gettimeofday(&end, NULL);
			time = end.tv_sec - start.tv_sec;
			time += (double)(end.tv_usec - start.tv_usec) / 1000000;
			printf("[%lu.%06lu] Received %lu bytes in %f seconds.\n", (long)end.tv_sec, (long)end.tv_usec, (long)chunk.size, time);
		}
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	free(chunk.memory);
}
