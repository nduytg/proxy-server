#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#include <sys/wait.h>		//Ham waitpid, kill child processes
#include <signal.h>			//Signal handler
//#include <pthread.h>		//Mutex

//Thu vien cho shared memory giua cac process
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>

//Cac gia tri define
#define BACK_LOG 20
#define KEY 123456
#define SHARE_SIZE sizeof(int)*4

//Cac bien toan cuc cho ham printReport
typedef struct
{
	int mutex;
	int g_success;
	int g_filter;
	int g_error;
} g_var;

char *filter_domain;
char *htmlcontent;
g_var g;

//==================Cac ham linh tinh=================
//Nhan SIGUSR 1
//Report lai cac thong tin chuong trinh khi chay
void printReport()
{
	printf("Not completed...\n");
}

//Nhan SIGUSR 2
//Shutdown chuong trinh
void shutDown();

//Nhan SIGINT
//Skip qua signal SIGINT (Ctrl-C)
void skip();
void error(char* msg);

//Ham clean zombie
void clean_zombie(int signal)
{
	wait(NULL);
}

//=====================================================


//==================Ham Main===========================
int main(int argc,char* argv[])
{
	//Signal catcher!
	signal(SIGCHLD,clean_zombie);			//clean zombie
	signal(SIGINT,skip);					//Skip Ctrl-C
	signal(SIGUSR2,shutDown);				//Shutdown when getting SIGUSR2
	signal(SIGUSR1,printReport);			//Printreport whenrecv SIGUSR1
	//---------------
	
	pid_t pid;
	struct sockaddr_in addr_in, cli_addr, serv_addr;
	struct hostent* host;
	int sockfd, newsockfd;
	int childCount = 0;
	
	//Bien dem toan cuc cho SIGUSR1
	//g.g_success = g.g_filter = g.g_error = g.mutex = 0;
	
	if(argc < 2)
		error("./MSSV <port_no> [filter-domain]");
	
	int filter = 0;	
	
	filter_domain = NULL;
	htmlcontent = NULL;
	if(argc == 3)
	{
		filter_domain = (char*)malloc(strlen(argv[2]));
		strcpy(filter_domain, argv[2]);
	}	
	  
	printf("\n*****WELCOME TO PROXY SERVER*****\n");
	printf("\nCopyright (c) 2014  GODLY T.ALIAS");
	printf("\nModified by Duy Nguyen - 1312084\n");
	printf("Proxy server run on port %s\n",argv[1]);
	if(argc == 3) 
		printf("Filter domain: %s\n",argv[2]);
	   
	//######## Cac buoc khoi tao socket Listening ########
	bzero((char*)&serv_addr,sizeof(serv_addr));
	bzero((char*)&cli_addr, sizeof(cli_addr));
	   
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[1]));
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	   
	//Tao socket!  
	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	if(sockfd < 0)
		error("Problem in initializing socket");
	
	if(bind(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0)
		error("Error on binding");
	//########################################################
	  
	//Decrease backlog from 50 -> 20  
	listen(sockfd,BACK_LOG);
	int clilen = sizeof(cli_addr);
	  
	 
	accepting:
	
	newsockfd = accept(sockfd,(struct sockaddr*)&cli_addr,&clilen);
	if(newsockfd < 0)
		error("Problem in accepting connection");
	
	//Tao vung nho shared memory cho cac process
	//***Create the segment***
	//key_t key = KEY;
	//int shmid;		//id de quan ly vung nho chung
	//int *shm;, *s	//Con tro de tro toi vung nho chung
	
	/*
	if( (shmid = shmget(key,SHARE_SIZE, IPC_CREAT | 0666))  < 0)
	{
		perror("shmget failed!\n");
		//exit(1);
		goto close;
	}
	//***Attach the segment to parent process data space.
	if( (shm = shmat(shmid, NULL,0)) == (g_var*) -1)
	{
		perror("shmat failed!\n");
		goto close;
	}
	*/
	
	//Fork process!!  
	pid = fork();
	
	//Neu process con dc tao ra!!
	if(pid == 0)
	{
		//------Shared Memory------
		//shmid = shmget(key, SHARE_SIZE, 0);
		//shmctl(shmid,cmd,g);
		//shm = shmat(shmid, 0, 0);
		//s = shm;
		//*s = '\0';
		//-------------------------
		
		
		struct sockaddr_in host_addr;
		int flag = 0, newsockfd1, n, port = 0, i, sockfd1;
		char buffer[510], t1[300], t2[300], t3[10];
		char method[5];
		char* temp = NULL;
		bzero((char*)buffer, 500);
		recv(newsockfd, buffer, 500, 0);
		   
		//t1 = GET (HEAD, POST)
		//t2 = link
		//t3 = HTTP version 
		sscanf(buffer,"%s %s %s",method,t2,t3);
		
		//Kiem tra filter case
		//printf("Checking filter...\n");
		char *p = NULL;
		if(filter_domain != NULL)
			p = strstr(t2,filter_domain);
		if(p != NULL)
		{
			char *p = strstr(t2, filter_domain);
			
			//Neu thieu prefix hay suffix thi bao 403
			if( (( (p - 1)[0] == '.' ) && ( ((p + strlen(filter_domain))[0]) == '.' )) 
				&& 	((strlen(t2) - 7) > strlen(p)) )
				
			{
				printf("Filter passed!\n");
			}
			else
			{
				//TH1 cua ERROR
				//shmctl(shmid,SHM_LOCK,g);
				//-------------
				
				
				char response[] = "403 (Forbidden) HTTP reponse.\n";
				send(newsockfd, response, strlen(response),0);
				printf("%s\n", response);
				goto close;
			}	
		}	
		//-----Ket thuc kiem tra filter-----
		
		//printf("Method = %s\n",method);
		printf("\n\n-----HTML Header-----\n");
		printf("%s\n-----------------\n\n",buffer);
	
		//============Cat header================
		char *pt;
		pt = strstr(buffer,"\r\n\r\n");
		if (pt != NULL)
			pt += 4;
		htmlcontent = (char*) malloc(strlen(buffer) - strlen(pt) + 1);
		strcpy(htmlcontent,pt);
		//printf("HTML Content:\n%s\n",htmlcontent);
		//======================================
		
		   
		//Them HEAD voi POST vo day!!!
		//3 phuong thuc HEAD, GET, POST cho giao thuc http version 1.0 va 1.1
		if ( ( (strncmp(method,"GET",3) == 0) || (strncmp(method,"HEAD",4) == 0 ) || (strncmp(method,"POST",4) == 0 ))
			&& ((strncmp(t3,"HTTP/1.1",8) == 0) || (strncmp(t3,"HTTP/1.0",8) == 0)) && (strncmp(t2,"http://",7) == 0))
		{
			strcpy(t1,t2);
			flag = 0;  
			 
			for (i=7; i<strlen(t2); i++)
			{
				if(t2[i]==':')
				{
					//URI: htttp://example.com:8820/......
					printf("%s\n",t2);
					flag = 1;
					break;
				}
			}
			   
			//Lay duong dan
			temp = strtok(t2,"//");
			if (flag == 0)
			{
				port = 80;
				temp = strtok(NULL,"/");
			}
			else
			{
				temp = strtok(NULL,":");
			}
			   
			//t2 tu fulllink => hostname
			sprintf(t2,"%s",temp);
			printf("host = %s",t2);
			host = gethostbyname(t2);
			   
			//Get port (URI) neu co
			if(flag == 1)
			{
				printf("Flag = 1\nTemp = %s\n",temp);
				temp=strtok(NULL,"/");
				port = atoi(temp);
			}	   
			   
			strcat(t1,"^]");
			temp = strtok(t1,"//");
			temp = strtok(NULL,"/");
			if(temp != NULL)
				temp = strtok(NULL,"^]");
			printf("\nPath = %s\nPort = %d\n",temp,port);
			   
			   
			bzero((char*)&host_addr,sizeof(host_addr));
			host_addr.sin_port = htons(port);
			host_addr.sin_family = AF_INET;
			bcopy((char*)host->h_addr,(char*)&host_addr.sin_addr.s_addr,host->h_length);
			   
			sockfd1 = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
			newsockfd1 = connect(sockfd1,(struct sockaddr*)&host_addr,sizeof(struct sockaddr));
			
			sprintf(buffer,"\nConnected to %s  IP - %s\n",t2,inet_ntoa(host_addr.sin_addr));
			printf("\n%s\n",buffer);
			
			if(newsockfd1 < 0 )
				error("Error in connecting to remote server");
			   
			
			bzero((char*)buffer,sizeof(buffer));
			if(temp != NULL)
				sprintf(buffer,"%s /%s %s\r\nHost: %s\r\nConnection: close\r\n\r\n",method, temp, t3, t2);
			else
				sprintf(buffer,"%s / %s\r\nHost: %s\r\nConnection: close\r\n\r\n",method, t3, t2);
				
			//Cho method POST
			if(htmlcontent != NULL)
				strcat(buffer,htmlcontent);
			
			//Send requests to web server
			n = send(sockfd1, buffer, strlen(buffer), 0);
			//printf("\n%s\n",buffer);
			printf("----My proxy request to http server----\n%s\n\n========END========",buffer);
			
			if(n<0)
				error("Error writing to socket");
			else
			{
				do
				{
					bzero((char*)buffer, 500);
					n = recv(sockfd1, buffer, 500, 0);
					if( !(n <= 0) )
						send(newsockfd, buffer, n, 0);
				}
				while(n > 0);
			}
		}
		else
		{
			char response[] = "501: NOT IMPLEMENTED\nONLY HTTP REQUEST: GET, HEAD, POST ALLOWED\n";
			send(newsockfd,response,strlen(response),0);
			//wait(NULL);
			//(*g_error)++;
			//printf("Gia tri g_error hien tai: %d\n",*g_error);
		}	
		close:
		
		close(sockfd1);
		close(newsockfd);
		close(sockfd);
		free(htmlcontent);
		if(filter_domain != NULL)
			free(filter_domain);
		_exit(0);		
	}
	else
	{
		close(newsockfd);
		goto accepting;
	}
	

	return 0;
}
//=============================================================

//Nhan SIGUSR 1
//Report lai cac thong tin chuong trinh khi chay
/*
void printReport()
{
	printf("\nReceive SIGUSR1.....reporting status:");
	printf("\n-- Processed %d request(s) successfully.",*g_success);
	
	if(filter_domain != NULL)
		printf("\n-- Filtering: %s",filter_domain);
	else
		printf("\n-- Filtering: (empty).");
		
	printf("\n-- Filtered %d request(s).",*g_filter);
	printf("\n-- Encountered %d request(s) in error.\n",*g_error);
	
}
*/

//Nhan SIGUSR 2
//Shutdown chuong trinh
void shutDown()
{
	printf("\nGet SIGUSR2, the program will shutdown\n");
	if(filter_domain != NULL)
		free(filter_domain);
	if(htmlcontent != NULL)
		free(htmlcontent);
	exit(1);
}

//Nhan SIGINT
//Skip qua signal SIGINT (Ctrl-C)
void skip()
{
	printf("\nSkip interrupt signal (Ctrl-C)\n");
}		

void error(char* msg)
{
	perror(msg);
	exit(0);
}


