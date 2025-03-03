#include "./include/proxy.h"

int extract_host(const char * header,char* remote_host,int* remote_port)
{
    
    char * _p = strstr(header,"CONNECT");  /* 在 CONNECT 方法中解析 隧道主机名称及端口号 */
    if(_p)
    {
        char * _p1 = strchr(_p,' ');

        char * _p2 = strchr(_p1 + 1,':');
        char * _p3 = strchr(_p1 + 1,' ');

        if(_p2)
        {
            char s_port[10];
            bzero(s_port,10);

            strncpy(remote_host,_p1+1,(int)(_p2  - _p1) - 1);
            //就这一个小问题，花了我两天
            remote_host[(int)(_p2  - _p1) - 1] = '\0';
            //////////////
            strncpy(s_port,_p2+1,(int) (_p3 - _p2) -1);
            *remote_port = atoi(s_port);

        } else 
        {
            strncpy(remote_host,_p1+1,(int)(_p3  - _p1) -1);
            *remote_port = 80;
        }
        
        
        return 0;
    }


    char * p = strstr(header,"Host:");
    if(!p) 
    {
        return BAD_HTTP_PROTOCOL;
    }
    char * p1 = strchr(p,'\n');
    if(!p1) 
    {
        return BAD_HTTP_PROTOCOL; 
    }

    char * p2 = strchr(p + 5,':'); /* 5是指'Host:'的长度 */

    if(p2 && p2 < p1) 
    {
        
        int p_len = (int)(p1 - p2 -1);
        char s_port[p_len];
        strncpy(s_port,p2+1,p_len);
        s_port[p_len] = '\0';
        *remote_port = atoi(s_port);

        int h_len = (int)(p2 - p -5 -1 );
        strncpy(remote_host,p + 5 + 1  ,h_len); //Host:
        //assert h_len < 128;
        remote_host[h_len] = '\0';
    } else 
    {   
        int h_len = (int)(p1 - p - 5 -1 -1); 
        strncpy(remote_host,p + 5 + 1,h_len);
        //assert h_len < 128;
        remote_host[h_len] = '\0';
        *remote_port = 80;
    }
    return 0;
}

void extract_server_path(const char * header,char * output)
{
    char * p = strstr(header,"GET /");
    if(p) {
        char * p1 = strchr(p+4,' ');
        strncpy(output,p+4,(int)(p1  - p - 4) );
    }
    
}