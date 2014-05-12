/**
 * getter.h
 *
 * Toke Høiland-Jørgensen
 * 2014-05-07
 */

#ifndef GETTER_H
#define GETTER_H

#include <curl/curl.h>
#include "options.h"

#define USLEEP_THRESHOLD 10000

int get_loop(struct options *opt);
void kill_workers();

#endif
