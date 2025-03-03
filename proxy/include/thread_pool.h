/*
 * thread_pool.h
 *
 *  Created on: 2015��2��4��
 *      Author: vae
 */

#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include "./thread_pool_global.h"

#define	  	THREAD_POLL_SIZE	 	15// the thread number of thread poll
// #define		BUFFER_SIZE				1024

typedef void* (*FUNC)(void* arg, int index);


typedef struct _thpool_job_funcion_parameter{
	char recv_buffer[1024];
	int  fd;
}thpool_job_funcion_parameter;

/**
 * define a task note
 */
//一个任务队列中的单个任务
typedef struct _thpool_job_t{
    FUNC             		function;		// function point
//    void*                 	arg;     		// function parameter
    thpool_job_funcion_parameter arg;
    time_t job_add_time;
    struct _thpool_job_t* 	prev;     		// point previous note
    struct _thpool_job_t* 	next;     		// point next note
}thpool_job_t;

/**
 * define a job queue
 */
//任务队列，含有队列的头和尾
typedef struct _thpool_job_queue{
   thpool_job_t*    head;            	//queue head point
   thpool_job_t*    tail;             	//queue tail point
   int              jobN;               //task number
   sem_t*           queueSem;           //queue semaphore
}thpool_jobqueue;

/*
sem_init(thpool->jobqueue->queueSem, 0, 0); 这行代码的作用是初始化thpool->jobqueue->queueSem指向的信号量，
将其设置为0。sem_init函数是一种信号量初始化函数，它接受三个参数：第一个参数是一个sem_t *类型的指针，指向要初始化的信号量；
第二个参数是一个int类型的值，表示信号量的范围，如果为0，表示信号量只能在同一个进程中的不同线程之间共享，如果为非0，表示信号量可
以在不同进程之间共享；第三个参数是一个unsigned int类型的值，表示信号量的初始值，通常为0或1。
这两行代码的目的是为了实现线程池的任务队列的同步机制，即当任务队列为空时，所有的线程都会等待信号量，直到有新的任务加入
队列时，信号量的值会增加，从而唤醒一个或多个线程去执行任务
*/

/**
 * thread pool
 */
//线程池
typedef struct _thpool_t{
   pthread_t*      	threads;    // thread point
   int             	threadsN;   // thread pool number
   thpool_jobqueue* jobqueue;  	// job queue
}thpool_t;

//每个线程的入口函数的参数
typedef struct _thpool_thread_parameter{
	thpool_t 		*thpool;
	int 			thread_index;
}thpool_thread_parameter;


thpool_t*  thpool_init(int thread_pool_numbers);
int thpool_add_work(thpool_t* tp_p, void *(*function_p)(void*arg, int index), /*void *arg_p*/int socket_fd, char *recev_buffer);
void thpool_destroy(thpool_t* tp_p);
int get_jobqueue_number(thpool_t* tp_p);
int delete_timeout_job(thpool_t* tp_p, int time_out);

#endif /* THREAD_POOL_H_ */
