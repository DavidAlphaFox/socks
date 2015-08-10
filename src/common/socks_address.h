#ifndef __SOCKS_ADDRESS_H__
#define __SOCKS_ADDRESS_H__

struct _socks_address_t
{
	int _inet6;
	struct sockaddr_in _addr;
	struct sockaddr_in6 _addr6;
};

struct _socks_address_t* socks_address_alloc(struct addrinfo* addr);
void socks_address_free(struct _socks_address_t* address);
void socks_address_get_in_addr(struct _socks_address_t* address,void* buf);
void socks_address_get_port(struct _socks_address_t* address,unsigned short int* port);

#endif