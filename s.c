#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <limits.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/select.h>

	FILE *flptr;	//file pointer for log file
	FILE *fcptr;	//file porint for client info db
	
	int p1;	//player 1
	int p2;	//plyaer 2
	int p3;	//player 3
	int winner;	//winner
	int firstAns;
	
struct player{
	int PlayerID;	//id
	char PlayerName[50];	//name
	int PlayerScore;	//target
	struct sockaddr_in playerAddr;	//connection stored
	int mode;	//winner of game
};

int gamefd,chatfd;	//game and chat fd
struct sockaddr_in gameServAddr, chatServAddr;	//sock addr
pthread_mutex_t lock;	//mutex lock
socklen_t len;	//sock len
int currentPlayers;	
int clientId;
int players;
pthread_t game_tid,chat_tid;
pthread_t gthread;
int actualresult;

struct player p[5];	//array of player

char question[20] = {0};

void *gamet(void *arg);
void *chatt(void *arg);

int genQues();

int main()
{
	firstAns=0;
	//open log file for writing
	flptr = fopen("Log.txt","w+");
	if(flptr == NULL)
	{
		printf("Error!");   
		exit(1);             
	}
	//writing to log file
	fprintf(flptr,"Log file sucess\n");fflush(flptr);
	
    fd_set read, write;
    int i = 0, nready, n;
    char buff[1024];
    socklen_t len;
	
	int gameport=55555;
	int chatport=66666;

	clientId=0;
	currentPlayers=0;

	//mutex initialization
	if (pthread_mutex_init(&lock, NULL) != 0) {
		printf("\n mutex init has failed\n");
		exit(1);
	}
		
	// Creating socket file descriptor
	if ((gamefd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	
//fd of chat socket
	if ((chatfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("Chat socket creation failed");
		exit(EXIT_FAILURE);
	}
		
		
	//clearing memory addresess
	memset(&gameServAddr, 0, sizeof(gameServAddr));
	memset(&chatServAddr, 0, sizeof(chatServAddr));


    // Filling server information
	gameServAddr.sin_family = AF_INET;
	gameServAddr.sin_addr.s_addr=inet_addr("127.0.0.1");
	
	gameServAddr.sin_port = htons(gameport);
	
	//Filling server chat information
	chatServAddr.sin_family = AF_INET;
	chatServAddr.sin_addr.s_addr=inet_addr("127.0.0.1");
	
	unsigned int chatportus=chatport%USHRT_MAX;	//to limit it in within range of unsigned short
	chatServAddr.sin_port = htons(chatportus);
	
	// Bind the socket with the server address
	if ( bind(gamefd, (struct sockaddr *)&gameServAddr, sizeof(gameServAddr)) < 0 )
	{
		perror("Bind Game server");
		exit(1);
	} 
	else 
	{
		printf("Game server bind successful\n");
		fprintf(flptr,"Game server bind successful\n");fflush(flptr);
	}

//bind chat socket
	if ( bind(chatfd, (struct sockaddr *)&chatServAddr, sizeof(chatServAddr)) < 0 )
	{
		perror("Bind chat server");
		exit(1);
	}
	else
    	{
		printf("Chat server bind successful\n");
		fprintf(flptr,"Chat server bind successful\n");fflush(flptr);
	}

	printf("============================Chat server=============================");
	fprintf(flptr,"Chat server\n");fflush(flptr);

//create thread for chat socket
	pthread_create(&chat_tid, NULL, &chatt, NULL);
	//thread for game socekt
	pthread_create(&game_tid, NULL, &gamet, NULL);
	
	
	//wait for thread to complete
	pthread_join(chat_tid, NULL);
	pthread_join(game_tid, NULL);

	return 0;
}

void *chatt(void *arg)
{
    int n;
    char* mess = "your turn to chat";
    char* conndisc = "Connection Discarded";
    
    char buff[1024] = {0};
    char sbuffer[1024] = {0};
    socklen_t len;
    char name[52];
    int itr=0;
    int discard;
    struct sockaddr_in recvAddr;
    len=sizeof(recvAddr);
    while(1)
    {
    	discard=0;
    	//recv form chat socket
        n = recvfrom(chatfd, (char *)buff, 1024, 0, ( struct sockaddr *)&recvAddr, &len);
        buff[n] = '\0';
        if(n<0){
        	perror("recvFrom:chatfd");
        }else{
        	printf("chatBuf:%s\n",buff);
        	fprintf(flptr,"chatBuf:%s\n",buff);fflush(flptr);
        }
        
        //if its a register command
        if(strncmp(buff, "R",1)==0){
        	printf("<<<<<<<<<<<<<       Register        >>>>>>>>>>>>>\n");
        	fprintf(flptr,"its a register command\n");
        	if(itr<5){
        		//register
        		//get name from register command
        		memcpy(name,&buff[9],sizeof(buff-9));
        		
			pthread_mutex_lock(&lock);
				//copy all items to struct
	       		strcpy(p[itr].PlayerName, name);
				p[itr].PlayerID = itr;
				p[itr].mode=0;
				p[itr].playerAddr=recvAddr;
				p[itr].PlayerScore=0;
				
			pthread_mutex_unlock(&lock);			
			
			char nm[20],write[250];//open file for client info db
			sprintf(nm,"CLI_%d",itr);
			fcptr = fopen(nm,"w+");
			if(fcptr == NULL)
			{
				printf("Error!");   
				exit(1);             
			}
			int score=0;
			fprintf(fcptr,"ID:%d Name:%s Score:%d\n",itr,name,score);
			fflush(fcptr);
			
			//inform client about its id
			snprintf(sbuffer, 1024, "Your User ID is: %d", p[itr].PlayerID);
			if(sendto(chatfd, (char *)sbuffer, sizeof(sbuffer), MSG_CONFIRM, (struct sockaddr *)&recvAddr, len)<0)
			{
				//perror("SENDFTO:id");
			}
			itr++;
        	}else{
        		//send message that no space
        		sendto(chatfd, (char *)conndisc, strlen(conndisc), MSG_CONFIRM, (struct sockaddr *)&recvAddr, len);
        		continue;
        	}
        }

		//its chat msg
        if(strncmp(buff, "msg",3)==0){
        	printf("<<<<<<<<<<    Massage    >>>>>>>>>>\n");
        	char msgdata[200];
        	memcpy(msgdata,&buff[4],sizeof(buff)-4);
        	printf("%s:\n",msgdata);
        	
        	
        	pthread_mutex_lock(&lock);
        	for(int i=0;i<5;i++){
			//if its a msg from playing player, discard it
        		if((p[i].playerAddr.sin_port==recvAddr.sin_port) && (p[i].mode==1))
        		{
        			printf("Discarding this ");
        			fprintf(flptr,"Discarding this ");fflush(flptr);
        			discard=1;
        			//continue;
        		}
        	}
        	pthread_mutex_unlock(&lock);
        	
        	
        	if(discard==1){
        		continue;
        	}
        	
        	pthread_mutex_lock(&lock);
        	for(int i=0;i<5;i++){
        		if(p[i].mode != 1)
        		{
				//send to all non playing players
				if(sendto(chatfd, (char *)msgdata, sizeof(msgdata), MSG_CONFIRM, (struct sockaddr *)&p[i].playerAddr, len)<0)
				{
					//perror("SENDFTO:id");
				}
        		}
        	}
        	pthread_mutex_unlock(&lock);
        }
    }
}
void *game(void *arg){
	printf("game th\n");
	sleep(10);
	
	printf("ITS a real game players are Winner[%d]:\n %d\n %d\n %d\n",winner,p1,p2,p3);
	char buffsend[200];
	sprintf(buffsend,"%d wins the game back to chat server\n",winner);
	
	char buffsend1[200];
	sprintf(buffsend1,"===================== Chat server ===============================\n");
	
	pthread_mutex_lock(&lock);
	for(int i=0;i<5;i++){
		
		if(p[i].mode!=1){
	
			if(sendto(gamefd, (const char *)buffsend1, sizeof(buffsend1), 0, ( struct sockaddr *)&p[i].playerAddr, sizeof(p[i].playerAddr))<0){
				perror("error sending game");
			}else{
				printf("sent winner\n");
			}
		}
		
		if(p[i].mode==1){
			//inform playing players that who won the game
			p[i].mode=0;
			if(sendto(gamefd, (const char *)buffsend, sizeof(buffsend), 0, ( struct sockaddr *)&p[i].playerAddr, sizeof(p[i].playerAddr))<0){
				perror("error sending game");
			}else{
				printf("sent winner\n");
			}
		}
	}
	pthread_mutex_unlock(&lock);

	char buffsendWait[200];
	//inform waiting players that they are moved to game area
	sprintf(buffsendWait,"======================= Waiting ==========================");
	pthread_mutex_lock(&lock);
	for(int i=0;i<5;i++){
		if(p[i].mode==2){
			p[i].mode=1;
			if(sendto(gamefd, (const char *)buffsendWait, sizeof(buffsendWait), 0, ( struct sockaddr *)&p[i].playerAddr, sizeof(p[i].playerAddr))<0){
				perror("error sending game");
			}else{
				printf("sent winner\n");
			}
		}
	}
	pthread_mutex_unlock(&lock);
	
	players=players-3;

}

void *gamet(void *arg)
{
    //int i = atoi(arg);
    
    int resCount=0;
    
    int n, nready, i, d, c, b = 1;
    fd_set read, write;
    struct sockaddr_in temp;
   
  	int citr=0;
   
    char buff[1024] = {0};
    //char question[20] = {0};
    char *answer = "garbage";
    char *result = "garbage";
    int gameStart=0;
    char *wait = "wait util 3 clients join";
    struct sockaddr_in grecvAddr;
    socklen_t glen=sizeof(grecvAddr);

	while(1)
	{
		//recv from gamesocket
		n = recvfrom(gamefd, (char *)buff, 1024, 0, ( struct sockaddr *)&grecvAddr, &glen);
		buff[n] = '\0';
		temp=grecvAddr;
		if(n<0){
			perror("recvfrom gamefd:");
		}else{
			printf("recieved from gamefd\n");
		}
		
		//its request to start the game

		if(strncmp(buff, "P",1)==0)
		{
			printf("Its request to play the game\n");
			
			if(players<3)
			{
				players++;
				printf("Added to Client DB\n");
				
				pthread_mutex_lock(&lock);
				for(int i=0;i<5;i++)
				{
					if(p[i].playerAddr.sin_port==temp.sin_port)
					{
						printf("Setting player Mode for this\n");
						p[i].mode=1;
						if(players==1){
							p1=p[i].PlayerID;
						}else if(players==2){
							p2=p[i].PlayerID;
						}else{
							p3=p[i].PlayerID;
						}
					}
				}
				pthread_mutex_unlock(&lock);
				
				
				pthread_mutex_lock(&lock);
				
				for(int i=0;i<5;i++){
				
					if(p[i].mode==1){
						//wait for players to reach 3
						if(players<3)
						{
							if(p[i].playerAddr.sin_port==temp.sin_port){
								char *hellomax = "Wait for 3 players to join";
								if(sendto(chatfd, (const char *)hellomax, strlen(hellomax),0, (struct sockaddr *)&p[i].playerAddr, sizeof(p[i].playerAddr))<0)				
								{
										perror("Send:<3");
								}
							}
									
						}
						else
						{
							//players have reached 3, so start game
							char waitout[100]="Player count have reached; going to start the game";
							if(sendto(gamefd, (const char *)waitout, sizeof(waitout), 0, ( struct sockaddr *)&p[i].playerAddr, sizeof(p[i].playerAddr))<0){
								perror("sendo reached 3");
							}else{
								printf("=================== 3 Players ==================================\n");
							}
						//going to start the game
							gameStart=1;
						
						}
					}

				}
				pthread_mutex_unlock(&lock);

			}
			else
			{
			
				pthread_mutex_lock(&lock);
					for(int i=0;i<5;i++){
						if(p[i].playerAddr.sin_port==temp.sin_port){
							p[i].mode=2;
							printf("Setting Waiting mode for this\n");
						}
					}
				pthread_mutex_unlock(&lock);
				char waitout[100]="No space in playing, wait";
				sendto(gamefd, (const char *)waitout, sizeof(waitout), 0, ( struct sockaddr *)&temp, sizeof(temp));
//				sendto(sockfd, (const char *)nameReg, sizeof(nameReg), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr)); 
			}
			if(gameStart==1){
				char question[200];

				//for(n=1;n<4;n++){
				int a=rand() % 12;
				int b=rand() % 12;
				actualresult=a*b;
				sprintf(question,"Solve %d x %d",a,b);

				pthread_mutex_lock(&lock);	
					for(int i=0;i<5;i++){

						if(p[i].mode==1){

							if(sendto(gamefd, (const char *)question, sizeof(question), 0, ( struct sockaddr *)&p[i].playerAddr, sizeof(p[i].playerAddr))<0)	{
								perror("Question Send error");
							}else{
								printf("Question sent\n");
							}			

						}
					}

				pthread_mutex_unlock(&lock);
				gameStart=0;
				//}
			}
			
		}
		
		if(strncmp(buff,"RE",2)==0){
		
			printf("=========================== Results ===================================\n");
			char bres[50];
			memcpy(bres,&buff[7],sizeof(bres-7));
			int res=atoi(bres);
			printf("Results have Released:%d\n",res);
			resCount++;

			pthread_mutex_lock(&lock);	
			for(int i=0;i<5;i++)
			{
				if(p[i].playerAddr.sin_port == temp.sin_port)
				{
					if(res==actualresult)
					{
						if(firstAns==0){
						
							winner=p[i].PlayerID;
							char nm[20],write[250];
							sprintf(nm,"CLI_%d",winner);
							fcptr = fopen(nm,"w+");
							if(fcptr == NULL)
							{
								printf("Error!");   
								exit(1);             
							}

							int score=10;
							fprintf(fcptr,"ID:%d Name:%s Score:%d\n",winner,p[i].PlayerName,score);
							fflush(fcptr);
							printf("winner:%d name:%s",winner,p[i].PlayerName);fflush(stdout);
							firstAns=1;
						}
					}
				}	
			}
			pthread_mutex_unlock(&lock);
					
			
			if(resCount==3){
				printf("startting game\n");
				pthread_create(&gthread, NULL, &game, NULL);
			}
			
				
		}
	}
}

