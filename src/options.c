/**
 * options.c
 *
 * Toke Høiland-Jørgensen
 * 2014-05-07
 */

#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include "options.h"

int parse_options(struct options *opt, int argc, char **argv);


int initialise_options(struct options *opt, int argc, char **argv)
{
	opt->run_length = 60;
	opt->output = stdout;
	opt->interval = 200;
	opt->threads = 4;
	opt->ai_family = AF_INET;
	gettimeofday(&opt->start_time, NULL);
	opt->urls_l = 0;
	memset(&opt->urls, 0, MAX_URLS * sizeof(&opt->urls));

	if(parse_options(opt, argc, argv) < 0)
		return -2;

	opt->initialised = 1;
	return 0;

}

void destroy_options(struct options *opt)
{
	if(!opt->initialised)
		return;
	opt->initialised = 0;
}

static void usage(const char *name)
{
	fprintf(stderr, "Usage: %s [-46] [-i <interval>] [-l <length>] [-n <threads>] [-o <output>] <url_file>\n", name);
}


int parse_options(struct options *opt, int argc, char **argv)
{
	int o;
	int val;
	FILE *output, *urlfile;
	char * line;
	size_t len = 0;
	ssize_t read;

	while((o = getopt(argc, argv, "46i:l:n:o:")) != -1) {
		switch(o) {
		case '4':
			opt->ai_family = AF_INET;
			break;
		case '6':
			opt->ai_family = AF_INET6;
			break;
		case 'i':
			// interval
			val = atoi(optarg);
			if(val < 1) {
				fprintf(stderr, "Invalid inverval value: %d\n", val);
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
				fprintf(stderr, "Invalid number of threads: %d\n", val);
				return -1;
			}
			opt->threads = val;
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
		case 'h':
		default:
			usage(argv[0]);
			return -1;
			break;
		}
	}
	if(optind >= argc) {
		usage(argv[0]);
		return -1;
	}
	if(strcmp(argv[optind], "-") == 0) {
		urlfile = stdin;
	} else {
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
		opt->urls[opt->urls_l] = malloc(len);
		memcpy(opt->urls[opt->urls_l], line, read);
		opt->urls[opt->urls_l][read-1] = '\0';
		opt->urls_l++;
	}
	free(line);
	fclose(urlfile);

	return 0;
}
