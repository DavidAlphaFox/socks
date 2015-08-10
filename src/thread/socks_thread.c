#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>
#include <ev.h>

#include <socks_common.h>

#include "../common/socks_queue.h"
#include "socks_thread.h"

void _socks_thread_clean(struct _socks_thread_t* thread);
void _socks_event_handler(struct ev_loop *loop, struct ev_io *watcher, int revents);
int _socks_thread_notify(struct _socks_thread_t* thread);

struct _socks_thread_t* socks_thread_alloc(socks_thread_handler_t handler)
{
	struct _socks_thread_t* thread ;
	thread = (struct  _socks_thread_t*)malloc(sizeof(struct _socks_thread_t));
	if(thread){
		memset(thread,0,sizeof(struct _socks_thread_t));
		thread->_notify_fds[0] = -1;
		thread->_notify_fds[1] = -1;
		thread->_queue = NULL;
		thread->_handler = handler;
	}
	return thread;
}

int socks_thread_init(struct _socks_thread_t* thread)
{
	thread->_loop = ev_loop_new(EVFLAG_AUTO);
	if(!thread->_loop) GOTO_ERR;
	
	if (pipe(thread->_notify_fds)) GOTO_ERR;
	
	thread->_notify_watcher = (struct ev_io*) malloc (sizeof(struct ev_io)); 
	if(!thread->_notify_watcher) GOTO_ERR;

	thread->_queue = socks_queue_alloc();
	if( NULL == thread->_queue ) GOTO_ERR;

	thread->_notify_watcher->data = thread;

	ev_io_init(thread->_notify_watcher, 
		_socks_event_handler, thread->_notify_fds[0], EV_READ);  
	ev_io_start(thread->_loop, thread->_notify_watcher);

	return 0;
_err:
	_socks_thread_clean(thread);
	return -1;
}

int socks_thread_request(struct _socks_thread_t* thread,void* request)
{
	struct socks_msg_t* msg = (struct socks_msg_t*)malloc(sizeof(struct socks_msg_t));
	if(NULL == msg) GOTO_ERR;

	msg->data = request;
	socks_queue_push(thread->_queue,msg);
	if(_socks_thread_notify(thread)) GOTO_ERR;

	return 0;
_err:
	if(msg){
	  free(msg);
	}
	return -1;
}


int _socks_thread_notify(struct _socks_thread_t* thread){
	
	if (write(thread->_notify_fds[1], "", 1) != 1)
		return -1;
	return 0;
}

void _socks_thread_clean(struct _socks_thread_t* thread)
{
	if(thread->_notify_watcher){
		free(thread->_notify_watcher);
		thread->_notify_watcher = NULL;
	}
	if(-1 != thread->_notify_fds[0]){
		close(thread->_notify_fds[0]);
		close(thread->_notify_fds[1]);
		thread->_notify_fds[0] = -1;
		thread->_notify_fds[1] = -1;
	}
	if(thread->_loop){
		ev_loop_destroy(thread->_loop);
		thread->_loop = NULL;
	}

	if(thread->_queue){
		socks_queue_free(thread->_queue);
		thread->_queue = NULL;
	}
}

void _socks_event_handler(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
  	LOG(LEVEL_INFORM,"handle an event");
	struct _socks_thread_t* thread = (struct _socks_thread_t*)watcher->data;
	struct socks_msg_t* msg =  NULL;
	char ping[1];
	if (read(thread->_notify_fds[0], ping, 1) != 1) GOTO_ERR;
	msg = socks_queue_pop(thread->_queue);
	if(NULL != msg){
	  LOG(LEVEL_INFORM,"we have a message in thread %p\n",thread);
	  thread->_handler(thread->_loop,msg->data);
	  free(msg);
	}else{
	  ev_io_stop(thread->_loop,thread->_notify_watcher);
	  ev_break(thread->_loop, EVBREAK_ALL);
	  return;
	}
_err:
	return;
}

void* _socks_thread_loop(void* data) {
	struct _socks_thread_t* thread = (struct _socks_thread_t*)data;
	LOG(LEVEL_INFORM,"start thread %p\n",thread);
	ev_run(thread->_loop, 0);
	_socks_thread_clean(thread);
	free(thread);
	return NULL;
}
int socks_thread_start(struct _socks_thread_t* thread){
	int ret = -1;
	if(NULL == thread) {
		return ret;
	}
	ret = pthread_create(&thread->_tid,NULL,_socks_thread_loop,(void*)thread);
	return ret;
}

int socks_thread_stop(struct _socks_thread_t* thread){
	void* status = NULL;
	if(0 == _socks_thread_notify(thread)) {
		pthread_join(thread->_tid,&status);
		return 0;
	}
	return -1;
}
