#include <ros/ros.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <comm_udp/Roadstate.h>
#include <comm_udp/RoadstateArray.h>
#include <time.h>
#include <thread>

using namespace std;

#define SERVER_PORT 51234
#define SERVER_IP_ADDRESS  "59.110.71.188"
#define CLIENT_PORT 2020
#define TIME 3
#define MAXLEN 1024

unsigned long long ntohll(unsigned long long val)
{
    if (__BYTE_ORDER == __LITTLE_ENDIAN)
    {
        return (((unsigned long long )htonl((int)((val << 32) >> 32))) << 32) | (unsigned int)htonl((int)(val >> 32));
    }
    else if (__BYTE_ORDER == __BIG_ENDIAN)
    {
        return val;
    }
}

unsigned long long htonll(unsigned long long val)
{
    if (__BYTE_ORDER == __LITTLE_ENDIAN)
    {
        return (((unsigned long long )htonl((int)((val << 32) >> 32))) << 32) | (unsigned int)htonl((int)(val >> 32));
    }
    else if (__BYTE_ORDER == __BIG_ENDIAN)
    {
        return val;
    }
}
int read_timeout(int fd,unsigned int wait_seconds);
void sendMsg2Ser(int sock_fd, char* send_buf, struct sockaddr_in addr_serv);
void request(int sock_fd,char* send_buf,struct sockaddr_in addr_serv,char* recv_buf,int len,int recv_num);
void get_time(char* send_buf);

void sendMsg2Ser(int sock_fd, char* send_buf, struct sockaddr_in addr_serv)
{
	while(ros::ok())
	{
		get_time(send_buf);
		int send_num = sendto(sock_fd, send_buf, strlen(send_buf), 0, (struct sockaddr *)&addr_serv, sizeof(addr_serv));
		if(send_num >= 0)
		{
			printf("client send: %s\n", send_buf);
			memset(send_buf, 0, sizeof(send_buf));
			strcat(send_buf,"FC");
		}
		ros::Duration(30).sleep();//30s send one request msg to serv 
	}
}


/* 重复发送消息直到收到回信 */
void request(int sock_fd,char* send_buf,struct sockaddr_in addr_serv,char* recv_buf,int len,int recv_num)
{

	std::cout<<"try to recv now..."<<std::endl;
	recv_num = recvfrom(sock_fd, recv_buf, MAXLEN, 0, (struct sockaddr *)&addr_serv, (socklen_t *)&len);
	if(recv_num < 0)
		std::cout<<"nothing recv!"<<std::endl;
	else
		std::cout<<" recv len :"<< recv_num <<std::endl;
}


/* 获取本地时间 */
void get_time(char* send_buf)
{
	time_t t = time(0);
	char currenttime[15];
	strftime(currenttime, sizeof(currenttime), "%Y%m%d%H%M%S", localtime(&t)); //年-月-日 时-分-秒
	//printf("%s",currenttime);
	strcat(send_buf,currenttime);
	send_buf[16]=0x20;
	send_buf[17]=0x20;
	strcat(send_buf,"IDID");
	send_buf[22]=0x20;
	send_buf[23]=0x20;
	send_buf[24]='\0';
	//printf("%s",send_buf);
}


int main (int argc, char** argv)
{
	ros::init(argc, argv, "udp_client");
	ros::NodeHandle nh;
	comm_udp::Roadstate road_state_;
	comm_udp::RoadstateArray road_states_;
	int num;//路的数量
	char send_buf[25] = "FC";
	char recv_buf[MAXLEN];
	ros::Publisher pub_roadstate=nh.advertise<comm_udp::RoadstateArray>("/roadstate_info",1);
	ros::Rate loop_rate(1.0);
	/* 建立udp socket */
	int sock_fd;//socket文件描述符
	if((sock_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket");
		exit(1);
	}
	struct sockaddr_in addr_serv;
	int len;
	int recv_num;
	memset(&addr_serv, 0, sizeof(addr_serv));
	string ipAdd = SERVER_IP_ADDRESS;
	int convert_ret = inet_pton(AF_INET, ipAdd.c_str(), &addr_serv.sin_addr);
	if( convert_ret != 1 )
		std::cout<<"convert error"<<std::endl;
	//addr_serv.sin_addr.s_addr = inet_addr(SERVER_IP_ADDRESS);
	addr_serv.sin_port = htons(SERVER_PORT);
	addr_serv.sin_family = AF_INET;
	len = sizeof(addr_serv);

	std::thread send_thread = std::thread(std::bind(sendMsg2Ser,sock_fd,send_buf,addr_serv));
	send_thread.detach();

	while(ros::ok())
	{
		request(sock_fd,send_buf,addr_serv,recv_buf,len,recv_num);
		//判断报文的准确性
		if(recv_buf[0]=='F' && recv_buf[1]=='C' && recv_buf[16]=='$')
		{
			num = recv_buf[17];
			road_states_.Roadstates.reserve(100);
			for(int i=0;i<num;i++)
			{
				road_state_.id=*(uint64_t*)&recv_buf[18+i*10];
				road_state_.frontstate=recv_buf[18+i*10+8];
				road_state_.rearstate=recv_buf[18+i*10+9];
				road_states_.Roadstates.push_back(road_state_);	
				std::cout<<htonll(road_state_.id)<<" , "<<road_state_.frontstate << " , " <<road_state_.rearstate<<std::endl;
			}
			pub_roadstate.publish(road_states_);
			loop_rate.sleep();
			printf("pharse success \n");
			//ROS_INFO("roadstate:ID=%d,frontstate=%c,rearstate=%c",road_states_.Roadstates.id,road_states_.Roadstates.frontstate,road_states_.Roadstates.rearstate);
		}
		//printf("client receive: %s\n", recv_buf);
		memset(recv_buf, 0, sizeof(recv_buf));
		recv_num=0;
	}
	close(sock_fd);
	return 0;
}
