# proxy-server
一个基于C语言实现的高并发正向代理服务器
基于 Epoll、线程池、连接池、tsung，实现一个Reactor模式高并发正向代理服务器，支持 HTTP/HTTPS协议。
1.主线程负责监听和处理连接事件，第二线程循环接收 TCP 连接并注册 Epoll 事件；
2.连接池存储已建立连接，线程池根据任务队列动态调整线程数量，定时检测、清理超时连接和任务。
3.支持HTTP、HTTPS协议，通过双线程模型实现正向代理，支持Keep-Alive功能；
4.使用tsung进行压力测试，在8核笔记本电脑上实现1500的并发连接数。
