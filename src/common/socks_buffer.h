#ifndef __SOCKS_BUFFER_H__
#define __SOCKS_BUFFER_H__

#define SOCKS_BUFFER_HAS_DATA(b)  ((b)->_offset)
#define SOCKS_BUFFER_USED(b)      ((b)->_data - (b)->_origin + (b)->_offset)
#define SOCKS_BUFFER_AVAILABLE(b) ((b)->_length - SOCKS_BUFFER_USED(b))
#define SOCKS_BUFFER_DEFAULT_SIZE 0x1000
struct _socks_buffer_t
{
    char* _data;
    char* _origin;
    size_t _offset;
    size_t _length;
    size_t _capacity;
    int _overflow;
};

struct _socks_buffer_t* socks_buffer_alloc(size_t length, size_t capacity);
void socks_buffer_free(struct _socks_buffer_t* buf);
int socks_buffer_read(struct _socks_buffer_t* buf, int fd);
int socks_buffer_write(struct _socks_buffer_t* buf, int fd);
int socks_buffer_overflow(struct _socks_buffer_t* buf);

#endif