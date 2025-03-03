/*
 * epoll_connect.c
 *
 *  Created on: 201610
 *      Author: leo
 */

//#include "log.h"
#include "epoll_connect.h"

//static char log_str_buf[LOG_STR_BUF_LEN];
static EPOLL_CONNECT epoll_connect_client[MAX_EVENTS];

//修改epoll客户端结构体的时候需要上锁
static void lock_event_state(int iEvent, int iLock)
{
	int iRet;

	if (iLock)
	{
		iRet = pthread_mutex_lock(&epoll_connect_client[iEvent].mutex);
	}
	else
	{
		iRet = pthread_mutex_unlock(&epoll_connect_client[iEvent].mutex);
	}
	if (iRet != 0)
	{
		// snprintf(log_str_buf, LOG_STR_BUF_LEN, "Event[%d] mutex Lock[%d]\n", iEvent, iLock);
		// log_string(LOG_LEVEL_ERROR, log_str_buf);
	}
}

//给每个客户端都初始化
void init_epoll_connect(void)
{
	int iIndex;
	int iRet;

	memset((char*)epoll_connect_client, 0, sizeof(epoll_connect_client));
	for (iIndex = 0; iIndex < MAX_EVENTS; iIndex++)
	{
		epoll_connect_client[iIndex].connect_fd = -1;
		epoll_connect_client[iIndex].socket_status = 0;
		iRet = pthread_mutex_init(&epoll_connect_client[iIndex].mutex, NULL);
		if (iRet != 0)
		{
			// snprintf(log_str_buf, LOG_STR_BUF_LEN, "file connection.c Event[%d] mutex init\n", iRet);
			// log_string(LOG_LEVEL_INFO, log_str_buf);
		}
	}
}

//选取没有被使用的可用epoll客户端 fd
int get_epoll_connect_free_event_index(void)
{
	int iIndex;

	for (iIndex = 0; iIndex < MAX_EVENTS; iIndex++)
	{
		if (epoll_connect_client[iIndex].connect_fd == -1)
		{
			return iIndex;
		}
	}
	return (-1);
}

//修改客户端结构体
void init_epoll_connect_by_index(int iEvent, int iConnectFD, char *uiClientIP)
{
	time_t now;

	time(&now);
	lock_event_state(iEvent, 1);
	epoll_connect_client[iEvent].now = now;
	memset(epoll_connect_client[iEvent].client_ip_addr, 0, IP_ADDR_LENGTH);
	memcpy(epoll_connect_client[iEvent].client_ip_addr, uiClientIP, IP_ADDR_LENGTH);
	epoll_connect_client[iEvent].connect_fd = iConnectFD;
	epoll_connect_client[iEvent].socket_status = 1;
	lock_event_state(iEvent, 0);
}

//查询匹配FD的客户端结构体
int get_matched_event_index_by_fd(int iConnectFD)
{
	int iIndex;

	for (iIndex = 0; iIndex < MAX_EVENTS; iIndex++)
	{
		if (epoll_connect_client[iIndex].connect_fd == iConnectFD)
		{
			return iIndex;
		}
	}
	return (-1);
}

//关闭连接后，重新初始化一个客户端结构体
void free_event_by_index(int index)
{
	if (index >=0 && index < MAX_EVENTS)
	{
		lock_event_state(index, 1);
		epoll_connect_client[index].connect_fd = -1;
		epoll_connect_client[index].socket_status = 0;
		lock_event_state(index, 0);
	}
}

//通过index查询fd
int get_fd_by_event_index(int index)
{
	if (index >=0 && index < MAX_EVENTS)
	{
		return epoll_connect_client[index].connect_fd;
	}
	else
	{
		return -1;
	}
}

//查询客户端结构体的连接建立时间
time_t get_event_connect_time_by_index(int index)
{
	if (index >=0 && index < MAX_EVENTS)
	{
		return epoll_connect_client[index].now;
	}
	else
	{
		time_t now;
		time(&now);
		return now;
	}
}

//得到客户端结构体的地址
char *get_client_addr_by_index(int index)
{
	if (index >=0 && index < MAX_EVENTS)
	{
		return epoll_connect_client[index].client_ip_addr;
	}
	else
	{
		return "0.0.0.0";
	}
}

