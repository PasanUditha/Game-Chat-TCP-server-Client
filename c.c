#include <pthread.h>
#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#define MAX 1024
//#define PORT 66666
#define SA struct sockaddr

struct sockaddr_in chatservaddr, gameservaddr;
socklen_t len = sizeof(chatservaddr);
socklen_t glen = sizeof(gameservaddr);
pthread_t recv_tid, send_tid;
int sockfd, gamefd;
char name[50] = {0};
int mode=0;
char nameReg[1024];

void *Recv(void *arg);
void *Send(void *arg);

void func()
{
    char buff[MAX] = {0};
    int n = 0;

    printf("Enter your ID: ");
    scanf("%s", buff);
    sendto(gamefd, (const char *)buff, strlen(buff), 0, (const struct sockaddr *)&gameservaddr, sizeof(gameservaddr)); 
    printf("Registered.\n");
    n = recvfrom(gamefd, (char *)buff, MAX, MSG_WAITALL, (struct sockaddr *) &gameservaddr, &glen);
    buff[n] = '\0'; 
    printf("%s", buff);

    //for (;;)
    //{
        n = recvfrom(gamefd, (char *)buff, MAX, MSG_WAITALL, (struct sockaddr *) &gameservaddr, &glen);
        buff[n] = '\0'; 
        printf("Question : %s", buff);
        bzero(buff, MAX);
        int i = 0;
        while ((buff[i++] = getchar()) != '\n');
        sendto(gamefd, (char *)buff, strlen(buff), MSG_CONFIRM, (const struct sockaddr *)&gameservaddr, sizeof(gameservaddr));
        bzero(buff, MAX);
        n = recvfrom(gamefd, (char *)buff, MAX, MSG_WAITALL, (struct sockaddr *) &gameservaddr, &len);
        buff[n] = '\0'; 
        printf("Result : %s", buff);
        close(gamefd);
    //}
}

void func1()
{

    pthread_create(&recv_tid, NULL, &Recv, NULL);
    pthread_create(&send_tid, NULL, &Send, NULL);
    pthread_join(recv_tid, NULL);
    pthread_join(send_tid, NULL);
    close(sockfd);
}

int main(int argc,char *argv[])
{
	if(argc!=2){
		printf("Wrong arguments\n");
		exit(0);
	}
	
	int CPORT=66666;
	int gportus=55555;
	unsigned cportus=CPORT%USHRT_MAX;
	int myport=atoi(argv[1]);

	// socket create and verification
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1) {
		printf("socket creation failed...\n");
		exit(0);
	}
	else
	{
		printf("Socket successfully created..\n");
	}
    
	bzero(&chatservaddr, sizeof(chatservaddr));
	bzero(&gameservaddr, sizeof(gameservaddr));
 
	// assign IP, PORT
	chatservaddr.sin_family = AF_INET;
	
	chatservaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	chatservaddr.sin_port = htons(cportus);

	gameservaddr.sin_family = AF_INET;
	
	gameservaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	gameservaddr.sin_port = htons(gportus);

	struct sockaddr_in myaddr;
	
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	myaddr.sin_port = htons(myport);
	
	if (bind(sockfd, (struct sockaddr *)&myaddr, sizeof (struct sockaddr_in)) < 0)
	{
		perror("Error in Binding");
		exit(0);
	}

	printf("Enter you name:");
	scanf("%s", name);

   
    sprintf(nameReg,"REGISTER %s",name);
	printf("nameReg:%s\n",nameReg);

	if(sendto(sockfd, (const char *)nameReg, sizeof(nameReg), MSG_CONFIRM, (const struct sockaddr *)&chatservaddr, sizeof(chatservaddr))<0){
		perror("Send Registration");
	}else{
		printf("Registration complete.\nTo play Game Type      P \nTO see Results Type   RE\nTo register again Type R\nTo send massage to chat room Type msg|{the massage} \n ");
	}
	// connect the client socket to server socket

    // function for chat
    //func(connfd);
    func1();
 
    // close the socket
    close(sockfd);
    //shutdown(sockfd, SHUT_RDWR);
}

void *Recv(void *arg)
{
    int n = 0;
    char buff[MAX] = {0};
    struct sockaddr_in addr;
    socklen_t laddr=sizeof(addr);
    
    while(1)
    {
        bzero(buff, MAX);
        n = recvfrom(sockfd, (char *)buff, MAX, MSG_WAITALL, (struct sockaddr *) &addr, &laddr);
        buff[n] = '\0'; 
        printf("\n>%s\n", buff);
        
    }
}

void *Send(void *arg)
{
    char buff[MAX] = {0};
    char game[MAX] = "\ngame;";
    int i = 0;
    while(1)
    {
        i = 0;
        bzero(buff, MAX);

	    printf("Enter the massage: ");
	
	    scanf("%s", buff);

        printf("buff:%s\n",buff);
        if(strncmp(buff, "P",1) == 0)
        {
		printf("====================Ready to Play Game===================\n");
		sendto(sockfd, (const char *)buff, sizeof(buff), 0, (const struct sockaddr *)&gameservaddr, sizeof(gameservaddr));
            	mode=1;
            	continue;
        }
        
        if(strncmp(buff,"RE",2)==0){
        	printf("==================Results will show soon=====================\n");
		sendto(sockfd, (const char *)buff, sizeof(buff), 0, (const struct sockaddr *)&gameservaddr, sizeof(gameservaddr));
		continue;
        }
        
        if(strncmp(buff,"msg:",3)==0){
        	printf("==================Massege sending=========================\n");
        	char buftemp[300];
        	memcpy(buftemp,&buff[4],sizeof(buff)-4);
        	char buffmsg[500];
        	sprintf(buffmsg,"msg:%s:%s",name,buftemp);
//        	memcpy(bu)
		sendto(sockfd, (const char *)buffmsg, sizeof(buffmsg), 0, (const struct sockaddr *)&chatservaddr, sizeof(chatservaddr));
		continue;
        }
        sendto(sockfd, (const char *)buff, strlen(buff), 0, (const struct sockaddr *)&chatservaddr, sizeof(chatservaddr));
    }
}
