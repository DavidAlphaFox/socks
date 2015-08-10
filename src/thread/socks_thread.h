#ifndef __SOCKS_THREAD_H__
#define __SOCKS_THREAD_H__


typedef void (*socks_thread_handler_t)(struct ev_loop* loop,void* data);

struct _socks_thread_t
{
	struct ev_loop* _loop;
	struct ev_io* _notify_watcher;
	int _notify_fds[2];

	struct _socks_queue_t* _queue;
	socks_thread_handler_t _handler;
	pthread_t _tid;
};

struct _socks_thread_t* socks_thread_alloc(socks_thread_handler_t handler);
int socks_thread_init(struct _socks_thread_t* thread);
int socks_thread_request(struct _socks_thread_t* thread,void* request);
int socks_thread_stop(struct _socks_thread_t* thread);
int socks_thread_start(struct _socks_thread_t* thread);
#endif