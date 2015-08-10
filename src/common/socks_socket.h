#ifndef __SOCKS_SOCKET_H__
#define __SOCKS_SOCKET_H__

struct _socks_socket_t;

typedef void (*socks_socket_handler_t)(struct _socks_socket_t* socket,int revents);

struct _socks_socket_t
{
	int _fd;
	struct ev_io _watcher;
	struct ev_loop* _loop;
	int _stoped;
	int _reading;
	int _writing;
	socks_socket_handler_t	_handler;
	void* _data;
};

struct _socks_socket_t* socks_socket_alloc(struct ev_loop* loop,
	int fd,socks_socket_handler_t handler);
void socks_socket_free(struct _socks_socket_t* socket);

int socks_socket_is_stoped(struct _socks_socket_t* socket);
int socks_socket_is_reading(struct _socks_socket_t* socket);
int socks_socket_is_writing(struct _socks_socket_t* socket);


int socks_socket_enable(struct _socks_socket_t* socket,int reading,int writing);
int socks_socket_disable(struct _socks_socket_t* socket,int reading,int writing);

int socks_socket_buffer_read(struct _socks_socket_t* socket,struct _socks_buffer_t* buffer);
int socks_socket_buffer_write(struct _socks_socket_t* socket,struct _socks_buffer_t* buffer);
int socks_socket_read(struct _socks_socket_t* socket,char* buffer,int length);
int socks_socket_write(struct _socks_socket_t* socket,char* buffer,int length);

void socks_socket_set_data(struct _socks_socket_t* socket,void* data);
void* socks_socket_get_data(struct _socks_socket_t* socket);

#endif