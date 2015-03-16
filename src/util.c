/**
 * util.c
 *
 * Toke Høiland-Jørgensen
 * 2015-03-16
 */

#include "util.h"
#include <unistd.h>
#include <stdio.h>

int msg_write(int fd, char* buf, int len)
{
	unsigned short msg_len = (unsigned short) len;
	int bytes_w = 0, bytes_w_tot = 0;
	if(write(fd, &msg_len, sizeof(msg_len)) < sizeof(msg_len)) {
		perror("Error writing msg len");
		return -1;
	}
	while(bytes_w_tot < msg_len) {
		if ((bytes_w = write(fd, buf, msg_len)) < 0) {
			perror("Error writing msg");
			return -1;
		}
		bytes_w_tot += bytes_w;
	}
	return 0;
}
int msg_read(int fd, char* buf, int max_len)
{
	unsigned short msg_len;
	int bytes_r = 0, bytes_r_tot = 0;
	if((bytes_r = read(fd, &msg_len, sizeof(msg_len))) < 0) {
		perror("Read msg_len");
		return -1;
	} else if(bytes_r < sizeof(msg_len)) {
		return 0;
	}
	if(msg_len > max_len -1) {
		fprintf(stderr, "Got msg_len %d larger than max_len %d\n", msg_len, max_len);
		return -1;
	}
	while(bytes_r_tot < msg_len) {
		if((bytes_r = read(fd, buf+bytes_r_tot, msg_len-bytes_r_tot)) < 0) return -1;
		else if(bytes_r == 0) {buf[bytes_r_tot] = '\0'; return 0;}
		bytes_r_tot += bytes_r;
	}
	buf[msg_len] = '\0';
	return msg_len;
}
