#ifndef __SOCKS_INTERCEPT_H__
#define __SOCKS_INTERCEPT_H__

#ifdef __FreeBSD__
#include <net/if.h>
#include <net/pfvar.h>
#endif

#ifdef __linux__
#include <linux/if.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6/ip6_tables.h>
#endif

struct socks_intercept_t
{
	struct in_addr addr;
	unsigned short int port;

};

struct socks_intercept_t* socks_intercept(struct _socks_address_t* local,
    struct _socks_address_t* remote,int silent);

#endif
