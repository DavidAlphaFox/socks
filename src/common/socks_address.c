#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <socks_common.h>

#include "socks_address.h"

const struct in6_addr v4_localhost = {{{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x01 }}};

const struct in6_addr v4_anyaddr = {{{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 }}};

const struct in6_addr v4_noaddr = {{{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }}};

const struct in6_addr v6_noaddr = {{{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }}};

int _socks_address_ipv4(struct sockaddr_in6* sockaddr)
{
	if(IN6_IS_ADDR_V4MAPPED( &sockaddr->sin6_addr )){
		return 0;
	}
	return -1;
}

void _socks_address_map6to4(struct in6_addr* in, struct in_addr* out)
{

    memset(out, 0, sizeof(struct in_addr));
    ((uint8_t *)&out->s_addr)[0] = in->s6_addr[12];
    ((uint8_t *)&out->s_addr)[1] = in->s6_addr[13];
    ((uint8_t *)&out->s_addr)[2] = in->s6_addr[14];
    ((uint8_t *)&out->s_addr)[3] = in->s6_addr[15];
}

void _socks_address_map4to6(struct in_addr* in, struct in6_addr* out)
{
    /* check for special cases */

    if ( in->s_addr == 0x00000000) {
        /* ANYADDR */
        memcpy(out,&v4_anyaddr,sizeof(struct in6_addr));
    } else if ( in->s_addr == 0xFFFFFFFF) {
        /* NOADDR */
        memcpy(out,&v4_noaddr,sizeof(struct in6_addr));
    } else {
        /* general */
        memcpy(out,&v4_anyaddr,sizeof(struct in6_addr));
        out->s6_addr[12] = ((uint8_t *)&in->s_addr)[0];
        out->s6_addr[13] = ((uint8_t *)&in->s_addr)[1];
        out->s6_addr[14] = ((uint8_t *)&in->s_addr)[2];
        out->s6_addr[15] = ((uint8_t *)&in->s_addr)[3];
    }
}

void _socks_address_get_in_addr6(struct _socks_address_t* address,void* buf)
{
	
  if(0 == _socks_address_ipv4(&address->_addr6)){
    _socks_address_map6to4(&address->_addr6.sin6_addr,buf);
  }else{
    memcpy(buf, &address->_addr6.sin6_addr, sizeof(struct in6_addr));
  }
}

void socks_address_get_in_addr(struct _socks_address_t* address,void* buf)
{    
	if (1 == address->_inet6){
	  printf("use ipv6\n");
	  _socks_address_get_in_addr6(address,buf);
	}else{
	  printf("use ipv4\n");

	  memcpy(buf,&address->_addr.sin_addr,sizeof(struct in_addr));
	  
	}
}

struct _socks_address_t* socks_address_alloc(struct addrinfo* addr)
{
  struct sockaddr_in* ipv4 = NULL;

    struct _socks_address_t* address;
    address = (struct _socks_address_t*)malloc(sizeof(struct _socks_address_t));
    if(NULL == address) GOTO_ERR;
    address->_inet6 = 0;

    if (AF_INET == addr->ai_family) {
        ipv4 = (struct sockaddr_in*)(addr->ai_addr);
        memcpy(&address->_addr,ipv4,sizeof(struct sockaddr_in));
        address->_inet6 = 0;
        return address;
    }

    struct sockaddr_in6* ipv6 = NULL;
    if(AF_INET6 == addr->ai_family) {
        ipv6 = (struct sockaddr_in6*)(addr->ai_addr);
        memcpy(&address->_addr,ipv6,sizeof(struct sockaddr_in6));
        address->_inet6 = 1;
        return address;
    }
    
    if (addr->ai_addr != NULL) {
      if (addr->ai_addrlen == sizeof(struct sockaddr_in6)) {
	ipv6 = (struct sockaddr_in6*)(addr->ai_addr);
	memcpy(&address->_addr,ipv6,sizeof(struct sockaddr_in6));
	address->_inet6 = 1;
	return address;
      } else if (addr->ai_addrlen == sizeof(struct sockaddr_in)) {
	ipv4 = (struct sockaddr_in*)(addr->ai_addr);
	memcpy(&address->_addr,ipv4,sizeof(struct sockaddr_in));
       	//address->_addr.sin_addr.s_addr = htonl(address->_addr.sin_addr.s_addr);
	//address->_addr.sin_port = htons(address->_addr.sin_port);
	address->_inet6 = 0;
	return address;
      }
    }

    if(address){
      free(address);
      return NULL;
    }
_err:
    return NULL;
}
void socks_address_free(struct _socks_address_t* address)
{
	free(address);
}
void socks_address_get_port(struct _socks_address_t* address,unsigned short int* port)
{
	if(1 == address->_inet6){
	  *port = ntohs(address->_addr6.sin6_port);
	  return;
	}else{
	  *port = ntohs(address->_addr.sin_port);
	  return;
	}

	*port = 0;
}
