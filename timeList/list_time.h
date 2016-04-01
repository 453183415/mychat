/*
 * list_time.h
 *
 *  Created on: 2015年9月31日
 *      Author: zjl
 */

#ifndef LIST_TIME_H_
#define LIST_TIME_H_
#include<time.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#define  BUFFER_SIZE 64
class t_timer;
struct client_data{
	sockaddr_in address;
	int sockfd;
	char buf[BUFFER_SIZE];
	t_timer *timer;
};

class t_timer{
public:
	t_timer():prve(NULL),next(NULL){}
public:
	time_t expire;//timeout
	void (*cb_func)(client_data *);//reback function
	client_data * user_data;
	t_timer *prve;
	t_timer * next;
};

class sort_timer_list{
private:
	t_timer *head;
	t_timer *tail;
public:
	sort_timer_list():head(NULL),tail(NULL){}
	virtual~sort_timer_list(){
		t_timer *tmp = head;
		while(tmp){
			head=tmp->next;
			delete tmp;
			tmp=head;
		}
	}
	void add_timer(const t_timer * timer){
		if(!t_timer){
			return ;
		}
		if(!head){
			head=tail=timer;
		}
		if(timer->expire < head->expire){
			timer->next=head;
			head->prve=timer;
			head=timer;
			return;
		}
		add_timer(timer,head);
	}
	void adjust_timer(const t_timer *timer){
		if(!timer){
			return;
		}
		t_timer * tmp = timer->next;
		if(!tmp || (timer->expire < tmp->expire)){
			return ;
		}
		if(timer == head){
			head = head->next;
			head->prve=NULL;
			timer->next=NULL;
			add_timer(timer,head);
		}
		else {
			timer->prve->next = timer ->next;
			timer->prve->prve = timer->prve;
			add_timer(timer,timer->next);
		}
	}
	void del_timer(t_timer * timer){
		if(! timer){
			return;
		}
		if((timer == head) && (timer == tail)){
			delete timer;
			head= NULL;
			tail = NULL;
			timer = NULL;
		}
		if(timer == head ){
			head= head->next;
			head->prve=NULL;
			delete timer;
			timer = NULL;
			return;
		}
		if(timer == tail){
			tail = tail->prve;
			tail->next = NULL;
			delete  timer;
			timer = NULL;
			return;
		}
		timer->prve->next=timer->next;
		timer->next->prve = timer ->prve;
		delete timer;
		timer=NULL;
	}
	void tick(){
		if(!head){
			return;
		}
		printf("timer tick\n");
		t_timer cur= time(NULL);
		t_timer * tmp=new t_timer;
		tmp=head;
		while(tmp){
			if(cur < tmp->expire){
				break;
			}
			tmp->cb_func(tmp->user_data);
			head = tmp->next;
			if(head){
				head ->prve = NULL;
			}
			delete tmp;
			tmp = head;
		}
	}
private:
	void add_timer(const t_timer* timer,const t_timer *lst_head ){
		t_timer * prve = lst_head;
		t_timer * tmp=prve->next;
		while(1){
			if(timer->expire < tmp->expire){
				prve->next = timer;
				timer->next=tmp;
				tmp->prve=timer;
				timer->prve=prve;
			}
			if(!tmp){
				prve->next = timer;
				timer->prve= prve;
				timer->next=NULL;
				tail = timer;
			}
		}
	}
};

#endif /* LIST_TIME_H_ */
