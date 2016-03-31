#define _GUN_SOURCE_ 1
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<error.h>
#include<assert.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<error.h>
#include<poll.h>

#define USER_LIMIT 5
#define BUFFER_SIZE 64
#define FD_LIMIT 65535

struct client_data{
	sockaddr_in address;
	char * write_buf;
	char buf[BUFFER_SIZE];
};

int setnonblock(int fd){
	int old_option = fcntl(fd,F_GETFL);
	int new_option=old_option | O_NONBLOCK;
	fcntl(fd,F_SETFL,new_option);
	return old_option;
}

int main(int argc, char** argv){

	if(argc <=2){
		printf("usage:%s ip_address port_number\n",basename(argv[0]));
		return 1;
	}

	const char * ip=argv[1];
	int port = atoi(argv[2]);
	int ret=0;
	struct sockaddr_in address;
	bzero(&address,sizeof(sockaddr_in));
	address.sin_family=AF_INET;
	address.sin_port=port;
	inet_pton(AF_INET,ip,&address.sin_addr);

	int listenfd= socket(PF_INET,SOCK_STREAM,0);
	assert(listenfd>=0);
	ret = bind(listenfd,(struct sockaddr *)&address,sizeof(address));
	assert(ret !=-1);

	ret = listen(listenfd,10);

	assert(ret !=-1);
	client_data *users = new client_data[FD_LIMIT];

	pollfd fds[USER_LIMIT];
	int user_number=0;

	for(int i=1;i<=USER_LIMIT;i++){
		fds[i].fd=-1;
		fds[i].events=0;
	}
	fds[0].fd=listenfd;
	fds[0].events=POLLIN |POLLERR;
	fds[0].revents = 0;
	while(1){
		ret = poll(fds,user_number+1,-1);
		if(ret <0){
			printf("poll failure\n");
			break;
		}
		for(int i=0;i<user_number+1;i++)
		{
			if((fds[i].fd==listenfd )&&(fds[i].revents & POLLIN)){
				struct sockaddr_in client_address;
				socklen_t client_address_length = sizeof(client_address);
				int confd = accept(listenfd,(struct sockaddr *)&client_address,&client_address_length);
				if(confd < 0)
				{
					printf("error is : %d \n",errno);
					continue;
				}
				if(user_number>=USER_LIMIT)
				{
					const char * info = "too many users\n";
					printf("%s\n",info);
					send(confd,info,strlen(info),0);
					close(confd);
					continue;
				}
				user_number++;
				users[confd].address = client_address;
				setnonblock(confd);
				fds[user_number].fd=confd;
				fds[user_number].events = POLLIN | POLLRDHUP | POLLERR;
				fds[user_number].revents = 0;
				printf("come anew user , now have %d users\n",user_number);
			}
			else if(fds[i].revents & POLLERR)
			{
				printf("get an error from %d\n",fds[i].fd);
				char errors[100];
				memset(errors,'\0',100);
				socklen_t length=sizeof(errors);
				if(getsockopt(fds[i].fd,SOL_SOCKET,SO_ERROR,&errors,&length) <0){
					printf("get socket option failed");
				}
				continue;
			}
			else if(fds[i].revents & POLLRDHUP)
			{
				users[fds[i].fd]= users[fds[user_number].fd];
				close(fds[i].fd);
				fds[i]=fds[user_number];
				i--;
				user_number--;
				printf("a client left\n");
			}
			else if(fds[i].revents& POLLIN)
			{
				int confd = fds[i].fd;
				memset (users[confd].write_buf,'\0',BUFFER_SIZE);
				ret = recv(confd,users[confd].write_buf,BUFFER_SIZE,0);
				printf("get %d bytes of client data %s from %d\n",ret,users[confd].write_buf,confd);
				if(ret <0)
				{
					if(errno != EAGAIN){
						close(confd);
						users[fds[i].fd]= user[fds[user_number].fd];
						fds[i]=fds[user_number];
						i--;
						user_number--;
					}
				}
				else if(ret ==0)
				{

				}
			else
			{
				for(int j=1;j<user_number;j++){
					if(fds[j].fd == listenfd){
						continue;
					}
					fds[j].events |= ~POLLIN;
					fds[j].events |= POLLOUT;
					users[fds[j].fd].write_buf = users[confd].buf;
				}
			}
		}
#if(1)
			else if(fds[i].revents  & POLLIN)
			{
				int confd = fds[i].fd;
				if( ! users [confd].write_buf)
				{
					continue;
				}
				ret = send(confd,users[confd].write_buf,strlen(users[confd].write_buf),0);
				users[confd].write_buf = NULL;
				fds[i].events |= ~ POLLOUT;
				fds[i].events |= POLLIN;
			}
#endif
		}
	}

	delete [] users;
	close(listenfd);
	return 0;
}



























}
