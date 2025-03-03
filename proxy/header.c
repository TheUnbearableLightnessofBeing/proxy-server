#include "./include/proxy.h"

int read_header(int fd, char* header_buffer,int io_flag)
{
    // bzero(header_buffer,sizeof(MAX_HEADER_SIZE));
    memset(header_buffer,0,MAX_HEADER_SIZE);
    char line_buffer[2048];
    char * base_ptr = header_buffer;

    for(;;)
    {
        memset(line_buffer,0,2048);

        int total_read = readLine(fd,line_buffer,2048,io_flag); /* 读取一行 */
        if(total_read <= 0)
        {
            return CLIENT_SOCKET_ERROR;
        }

        //防止header缓冲区蛮越界，把line buffer的数据赋给header_buffer
        if(base_ptr + total_read - header_buffer <= MAX_HEADER_SIZE)
        {
           strncpy(base_ptr,line_buffer,total_read); 
           base_ptr += total_read;
        } else 
        {
            return HEADER_BUFFER_FULL;
        }

        //读到了空行，http头结束
        if(strcmp(line_buffer,"\r\n") == 0 || strcmp(line_buffer,"\n") == 0)
        {
            break;
        }

    }
    return 0;

}

void forward_header(int destination_sock, char* header_buffer,int io_flag)
{
    rewrite_header(header_buffer);

    int len = strlen(header_buffer);
    send_data(destination_sock,header_buffer,len,io_flag);
}

/* 代理中的完整URL转发前需改成 path 的形式 */
void rewrite_header(char* header_buffer)
{
    char * p = strstr(header_buffer,"http://");
    char * p0 = strchr(p,'\0');
    char * p5 = strstr(header_buffer,"HTTP/"); /* "HTTP/" 是协议标识 如 "HTTP/1.1" */
    int len = strlen(header_buffer);
    if(p)
    {
        char * p1 = strchr(p + 7,'/');
        if(p1 && (p5 > p1)) 
        {
            //转换url到 path
            memcpy(p,p1,(int)(p0 -p1));
            int l = len - (p1 - p) ;
            header_buffer[l] = '\0';


        } else 
        {
            char * p2 = strchr(p,' ');  //GET http://3g.sina.com.cn HTTP/1.1

            // printf("%s\n",p2);
            memcpy(p + 1,p2,(int)(p0-p2));
            *p = '/';  //url 没有路径使用根
            int l  = len - (p2  - p ) + 1;
            header_buffer[l] = '\0';

        }
    }
}