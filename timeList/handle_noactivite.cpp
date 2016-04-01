/*
 * handle_noactivite.cpp
 *
 *  Created on: 2015年10月1日
 *      Author: zjl
 */


#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<signal.h>
#include<unistd.h>
#include<error.h>
#include<fcntl.h>
#include<stdlib.h>
#include<sys/epoll.h>
#include<pthread.h>
#include"list_time.h"

#define FD_LIMIT 65536
#define MAX_EVENT_NUMBER 1024
#define TIMELOT 5

static int pipefd[2];
static sort_timer_list timer_list;

static int epollfd;

int setnonblock(int fd){
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd,F_SETFL , new_option);
	return old_option;
}

void addfd(int epollfd, int fd){
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
	epoll_ctl(epollfd,EPOLL_CTL_ADD, fd ,& event);
	setnonblock(fd);
}

void sig_handler(int sig){
	int save_error = errno;
	int msg = sig;
	send(pipefd[1],(char *)&msg,1,0);
	errno = save_error;

}

void addsig(int sig){
	struct sigaction sa;
	memset(&sa,'\0',sizeof(sa));
	sa.__sigaction_handler = sig_handler;
	sa.sa_flags |= SA_RESTART;
	sigfillset(&sa.sa_mask);
	assert(sigaction(sig,&sa,NULL) != -1);
}

void timer_handler(){
	timer_list.tick();
	alarm(TIMESOLT);
}

void cb_func(client_data * user_data){
	epoll_ctl(epollfd,EPOLL_CTL_DEL,user_data->sockfd,0);
	assert(user_data);
	close(user_data->sockfd);
	printf("close fd %d\n",user_data->sockfd);
}

int main(int argc, char ** argv)
{
	if(argc <= 2){
		printf("usage:  %s ip_address port_number \n",basename(argv[0]));
	}

	const char * ip = argv[1];
	int port = atoi(argv[2]);
	int ret = 0;
	struct sockaddr_in address;
	bzero(&address,sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	inet_pton(AF_INET,ip,&address.sin_addr);

	int listenfd = socket(PF_INET,SOCK_STREAM,0);
	assert(listenfd > 0);
	ret = bind(listenfd,(struct sockaddr *)&address,sizeof(address));
	assert(ret != -1);

	ret = listen(listenfd,5);
	assert(ret !=  -1);

	epoll_event event[MAX_EVENT_NUMBER];
	int epollfd=epoll_create(5);
	assert(epollfd != -1);
	addfd(epollfd,listenfd);
	ret =socketpair(PF_UNIX, SOCK_STREAM, 0 , pipefd);

	assert(ret != -1);
	setnonblock(pipefd[1]);
	addfd(epollfd,pipefd[0]);

	addsig(SIGALRM);
	addsig(SIGTERM);
	bool stop_server = false;

	client_data * users = new client_data[FD_LIMIT];
	bool timeout = false;
	alarm(TIMESLOT);
	while(!stop_server){
		int number = epoll_wait(epollfd,event,MAX_EVENT_NUMBER,-1);
		if((number < 0) && (errno != EINTR)){
			printf("epoll failure \n");
			break;
		}
		for(int i=0 ; i<number ; i++){
			int sockfd = event[i].data;
			if(sockfd == listenfd){
				struct sockaddr_in  client_address;
				socklen_t client_length = sizeof(client_address);
				int confd = accept(listenfd,(struct sockaddr * )&client_address,&client_length);
				addfd(epollfd,confd);
				users[confd].address = client_address;
				users[confd].sockfd = confd;
				t_timer  * timer = new t_timer;
				timer->user_data = & users[confd];
				timer->cb_func = cb_func;
				time_t cur=time(NULL);
				timer->expire = cur+ 3 * TIMESLOT;
				users[confd].timer = timer;
				timer_list.add_timer(timer);
			}
			else if((sockfd == pipefd[0])&& (event[i].events & EPOLLIN)){
				int sig;
				char signals[100];
				ret = recv(pipefd[0],signals,sizeof(signals),0);
				if(ret ==  -1){
					continue;
				}else if(ret == 0){
					continue;
				}else {
					for ( int i=0 ; i< ret; i++){
						switch(signals[i]){
							case SIGALRM:{
								timeout = true;
								break;
							}
							case SIGTERM:{
								stop_server = true;
						}
					}
				}
			}
		}
			else if(event[i].events & EPOLLIN){
				memset (users[sockfd].buf,'\0',sizeof(users[sockfd].buf));
				ret recv(sockfd,users[sockfd].buf,BUFFER_SIZE-1,0);
				printf(" get %d bytes of client data %s from %d \n",ret,users[sockfd].buf,sockfd);
				t_timer *timer = users[sockfd].timer;
				if (ret <0){
					if(errno != EAGAIN){
						cb_func(&users[sockfd]);
						if(timer){
							timer_list.del_timer(timer);
						}
					}
				}else if(ret == 0){
					cb_func(&users[sockfd]);
					if(timer){
						timer_list.del_timer(timer);
					}
				}else{
					if(timer){
						time_t cur = time(NULL);
						timer->expire = cur + 3 * TIMESLOT;
						printf("adjust timer once \n");
						timer_list.adjust_timer(timer);
					}
				}
			}else{
//				others
			}
	}
		if(timeout){
			timer_handler();
			timeout = false;
		}
	}
		close(listenfd);
		close(pipefd[0]);
		close(pipefd[1]);
		delete []  users;
		users=NULL;
		return 0;
}


















