/*
 * time_wheel_timer.h
 *
 *  Created on: 2015年10月2日
 *      Author: zjl
 */

#ifndef TIME_WHEEL_TIMER_H_
#define TIME_WHEEL_TIMER_H_

#include<time.h>
#include<netinet/in.h>
#include<stdio.h>

#define BUFFER_SIZE 64
class tw_timer;

struct client_data{
	sockaddr_in address;
	int sockfd;
	char buf[BUFFER_SIZE];
	tw_timer * timer;
};

class tw_timer{
	public:
		int rotation;
		int time_solt;
		void (*cb_func)(client_data *);
		client_data *user_data;
		tw_timer * next;
		tw_timer * prve;
	public:
		tw_timer(int rot,int ts) {
			this->rotation=rot;
			this->time_solt= ts;
			this->next=NULL;
			this->prve= NULL;
		}
};

class time_wheel{
	private:
		static const int N= 60;
		static const int SI =1;
		tw_timer * slots[N];
		int cur_slot;
	public:
		time_wheel() : cur_slot(0){
			for(int i=0;i<N;i++){
				slots[i] = NULL;
			}
		}
		virtual ~time_wheel(){
			for(int i ;i<N ;i++){
				tw_timer * tmp=slots[i];
				while(tmp){
					slots[i]=tmp->next;
					delete tmp;
					tmp=slots[i];
				}
			}
		}
		tw_timer * add_timer(int timeout){
			if(timeout< 0){
				return NULL;
			}
			int ticks = 0;
			if(timeout < SI){
				ticks = 1;
			}else{
				ticks = timeout/SI;
			}
			int rotation = ticks/N;
			int ts=(cur_slot +(ticks%N))%N;
			tw_timer * timer = new tw_timer(rotation,ts);
			if(!slots[ts]){
				printf("add timer, roation is %d, ts is %d, cur_solts is %d\n",rotation,ts,cur_slot);
				slots[ts];
			}else{
				timer->next - slots[ts];
				slots[ts]->prve=timer;
				slots[ts] = timer;
			}
			return timer;
		}
		void del_timer(tw_timer * timer){
			if(! timer){
				return;
			}
			int ts= timer->time_solt;
			if(timer == slots[ts]){
				slots[ts] = slots[ts]->next;
				if(slots[ts]){
					slots[ts]->prve=NULL;
				}
				delete timer;
			}else{
				timer->prve->next = timer->next;
				if(timer->next){
					timer->next->prve = timer->prve;
				}
				delete timer;
			}
		}
		void tick(){
			tw_timer * tmp = slots[cur_slot];
			printf("cur_slot is %d\n",cur_slot);
			while(tmp){
				printf("tick the tiimer once \n");
				if(tmp->rotation > 0){
					tmp->rotation--;
					tmp=tmp->next;
				}else{
					tmp->cb_func(tmp->user_data);
					if(tmp == slots[cur_slot]){
						printf("delete header in cur_slot\n");
						slots[cur_slot] = tmp->next;
						delete tmp;
						if(slots[cur_slot]){
							slots[cur_slot]->prve = NULL;
						}
						tmp = slots[cur_slot];
					}
					else{
						tmp->prve->next = tmp->next;
						if(tmp->next){
							tmp->next->prve=tmp->prve;
						}
						tw_timer * tmp2 = tmp->next;
						delete tmp;
						tmp = tmp2;
					}
				}
			}
			cur_slot= ++cur_slot % N ;
		}
};
#endif /* TIME_WHEEL_TIMER_H_ */
