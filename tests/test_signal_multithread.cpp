#include <unistd.h>
#include <sys/param.h>
#include <rpc/types.h>
#include <getopt.h>
#include <strings.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/epoll.h>

#include <pthread.h>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

#define MAX_EVENTS 10

volatile std::atomic_bool timerexpired(false);

std::vector<pthread_t> thread_arr;

int mypipe[2];

static void alarm_handler(int signal)
{
	timerexpired = true;
    const char* buff = "time out";
    write(mypipe[1], buff, sizeof(buff));
    printf("timeout\n");
}

int Socket(const char *host, int port)
{
    int sock;
    unsigned long inaddr;
    struct sockaddr_in ad;
    struct hostent *hp;

    memset(&ad, 0, sizeof(ad));
    ad.sin_family = AF_INET;

    inaddr = inet_addr(host);
    if (inaddr != INADDR_NONE)
        memcpy(&ad.sin_addr, &inaddr, sizeof(inaddr));
    else
    {
        hp = gethostbyname(host);
        if (hp == NULL)
            return -1;
        memcpy(&ad.sin_addr, hp->h_addr, hp->h_length);
    }
    ad.sin_port = htons(port);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return sock;
    if (connect(sock, (struct sockaddr *)&ad, sizeof(ad)) < 0)
    {
        close(sock);
        return -1;
    }
    return sock;
}


void* func(void* data){
    int *id_ptr = (int*)data;
    int id = *id_ptr;
    std::cout << "begin_" << id << std::endl;
    int sock = Socket("0.0.0.0", 1998);
    char buff[1024];
    
    struct epoll_event ev, events[MAX_EVENTS];
    int epfd;

    /* Code to set up listening socket, 'listen_sock',
        (socket(), bind(), listen()) omitted */

    epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = sock;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev) == -1) {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = mypipe[0];
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, mypipe[0], &ev) == -1) {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);

    if (nfds == -1) {
        perror("epoll_wait");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i < nfds; ++i){
        int fd = events[i].data.fd;
        if(fd == mypipe[0]){
            std::cout << "time out in child_" << id << std::endl;
            break;
        }
        else if(fd == sock){
            int ret = read(sock, buff, sizeof(buff));
            if(ret == -1){
                std::cout << "read fail" << std::endl;
            }
        }
    }

    std::cout << "end_" << id << std::endl;
    return nullptr;
}

int main(){
    int num = 5;
    int timeout = 3;

    if(pipe(mypipe) < 0){
        perror("pipe");
        exit(1);
    }

    struct sigaction sa;
	/* setup alarm signal handler */
	sa.sa_handler = alarm_handler;
	sa.sa_flags = 0;
	if (sigaction(SIGALRM, &sa, NULL))
		exit(3);
	alarm(timeout);

    int fuck[num] = {0};
    for(int i = 0; i < num; ++i){
        pthread_t pid;
        fuck[i] = i;
        pthread_create(&pid, nullptr, func, (void*)(&fuck[i]));
        thread_arr.push_back(pid);
    }

    for(int i = 0; i < num; ++i){
        pthread_join(thread_arr[i], nullptr);
    }

    close(mypipe[0]);
    close(mypipe[1]);
    return 0;
}