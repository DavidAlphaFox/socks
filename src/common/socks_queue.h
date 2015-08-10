#ifndef __SOCKS_QUEUE_H__
#define __SOCKS_QUEUE_H__
struct socks_msg_t
{
    struct socks_msg_t* _next;
    void* data;
};

struct _socks_queue_t
{
    struct socks_msg_t* _msg_head;
    struct socks_msg_t* _msg_tail;
    int _count;
    pthread_mutex_t _lock;
};

struct _socks_queue_t* socks_queue_alloc();
void socks_queue_free(struct _socks_queue_t* q);
void socks_queue_push(struct _socks_queue_t* q, struct socks_msg_t* msg);
struct socks_msg_t* socks_queue_pop(struct _socks_queue_t* q);

#endif