#ifndef __SOCKS_TASK_H__
#define __SOCKS_TASK_H__

struct socks_task_t
{
	int server_sockfd;
	int client_sockfd;
};

void socks_task_thread_handler(struct ev_loop* loop,void* data);

#endif