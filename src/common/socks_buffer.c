#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <sys/ioctl.h>
#include <sys/socket.h>

#include "socks_buffer.h"

void _socks_buffer_reset(struct _socks_buffer_t* buf);
int _socks_buffer_expand(struct _socks_buffer_t* buf, size_t need);

struct _socks_buffer_t* socks_buffer_alloc(size_t length, size_t capacity)
{
    struct _socks_buffer_t* buf;
    
    buf = malloc(sizeof(struct _socks_buffer_t));
    buf->_origin = malloc(length);
    buf->_data = buf->_origin;
    buf->_offset = 0;
    buf->_length = length;
    buf->_capacity = capacity;
    buf->_overflow = 0;
    
    return buf;
}

void socks_buffer_free(struct _socks_buffer_t* buf)
{
    if (buf) {
      if(NULL != buf->_origin){
        free(buf->_origin);
      }
        free(buf);
    }
}

void _socks_buffer_drain(struct _socks_buffer_t* buf, size_t length)
{
    if (length > buf->_offset) {
        _socks_buffer_reset(buf);
    } else {
        buf->_data += length;
        buf->_offset -= length;
    }
}

int _socks_buffer_add(struct _socks_buffer_t* buf, void* source, size_t length)
{
    size_t used = buf->_data - buf->_origin + buf->_offset;
    size_t need = used + length - buf->_length;
    
    if (need > 0) {
        if (!_socks_buffer_expand(buf, need)) {
            return 0;
        }
    }
    
    memcpy(buf->_data + buf->_offset, source, length);
    buf->_offset += length;
    
    return 1;
}


void _socks_buffer_reset(struct _socks_buffer_t* buf)
{
    buf->_data = buf->_origin;
    buf->_offset = 0;
}

int _socks_buffer_expand(struct _socks_buffer_t* buf, size_t need)
{

    size_t pos = buf->_data - buf->_origin;
    size_t expand = 0;
    size_t new_size = 0;

    if (need <= pos) {
        memmove(buf->_origin, buf->_data, buf->_offset);
        buf->_data = buf->_origin;
        return 1;
    }
    
    expand = buf->_length;
    while (expand < need) {
        expand = expand * 2;
    }
    
    new_size = buf->_length + expand;
    if (buf->_capacity > 0 && new_size > buf->_capacity) {
        return 0;
    }
    
    buf->_origin = realloc(buf->_origin, new_size);
    buf->_data = buf->_origin + pos;
    buf->_length = new_size;
    return 1;
}



int socks_buffer_read(struct _socks_buffer_t* buf, int fd)
{
    int n;
    int need;
    buf->_overflow = 0;
    if (ioctl(fd, FIONREAD, &n) == -1 || n > 4096) {
        n = 4096;
    }
    
    need = n - SOCKS_BUFFER_AVAILABLE(buf);
    if (need > 0 && !_socks_buffer_expand(buf, need)) {
        buf->_overflow = 1;
        return -1;
    }
    
    n = recv(fd, buf->_data + buf->_offset, n, 0);
    if (n > 0) {
        buf->_offset += n;
    }
    
    return n;
}

int socks_buffer_write(struct _socks_buffer_t* buf, int fd)
{
    int n;
    n = send(fd, buf->_data, buf->_offset, 0);
    if (n > 0) {
        _socks_buffer_drain(buf, n);
    }
    return n;
}
int socks_buffer_overflow(struct _socks_buffer_t* buf)
{
    if(NULL == buf){
        return -1;
    }
    return buf->_overflow;
}