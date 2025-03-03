#include "./include/proxy.h"




static int epoll_fd = -1; // the epoll fd
static int listen_fd = -1; // the socket fd socket create return
static int local_port = DEFAULT_LOCAL_PORT;
static pthread_t accep_thread_t;
// static pthread_t send_thread_t;

static int current_connected_total = 0; // the connections number
static int exit_accept_flag = 0; // is exit the accept
static int exit_flag = 0; // is exit the epoll wait

pthread_mutex_t connect_total_count_mutex = PTHREAD_MUTEX_INITIALIZER;

static void closesocket(int fd);
static void connect_total_count_opration(BOOL add_or_subract, int value);
static void signal_handler_reboot(int32_t theSignal);
static void* t1(void* arg);
static void* t2(void* arg);


static void connect_total_count_opration(BOOL add_or_subract, int value)
{
	pthread_mutex_lock(&connect_total_count_mutex);
	if (add_or_subract)
	{
		current_connected_total = current_connected_total + value;
	}
	else
	{
		current_connected_total = current_connected_total - value;
	}
	pthread_mutex_unlock(&connect_total_count_mutex);
}

/* 处理客户端的连接 */
void* handle_client(thpool_job_funcion_parameter *parameter, int thread_index)//连接套接字和客户端地址
{	
	int client_sock = parameter->fd;
	// char* buf = "I have got your information";
	// send(client_sock, buf, strlen(buf),0);
	// close(client_sock);
	// return;
	char* header_buffer = (char *) malloc(MAX_HEADER_SIZE);
	int remote_sock = -1;
    int is_http_tunnel = 0;
	char remote_host[128];
	int remote_port;
	int io_flag = FLG_NONE;
    if(strlen(remote_host) == 0) /* 未指定远端主机名称从http 请求 HOST 字段中获取 */
    {

        if(read_header(client_sock,header_buffer,io_flag) < 0)
        {
            LOG("Read Http header failed\n");
            return;
        } else 
        {	
            char * p = strstr(header_buffer,"CONNECT"); /* 判断是否是http 隧道请求 */
            if(p) 
            {
                LOG("receive CONNECT request\n");
                is_http_tunnel = 1;
            }

            //返回mproxy基本运行信息
            if(strstr(header_buffer,"GET /mproxy") >0 ) 
            {
                LOG("====== hand mproxy info request ====");
                //返回mproxy的运行基本信息
                //hand_mproxy_info_req(client_sock,header_buffer);
                return; 
            }
		
            //提取出IP地址
            if(extract_host(header_buffer,remote_host,&remote_port) < 0) 
            {
                LOG("Cannot extract host field,bad http protrotol");
                return;
            }
            LOG("Host:%s port: %d io_flag:%d\n",remote_host,remote_port,io_flag);

        }
    }

    if ((remote_sock = create_connection(remote_host,remote_port)) < 0) {
        LOG("Cannot connect to host [%s:%d],errocode:%d--\n",remote_host,remote_port,remote_sock);
        return;
    }

	// if (setnonblocking(remote_sock) < 0)
	// {
	// 	LOG( "set non-blocking socket = %d error.\n", remote_sock);
	// 	if (remote_sock != -1)
	// 	{
	// 		closesocket(remote_sock);
	// 	}
	pthread_t task1;
	pthread_t task2;
	task* arg1 = (task*)malloc(sizeof(task));
	task* arg2 = (task*)malloc(sizeof(task));
	//
	arg1->client_sock = client_sock;
	arg1->header_buffer = header_buffer;
	arg1->io_flag = io_flag;
	arg1->is_http_tunnel = is_http_tunnel;
	arg1->remote_sock = remote_sock;
	//
	arg2->client_sock = client_sock;
	arg2->header_buffer = header_buffer;
	arg2->io_flag = io_flag;
	arg2->is_http_tunnel = is_http_tunnel;
	arg2->remote_sock = remote_sock;
	//线程1
	pthread_create(&task1, NULL, t1, arg1);
	//线程2
	pthread_create(&task2, NULL, t2, arg2);
	
	pthread_join(task1,NULL);
	pthread_join(task2,NULL);
    shutdown(client_sock,SHUT_RDWR);
    shutdown(remote_sock,SHUT_RDWR);
	close(client_sock);
	close(remote_sock);
}

void* t1(void* arg){
	task* ARG = (task*)arg;
	if(strlen(ARG->header_buffer) > 0 && !ARG->is_http_tunnel) 
    {
        forward_header(ARG->remote_sock,ARG->header_buffer,ARG->io_flag); //普通的http请求先转发header
    } 
        forward_data(ARG->client_sock, ARG->remote_sock,ARG->io_flag);
		free(ARG);
        pthread_exit(NULL);
}

void* t2(void* arg){
	task* ARG = (task*)arg;
	if(ARG->io_flag == W_S_ENC)
    {
        ARG->io_flag = R_C_DEC; //发送请求给服务端进行编码，读取服务端的响应则进行解码
    } else if (ARG->io_flag == R_C_DEC)
    {
        ARG->io_flag = W_S_ENC; //接收客户端请求进行解码，那么响应客户端请求需要编码
    }

    if(ARG->is_http_tunnel)
    {
        send_tunnel_ok(ARG->client_sock);
     } 

    forward_data(ARG->remote_sock, ARG->client_sock,ARG->io_flag);

	free(ARG);
	pthread_exit(NULL);
}

/*连接目标服务器*/
int create_connection(char* remote_host, int remote_port) {
    struct sockaddr_in server_addr;
    struct hostent *server;
    int sock;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return CLIENT_SOCKET_ERROR;
    }

    if ((server = gethostbyname(remote_host)) == NULL) {
        errno = EFAULT;
        return CLIENT_RESOLVE_ERROR;
    }

    LOG("======= forward request to remote host:%s port:%d ======= \n",remote_host,remote_port);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
	if(server->h_length <= sizeof(server_addr.sin_addr.s_addr))
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	//memcpy_s(&server_addr.sin_addr.s_addr, sizeof(server_addr.sin_addr.s_addr), server->h_addr, server->h_length);
	// close(sock);
	// return;
    server_addr.sin_port = htons(remote_port);

    if (connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        return CLIENT_CONNECT_ERROR;
    }

    return sock;
}



/* 处理僵尸进程 */
void sigchld_handler(int signal) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}



void stop_server()
{
    kill(m_pid, SIGKILL);        
}

void usage(void)
{
    printf("Usage:\n");
    printf(" -l <port number>  specifyed local listen port \n");
    printf(" -h <remote server and port> specifyed next hop server name\n");
    printf(" -d <remote server and port> run as daemon\n");
    printf("-E encode data when forwarding data\n");
    printf ("-D decode data when receiving data\n");
    exit (8);
}


void* start_server(void* arg)
{   
	int connect_fd = -1;
	int optval=1;
	socklen_t clilen = 0;
	struct sockaddr_in server_addr;
	struct sockaddr_in clientaddr;
	struct sockaddr cliaddr;
	struct epoll_event ev;
	int val = 1;
	int err = 0;
	int bufsize = 32 * 1024; //1024*2048;
	int epoll_connect_event_index = -1;
    //初始化全局变量

	//分离属性
	pthread_detach(pthread_self());

    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return SERVER_SOCKET_ERROR;
    }
	
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        return SERVER_SETSOCKOPT_ERROR;
    }

	val = 2;
	if(setsockopt(listen_fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &val, sizeof(val) != 0)){
		LOG("setsockopt TCP_DEFER_ACCEPT error.\n");
	}

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(local_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        return SERVER_BIND_ERROR;
    }

    if (listen(listen_fd, LISTENQ*10) < 0) {
		LOG("压力测试出错1\n");
        return SERVER_LISTEN_ERROR;
    }

    //开始无限循环epoll
	clilen = sizeof(cliaddr);
	while (!exit_accept_flag)//这是一个无限循环
	{
		if (current_connected_total < MAX_EVENTS) // effective
		{
			if ((connect_fd = accept(listen_fd, (struct sockaddr *)&clientaddr, &clilen)) < 0)
			{
				if ((errno != EAGAIN) && (errno != ECONNABORTED) && (errno != EPROTO) && (errno != EINTR))
				{
					LOG("accept error %d,%s.\n", errno, strerror(errno));
				}
				continue;
			}
		}
		else
		{
			sleep(1);
			LOG("current accept number achieve to maximum(%d) .\n", MAX_EVENTS);
			continue;
		}
		//获得一个还未被使用的客户端结构体,顺序查找，可以优化
		epoll_connect_event_index = get_epoll_connect_free_event_index();
		// no free epoll event，客户端结构体满了
		if (epoll_connect_event_index == -1) // no the free connect event
		{
			LOG( "Not find free connect.\n");
			if (connect_fd != -1)
			{
				closesocket(connect_fd);
				connect_fd = -1;
			}
			continue;
		}//关闭刚刚才建立的连接

		if (0 != err)
		{
			if (connect_fd != -1)
			{
				closesocket(connect_fd);
				connect_fd = -1;
			}
			continue;
		}
		//修改连接套接字的信息，上锁修改
		init_epoll_connect_by_index(epoll_connect_event_index, connect_fd, inet_ntoa(clientaddr.sin_addr));
		// 增加一个epoll事件
		ev.data.fd = connect_fd;
		ev.events = EPOLLIN; // set epoll event type
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connect_fd, &ev) == -1)
		{
			LOG("EPOLL_CTL_ADD %d,%s.\n", errno, strerror(errno));
			if (connect_fd != -1)
			{
				closesocket(connect_fd);
				connect_fd = -1;
			}
			continue;
		}
		//已有连接数+1
		connect_total_count_opration(TRUE, 1);

	}
	if (-1 != listen_fd) // out the while then close the socket
	{
		closesocket(listen_fd);
		listen_fd = -1;
	}
	
	return NULL;
}

// 设置文件描述符为非阻塞模式
int setnonblocking(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    if (flag == -1) {
        LOG("fcntl 1 error");
        return -1;
    }
    flag |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flag) == -1) {
        LOG("fcntl 2 error");
        return -1;
    }
	return 0;
}

//关闭套接字
static void closesocket(int fd)
{
	shutdown(fd, SHUT_RDWR);
	close(fd);
}

int create_accept_task(void){
    return pthread_create(&accep_thread_t, NULL, start_server, NULL);
}

static void signal_handler_reboot(int32_t theSignal)
{
	int i;
	int sockfd;
	signal(SIGPIPE, SIG_IGN);
	if (SIGKILL == theSignal || SIGTERM == theSignal || SIGINT == theSignal) //we can know when system excute child thread is end
	{
		LOG("receive kill signal exit the server.");
		if (listen_fd != -1)
		{
			closesocket(listen_fd);
			listen_fd = -1;
		}
		exit_flag = 1;
		exit_accept_flag = 1;
		for (i = 0; i < MAX_EVENTS; i++)
		{
			sockfd = get_fd_by_event_index(i);
			if (sockfd != -1)
			{
				closesocket(sockfd);
			}
		}
	}
	else if (SIGPIPE == theSignal)
	{
		LOG("SIGPIPE received.\n");
	}
}


//本程序只有启动时不指定IP而是发送HTTP请求报文包括connect才能建立隧道，直接开头指定IP
int main(int argc, char *argv[]) 
{	

	struct rlimit rl;
	thpool_t *thpool = NULL;
	time_t now;
	time_t prevTime, eventTime = 0;
	struct epoll_event ev, events[MAX_EVENTS];
	int epoll_events_number = 0;
	int index = 0;
	int connect_socket_fd_temp = -1;
	int delete_pool_job_number = 0;

    int daemon = 0; 


	//
	pid_t pidfd = fork();
	if (pidfd < 0)
	{
		exit(1);
	}
	if (pidfd != 0)
	{
		exit(0);
	}
	setsid();
    rl.rlim_cur = MAX_EVENTS * 5;
	rl.rlim_max = MAX_EVENTS * 5;
	setrlimit(RLIMIT_NOFILE, &rl);
	getrlimit(RLIMIT_NOFILE, &rl);

	//注册程序终止处理函数
	signal(SIGKILL, signal_handler_reboot); // register the signal
	signal(SIGTERM, signal_handler_reboot);
	signal(SIGPIPE, signal_handler_reboot);
	signal(SIGINT, signal_handler_reboot);
	//epoll 初始化
	init_epoll_connect();
	epoll_fd = epoll_create(MAX_FDS); // 10240
	if (0 >= epoll_fd)
	{
		LOG("poll_create error.\n");
		exit(1);
	}

	//初始化线程池
	thpool = thpool_init(1000);
	if (NULL == thpool)
	{
		LOG("create thread pool error.\n");
		exit(1);
	}

	//启动第二线程，一个无限循环epoll
    create_accept_task();

	//主线程
	while (!exit_flag)
	{	LOG("当前总连接数为:%d\n",current_connected_total);
		time(&now);
		// abs(now - eventTime) >= SERVER_TIMEOUT
		if (abs(now - eventTime) >= SERVER_TIMEOUT) //SERVER_TIMEOUT second detect one time delete the time out event
		{
			eventTime = now;
			for (index = 0; index < MAX_EVENTS; index++)
			{
				connect_socket_fd_temp = get_fd_by_event_index(index);
				if (connect_socket_fd_temp != -1)
				{
					if ((now - get_event_connect_time_by_index(index)) > SERVER_TIMEOUT)
					{
						LOG("Epoll event[%d] timeout closed and fd= %d.\n", index, connect_socket_fd_temp);
						free_event_by_index(index);
						if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, connect_socket_fd_temp, &ev) == -1)
						{
							LOG("EPOLL_CTL_DEL %d,%s.\n", errno, strerror(errno));
							
						}
						connect_total_count_opration(FALSE, 1);
						closesocket(connect_socket_fd_temp);
						connect_socket_fd_temp = -1;
					}
				}
			}
			// delete the pool job time out job
			delete_pool_job_number = delete_timeout_job(thpool, SERVER_TIMEOUT);
			connect_total_count_opration(FALSE, delete_pool_job_number);
			LOG("pool queque delete job number = %d.\n", delete_pool_job_number);
		}

		//等待两秒钟
		epoll_events_number = epoll_wait(epoll_fd, events, MAX_EVENTS, 2000); //2seconds
		for (index = 0; index < epoll_events_number; ++index) // deal with the event
		{
			connect_socket_fd_temp = events[index].data.fd; // get the socket fd
			// delete epoll event
			if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[index].data.fd, &ev) == -1)
			{
				LOG("EPOLL_CTL_DEL %d,%s.\n", errno, strerror(errno));
				events[index].data.fd = -1;
			}
			if (events[index].events & EPOLLIN) //have read event
			{
				int event_index = -1;
				int recv_length = 0;

				if (connect_socket_fd_temp < 0)
				{
					connect_total_count_opration(FALSE, 1);
					LOG("Event[%d] read invalid handle.\n", index);
					continue;
				}
				event_index = get_matched_event_index_by_fd(connect_socket_fd_temp);
				LOG("Epoll get Event[%d] fd = %d.\n", event_index, connect_socket_fd_temp);
				// no the event
				if (event_index < 0)
				{
					connect_total_count_opration(FALSE, 1);
					LOG("not find matched fd = %d.\n", connect_socket_fd_temp);
					free_event_by_index(event_index);
					if (connect_socket_fd_temp != -1)
					{
						closesocket(connect_socket_fd_temp);
						connect_socket_fd_temp = -1;
					}
					continue;
				}

					thpool_add_work(thpool, (void*)handle_client, connect_socket_fd_temp, NULL);
				
			}
		}
	}
	if (listen_fd != -1)
	{
		closesocket(listen_fd);
		listen_fd = -1;
	}

	exit_accept_flag = 1;
	thpool_destroy(thpool);
	printf("[%s %s %d]Done...\n", __FILE__, __FUNCTION__, __LINE__);
	return 1;

}
