#ifndef __SOCKS_PIPE_H__
#define __SOCKS_PIPE_H__ 
struct _socks_pipe_t 
{
	struct _socks_socket_t* _server_socket;
	struct _socks_socket_t* _client_socket;
  	struct _socks_buffer_t* _server_buffer;
  	struct _socks_buffer_t* _client_buffer;

  	int _connected;
};

struct _socks_pipe_t* socks_pipe_alloc(struct _socks_socket_t* server_socket,
	struct _socks_socket_t* client_socket);
void socks_pipe_free(struct _socks_pipe_t* pipe);
void socks_pipe_server_handler(struct _socks_socket_t* socket, int revents);
void socks_pipe_client_handler(struct _socks_socket_t* socket, int revents);

#endif
