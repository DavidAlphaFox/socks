#ifndef __SOCKS_STRING_H__
#define __SOCKS_STRING_H__

typedef char* socks_string_t; 

struct _socks_string_t 
{
    size_t _offset;
    size_t _length;
    char* data;
};


socks_string_t socks_string_alloc();
void soks_string_free(socks_string_t s);

static inline size_t socks_string_len(socks_string_t s) {
    struct _socks_string_t* _s = (void*)(s -(sizeof(struct _socks_string_t)));
    return _s->_offset;
}

static inline size_t socks_string_avail(const sds s) {
    struct _socks_string_t* _s = (void*)(s - (sizeof(struct _socks_string_t)));
    return _s->_length;
}

#endif