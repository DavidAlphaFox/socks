#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <socks_common.h>
#include "../common/socks_address.h"
#include "socks_intercept.h"

void print_ip(int ip)
{
  unsigned char bytes[4];
  bytes[0] = ip & 0xFF;
  bytes[1] = (ip >> 8) & 0xFF;
  bytes[2] = (ip >> 16) & 0xFF;
  bytes[3] = (ip >> 24) & 0xFF;
  printf("%d.%d.%d.%d\n", bytes[3], bytes[2], bytes[1], bytes[0]);
}

#ifdef __FreeBSD__

struct socks_intercept_t* pf_interception(struct _socks_address_t* local,
    struct _socks_address_t* remote,int silent)
{
    struct pfioc_natlook nl;
    static int pffd = -1;
    struct socks_intercept_t* intercept;
    intercept = (struct socks_intercept_t*)malloc(sizeof(struct socks_intercept_t));
    if(!intercept) GOTO_ERR;

    if (pffd < 0)
        pffd = open("/dev/pf", O_RDONLY);

    if (pffd < 0) {
        if (!silent) {
            LOG(LEVEL_ERROR, "PF open failed.\n");
        }
        return NULL;
    }

    memset(&nl, 0, sizeof(struct pfioc_natlook));
    socks_address_get_in_addr(remote,&nl.saddr.v4);
    socks_address_get_port(remote,&nl.sport);

    printf("client ip: ");
    print_ip(nl.saddr.v4.s_addr);

    printf("port:");
    printf("%d\r\n",nl.sport);
    nl.sport = htons(nl.sport);

    socks_address_get_in_addr(local,&nl.daddr.v4);
    socks_address_get_port(local,&nl.dport);
    
    printf("my ip: ");
    print_ip(nl.daddr.v4.s_addr);
    printf("port:");
    printf("%d\r\n",nl.dport);
    nl.dport = htons(nl.dport);
    
    nl.af = AF_INET;
    nl.proto = IPPROTO_TCP;
    nl.direction = PF_OUT;

    if (ioctl(pffd, DIOCNATLOOK, &nl)) {
        if (errno != ENOENT) {
            if (!silent) {
                 LOG(LEVEL_ERROR,"PF lookup failed: ioctl(DIOCNATLOOK)\n");
            }
            close(pffd);
            pffd = -1;
        }
	printf("PF lookup fail %d\n",errno);
        return NULL;
    } else {
        memcpy(&intercept->addr,&nl.rdaddr.v4,sizeof(struct in_addr));
	printf("redirect ip:");
	print_ip(nl.rdaddr.v4.s_addr);
        intercept->port =  nl.rdport;
        return intercept;
    }
_err:
    return NULL;
}
#endif


struct socks_intercept_t* socks_intercept(struct _socks_address_t* local,
    struct _socks_address_t* remote,int silent){
    #ifdef __FreeBSD__
        return pf_interception(local,remote,silent);
    #endif
    #ifdef __linux__
    #endif
    return NULL;
}
