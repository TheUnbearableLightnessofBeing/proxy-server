#include <arpa/inet.h>
#include <errno.h>
#include <libgen.h>
#include <netdb.h>
#include <resolv.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/wait.h>
#include <netinet/in.h> 
#include <string.h>
#include <fcntl.h>
#include <sys/epoll.h>
// #include <pthread.h>
// #include <semaphore.h>

#include "./thread_pool.h"
#include "./epoll_connect.h"


#define BUF_SIZE 8192

#define READ  0
#define WRITE 1

#define DEFAULT_LOCAL_PORT    8080  
#define DEFAULT_REMOTE_PORT   8081 
#define SERVER_SOCKET_ERROR -1
#define SERVER_SETSOCKOPT_ERROR -2
#define SERVER_BIND_ERROR -3
#define SERVER_LISTEN_ERROR -4
#define CLIENT_SOCKET_ERROR -5
#define CLIENT_RESOLVE_ERROR -6
#define CLIENT_CONNECT_ERROR -7
#define CREATE_PIPE_ERROR -8
#define BROKEN_PIPE_ERROR -9
#define HEADER_BUFFER_FULL -10
#define BAD_HTTP_PROTOCOL -11

#define MAX_HEADER_SIZE 8192
#define	CONNECT_TO_SQL_SUCCESS 60
#define SERVER_TIMEOUT 10

#define MAX(a, b)					((a)>(b)?(a):(b))

#if defined(OS_ANDROID)
#include <android/log.h>
 
#define LOG(fmt...) __android_log_print(ANDROID_LOG_DEBUG,__FILE__,##fmt)
 
#else
#define LOG(fmt...)  do { fprintf(stderr,"%s %s ",__DATE__,__TIME__); fprintf(stderr, ##fmt); } while(0)
#endif

typedef struct _task{
	int client_sock;
	int remote_sock;
    char* header_buffer;
    int is_http_tunnel;
    int io_flag;
}task;


pid_t m_pid;

enum 
{
    FLG_NONE = 0,       /* 正常数据流不进行编解码 */
    R_C_DEC = 1,        /* 读取客户端数据仅进行解码 */
    W_S_ENC = 2         /* 发送到服务端进行编码 */
};


ssize_t readLine(int fd, void *buffer, size_t n,int io_flag);
void stop_server();
void* handle_client(thpool_job_funcion_parameter *parameter, int thread_index);
void forward_header(int destination_sock, char* header_buffer,int io_flag);
void forward_data(int source_sock, int destination_sock,int io_flag);
void rewrite_header(char* header_buffer);
int send_data(int socket,char * buffer,int len,int io_flag);
int receive_data(int socket, char * buffer, int len,int io_flag);
void hand_mproxy_info_req(int sock,char * header_buffer) ;
void get_info(char * output);
const char * get_work_mode() ;
int create_connection(char* remote_host, int remote_port) ;
int setnonblocking(int fd);

int create_accept_task(void);