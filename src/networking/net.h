#ifndef _NET_H
#define _NET_H
#include <stdbool.h>
#include <stddef.h>

void net_init_client(char *url);
bool net_req_http(char *url, void (*parser)(char *buf, size_t len));

#endif