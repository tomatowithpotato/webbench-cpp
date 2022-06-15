#include "util.h"

int Socket(const char *host, int clientPort)
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
    ad.sin_port = htons(clientPort);

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

void build_request(Config &config, const char *url)
{
    char tmp[10];
    int i;

    bzero(config.host, MAXHOSTNAMELEN);
    bzero(config.request, REQUEST_SIZE);

    switch(config.http_method)
    {
        default:
        case HTTP_METHOD::GET: strcpy(config.request,"GET");break;
        case HTTP_METHOD::HEAD: strcpy(config.request,"HEAD");break;
    }
            
    strcat(config.request," ");

    if(NULL==strstr(url,"://"))
    {
        fprintf(stderr, "\n%s: is not a valid URL.\n",url);
        exit(2);
    }
    if(strlen(url)>1500)
    {
        fprintf(stderr,"URL is too long.\n");
        exit(2);
    }
    if(config.proxyhost==NULL){
        if (0!=strncasecmp("http://",url,7)) 
        { 
            fprintf(stderr,"\nOnly HTTP protocol is directly supported, set --proxy for others.\n");
            exit(2);
        }
    }
    /* protocol/host delimiter */
    i=strstr(url,"://")-url+3;
    /* printf("%d\n",i); */

    if(strchr(url+i,'/')==NULL) {
        fprintf(stderr,"\nInvalid URL syntax - hostname don't ends with '/'.\n");
        exit(2);
    }
    if(config.proxyhost==NULL)
    {
        /* get port from hostname */
        if(index(url+i,':')!=NULL &&
            index(url+i,':')<index(url+i,'/'))
        {
            strncpy(config.host,url+i,strchr(url+i,':')-url-i);
            bzero(tmp,10);
            strncpy(tmp,index(url+i,':')+1,strchr(url+i,'/')-index(url+i,':')-1);
            /* printf("tmp=%s\n",tmp); */
            config.proxyport=atoi(tmp);
            if(config.proxyport==0) config.proxyport=80;
        } else
        {
            strncpy(config.host,url+i,strcspn(url+i,"/"));
        }
        // printf("Host=%s\n",host);
        strcat(config.request+strlen(config.request),url+i+strcspn(url+i,"/"));
    } 
    else
    {
        // printf("ProxyHost=%s\nProxyPort=%d\n",proxyhost,proxyport);
        strcat(config.request,url);
    }

    if(config.http_version == HTTP_1_0)
        strcat(config.request," HTTP/1.0");
    else if (config.http_version == HTTP_1_1)
        strcat(config.request," HTTP/1.1");
    else{
        strcat(config.request," HTTP/2.0");
    }
    strcat(config.request,"\r\n");

    if(config.http_version > HTTP_VERSION::HTTP_0_9){
        strcat(config.request,"User-Agent: WebBench "PROGRAM_VERSION"\r\n");
    }
    
    if(config.proxyhost==NULL && config.http_version > HTTP_VERSION::HTTP_0_9)
    {
        strcat(config.request, "Host: ");
        strcat(config.request, config.host);
        strcat(config.request, "\r\n");
    }
    if(config.force_reload && config.proxyhost!=NULL)
    {
        strcat(config.request,"Pragma: no-cache\r\n");
    }
    if(config.http_version > HTTP_VERSION::HTTP_1_0){
        strcat(config.request,"Connection: close\r\n");
    }
    /* add empty line at end */
    if(config.http_version > HTTP_VERSION::HTTP_0_9){ 
        strcat(config.request,"\r\n"); 
    }
    // printf("Req=%s\n",request);
}