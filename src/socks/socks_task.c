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

#include "../common/socks_buffer.h"
#include "../common/socks_socket.h"
#include "../pipe/socks_pipe.h"
#include "../thread/socks_thread.h"

#include "socks_task.h"

void socks_task_thread_handler(struct ev_loop* loop,void* data)
{
    struct socks_task_t* task = (struct socks_task_t*) data;

    struct _socks_socket_t* client = NULL;
    struct _socks_socket_t* server = NULL;
    struct _socks_pipe_t* pipe = NULL;
    client = socks_socket_alloc(loop,task->client_sockfd,socks_pipe_client_handler);
    if(!client) GOTO_ERR;

    server = socks_socket_alloc(loop,task->server_sockfd,socks_pipe_server_handler);
    if(!server) GOTO_ERR;

    pipe = socks_pipe_alloc(server,client);
    if(!pipe) GOTO_ERR;

    socks_socket_set_data(client,pipe);
    socks_socket_set_data(server,pipe);

    socks_socket_enable(client,1,0);
    socks_socket_enable(server,1,1);

    free(task);
    task = NULL;

    LOG(LEVEL_INFORM,"starting transfer\n");
    return;
_err:
	if(task){
		if(-1 != task->server_sockfd){
			close(task->server_sockfd);
		}
		if(-1 != task->client_sockfd){
			close(task->client_sockfd);
		}
		free(task);
	}
	if(client){
		free(client);
	}
	if(server){
		free(server);
	}

	return;
}