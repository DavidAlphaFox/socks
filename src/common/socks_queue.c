#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include "socks_queue.h"

void _scoks_queue_lock(struct _socks_queue_t* q)
{
    pthread_mutex_lock(&q->_lock);
}

void _scoks_queue_unlock(struct _socks_queue_t* q)
{
    pthread_mutex_unlock(&q->_lock);
}

struct _socks_queue_t* socks_queue_alloc()
{
	struct _socks_queue_t* q = (struct _socks_queue_t*)malloc(sizeof(struct _socks_queue_t));
	if(NULL == q){
		return NULL;
	}
    q->_msg_head = q->_msg_tail = NULL;
    q->_count = 0;
    pthread_mutex_init(&q->_lock, NULL);
    return q;
}

void socks_queue_free(struct _socks_queue_t* q)
{
    struct socks_msg_t *msgs = q->_msg_head, *last = NULL;
    pthread_mutex_destroy(&q->_lock);
    free(q);
    while (msgs){
        last = msgs;
        msgs = msgs->_next;
        free(last);
    }
}

void socks_queue_push(struct _socks_queue_t* q, struct socks_msg_t* msg)
{
    _scoks_queue_lock(q);
    msg->_next = NULL;
    if (q->_msg_tail){
        q->_msg_tail->_next = msg;
    }
    q->_msg_tail = msg;
    if (q->_msg_head == NULL){
    	q->_msg_head = msg;
    }  
    q->_count++;
    _scoks_queue_unlock(q);
}

struct socks_msg_t* socks_queue_pop(struct _socks_queue_t* q)
{
    struct socks_msg_t* msg = NULL;

    _scoks_queue_lock(q);
    if (q->_count > 0){
        msg = q->_msg_head;
        if (msg){
            q->_msg_head = msg->_next;
            if (q->_msg_head == NULL)
                q->_msg_tail = NULL;
            msg->_next = NULL;
        }
        q->_count--;
    }


    _scoks_queue_unlock(q);
    return msg;
}