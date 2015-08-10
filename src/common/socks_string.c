#include <stdio.h>
#include <stdlib.h>
#include <string.h>
socks_string_t socks_string_alloc()
{
	_socks_string_t* s = malloc(sizeof(struct _socks_string_t));
	if(NULL != s){
		s->_length = 0;
		s->_offset = 0;
		return s;
		
	}
}