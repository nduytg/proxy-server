#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<netdb.h>
#include<errno.h>
   
void error(char* msg)
{
	perror(msg);
	exit(0);
}
  
int main(int argc,char* argv[])
{
	pid_t pid;
	struct sockaddr_in addr_in, cli_addr, serv_addr;
	struct hostent* host;
	int sockfd, newsockfd;
	   
	if(argc<2)
		error("./MSSV <port_no>");
	
	int filter = 0;	
	char *filter_domain = NULL;
	if(argc == 3)
	{
		filter_domain = (char*)malloc(strlen(argv[2]));
		strcpy(filter_domain, argv[2]);
	}	
	  
	printf("\n*****WELCOME TO PROXY SERVER*****\n");
	printf("\nCopyright (c) 2014  GODLY T.ALIAS");
	printf("\nModified by Duy Nguyen - 1312084\n\n");
	printf("Proxy server run on port %s\n",argv[1]);
	   
	//######## Cac buoc khoi tao socket Listening ########
	bzero((char*)&serv_addr,sizeof(serv_addr));
	bzero((char*)&cli_addr, sizeof(cli_addr));
	   
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[1]));
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	   
	//Tao socket!  
	sockfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	
	if(sockfd<0)
		error("Problem in initializing socket");
	   
	if(bind(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0)
		error("Error on binding");
	//########################################################
	  
	//Decrease backlog to 20  
	//listen(sockfd,50);
	listen(sockfd,20);
	int clilen = sizeof(cli_addr);
	  
	 
	accepting:
	
	//Accecpt connections
	//Multi thread cho nay!!
	newsockfd = accept(sockfd,(struct sockaddr*)&cli_addr,&clilen);
	   
	if(newsockfd < 0)
		error("Problem in accepting connection");
	  
	//Fork process!!  
	pid = fork();
	
	//Neu process con dc tao ra!!
	if(pid == 0)
	{
		struct sockaddr_in host_addr;
		int flag = 0, newsockfd1, n, port = 0, i, sockfd1;
		//why 510?
		char buffer[510], t1[300], t2[300], t3[10];
		char method[5];
		char* temp = NULL;
		bzero((char*)buffer, 500);
		recv(newsockfd, buffer, 500, 0);
		   
		//t1 = GET (HEAD, POST)
		//t2 = link
		//t3 = HTTP version
		//Nhan va phan tach request thanh tung phan   
		sscanf(buffer,"%s %s %s",method,t2,t3);
		printf("t2 = %s\n",t2);
		
		//Kiem tra filter case
		if(filter == 1)
		{
			char *p = strstr(t2,filter_domain);
			printf("P - 1 = %c\n",(p-1)[0]);
			printf("P + strlen(filter) + 1 = %c\n", (p + strlen(filter_domain))[0]);
			if( !((char)(p - 1)[0] != "." && (char)(p + strlen(filter_domain))[0] != "."))
			{
				char response[] = "403 (Forbidden) HTTP reponse.";
				send(newsockfd,response,strlen(response),0);
				printf("%s\n",response);
			}
			else
			{
				printf("Filter hop le!\n");
			}
			goto close;
		}	
		
		
		//printf("Method = %s\n",method);
		printf("-----HTML Header-----\n");
		printf("%s\n-----------------\n\n",buffer);
		
		//Chua xoa
		//Cat header
		char *pt;
		pt = strstr(buffer,"\r\n\r\n");
		if (pt != NULL)
			pt += 4;
		char *htmlcontent = (char*) malloc(strlen(buffer) - strlen(pt) + 1);
		strcpy(htmlcontent,pt);
		//printf("HTML Content:\n%s\n",htmlcontent);
		//#########Cat xong header##########
		
		   
		//Them HEAD voi POST vo day!!!
		if ( ( (strncmp(method,"GET",3) == 0) || (strncmp(method,"HEAD",4) == 0 ) || (strncmp(method,"POST",4) == 0 ))
			&& ((strncmp(t3,"HTTP/1.1",8) == 0) || (strncmp(t3,"HTTP/1.0",8) == 0)) && (strncmp(t2,"http://",7) == 0))
		{
			strcpy(t1,t2);
			flag = 0;  
			 
			for (i=7; i<strlen(t2); i++)
			{
				if(t2[i]==':')
				{
					printf("%s\n",t2);
					//Flag nay chi ta?
					flag=1;
					break;
				}
			}
			   
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
			   
			sprintf(t2,"%s",temp);
			printf("host = %s",t2);
			host = gethostbyname(t2);
			   
			if(flag == 1)
			{
				printf("Flag = 1\nTemp = %s\n",temp);
				temp=strtok(NULL,"/");
				port = atoi(temp);
			}
			   
			   
			strcat(t1,"^]");
			temp=strtok(t1,"//");
			temp=strtok(NULL,"/");
			if(temp!=NULL)
			temp=strtok(NULL,"^]");
			printf("\npath = %s\nPort = %d\n",temp,port);
			   
			   
			bzero((char*)&host_addr,sizeof(host_addr));
			host_addr.sin_port=htons(port);
			host_addr.sin_family=AF_INET;
			bcopy((char*)host->h_addr,(char*)&host_addr.sin_addr.s_addr,host->h_length);
			   
			sockfd1 = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
			newsockfd1 = connect(sockfd1,(struct sockaddr*)&host_addr,sizeof(struct sockaddr));
			
			sprintf(buffer,"\nConnected to %s  IP - %s\n",t2,inet_ntoa(host_addr.sin_addr));
			
			if(newsockfd1<0)
				error("Error in connecting to remote server");
			   
			printf("\n%s\n",buffer);
			//send(newsockfd,buffer,strlen(buffer),0);
			bzero((char*)buffer,sizeof(buffer));
			
			if(temp != NULL)
				//sprintf(buffer,"GET /%s %s\r\nHost: %s\r\nConnection: close\r\n\r\n",temp,t3,t2);
				sprintf(buffer,"%s /%s %s\r\nHost: %s\r\nConnection: close\r\n\r\n",method, temp, t3, t2);
			else
				//sprintf(buffer,"GET / %s\r\nHost: %s\r\nConnection: close\r\n\r\n",t3,t2);
				sprintf(buffer,"%s / %s\r\nHost: %s\r\nConnection: close\r\n\r\n",method, t3, t2);
				
			if(htmlcontent!=NULL)
				strcat(buffer,htmlcontent);
			
			n = send(sockfd1,buffer,strlen(buffer),0);
			//printf("\n%s\n",buffer);
			printf("----My proxy request to http server----\n%s\n\n========END======",buffer);
			
			if(n<0)
				error("Error writing to socket");
			else
			{
				do
				{
					bzero((char*)buffer, 500);
					n = recv(sockfd1, buffer, 500, 0);
					if(!(n<=0))
						send(newsockfd, buffer, n, 0);
				}
				while(n>0);
			}
		}
		else
		{
			char response[] = "501 : NOT IMPLEMENTED\nONLY HTTP REQUEST: GET, HEAD, POST ALLOWED\n";
			//send(newsockfd,"400 : BAD REQUEST\nONLY HTTP REQUESTS ALLOWED",18,0);
			send(newsockfd,response,strlen(response),0);
		}
		
		close:
		
		close(sockfd1);
		close(newsockfd);
		close(sockfd);
		_exit(0);
		free(htmlcontent);
	}
	else
	{
		close(newsockfd);
		goto accepting;
	}
	return 0;
}
