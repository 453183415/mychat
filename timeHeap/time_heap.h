/*
 * time_heap.h
 *
 *  Created on: 2016年4月1日
 *      Author: zjl
 */

#ifndef TIME_HEAP_H_
#define TIME_HEAP_H_

#include<iostream>
#include<netinet/in.h>
using std::exception;

#define BUFFER_SIZE 64
class heap_timer;
struct client_data{
	sockaddr_in address;
	int sockfd;
	char buf [BUFFER_SIZE];
	heap_timer * timer;
};
class heap_timer{

public:
		time_t expire;
		void (*cb_func)(client_data);
		client_data * user_data;
public:
		heap_timer(int delay){
			expire = time(NULL);
		}
};
class time_heap{
	private:
		heap_timer ** array;
		int capacity;
		int cur_size;
	private:
		void percolate_down(int hole){
			heap_timer * tmp = array[hole];
			int child =0;
			for(;((hole*2+1)<=(cur_size-1));hole=child){
				child = hole*2+1;
				if((child<(cur_size-1)) && (array[child+1]->expire<array[child]->expire)){
					++child;
				}
				if(array[child]->expire < tmp->expire){
					array[hole] = array[child];
				}else{
					break;
				}
			}
			array[hole] = tmp;
		}
		void resize()throw(std::exception){
			heap_timer** tmp=new heap_timer *[2*capacity];
			for(int i=0; i<2*capacity; i++){
				tmp[i]=NULL;
			}
			if(!tmp){
				throw std::exception();
			}
			capacity*=2;
			for(int i=0; i<cur_size;i++){
				tmp[i] = array[i];
			}
			delete [] array;
			array = tmp;
		}
		time_heap( int cap) :capacity(cap),cur_size(0){
			array = new heap_timer *[capacity];
			if(!array){
				throw std::exception();
			}
			for(int i=0 ;i<capacity; ++i){
				array[i] = NULL;
			}
		}
		time_heap(heap_timer ** init_array,int size,int capacity): cur_size(size),capacity(capacity){
			if(capacity <size){
				throw std::exception();
			}
			array = new heap_timer *[capacity];
			if(!array){
				throw std::exception();
			}
			for(int i=0; i<capacity; i++){
				array[i]=NULL;
			}
			if(size != 0){
				for(int i=0; i<size ; i++){
					array[i] = init_array[i]
				}
				for(int i=(cur_size-1)/2;i>=0;i++){
					percolate_down(i);
				}
			}
		}
		virtual ~time_heap(){
			for(int i=0; i< cur_size; i++){
				delete array[i];
			}
			delete [] array;
		}
	public:
		void add_timer(heap_timer * timer){
			if(!timer){
				return ;
			}
			if(cur_size >= capacity){
				resize();
			}
			int hole = cur_size++;
			int parent = 0;
			for(;hole>0;hole=parent){
				parent = (hole-1)/2;
				if(array[parent]->expire <= timer->expire){
					break;
				}
				array[hole]= array[parent];
			}
			array[hole]= timer;
		}
		void del_timer(heap_timer * timer){
			if(!time){
				return;
			}
			timer->cb_func = NULL;
		}
		heap_timer * top() const{
			if(empty()){
				return NULL;
			}
			return array[0];
		}
		void pop_timer(){
			if(empty()){
				return;
			}
			if(array[0]){
				delete array[0];
				array[0]=array[--cur_size];
				percolate_down(0);
			}
		}
		void tick(){
			heap_timer * tmp = array[0];
			time_t cur = time(NULL);
			while(!empty()){
				if(!tmp){
					break;
				}
				if(tmp->expire > cur){
					break;
				}
				if(array[0]->cb_func){
					array[0]->cb_func(array[0]->user_data);
				}
				pop_timer();
				tmp =array[0];
			}
		}
		bool empty()const{
			return cur_size == 0;
		}
};
#endif /* TIME_HEAP_H_ */
