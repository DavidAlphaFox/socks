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
#include "../common/socks_address.h"
#include "../intercept/socks_intercept.h"
#include "../pipe/socks_pipe.h"
#include "../thread/socks_thread.h"
#include "socks_task.h"

struct ev_loop *g_loop = NULL;  
struct ev_io g_acceptor_watcher;
int g_acceptor_sockfd = -1;
int g_last_thread_id = 0;
const int g_max_thread = 3;

struct _socks_thread_t* g_threads[g_max_thread];

int _srv_connector_init(int client_sockfd,struct addrinfo* remote);

void _srv_signal_callback(int sig)
{
    switch(sig){
        case SIGINT:
        case SIGTERM:
            ev_io_stop(g_loop, &g_acceptor_watcher);
            ev_break(g_loop, EVBREAK_ALL);
            LOG(LEVEL_INFORM, "signal [%d], exit.\n", sig);
            exit(0);
            break;
        default:
            LOG(LEVEL_INFORM, "signal [%d], not supported.\n", sig);
            break;
    }
}

void _srv_signal_init()
{
    int sig;

    // Ctrl + C 
    sig = SIGINT;
    if (SIG_ERR == signal(sig, _srv_signal_callback)){
        LOG(LEVEL_WARNING, "%s signal[%d] failed.\n", __func__, sig);
    }

    // kill/pkill -15
    sig = SIGTERM;
    if (SIG_ERR == signal(sig, _srv_signal_callback)){
        LOG(LEVEL_WARNING, "%s signal[%d] failed.\n", __func__, sig);
    }
}


int _srv_acceptor_init(uint16_t port, int32_t backlog)
{
    struct sockaddr_in serv;
    int sockfd;
    int opt;
    int flags;
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        LOG(LEVEL_ERROR, "socket error!\n");
        return -1;
    }

    memset((char *)&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    serv.sin_port = htons(port);
    
    if (-1 == (flags = fcntl(sockfd, F_GETFL, 0)))
        flags = 0;
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    
    opt = 1;
    if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (int8_t *)&opt, sizeof(opt))){
        LOG(LEVEL_ERROR, "setsockopt SO_REUSEADDR fail.\n");
        return -1;
    }

    #ifdef SO_NOSIGPIPE 
    if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt))){
        LOG(LEVEL_ERROR, "setsockopt SO_NOSIGPIPE fail.\n");
        return -1;
    }
    #endif

    if (bind(sockfd, (struct sockaddr *)&serv, sizeof(serv)) < 0){
        LOG(LEVEL_ERROR, "bind error [%d]\n", errno);
        return -1;
    }
    
    if (listen(sockfd, backlog) < 0){
        LOG(LEVEL_ERROR, "listen error!\n");
        return -1;
    }

    return sockfd;
}

void _srv_accpet_callback(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    struct addrinfo addr; 
    int client_sockfd = 0;
    int server_sockfd;

    if(EV_ERROR & revents){  
        LOG(LEVEL_ERROR, "error event in accept.\n");
        return;  
    }  
    struct socks_task_t* task = (struct socks_task_t*)malloc(sizeof(struct socks_task_t));

    if(!task){
        return;
    }
    addr.ai_addr = (struct sockaddr*)malloc(sizeof(struct sockaddr_in6));
    memset(addr.ai_addr, 0, sizeof(struct sockaddr_in6));
    addr.ai_addrlen = sizeof(struct sockaddr_in6); 
    
    client_sockfd = accept(watcher->fd, addr.ai_addr, &addr.ai_addrlen);
    LOG(LEVEL_INFORM,"Client is accepted\n");
    if (client_sockfd < 0) {
        free(addr.ai_addr);
        free(task);
        return;  
    }  
    
    if (-1 == (server_sockfd = _srv_connector_init(client_sockfd,&addr))){
        LOG(LEVEL_ERROR, "open remote fail.\n");
        free(addr.ai_addr);
        close(client_sockfd);
        free(task);
        return;
    }
    task->server_sockfd = server_sockfd;
    task->client_sockfd = client_sockfd;
    socks_thread_request(g_threads[g_last_thread_id],(void*)task);
    g_last_thread_id = (g_last_thread_id + 1)% g_max_thread;
    
    free(addr.ai_addr);
}

int _srv_connector_init(int client_sockfd,struct addrinfo* remote)
{
    struct addrinfo local;
    int server_sockfd = -1; 
    struct sockaddr_in addr;
    int flags;
    int opt = 1;

    struct socks_intercept_t* intercept = NULL;
    struct _socks_address_t* raddress = NULL;
    struct _socks_address_t* laddress = NULL;

    local.ai_addr = (struct sockaddr*)malloc(sizeof(struct sockaddr_in6));
    memset(local.ai_addr, 0, sizeof(struct sockaddr_in6));
    local.ai_addrlen = sizeof(struct sockaddr_in6); 
    
    if(getsockname(client_sockfd, local.ai_addr, &local.ai_addrlen) != 0) {
        LOG(LEVEL_ERROR,"getsockname() failed to locate local-IP\n");
        GOTO_ERR;
    }

    raddress = socks_address_alloc(remote);
    laddress = socks_address_alloc(&local);

    intercept = socks_intercept(laddress,raddress,0);
    if(!intercept) GOTO_ERR;

    if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) GOTO_ERR;
    memset(&addr,0,sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr,&intercept->addr,sizeof(struct in_addr));
    addr.sin_port = intercept->port;
    
    #ifdef SO_NOSIGPIPE
    setsockopt(server_sockfd, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt));
    #endif
    LOG(LEVEL_INFORM,"conneting ....\n");
    if (-1 == (flags = fcntl(server_sockfd, F_GETFL, 0)))
      flags = 0;
    fcntl(server_sockfd, F_SETFL, flags | O_NONBLOCK); 
    if( connect(server_sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0){
        if(errno != EINPROGRESS){
            close(server_sockfd);
            server_sockfd = -1;
            GOTO_ERR;
        }
    }

_err:
    if(local.ai_addr){
        free(local.ai_addr);
    }
    if(raddress){
        socks_address_free(raddress);
    }
    if(laddress){
        socks_address_free(laddress);
    }
    if(intercept){
        free(intercept);
    }
    return server_sockfd;
}



int main(int argc, char **argv)
{

    int i = 0;

    _srv_signal_init();

    g_acceptor_sockfd = _srv_acceptor_init(3129, 10);
    if (-1 == g_acceptor_sockfd){
        return -1;
    }

    g_loop = ev_loop_new(EVFLAG_AUTO);
    ev_io_init(&g_acceptor_watcher, _srv_accpet_callback, g_acceptor_sockfd, EV_READ);  
    ev_io_start(g_loop, &g_acceptor_watcher);

    memset(g_threads,0,sizeof(struct _socks_thread_t*) * g_max_thread);
    for(i = 0; i < g_max_thread; i++){
        g_threads[i] = socks_thread_alloc(socks_task_thread_handler);
        if(NULL == g_threads[i]) GOTO_ERR;
        socks_thread_init(g_threads[i]);
        socks_thread_start(g_threads[i]);
    }
    LOG(LEVEL_INFORM,"starting socks5\n");
    ev_run(g_loop, 0); 
    for( i = 0 ; i< g_max_thread; i++){
        socks_thread_stop(g_threads[i]);
    }
    return 0;
_err:
    for(i = 0; i< g_max_thread; i++){
        if(NULL != g_threads[i]){
            socks_thread_stop(g_threads[i]);
        }
    }
    return -1;
}

