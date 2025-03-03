#include "./include/proxy.h"

int send_data(int socket,char * buffer,int len,int io_flag)
{

    if(io_flag == W_S_ENC)
    {
        int i;
        for(i = 0; i < len ; i++)
        {
            buffer[i] ^= 1;
           
        }
    }

    return send(socket,buffer,len,0);
}

/**/
int receive_data(int socket, char * buffer, int len,int io_flag)
{
    int n = recv(socket, buffer, len, 0);
    //表示是否加密，每个字符与1做异或
    if(io_flag == R_C_DEC && n > 0)
    {
        int i; 
        for(i = 0; i< n; i++ )
        {
            buffer[i] ^= 1;
            // printf("%d => %d\n",c,buffer[i]);
        }
    }

    return n;
}

void forward_data(int source_sock, int destination_sock,int io_flag) {
    char buffer[BUF_SIZE];
    int n;
    while ((n = receive_data(source_sock, buffer, BUF_SIZE,io_flag)) > 0) 
    { 
        send_data(destination_sock, buffer, n,io_flag);//可以改为sendmsg
    }
 

    // shutdown(source_sock,SHUT_RDWR);
    // shutdown(destination_sock,SHUT_RDWR);
}

/* 响应隧道连接请求  */
int send_tunnel_ok(int client_sock,int io_flag)
{
    char * resp = "HTTP/1.1 200 Connection Established\r\n\r\n";
    int len = strlen(resp);
    char buffer[len+1];
    strcpy(buffer,resp);
    if(send_data(client_sock,buffer,len,io_flag) < 0)
    {
        perror("Send http tunnel response  failed\n");
        return -1;
    }
    return 0;
}

