#include <unistd.h>
#include <sys/param.h>
#include <rpc/types.h>
#include <getopt.h>
#include <strings.h>
#include <time.h>
#include <signal.h>

#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

#include "./util.h"

/* values */
volatile std::atomic_bool timerexpired(false);
volatile std::atomic_int speed(0);
volatile std::atomic_int failed(0);
volatile std::atomic_int bytes(0);

static void alarm_handler(int signal)
{
	timerexpired = true;
}

void benchcore(const Config &config)
{
    int rlen;
    char buf[1500];
    int s,i;

    rlen = strlen(config.request);
    while(1)
    {
        if(timerexpired)
        {
            if(failed > 0)
            {
                /* fprintf(stderr,"Correcting failed by signal\n"); */
                failed--;
            }
            return;
        }
        s = Socket(config.host, config.proxyport);                          
        if(s < 0) { 
            failed++;
            continue;
        } 
        if(rlen != write(s, config.request, rlen)) {
            failed++;
            close(s);
            continue;
        }
        if(config.http_version == HTTP_VERSION::HTTP_0_9){ 
            if(shutdown(s,1)) { 
                failed++;
                close(s);
                continue;
            }
        }

        bool nexttry = false;
        if(config.force == 0) 
        {
            /* read all available data from socket */
            while(1)
            {
                if(timerexpired) break; 
                i = read(s,buf,1500);
                    /* fprintf(stderr,"%d\n",i); */
                if(i<0) 
                { 
                    failed++;
                    close(s);
                    nexttry = true;
                    break;
                }
                else if(i==0){ 
                    break;
                }
                else{
                    bytes+=i;
                }
            }
        }
        if(nexttry){
            continue;
        }

        if(close(s)) {
            failed++;
            continue;
        }
        speed++;
    }
}


static int bench(Config config)
{
    int i,j,k;	
    pid_t pid=0;
    FILE *f;

    /* check avaibility of target server */
    i = Socket(config.proxyhost==NULL ? config.host : config.proxyhost, config.proxyport);
    if(i < 0) { 
        fprintf(stderr,"\nConnect to server failed. Aborting benchmark.\n");
        return 1;
    }
    close(i);
    /* not needed, since we have alarm() in childrens */
    /* wait 4 next system clock tick */
    /*
    cas=time(NULL);
    while(time(NULL)==cas)
            sched_yield();
    */

    struct sigaction sa;
	/* setup alarm signal handler */
	sa.sa_handler = alarm_handler;
	sa.sa_flags = 0;
	if (sigaction(SIGALRM, &sa, NULL))
		exit(3);
	alarm(config.benchtime);

    std::vector<std::thread> threads;
    for(int i = 0; i < config.clients; i++)
    {
        threads.push_back(std::thread(
            [&config](){
                benchcore(config);
            }
        ));
    }

    for(int i = 0; i < config.clients; ++i){
        if(threads[i].joinable()){
            threads[i].join();
        }
    }

    printf("\nSpeed=%d pages/min, %d bytes/sec.\nRequests: %d susceed, %d failed.\n",
            (int)((speed+failed)/(config.benchtime/60.0f)),
            (int)(bytes/(float)config.benchtime),
            speed.load(),
            failed.load());
    return 0;
}


int main(int argc, char *argv[])
{
    Config config;

    int opt=0;
    int options_index=0;
    char *tmp=NULL;

    if(argc==1)
    {
        usage();
        return 2;
    } 

    while((opt=getopt_long(argc,argv,"912Vfrt:p:c:?h", config.long_options, &options_index))!=EOF )
    {
    switch(opt)
    {
        case  0 : break;
        case 'f': config.force = 1; break;
        case 'r': config.force_reload = 1; break; 
        case '9': config.http_version = HTTP_VERSION::HTTP_0_9; break;
        case '1': config.http_version = HTTP_VERSION::HTTP_1_0; break;
        case '2': config.http_version = HTTP_VERSION::HTTP_1_1; break;
        case '3': config.http_version = HTTP_VERSION::HTTP_2_0; break;
        case 'V': printf(PROGRAM_VERSION"\n");exit(0);
        case 't': config.benchtime=atoi(optarg);break;	     
        case 'p': 
            /* proxy server parsing server:port */
            tmp=strrchr(optarg,':');
            config.proxyhost=optarg;
            if(tmp==NULL)
            {
                break;
            }
            if(tmp==optarg)
            {
                fprintf(stderr,"Error in option --proxy %s: Missing hostname.\n",optarg);
                return 2;
            }
            if(tmp==optarg+strlen(optarg)-1)
            {
                fprintf(stderr,"Error in option --proxy %s Port number is missing.\n",optarg);
                return 2;
            }
            *tmp='\0';
            config.proxyport=atoi(tmp+1);break;
        case ':':
        case 'h':
        case '?': usage();return 2;break;
        case 'c': config.clients = atoi(optarg);break;
    }
    }
    
    if(optind==argc) {
        fprintf(stderr,"webbench: Missing URL!\n");
        usage();
        return 2;
    }

    if(config.clients==0) config.clients=1;
    if(config.benchtime==0) config.benchtime=60;
    /* Copyright */
    fprintf(stderr,"Webbench - Simple Web Benchmark "PROGRAM_VERSION"\n"
        "Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.\n"
        );
    build_request(config, argv[optind]);
    /* print bench info */
    printf("\nBenchmarking: ");
    switch(config.http_method)
    {
        case HTTP_METHOD::GET:
        default:
            printf("GET");break;
        case HTTP_METHOD::HEAD:
            printf("HEAD");break;
    }
    printf(" %s",argv[optind]);
    switch(config.http_version)
    {
        case 0: printf(" (using HTTP/0.9)");break;
        case 2: printf(" (using HTTP/1.1)");break;
    }
    printf("\n");
    if(config.clients==1) printf("1 client");
    else
        printf("%d clients",config.clients);

    printf(", running %d sec", config.benchtime);
    if(config.force) printf(", early socket close");
    if(config.proxyhost!=NULL) printf(", via proxy server %s:%d",config.proxyhost,config.proxyport);
    if(config.force_reload) printf(", forcing reload");
    printf(".\n");

    bench(config);
    return 0;
}