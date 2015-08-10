#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include <fcntl.h>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
 
#include <ev.h>

#include <socks_common.h>

#include "socks_buffer.h"
#include "socks_socket.h"


void _socks_socket_callback(struct ev_loop *loop, struct ev_io *watcher, int revents);

struct _socks_socket_t* socks_socket_alloc(struct ev_loop* loop,
	int fd,socks_socket_handler_t handler)
{
	if(fd < 0 || NULL == loop){
		return NULL;
	}

	struct _socks_socket_t* socket;
	socket = (struct _socks_socket_t*)malloc(sizeof(struct _socks_socket_t));
	if(NULL == socket) GOTO_ERR;
	memset(socket,0,sizeof(struct _socks_socket_t));

	socket->_fd = fd;
	socket->_handler = handler;
	socket->_loop = loop;
	socket->_watcher.data = socket;
	socket->_stoped = 1;

	ev_init(&socket->_watcher,_socks_socket_callback);
	return socket;

_err:
	if(NULL != socket){
		free(socket);
	}
	return NULL;
}

void socks_socket_free(struct _socks_socket_t* socket)
{
	if(NULL == socket){
		return;
	}

	if(!socket->_stoped){
		ev_io_stop(socket->_loop,&socket->_watcher);
	}
	if(-1 != socket->_fd){
		close(socket->_fd);
	}
	free(socket);
}

void _socks_socket_callback(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
	struct _socks_socket_t* socket = (struct _socks_socket_t*)watcher->data;
	socks_socket_handler_t handler = socket->_handler;
	if(socket->_stoped){
		return;
	}
	LOG(LEVEL_INFORM,"handle event %d",revents);
	handler(socket,revents);
	/*if(socket->_connecting){
		if(revents & EV_ERROR){
			handler(socket,revents);
		}else{
			socket->_connecting = 0;
			socket->_connected = 1;
			handler(socket,revents);
		}
	}else{
		handler(socket,revents);
	}*/
}

int socks_socket_is_stoped(struct _socks_socket_t* socket)
{
	if(NULL == socket){
		return -1;
	}
	return socket->_stoped;
}
int socks_socket_is_reading(struct _socks_socket_t* socket)
{
	if(NULL == socket){
		return -1;
	}
	return socket->_reading;
}
int socks_socket_is_writing(struct _socks_socket_t* socket)
{
	if(NULL == socket){
		return -1;
	}
	return socket->_writing;
}

int socks_socket_enable(struct _socks_socket_t* socket,int reading,int writing)
{
	if(1 == (reading & writing)){
		if(socket->_reading && socket->_writing){
			return 0;
		}
		if(!socket->_stoped){
			ev_io_stop(socket->_loop,&socket->_watcher);
		}else{
			socket->_stoped = 0;
		}
		ev_io_set(&socket->_watcher,socket->_fd,EV_READ|EV_WRITE);
		ev_io_start(socket->_loop,&socket->_watcher);
		socket->_reading = 1;
		socket->_writing = 1;
		return 0;
	} 

	if(reading){
		//socket is reading now
		if(socket->_reading){
			return 0;
		}
		//stop watching events
		if(!socket->_stoped){
			ev_io_stop(socket->_loop,&socket->_watcher);
		}else{
			socket->_stoped = 0;
		}

		if(socket->_writing){
			ev_io_set(&socket->_watcher,socket->_fd,EV_READ|EV_WRITE);	
		}else{
			ev_io_set(&socket->_watcher,socket->_fd,EV_READ);
		}
			
		ev_io_start(socket->_loop,&socket->_watcher);
		socket->_reading = 1;
		return 0;
	}

	if(writing){
		if(socket->_writing){
			return 0;
		}
			//stop watching events
		if(!socket->_stoped){
			ev_io_stop(socket->_loop,&socket->_watcher);
		}else{
			socket->_stoped = 0;
		}

		if(socket->_reading){
			ev_io_set(&socket->_watcher,socket->_fd,EV_READ|EV_WRITE);	
		}else{
			ev_io_set(&socket->_watcher,socket->_fd,EV_WRITE);
		}
			
		ev_io_start(socket->_loop,&socket->_watcher);
		socket->_writing = 1;
		return 0;
	}

	return -1;
}
int socks_socket_disable(struct _socks_socket_t* socket,int reading,int writing){
	if(1 == (reading & writing)){
		if(socket->_stoped){
			return 0;
		}
		ev_io_stop(socket->_loop,&socket->_watcher);
		socket->_stoped = 1;
		socket->_reading = 0;
		socket->_writing = 0;
		return 0;
	}

	if(reading){
		if(!socket->_reading){
			return 0;
		}
		if(socket->_writing){
			ev_io_stop(socket->_loop,&socket->_watcher);
			ev_io_set(&socket->_watcher,socket->_fd,EV_WRITE);
			ev_io_start(socket->_loop,&socket->_watcher);
		}else{
			ev_io_stop(socket->_loop,&socket->_watcher);
			socket->_stoped = 1;
		}

		socket->_reading = 0;
		return 0;
	}

	if(writing){
		if(!socket->_writing){
			return 0;
		}
		if(socket->_reading){
			ev_io_stop(socket->_loop,&socket->_watcher);
			ev_io_set(&socket->_watcher,socket->_fd,EV_READ);
			ev_io_start(socket->_loop,&socket->_watcher);
		}else{
			ev_io_stop(socket->_loop,&socket->_watcher);
			socket->_stoped = 1;
		}

		socket->_writing = 0;
		return 0;
	}
	return -1;
}

int socks_socket_buffer_read(struct _socks_socket_t* socket,struct _socks_buffer_t* buffer)
{
	//LOG(LEVEL_INFORM,"socks_socket_buffer_read, buffer:%p\n",buffer);
	int read_bytes = 0;

	if(NULL == socket) GOTO_ERR;

	if(socket->_stoped) GOTO_ERR;

	read_bytes = socks_buffer_read(buffer,socket->_fd);

	return read_bytes;

_err:
	return -1;
}
int socks_socket_buffer_write(struct _socks_socket_t* socket,struct _socks_buffer_t* buffer)
{
//	LOG(LEVEL_INFORM,"socks_socket_buffer_write, buffer:%p\n",buffer);
	int write_bytes = 0;

	if(NULL == socket) GOTO_ERR;

	if(socket->_stoped) GOTO_ERR;

	write_bytes = socks_buffer_write(buffer,socket->_fd);

	return write_bytes;

_err:
	return -1;
}

int socks_socket_read(struct _socks_socket_t* socket,char* buffer,int length)
{
	int read_bytes = 0;

	if(NULL == socket) GOTO_ERR;

	if(socket->_stoped) GOTO_ERR;

    read_bytes = recv(socket->_fd,buffer,length, 0);

	return read_bytes;

_err:
	return -1;
}

int socks_socket_write(struct _socks_socket_t* socket,char* buffer,int length)
{
	int write_bytes = 0;

	if(NULL == socket) GOTO_ERR;

	if(socket->_stoped) GOTO_ERR;

	write_bytes = send(socket->_fd, buffer,length, 0);

	return write_bytes;

_err:
	return -1;
}

void socks_socket_set_data(struct _socks_socket_t* socket,void* data)
{
	if(NULL != socket){
		socket->_data = data;
	}
}

void* socks_socket_get_data(struct _socks_socket_t* socket)
{
	if(NULL != socket){
		return socket->_data;
	}
	return NULL;
}
