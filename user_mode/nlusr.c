#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/socket.h>
#include <mysql/mysql.h>
//#include "zlibTool.h"

#define MAX_PAYLOAD 1024 /*消息最大负载为1024字节*/
#define CON_START 5
#define ADD_PATTERN 6

struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int sock_fd=-1;
struct msghdr msg;

int sendto_kernel(char *content, int type) {

    memset(nlh,0,NLMSG_SPACE(MAX_PAYLOAD));
    /*设置Netlink的消息内容(跳过消息头部)，来自我们命令行输入的第一个参数*/
    strcpy(NLMSG_DATA(nlh), content);
    /* 填充Netlink消息头部 */
    nlh->nlmsg_len = strlen(content) + NLMSG_HDRLEN;
    nlh->nlmsg_pid = getpid();
    nlh->nlmsg_type = type; //指明我们的Netlink是消息负载是一条空消息
    nlh->nlmsg_flags = 0;

    /*这个是模板，暂时不用纠结为什么要这样用。有时间详细讲解socket时再说*/
    memset(&iov, 0, sizeof(iov));
    iov.iov_base = (void *)nlh;
    iov.iov_len = nlh->nlmsg_len;

    memset(&msg, 0, sizeof(msg));
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    int len = sendmsg(sock_fd, &msg, 0); //通过Netlink socket向内核发送消息
    if (len <= 0) {
        printf("send to kernel error!\n");
        return -1;
    }
    else {
        printf("send to kernel successful!\tlen:%d\n", len);
        return 1;
    }
}

void view(char *source, int len) {
    int i = 0;
    for (i = 0;i < len;i++)
        printf("%c", source[i]);
    printf("\n");
}
int receive_from_kernel() {
    //设置为最大长度，否则可能接受的数据是上次向内核发送的数据长度
    nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
    iov.iov_len = NLMSG_SPACE(MAX_PAYLOAD);
    recvmsg(sock_fd, &msg, 0);
    printf("Recive from kernel:%s\n len:%d\n pid:%d\n seq:%d\n flags:%d\n", NLMSG_DATA(nlh), (nlh->nlmsg_len - NLMSG_HDRLEN), (nlh->nlmsg_pid), (nlh->nlmsg_seq), (nlh->nlmsg_flags));

    view(NLMSG_DATA(nlh), (nlh->nlmsg_len - NLMSG_HDRLEN));
    return 1;
}

int search_mysql(char *str) {
	MYSQL *mysql;
	mysql = mysql_init(NULL);
	if (!mysql_real_connect(mysql, "localhost", "root", "123", "students", 3306, NULL, 0)) {
        printf("连接错误:%s!\n",  mysql_error(mysql));
        return -1;
	}
	mysql_query(mysql, "set names utf8");
    if (mysql_query(mysql, "select * from student")) {
        printf("查询失败:%s!\n", mysql_error(mysql));
        return -1;
    }
    MYSQL_RES *result = mysql_store_result(mysql);
    MYSQL_ROW row;
    int lie = mysql_num_fields(result);
    int hang = mysql_num_rows(result);
    int j = 0;
    while (row = mysql_fetch_row(result)) {
        for (j = 0;j < lie;j++){
            printf("%s\t", row[j]);
        }
        printf("\n");
        strcpy(str, row[0]);
    }
    printf("行数:%d\t列数:%d\n", hang, lie);
    mysql_free_result(result);
    mysql_close(mysql);
	return 0;
}

int main(int argc, char* argv[])
{

    if(-1 == (sock_fd=socket(PF_NETLINK, SOCK_RAW, NETLINK_NETFILTER))){ //创建套接字
            perror("can't create netlink socket!");
            return 1;
    }

    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();

    if(-1 == bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr))){
          perror("can't bind sockfd with sockaddr_nl!");
          return 1;
    }

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0; /*我们的消息是发给内核的*/
    dest_addr.nl_groups = 0; /*在本示例中不存在使用该值的情况*/

    //将套接字和Netlink地址结构体进行绑定
    if(NULL == (nlh=(struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD)))){
          perror("alloc mem failed!");
          return -1;
    }

    char *str = malloc(sizeof(char)*20);
    memset(str, 0, 20);
    search_mysql(str);
    printf("%s\n", str);
    sendto_kernel(str, ADD_PATTERN);
    free(str);
    while (1)
        receive_from_kernel();

    //控制url过滤启动
//    char content[] = "hello kernel";
//    sendto_kernel(content, CON_START);
//    while (1)
//        receive_from_kernel();

    close(sock_fd);
    free(nlh);
    return 0;
}
