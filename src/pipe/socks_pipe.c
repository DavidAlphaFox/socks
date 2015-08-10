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
#include "socks_pipe.h"

void _socks_pipe_server_transfer(struct _socks_pipe_t* pipe);
void _socks_pipe_client_transfer(struct _socks_pipe_t* pipe);
void _socks_pipe_server_stop_transfer(struct _socks_pipe_t* pipe);
void _socks_pipe_client_stop_transfer(struct _socks_pipe_t* pipe);


struct _socks_pipe_t* socks_pipe_alloc(struct _socks_socket_t* server_socket,
  struct _socks_socket_t* client_socket)
{
  struct _socks_pipe_t* pipe = (struct _socks_pipe_t*)malloc(sizeof(struct _socks_pipe_t));
  if(!pipe) GOTO_ERR;

  pipe->_server_buffer = socks_buffer_alloc(SOCKS_BUFFER_DEFAULT_SIZE,SOCKS_BUFFER_DEFAULT_SIZE * 10);
  if(!pipe->_server_buffer) GOTO_ERR;

  pipe->_client_buffer = socks_buffer_alloc(SOCKS_BUFFER_DEFAULT_SIZE,SOCKS_BUFFER_DEFAULT_SIZE * 10);
  if (!pipe->_client_buffer) GOTO_ERR;

  pipe->_server_socket = server_socket;
  pipe->_client_socket = client_socket;
  pipe->_connected = 0;
  return pipe;
_err:
  socks_pipe_free(pipe);
  return NULL;
}

void socks_pipe_free(struct _socks_pipe_t* pipe)
{
    if(NULL == pipe){
        return;
    }
    if(pipe->_client_socket){
        socks_socket_free(pipe->_client_socket);
    }
    if (pipe->_server_socket){
        socks_socket_free(pipe->_server_socket);
    }
    if(pipe->_server_buffer){
        socks_buffer_free(pipe->_server_buffer);
    }
    if(pipe->_client_buffer){
        socks_buffer_free(pipe->_client_buffer);
    }
    
    free(pipe);
}


void socks_pipe_server_handler(struct _socks_socket_t* socket, int revents)
{

  int read = 0;
  LOG(LEVEL_INFORM,"socks_pipe_server_handler handle event %d\n",revents);
  struct _socks_pipe_t* pipe = (struct _socks_pipe_t*)socks_socket_get_data(socket);
  if(EV_ERROR & revents){  
    LOG(LEVEL_ERROR, "error event in server.\n");  
    socks_pipe_free(pipe);
    return;  
  }
  if(0 == pipe->_connected){
    pipe->_connected = 1;
  }

  if(EV_READ & revents){
    read = socks_socket_buffer_read(pipe->_server_socket,pipe->_server_buffer);
    if(read > 0){
      _socks_pipe_server_transfer(pipe);
    }else if( read == 0){
      _socks_pipe_server_stop_transfer(pipe);
    }else{
      if(!socks_buffer_overflow(pipe->_server_buffer)){
        if (errno != EAGAIN && errno != EWOULDBLOCK) {  
          LOG(LEVEL_ERROR, "server stop all pipe transfer.\n");  
          socks_pipe_free(pipe);
          return;
        }
      }
    }
  }

  if(EV_WRITE & revents){
    _socks_pipe_client_transfer(pipe);
  }

}

void socks_pipe_client_handler(struct _socks_socket_t* socket, int revents)
{
  int read = 0;
  LOG(LEVEL_INFORM,"socks_pipe_client_handler handle event %d\n",revents);
  struct _socks_pipe_t* pipe = (struct _socks_pipe_t*)socks_socket_get_data(socket);
  if(EV_ERROR & revents){  
    LOG(LEVEL_ERROR, "error event in client.\n");  
    socks_pipe_free(pipe);
    return;  
  }
  if(EV_READ & revents){
    read = socks_socket_buffer_read(pipe->_client_socket,pipe->_client_buffer); 
    if (read > 0){
      _socks_pipe_client_transfer(pipe);
    }else if (0 == read){
      _socks_pipe_client_stop_transfer(pipe);
    }else {
      if(!socks_buffer_overflow(pipe->_client_buffer)){
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
          LOG(LEVEL_ERROR, "client stop all pipe transfer.\n");  
          socks_pipe_free(pipe);
          return;
        }
      }
  //LOG(LEVEL_INFORM, "client read finish %d, errno:%d\n",read,errno);  
    }
  }

  if(EV_WRITE & revents){
    _socks_pipe_server_transfer(pipe);
  }
}

void _socks_pipe_server_transfer(struct _socks_pipe_t* pipe)
{
    int write = 0;
    if(NULL == pipe->_client_socket){
      socks_pipe_free(pipe);
      return;
    }
    write = socks_socket_buffer_write(pipe->_client_socket,pipe->_server_buffer);
    if(write >= 0){
      if(!SOCKS_BUFFER_HAS_DATA(pipe->_server_buffer)){
        socks_socket_disable(pipe->_client_socket,0,1);
      }else{
        socks_socket_enable(pipe->_client_socket,0,1);
      }
    }else if(write < 0){
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        socks_pipe_free(pipe);
      }else{
        socks_socket_enable(pipe->_client_socket,0,1);
      }
    }
}

void _socks_pipe_client_transfer(struct _socks_pipe_t* pipe)
{
    int write = 0;
    if(0 == pipe->_connected){
      return;
    }
    if(NULL == pipe->_server_socket){
      socks_pipe_free(pipe);
    }
    write = socks_socket_buffer_write(pipe->_server_socket,pipe->_client_buffer);
    if ( write >= 0 ){
        if(SOCKS_BUFFER_HAS_DATA(pipe->_client_buffer)){
          socks_socket_enable(pipe->_server_socket,0,1);
        }else{
          socks_socket_disable(pipe->_server_socket,0,1);
        }
    }else if (write < 0 ){
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
          socks_pipe_free(pipe);
        }else{
          socks_socket_enable(pipe->_server_socket,0,1);
        }
    }
}

void _socks_pipe_server_stop_transfer(struct _socks_pipe_t* pipe)
{
  socks_socket_free(pipe->_server_socket);
  pipe->_server_socket = NULL;
  if(!SOCKS_BUFFER_HAS_DATA(pipe->_server_buffer)){
    socks_pipe_free(pipe);
  }else{
    socks_socket_enable(pipe->_client_socket,0,1);
  }
}

void _socks_pipe_client_stop_transfer(struct _socks_pipe_t* pipe)
{
  socks_socket_free(pipe->_client_socket);
  pipe->_client_socket = NULL;
  if(!SOCKS_BUFFER_HAS_DATA(pipe->_client_buffer)){
    socks_pipe_free(pipe);
  }else{
    socks_socket_enable(pipe->_server_socket,0,1);
  }
}

