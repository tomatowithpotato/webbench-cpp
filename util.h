#ifndef __WEBBENCHCPP_UTIL_H__
#define __WEBBENCHCPP_UTIL_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/param.h>
#include <getopt.h>

#define PROGRAM_VERSION "1.5"
#define REQUEST_SIZE 2048

enum HTTP_VERSION{
    HTTP_0_9,
    HTTP_1_0,
    HTTP_1_1,
    HTTP_2_0,
};

enum HTTP_METHOD{
    GET,
    // POST,
    HEAD,
};

struct Config{
    int http10=1; /* 0 - http/0.9, 1 - http/1.0, 2 - http/1.1 */
    /* Allow: GET, HEAD, OPTIONS, TRACE */
    int http_version = HTTP_VERSION::HTTP_1_1;
    int http_method = HTTP_METHOD::GET;
    int clients = 1;
    int force = 0;
    int force_reload = 0;
    int proxyport = 80;
    char *proxyhost = NULL;
    int benchtime = 30;
    /* internal */
    char host[MAXHOSTNAMELEN];
    char request[REQUEST_SIZE];

    struct option long_options[13] = 
    {
        {"force",no_argument,&force,1},
        {"reload",no_argument,&force_reload,1},
        {"time",required_argument,NULL,'t'},
        {"help",no_argument,NULL,'?'},
        {"http09",no_argument,NULL,'9'},
        {"http10",no_argument,NULL,'1'},
        {"http11",no_argument,NULL,'2'},
        {"get",no_argument,&http_method, HTTP_METHOD::GET},
        {"head",no_argument,&http_method, HTTP_METHOD::HEAD},
        {"version",no_argument,NULL,'V'},
        {"proxy",required_argument,NULL,'p'},
        {"clients",required_argument,NULL,'c'},
        {NULL,0,NULL,0}
    };
};

static void usage(void)
{
   fprintf(stderr,
	"webbench [option]... URL\n"
	"  -f|--force               Don't wait for reply from server.\n"
	"  -r|--reload              Send reload request - Pragma: no-cache.\n"
	"  -t|--time <sec>          Run benchmark for <sec> seconds. Default 30.\n"
	"  -p|--proxy <server:port> Use proxy server for request.\n"
	"  -c|--clients <n>         Run <n> HTTP clients at once. Default one.\n"
	"  -9|--http09              Use HTTP/0.9 style requests.\n"
	"  -1|--http10              Use HTTP/1.0 protocol.\n"
	"  -2|--http11              Use HTTP/1.1 protocol.\n"
	"  --get                    Use GET request method.\n"
	"  --head                   Use HEAD request method.\n"
	"  --options                Use OPTIONS request method.\n"
	"  --trace                  Use TRACE request method.\n"
	"  -?|-h|--help             This information.\n"
	"  -V|--version             Display program version.\n"
	);
};

int Socket(const char *host, int clientPort);

void build_request(Config &config, const char *url);

#endif