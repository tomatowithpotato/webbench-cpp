#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <thread>
#include <vector>

#define SERVERPORT 1998
#define FMT_STAMP "%lld\r\n"
#define BUFSIZE 1024
#define IPSTRSIZE 40

#define NAMESIZE 11

/*
  htons htonl 将主机字节序转为网络字节序
  ntohs ntohl 将网络字节序转为主机字节序
  s代表无符号short类型 l代表无符号long类型

  inet_pton 将点分式ip地址转为数值型ip，存入第三个参数
*/

static void server_job(int sd){
    // char buf[BUFSIZE];
    // int len;
    // len = sprintf(buf, FMT_STAMP, (long long)time(NULL));
    // if(send(sd, buf, len, 0) < 0){
    //     perror("send()");
    //     exit(1);
    // }
    sleep(15);
    const char* buff = "aaaa";
    write(sd, buff, 4);
    // while(1){
    //     // 啥也不干
    // }
}

static void worker(int newsd, struct sockaddr_in remote_addr){
    char ipstr[IPSTRSIZE];

    if(newsd < 0){
        perror("accept()");
        exit(1);
    }
    
    inet_ntop(AF_INET, &remote_addr.sin_addr, ipstr, IPSTRSIZE);
    printf("---detect client %s:%d---\n", ipstr, ntohs(remote_addr.sin_port));

    server_job(newsd);

    printf("---close client %s:%d---\n", ipstr, ntohs(remote_addr.sin_port));

    // 千万别忘了关
    close(newsd); 
}

int main(){
    int sd;
    int ret;
    char ipstr[IPSTRSIZE];
    // struct msg_st rbuf, sbuf;
    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;

    sd = socket(AF_INET, SOCK_STREAM, 0);
    if(sd < 0){
        perror("socket()");
        exit(1);
    }

    int val = 1;
    ret = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if(ret < 0){
        perror("setsockopt()");
        exit(1);
    }

    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(SERVERPORT);
    inet_pton(AF_INET, "0.0.0.0", &local_addr.sin_addr);

    // 服务端 必须bind
    ret = bind(sd, (struct sockaddr *)&local_addr, sizeof(local_addr));
    if(ret < 0){
        perror("bind()");
        exit(1);
    }

    ret = listen(sd, 250);
    if(ret < 0){
        perror("listen()");
        exit(1);
    }

    socklen_t raddr_len = sizeof(remote_addr);
    std::vector<std::thread> v;

    printf("----------------start----------------\n");

    while(1){
        int newsd = accept(sd, (struct sockaddr *)&remote_addr, &raddr_len);
        v.push_back(std::thread(
            [&newsd, &remote_addr](){
                worker(newsd, remote_addr);
            }
        ));
    }

    close(sd);

    return 0;
}