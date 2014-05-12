/**
 * options.c
 *
 * Toke Høiland-Jørgensen
 * 2014-05-07
 */

#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "options.h"

int parse_options(struct options *opt, int argc, char **argv);


int initialise_options(struct options *opt, int argc, char **argv)
{
	opt->debug = 0;
	opt->run_length = 0;
	opt->count = 0;
	opt->output = stdout;
	opt->interval = 1000;
	opt->workers = 4;
	opt->timeout = 5000;
	opt->dns_servers = NULL;
	opt->ai_family = 0;
	gettimeofday(&opt->start_time, NULL);
	opt->urls_l = 0;
	memset(&opt->urls, 0, MAX_URLS * sizeof(&opt->urls));
	opt->urls_loc = NULL;

	if(parse_options(opt, argc, argv) < 0)
		return -2;

	opt->initialised = 1;
	return 0;

}

void destroy_options(struct options *opt)
{
	int i;
	if(!opt->initialised)
		return;
	opt->initialised = 0;
	free(opt->dns_servers);
	free(opt->urls_loc);
	for(i = 0; i < opt->urls_l; i++) free(opt->urls[i]);
}

static void usage(const char *name)
{
	fprintf(stderr, "Usage: %s [-46Dh] [-c <count>] [-d <dns_servers>] [-i <interval>] [-l <length>] [-n <workers>] [-o <output>] [-t <timeout>] [url_file]\n", name);
}


int parse_options(struct options *opt, int argc, char **argv)
{
	int o;
	int val;
	FILE *output, *urlfile;
	char * line;
	size_t len = 0;
	ssize_t read;

	while((o = getopt(argc, argv, "46Dhc:d:i:l:n:o:t:")) != -1) {
		switch(o) {
		case '4':
			opt->ai_family = AF_INET;
			break;
		case '6':
			opt->ai_family = AF_INET6;
			break;
		case 'c':
			val = atoi(optarg);
			if(val < 1) {
				fprintf(stderr, "Invalid count value: %d\n", val);
				return -1;
			}
			opt->count = val;
			break;
		case 'D':
			opt->debug++;
			break;
		case 'd':
			opt->dns_servers = malloc(strlen(optarg)+1);
			if(!opt->dns_servers) {
				perror("malloc");
				return -1;
			}
			strcpy(opt->dns_servers, optarg);
			break;
		case 'i':
			val = atoi(optarg);
			if(val < 1) {
				fprintf(stderr, "Invalid interval value: %d\n", val);
				return -1;
			}
			opt->interval = val;
			break;
		case 'l':
			val = atoi(optarg);
			if(val < 1) {
				fprintf(stderr, "Invalid length: %d\n", val);
				return -1;
			}
			opt->run_length = val;
			break;
		case 'n':
			val = atoi(optarg);
			if(val < 1) {
				fprintf(stderr, "Invalid number of workers: %d\n", val);
				return -1;
			}
			opt->workers = val;
			break;
		case 'o':
			if(opt->output != stdout) {
				fprintf(stderr, "Output file already set.\n");
				return -1;
			}
			if(strcmp(optarg, "-") != 0) {
				output = fopen(optarg, "w");
				if(output == NULL) {
					perror("Unable to open output file");
					return -1;
				}
				opt->output = output;
			}
			break;
		case 't':
			val = atoi(optarg);
			if(val < 1) {
				fprintf(stderr, "Invalid timeout value: %d\n", val);
				return -1;
			}
			opt->timeout = val;
			break;
		case 'h':
		default:
			usage(argv[0]);
			return -1;
			break;
		}
	}
	if(optind >= argc || strcmp(argv[optind], "-") == 0) {
		urlfile = stdin;
	} else {
		if(strncmp(argv[optind], "http://", 7) == 0 ||
			strncmp(argv[optind], "https://", 8) == 0) {
			opt->urls_loc = malloc(strlen(argv[optind])+1);
			strcpy(opt->urls_loc, argv[optind]);
			return 0;
		}
		urlfile = fopen(argv[optind], "r");
		if(urlfile == NULL) {
			perror("Unable to open url file.");
			return -1;
		}
	}

	while((read = getline(&line, &len, urlfile)) != -1) {
		if(opt->urls_l >= MAX_URLS) {
			fprintf(stderr, "Max number of urls (%d) exceeded.\n", MAX_URLS);
			return -1;
		}
		if(*line == '#') continue;
		opt->urls[opt->urls_l] = malloc(len);
		memcpy(opt->urls[opt->urls_l], line, read);
		opt->urls[opt->urls_l][read-1] = '\0';
		opt->urls_l++;
	}
	free(line);
	fclose(urlfile);

	return 0;
}
