#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include<regex.h>
#include<memory.h>

#include <ifaddrs.h>

#include <time.h>
#include <sys/vfs.h>
#include <signal.h>
#include <sys/stat.h>


/* define macros*/
#define MAXBUF 1024
# define STDIN_FILENO 0
# define STDOUT_FILENO 1

/* define FTP reply code */
# define USERNAME 220
# define PASSWORD 331
# define LOGIN 230
# define PATHNAME 257
# define CLOSEDATA 226
# define ACTIONOK 250
# define PASVMODE 227
# define ACTVMODE 666
# define BINARY 911
# define ASCII 726
#define PUT 888
#define GET 999
#define LIST 987
#define REGET 998
#define UNIX 1000
/* DefinE global variables */
char  host[30]; /* hostname or dotted-decimal string */
char port[10];
char  read_buffer[MAXBUF],  read_buffer1[MAXBUF]; /* pointer that is malloc'ed */
char  write_buffer[MAXBUF],  write_buffer1[MAXBUF]; /* pointer that is malloc'ed */
struct sockaddr_in servaddr;
int server_system =0;

int cliopen(char * host, char * port);
void strtosrv(char * str, char * host, char * port);
int mode;
int a_or_b_mode;
int speed;
void cmd_tcp(int sockfd);
void ftp_list(int sockfd);
void ftp_get(int sck, char * pDownloadFileName_s);
void ftp_put(int sck, char * pUploadFileName_s);
void ftp_get_limit(int sck, char * pDownloadFileName_s,int speed );
void ftp_put_limit(int sck, char * pDownloadFileName_s,int speed );
int startListening(int controlFd);
char *getFileContent(char *fname,int *length);
void getParaName(char *str, char * paraName,int *length);
void getSecondParaName(char *str, char * paraName,int *length);
void ftp_reget(int sck, char * pUploadFileName_s);
int fd;
int data_fd;
int listen_fd;
char file_name_to_transfer[100];
char file_name_to_retransmit[100];
int offset;



int passive_transfer_mode;
int main(int argc, char * argv[]) {
	

	if (2 != argc) {
		printf("%s\n", "missing <hostname>");
		exit(0);
	}
	strcpy(host ,argv[1]);
	
	strcpy(port , "21");

	/*****************************************************************
	//1. code here: Allocate the read and write buffers before open().
	*****************************************************************/

	fd = cliopen(host, port);
	cmd_tcp(fd);

	exit(0);
}


/* Establish a TCP connection from client to server */
int cliopen(char * host, char * port) {
	/*************************************************************
	//2. code here 
	*************************************************************/
    struct hostent *hptr;
    char   ip_addr[32];

    if((hptr = gethostbyname(host)) == NULL)
    {
        printf(" gethostbyname error for host:\n");
        return 0; 
    }
    inet_ntop(hptr->h_addrtype, hptr->h_addr, ip_addr, sizeof(ip_addr));
          

    struct sockaddr_in server_addr;
    int sockfd;
    while((sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1);
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(port));
    server_addr.sin_addr.s_addr=inet_addr(ip_addr);
    bzero(&(server_addr.sin_zero), 8);
    while(connect(sockfd,(struct sockaddr*)&server_addr,sizeof(struct sockaddr)) == -1);
    
    return sockfd;
}


/*
   Compute server's port by a pair of integers and store it in char *port
   Get server's IP address and store it in char *host
*/

char *strrpc(char *str,char *oldstr,char *newstr){
    char bstr[strlen(str)];//转换缓冲区
    memset(bstr,0,sizeof(bstr));
    int i;
    for( i = 0;i < strlen(str);i++){
        if(!strncmp(str+i,oldstr,strlen(oldstr))){//查找目标字符串
            strcat(bstr,newstr);
            i += strlen(oldstr) - 1;
        }else{
        	strncat(bstr,str + i,1);//保存一字节进缓冲区
	    }
    }
 
    strcpy(str,bstr);
    return str;
}

void strtosrv(char * bematch, char * host, char * port) {

    //  char * bematch = "227 Entering Passive Mode (10,3,255,85,42,90).\r\n";
     char * pattern = "^.*\\(([0-9]{1,3}).([0-9]{1,3}).([0-9]{1,3}).([0-9]{1,3}).([0-9]{1,3}).([0-9]{1,3}).*$";
     char errbuf[1024];
     char match[100];
     char ip_str[16]="";
     char port_str[6]="";
     int port_num=0;
     regex_t reg;
     int err, nm = 10;
     regmatch_t pmatch[nm];

     if (regcomp( & reg, pattern, REG_EXTENDED) < 0) {
         regerror(err, & reg, errbuf, sizeof(errbuf));
         printf("err:%s\n", errbuf);
     }

     err = regexec( & reg, bematch, nm, pmatch, 0);

     if (err == REG_NOMATCH) {
         printf("no match\n");
         exit(-1);
     } else if (err) {
         regerror(err, & reg, errbuf, sizeof(errbuf));
         printf("err:%s\n", errbuf);
         exit(-1);
     }
    int i;
     for (i = 0; i < 10 && pmatch[i].rm_so != -1; i++) {
         int len = pmatch[i].rm_eo - pmatch[i].rm_so;
         if (len) {
             memset(match, '\0', sizeof(match));
             memcpy(match, bematch + pmatch[i].rm_so, len);
            if(i==1){
                strcat(ip_str,match); 
                strcat(ip_str,"."); 
             }
            if(i==2){
                strcat(ip_str,match); 
                strcat(ip_str,"."); 
             }
            if(i==3){
                strcat(ip_str,match); 
                strcat(ip_str,"."); 
             }
            if(i==4){
                strcat(ip_str,match); 
                
             }

             if(i==5){
                port_num+=atoi(match)*256;
                
             }
             if(i==6){
                port_num+=atoi(match);
             }
            
         }

         
    }
    sprintf(port_str,"%d",port_num);
    strcpy(host,ip_str);
    strcpy(port,port_str);
    return;
 }


/* Read and write as command connection */
void cmd_tcp(int sockfd) {
	int maxfdp1, nread, nwrite, fd, replycode;
	
	fd_set rset;

	FD_ZERO( & rset);
	maxfdp1 = sockfd + 1; /* check descriptors [0..sockfd] */

	for (;;) {
		// FD_SET(data_fd,&wset);
		FD_SET(STDIN_FILENO, & rset);
		FD_SET(sockfd, & rset);

		if (select(maxfdp1, & rset, NULL, NULL, NULL) < 0)
			printf("select error\n");


	

		/* data to read on stdin */
		if (FD_ISSET(STDIN_FILENO, & rset)) {
			if ((nread = read(STDIN_FILENO, read_buffer, MAXBUF)) < 0)
				printf("read error from stdin\n");
           
            read_buffer[nread]='\0';
			/* send username to server */
			if (replycode == USERNAME) {
            
				sprintf(write_buffer, "USER %s\n", read_buffer);
				
                nwrite = nread + 5;
				write(sockfd, write_buffer, strlen(write_buffer));
				
                continue;
			}

			/*************************************************************
			//4. code here: send password
			*************************************************************/
            // if(replycode==PASSWORD){
            //     // sprintf(write_buffer, "PASS %s", read_buffer);
				
			// 	sprintf(write_buffer, "PASS student\n");
            //     nwrite = nread + 5;
			// 	if (write(sockfd, write_buffer, 13) != 13)
			// 		printf("write error\n");  
			// 	continue;  
            // }

			/* send command */
			if (replycode == LOGIN || replycode == CLOSEDATA || replycode == PATHNAME 
			|| replycode == ACTIONOK|| replycode==PASVMODE ) {
				/* ls - list files and directories*/
				if (strncmp(read_buffer, "ls", 2) == 0) {
					// sprintf(write_buffer, "%s", );
					if(passive_transfer_mode){
						write(sockfd, "PASV\n", 5);
					}
					else{
						listen_fd=startListening(sockfd);
					}
					
					// replycode=PASVMODE;
					sprintf(write_buffer, "%s", "LIST -al\n");
					write(sockfd, write_buffer, 9);
					mode=LIST;
					
					continue;
				}

				

				


				if (strncmp(read_buffer, "passive", 7) == 0) {
					
					if(passive_transfer_mode){
						
						write(STDOUT_FILENO,"Passive mode off\n",17);
						
					}else{
						write(STDOUT_FILENO,"Passive mode on\n",16);
					}
					
					passive_transfer_mode=!passive_transfer_mode;
					continue;
				}
			

                  


				/*************************************************************
				// 5. code here: cd - change working directory/
				*************************************************************/
				if (strncmp(read_buffer, "cd", 2) == 0) {
					int length=0;
					char paraName[100]="";
					getParaName(read_buffer,paraName,&length);
					if(length==0){
						write(STDOUT_FILENO,"please input the directory name\n",32);
						continue;
					}
					sprintf(read_buffer,"CWD %s\n",paraName);
					write(sockfd, read_buffer, length+5);
					continue;
				}



				/* pwd -  print working directory */
				if (strncmp(read_buffer, "pwd", 3) == 0) {
					sprintf(write_buffer, "%s", "XPWD\n");
					write(sockfd, write_buffer, 5);
					continue;
				}

				if (strncmp(read_buffer, "mkdir", 5) == 0) {
					int length=0;
					char paraName[100]="";
					getParaName(read_buffer,paraName,&length);
					if(length==0){
						write(STDOUT_FILENO,"please input the directory name\n",32);
						continue;
					}
					sprintf(read_buffer,"MKD %s\n",paraName);
					write(sockfd, read_buffer, length+5);
					continue;
				}

				if (strncmp(read_buffer, "delete", 6) == 0) {
					int length=0;
					char paraName[100]="";
					getParaName(read_buffer,paraName,&length);
					if(length==0){
						write(STDOUT_FILENO,"please input the file name\n",27);
						continue;
					}
					sprintf(read_buffer,"DELE %s\n",paraName);
					write(sockfd, read_buffer, length+6);
					continue;
				}

				if (strncmp(read_buffer, "rename", 6) == 0) {
					int length1=0;
					char oldName[100]="";
					getParaName(read_buffer,oldName,&length1);

					int length2=0;
					char newName[100]="";
					getSecondParaName(read_buffer,newName,&length2);

					if(length1==0){
						write(STDOUT_FILENO,"please input the old file name and new file name\n",49);
						continue;
					}
					if(length2==0){
						write(STDOUT_FILENO,"please input the new file name\n",31);
						continue;
					}
					
					sprintf(read_buffer,"RNFR %s\n",oldName);
					write(sockfd, read_buffer, length1+6);
					
					
					sprintf(read_buffer,"RNTO %s\n",newName);
					write(sockfd, read_buffer, length2+6);
					continue;
				}


				/*************************************************************
				// 6. code here: quit - quit from ftp server
				*************************************************************/
				if (strncmp(read_buffer, "quit", 4) == 0) {
					
					write(sockfd, "QUIT\n", 5);
					continue;
				}

				if (strncmp(read_buffer, "ascii", 5) == 0) {
					a_or_b_mode=ASCII;
					write(sockfd, "TYPE A\n", 7);
					continue;
				}
				if (strncmp(read_buffer, "binary", 6) == 0) {
					a_or_b_mode=BINARY;
					write(sockfd, "TYPE I\n", 7);
					continue;
				}
				/*************************************************************
				// 7. code here: get - get file from ftp server
				*************************************************************/
				if (strncmp(read_buffer, "get", 3) == 0) {
					char *str=read_buffer;

					
					int length=0;
					char fileName[100]="";
					getParaName(read_buffer,fileName,&length);
					if(length==0){
						write(STDOUT_FILENO,"please input the file name\n",27);
						continue;
					}
					int length2=0;
					char speedStr[20];
					getSecondParaName(read_buffer,speedStr,&length2);
					if(length2==0){
						speed=-1;	
					}else{
						speed=atoi(speedStr);
						char buf[30];

						sprintf(buf,"speed in strncmp(read_buffer, get, 3):%d\n",speed);
						write(1,buf,strlen(buf));
					}

					FILE *fpRead = fopen(fileName, "r");
						if(fpRead!=NULL){
							fclose(fpRead);
							write(1,"The file already exists, do you want to overwrite it?(y/n) ",59);
							char option;
							option=getchar();
							fgetc(stdin);
							if(option=='y'||option=='Y'){
								
							}else{
								continue;
							}

						} 

				
					strcpy(file_name_to_transfer,fileName);
	
					if(passive_transfer_mode){
						write(sockfd, "PASV\n", 5);
					}
					else{
						listen_fd=startListening(sockfd);
					}
					
					sprintf(read_buffer,"RETR %s\n",fileName);
					write(sockfd, read_buffer, length+6);
					mode=GET;
					continue;
				}

				/*************************************************************
				// 8. code here: put -  put file upto ftp server
				*************************************************************/
				if (strncmp(read_buffer, "put", 3) == 0) {
					
					int length=0;
					char fileName[100]="";
					getParaName(read_buffer,fileName,&length);
					if(length==0){
						write(STDOUT_FILENO,"please input the file name\n",27);
						continue;
					}
					
					strcpy(file_name_to_transfer,fileName);
					
					char speedStr[10]="";
					int length2=0;
					getSecondParaName(read_buffer,speedStr,&length2);
					if(length2==0){
						speed=-1;	
					}else{
						speed=atoi(speedStr); 
					}

					if(passive_transfer_mode){
						write(sockfd, "PASV\n", 5);
					}
					else{
						listen_fd=startListening(sockfd);
					}
					
					sprintf(read_buffer,"STOR %s\n",fileName);
					write(sockfd, read_buffer, length+6);
					mode=PUT;
					continue;
				}

				if(strncmp(read_buffer, "reget", 5) == 0){

    				
					write(sockfd, "PASV\n", 5);
					sprintf(read_buffer,"REST %d\n",offset);
					write(sockfd, read_buffer, strlen(read_buffer));
					
					sprintf(read_buffer,"RETR %s\n",file_name_to_retransmit);
					int nnwrite=write(sockfd, read_buffer, strlen(read_buffer));
					
					mode=REGET;
					continue;

					
				}
				// write(sockfd, read_buffer, nread);

			}

		
		if (strncmp(read_buffer, "\n", 1) == 0) {
			continue;
		}

		write(STDOUT_FILENO,"invaild command\n",17);

		}

		/* data to read from socket */
		if (FD_ISSET(sockfd, &rset)) {
			if ((nread = recv(sockfd, read_buffer, MAXBUF, 0)) < 0){
                printf("read_buffer:%d\n",sockfd);
				printf("recv error\n");
            }
			else if (nread == 0) break;
            read_buffer[nread]='\0';
			

			
			

			/* set replycode and wait for user's input */
			if (strncmp(read_buffer, "220", 3) == 0 || strncmp(read_buffer, "530", 3) == 0) {
				strcat(read_buffer, "your name: ");
				nread += 12;
				replycode = USERNAME;
                
			}

            if (strncmp(read_buffer, "331", 3) == 0 ) {
				
				// replycode = PASSWORD;
                // strcat(read_buffer,"your password:");
				char *password=getpass("your password:");
				
				sprintf(read_buffer,"PASS %s\n",password);
				write(sockfd,read_buffer,strlen(read_buffer));
                continue;
			}


            if (strncmp(read_buffer, "230", 3) == 0 ) {
				
				replycode = LOGIN;
                strcpy(read_buffer,"login successful!\n");
                nread=18;
				passive_transfer_mode=1;
				write(sockfd,"SYST\n",5);
				write(sockfd, "TYPE I\n", 7);
				
                
			}

				
			if(strncmp(read_buffer, "215 UNIX", 8) == 0){
				
				server_system=UNIX;
			}		


			/*************************************************************
			// 9. code here: handle other response coming from server
			*************************************************************/

			/* open data connection*/
			if(strncmp(read_buffer, "200 PORT", 8) == 0){

				struct sockaddr_in client_addr;
    			memset(&client_addr, 0x00, sizeof(client_addr));
        		int addrLen = sizeof(client_addr);
				


				if(mode==GET){
					int nread1;
					nread1=recv(sockfd, read_buffer1, MAXBUF, 0) ;
					
					if(strncmp(read_buffer1,"550",3)==0){
						close(listen_fd);	
						write(STDOUT_FILENO, read_buffer1, strlen(read_buffer1));
						continue;		
					}else{	
						data_fd = accept(listen_fd, (struct sockaddr * ) &client_addr, (socklen_t*)(&addrLen)); //create a new socket
						printf("Accept client: %s \n",inet_ntoa(client_addr.sin_addr));
						write(STDOUT_FILENO, "PORT mode\n", 10);
						if(speed==-1){
							ftp_get(data_fd,file_name_to_transfer);
						}else{
							printf("speed :%d",speed);
							ftp_get_limit(data_fd,file_name_to_transfer,speed);
						}
						

					}
					
				}	


				


				if(mode==PUT){
					int nread1;
					nread1=recv(sockfd, read_buffer1, MAXBUF, 0) ;
					if(strncmp(read_buffer1,"553",3)==0){
						close(listen_fd);			
					}else{	
						data_fd = accept(listen_fd, (struct sockaddr * ) &client_addr, (socklen_t*)(&addrLen)); //create a new socket
						printf("Accept client: %s \n",inet_ntoa(client_addr.sin_addr));
						write(STDOUT_FILENO, "PORT mode\n", 10);
						if(speed==-1){
						ftp_put(data_fd,file_name_to_transfer);
						}else{
							ftp_put_limit(data_fd,file_name_to_transfer,speed);
						}
					}
					


					
					
				}
				if(mode==LIST){
					data_fd = accept(listen_fd, (struct sockaddr * ) &client_addr, (socklen_t*)(&addrLen)); //create a new socket
				printf("Accept client: %s \n",inet_ntoa(client_addr.sin_addr));
				write(STDOUT_FILENO, "PORT mode\n", 10);
					ftp_list(data_fd);
				}
				

				if (write(STDOUT_FILENO, read_buffer1, strlen(read_buffer1)) != strlen(read_buffer1))
						printf("write error to stdout\n");

					
				close(listen_fd);
				mode=0;
				replycode =CLOSEDATA;
				
				
				continue;


			}
			
		
			
			if (strncmp(read_buffer, "227", 3) == 0) {
				char host[20]="";
				char port[10]=""; 
				strtosrv(read_buffer, host,  port) ;
				data_fd = cliopen(host, port);
				
				write(STDOUT_FILENO, "PASV mode\n", 10);
				

				if(mode==GET){
					int nread1;
					nread1=recv(sockfd, read_buffer1, MAXBUF, 0) ;
					
					if(strncmp(read_buffer1,"550",3)==0){
		
						close(data_fd);
						
									
					}else{	

						if(speed==-1){
							ftp_get(data_fd,file_name_to_transfer);
						}else{
							printf("speed :%d Bytes/s\n",speed);
							ftp_get_limit(data_fd,file_name_to_transfer,speed);
						}
						

					}
				}	

				if(mode==REGET){
					if(speed==-1){
						ftp_reget(data_fd,file_name_to_retransmit);
					// }else{
					// 	ftp_put_limit(data_fd,file_name_to_transfer,speed);
					// }
					}
				}
				

				if(mode==PUT){
					if(speed==-1){
						ftp_put(data_fd,file_name_to_transfer);
					}else{
						ftp_put_limit(data_fd,file_name_to_transfer,speed);
					}
					
				}
				



				if(mode==LIST){
					
					ftp_list(data_fd);
				}
				

				if (write(STDOUT_FILENO, read_buffer1, strlen(read_buffer1)) != strlen(read_buffer1))
						printf("write error to stdout\n");

					
				
				mode=0;
				replycode =CLOSEDATA;
				speed=-1;
				
				continue;
			}

			 /* start data transfer */
			if (write(STDOUT_FILENO, read_buffer, nread) != nread)
				printf("write error to stdout\n");
			speed=-1;
		}
	}


	if (close(sockfd) < 0)
		printf("close error\n");
}


/* Read and write as data transfer connection */

void ftp_list(int data_fd) {
	

	int nread=0;
	char read_buffer2[100];
	bzero(read_buffer2, sizeof(read_buffer2));  
	while((nread = read(data_fd, read_buffer2, sizeof(read_buffer2)))>0)  {
		/* data to read from socket */

		if (write(STDOUT_FILENO, read_buffer2, nread) != nread){
			printf("send error to stdout\n");
		}
			



	 }
	
	close(data_fd);
	
}


/* download file from ftp server */
void ftp_get(int sck, char * pDownloadFileName_s) {
	
	FILE *fp = fopen(pDownloadFileName_s, "w"); 
	
    if (fp == NULL)  
    {  
        printf("File:\t%s Can Not Open To Write!\n", pDownloadFileName_s);  
    }  

	bzero(read_buffer1, sizeof(read_buffer1));  
    int readNum = 0;  
	
    while((readNum = read(sck, read_buffer1, sizeof(read_buffer1)))>0)  
    {  
		
        if (readNum < 0)  
        {  
            printf("Recieve Data From Server Failed!\n");  
            break;  
        }  
		
		if(a_or_b_mode==ASCII && server_system!=UNIX){
			strrpc(read_buffer1,"\r","");
		}

        int write_length = fwrite(read_buffer1, sizeof(char), readNum, fp); 
		
        if (write_length < readNum)  
        {  
            printf("File: Write Failed!\n");  
            break;  
        }  
       bzero(read_buffer1, sizeof(read_buffer1));  
	  
    }

	fclose(fp);  
    if (close(sck) < 0)
		printf("close error\n"); 

}


void ftp_reget(int sck, char *pDownloadFileName_s) {
	
	FILE *fp = fopen(pDownloadFileName_s, "a"); 
	
    if (fp == NULL)  
    {  
        printf("File:\t%s Can Not Open To Write!\n", pDownloadFileName_s);  
    }  

	bzero(read_buffer1, sizeof(read_buffer1));  
    int readNum = 0;  
	
    while((readNum = read(sck, read_buffer1, sizeof(read_buffer1)))>0)  
    {  
		
        if (readNum < 0)  
        {  
            printf("Recieve Data From Server Failed!\n");  
            break;  
        }  
		
        int write_length = fwrite(read_buffer1, sizeof(char), readNum, fp); 
		
        if (write_length < readNum)  
        {  
            printf("File: Write Failed!\n");  
            break;  
        }  
       bzero(read_buffer1, sizeof(read_buffer1));  
	  
    }

	fclose(fp);  
    if (close(sck) < 0)
		printf("close error\n");
}


void ftp_get_limit(int sck, char * pDownloadFileName_s,int speed ) {
	
	FILE *fp = fopen(pDownloadFileName_s, "w"); 
	
    if (fp == NULL)  
    {  
        printf("File:\t%s Can Not Open To Write!\n", pDownloadFileName_s);  
    }  
	char read_buffer1[speed/10];
	bzero(read_buffer1, sizeof(read_buffer1));  
    int readNum = 0;  
	int i=0;
	const char *lable = "|/-\\";


	fd_set rset;
	FD_ZERO( & rset);
	char read_buffer3[20];
	struct timeval timeout;
	timeout.tv_sec=0;
	timeout.tv_usec=0;
	int interruptedPos=0;

    while((readNum = read(sck, read_buffer1, sizeof(read_buffer1)))>0)  
    {  
		
		interruptedPos+=sizeof(read_buffer1);
        printf("[%c]\r", lable[i++ % 4]);
		fflush(stdout);
        if (readNum < 0)  {

            printf("Recieve Data From Server Failed!\n");  
            break;  
        }  
		
        int write_length = fwrite(read_buffer1, sizeof(char), readNum, fp); 
		
        if (write_length < readNum)  
        {  
            printf("File: Write Failed!\n");  
            break;  
        }  
       bzero(read_buffer1, sizeof(read_buffer1)); 
	   usleep(100000);


	   FD_SET(STDIN_FILENO, & rset);
		if (select(STDIN_FILENO+1, & rset, NULL, NULL, &timeout) < 0){
			printf("select error\n");
		}
			
		if (FD_ISSET(STDIN_FILENO, & rset)){
			printf("transfer interrupted\n");
			strcpy(file_name_to_retransmit,pDownloadFileName_s);
			offset=interruptedPos;  
			break;
			
		} 
	  
    }
	printf("\n");

	fclose(fp);  
    if (close(sck) < 0)
		printf("close error\n"); 
}



/* upload file to ftp server */
void ftp_put(int sck, char * pUploadFileName_s) {


	int length=0;
	char *file_content=getFileContent(pUploadFileName_s, &length);
	if(a_or_b_mode==ASCII && server_system!=UNIX){
			strrpc(file_content,"\n","\r\n");
	}
	 if( write(sck, file_content, length) < 0){  
        printf("send msg error\n");  
     
     } 
	free(file_content);
	if (close(sck) < 0)
		printf("close error\n");
}



void ftp_put_limit(int sck, char * pUploadFileName_s,int speed) {


	int length=0;
	char *file_content=getFileContent(pUploadFileName_s, &length);
	char *file_content_ori=file_content;


	float totaltime=length/speed; 
	float interval=totaltime/50;	
	int packageSize=length/50;
	int i = 1;
 
    char bar[52];
    const char *lable = "|/-\\";
	
    while (i <= 50)
    {
		
		

		if( write(sck, file_content, packageSize) < 0){
			printf("send msg error\n");  
		}
		file_content+=packageSize; 
		
		

        
        printf("[%-50s][%d%%][%c]\r", bar, 2*i, lable[i % 4]);
        fflush(stdout);
        bar[i] = '#';
        i++;
        bar[i] = 0;
        usleep(interval*1000000);

        
    }

	write(sck, file_content, length-50*packageSize);
	
	
    printf("\n");

	free(file_content_ori);
	if (close(sck) < 0)
		printf("close error\n");

}



char *getFileContent(char *fname,int *length){

    int  fileLight = 0;
    char *pBuf;                  //定义文件指针    
    FILE *pFile;
    
    pFile = fopen(fname,"r");    //获取文件的指针
    if(pFile == NULL)
    {
        printf(" \nOpen file  fail\n");
        return    NULL;    
    }

    fseek(pFile,0,SEEK_END);      //把指针移动到文件的结尾 ，获取文件长度
    fileLight = ftell(pFile);     //获取文件长度
   
    pBuf =(char *)malloc((fileLight+1)*(sizeof(char)));  

    rewind(pFile);                 //把指针移动到文件开头 因为我们一开始把指针移动到结尾，如果不移动回来 会出错
    fread(pBuf,1,fileLight,pFile); //读文件
    pBuf[fileLight]='\0';             //把读到的文件最后一位 写为0 要不然系统会一直寻找到0后才结束
    fclose(pFile);               // 关闭文件    
    *length = fileLight;
    return pBuf;
}

//get    abc.txt

//mkdir    qwe
//cd 
//cd zxcv
void getParaName(char *str, char * paraName,int *length){
	int nameStart=0;
	for(nameStart=0;str[nameStart]!=' ';nameStart++);
	for(nameStart;str[nameStart]==' ';nameStart++);
    if(nameStart>strlen(str)){
            strcpy(paraName,"");
            *length=0;
            return;
        }
	int nameEnd=0;
	for(nameEnd=nameStart;str[nameEnd]!=' '&&str[nameEnd]!='\0'&&str[nameEnd]!='\n';nameEnd++);
	nameEnd--;
	char fileName[100]="";
	strncpy(paraName,str+nameStart,nameEnd-nameStart+1);
	*length=nameEnd-nameStart+1;
}


//rename oldname newname
//hgf kjghfg kjghfg
//jhgffdgb   hgfdgb   765ryttg
void getSecondParaName(char *str, char * paraName,int *length){
	int nameStart=0;
	for(nameStart=0;str[nameStart]!=' ';nameStart++);
	for(nameStart;str[nameStart]==' ';nameStart++);
	for(nameStart;str[nameStart]!=' ';nameStart++);
	for(nameStart;str[nameStart]==' ';nameStart++);
    if(nameStart>strlen(str)){
            strcpy(paraName,"");
            *length=0;
            return;
        }
	int nameEnd=0;
	for(nameEnd=nameStart;str[nameEnd]!=' '&&str[nameEnd]!='\0'&&str[nameEnd]!='\n';nameEnd++);
	nameEnd--;
	char fileName[100]="";
	strncpy(paraName,str+nameStart,nameEnd-nameStart+1);
	*length=nameEnd-nameStart+1;
}



void getLocalIP (char *ip) 
{
 
    struct ifaddrs * ifAddrStruct=NULL;
    void * tmpAddrPtr=NULL;
 
    getifaddrs(&ifAddrStruct);
 
    while (ifAddrStruct!=NULL) 
	{
        if (ifAddrStruct->ifa_addr->sa_family==AF_INET)
		{   // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            if(strncmp(ifAddrStruct->ifa_name,"eth0",4)==0){
                strcpy(ip,addressBuffer);
                
                return;
            }
        }
		
        ifAddrStruct = ifAddrStruct->ifa_next;
    }
    return ;

}

int startListening(int controlFd){
	srand(time(NULL));
	int port=(rand()%64511)+1025;
	int p1=port/256;
	int p2=port-p1*256;
	char localIP[30];
	getLocalIP (localIP) ;
	strrpc(localIP,".",",");
	sprintf(read_buffer1,"PORT %s,%d,%d\n",localIP,p1,p2);
	write(controlFd,read_buffer1,strlen(read_buffer1));
	int data_fd;
	int listen_fd;

    struct sockaddr_in server_addr;
    
    char buff[BUFSIZ];

    memset( &server_addr, 0, sizeof(server_addr)); /*Zero out structure*/
    server_addr.sin_family = AF_INET; /* Internet addr family */
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); /*Server IP address*/
    server_addr.sin_port = htons(port); /* Server port */
    
	listen_fd  = socket(AF_INET, SOCK_STREAM, 0); // create the socket
   
    /* Bind to the local address */
    if ((bind(listen_fd, (struct sockaddr * ) &server_addr,
            sizeof(server_addr))) < 0){
                printf("bind() failed.\n");
            }
	printf("bind success!\n");		
   
    
    listen(listen_fd, 20); //listen


    return listen_fd;
}

