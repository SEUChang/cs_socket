#include <ros/ros.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
 
#define MYPORT  8080
#define BUFFER_SIZE 1024
char* SERVER_IP = "192.168.11.128"; 
int i=17;
 
int main (int argc, char** argv)
{
  ros::init(argc, argv, "client_node");
  ros::NodeHandle nh;
    ///定义sockfd
    int sock_cli = socket(AF_INET,SOCK_STREAM, 0);
    
    ///定义sockaddr_in
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(MYPORT);  ///服务器端口
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);  ///服务器ip
    
    printf("connect %s  %d\n",SERVER_IP,MYPORT);
    ///连接服务器，成功返回0，错误返回-1
    if (connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("connect");
        exit(1);
    }
    printf("connected successfully\n");
    char sendbuf[BUFFER_SIZE];
    char recvbuf[BUFFER_SIZE];

   //一应一答
   /* while (fgets(sendbuf, sizeof(sendbuf), stdin) != NULL)
    {
        printf("send data to server：%s\n",sendbuf);
        send(sock_cli, sendbuf, strlen(sendbuf),0); ///发送
        if(strcmp(sendbuf,"exit\n")==0)
            break;
        recv(sock_cli, recvbuf, sizeof(recvbuf),0); ///接收
        printf("receive data from server：%s\n",recvbuf);
        
        memset(sendbuf, 0, sizeof(sendbuf));
        memset(recvbuf, 0, sizeof(recvbuf));
    }*/


    while (ros::ok())
    {
        recv(sock_cli, recvbuf, sizeof(recvbuf),0); ///接收

	if(recvbuf[0]=='F' && recvbuf[1]=='C' && recvbuf[16]=='$')
	{
		printf("receive data from TCP server：%s\n",recvbuf);
		for(i;recvbuf[i]!=0;i=i+10)
		{
		ROS_INFO("ID%c%c%c%c%c%c%c%c front:%c ;rear:%c",recvbuf[i],recvbuf[i+1],recvbuf[i+2],recvbuf[i+3],recvbuf[i+4],recvbuf[i+5],recvbuf[i+6],recvbuf[i+7],recvbuf[i+8],recvbuf[i+9]);
		}

		i=17;

	}

       
        memset(recvbuf, 0, sizeof(recvbuf));
    }
    
    close(sock_cli);
    return 0;
}
