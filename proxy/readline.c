#include "./include/proxy.h"

ssize_t readLine(int fd, void *buffer, size_t n,int io_flag)
{
    ssize_t numRead;                    
    size_t totRead;                     
    char *buf;
    char ch;

    if (n <= 0 || buffer == NULL) {
        errno = EINVAL;
        return -1;
    }

    buf = buffer;                       

    totRead = 0;
    for (;;) {
        numRead = receive_data(fd, &ch, 1,io_flag); /* 读取一个字符，可能加密 */

        if (numRead == -1) {
            if (errno == EINTR)         /* 执行系统调用而中断 */     
                continue;
            else
                return -1;              /* 未知错误 */

        } else if (numRead == 0) {      /* EOF */
            if (totRead == 0)           /* No bytes read; return 0 */
                return 0;
            else                        /* Some bytes read; add '\0' */
                break;

        } else {     
                                      
            if (totRead < n - 1) {      /* Discard > (n - 1) bytes */
                totRead++;
                *buf++ = ch;            /* 先赋值，再移位 */
            }

            if (ch == '\n')
                break;
        }
    }

    *buf = '\0';
    return totRead;
}