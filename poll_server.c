#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include <sys/ipc.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>

#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include <errno.h>
#define SERVERPORT 6666
#define MAXLINE 128
#define MAXSize 1024
int main(int argc, char* argv[])
{
    int listen_fd,connect_fd;//两个套接字
    char rbuf[MAXSize];
    int addrlen;   
    int rc=0;                //return code
    int i,n;
    int ready_num;
    struct sockaddr_in serv_addr, client_addr;
    struct pollfd event_set[MAXLINE];//事件结构体数组

    //准备结构体
    serv_addr.sin_family = AF_INET;//地址族
    serv_addr.sin_port = SERVERPORT;//端口号
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);//IPV4下设为任意连接
    memset(serv_addr.sin_zero, 0, 8);//置0 大小和sockaddr 一样

    //创建socket
    if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("create socket error");
    }
    //绑定bind
    if((rc = bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr))) < 0)
    {
        perror("bind error");
    }
    //监听listen
    if((rc = listen(listen_fd, MAXLINE)) < 0)
    {
        perror("listen error");
    }
    //准备事件结构体
    event_set[0].fd = listen_fd;
    event_set[0].events = POLLIN ;//想要检测的事件
    for(i = 1; i<MAXLINE; i++)
    {
        event_set[i].fd =  -1;//把没用的置为-1
    }
    while(1)
    {
        printf("等待事件中......\n");
        if((ready_num = poll(event_set, MAXLINE, -1)) < 0)
        {
            perror("poll error");
        }
        if(event_set[0].revents & POLLIN)//有可读事件
        {
            //连接accept
            connect_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &addrlen);
            printf("Client_IP:%d\n",ntohl(client_addr.sin_addr.s_addr));
            for(i = 1; i < MAXLINE; i++)//找到空位存储新的连接fd并设置监听的事件
            {
                if(event_set[i].fd <= 0)
                {
                    event_set[i].fd = connect_fd;
                    event_set[i].events = POLLIN;
                    printf("新的socket：%d 已连接\n",connect_fd);
                    break;
                }
            }
            if(i ==MAXLINE)//满了
            {
                printf("socket存储满了");
            }
            ready_num--;
            if(ready_num == 0)
            continue;
        }
        //遍历数组找到未处理事件
        for(i = 1; i < MAXLINE ; i++)
        {
            int socket_fd = event_set[i].fd;
            if(socket_fd  < 0) //小于0就没必要处理
            continue;
            if(event_set[i].revents & POLLIN)//连接fd有数据可读
            {
                memset(rbuf,0,MAXLINE);
                if( (n = recv(socket_fd, rbuf, MAXSize, 0)) > 0)
                {
                    printf("收到：%s\n",rbuf);
                    send(socket_fd, rbuf, MAXSize, 0);
                }else if(n == 0)
                {
                    printf("客户端：%d 准备断开连接\n",socket_fd);
                    event_set[i].fd = -1;
                    close(socket_fd);
                }else{
                    perror("recv error");
                }
                ready_num--;
                if(ready_num <=0)
                break;
            }
        }

    }
    return 0;
}