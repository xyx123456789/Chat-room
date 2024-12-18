#define _REENTRANT  //函数自动转化为线程安全函数
#include <iostream>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
using namespace std;

void *read(void *);
void *contact(void *);

char username[20];

int main() {
	cout << "input name(less than 20 B): ";
	cin >> username;    //输入用户名

	char *ip = "127.0.0.1";
	char *port = "9955";

	pthread_t pread, pcontact;  //创建读和写的线程ID变量
	void *t_res;   //记录线程返回值

	int sock = socket(PF_INET, SOCK_STREAM, 0); //创建TCP套接字
	struct sockaddr_in server_addr;

	memset(&server_addr, 0, sizeof(server_addr)); //填写服务器地址信息
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(atoi(port));

	if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) //请求连接
		cout << "connect error" << endl;

	pthread_create(&pcontact, NULL, contact, (void *)&sock); //创建线程
	pthread_create(&pread, NULL, read, (void *)&sock);
	pthread_join(pcontact, &t_res); //因为要等到两个线程都结束才close，所以必须用join
	pthread_join(pread, &t_res);

	close(sock);
	return 0;
}

void *read(void *arg) {
	int client_socket = *((int *)arg);
	char buf[1024];
	while (recv(client_socket, buf, sizeof(buf), 0)) //服务端关闭套接字后，客户端关闭接收
		cout << buf << endl;
	return NULL;
}

void *contact(void *arg) {
	int client_socket = *((int *)arg);
	char choice;  //服务端功能选项
	char name[20];  //接收方名字
	char msg[1000];  //信息
	char buf[1024];  //服务端传回信息
	while (1) {
		memset(buf, 0, sizeof(buf));
		cout << "0:login | 1:send | 2:show | 3:logout: " << endl;
		cin >> choice;
		buf[0] = choice;
		switch (choice) {
			case '0':   //表示登陆，向服务端发送自己的名字
				strncpy(&buf[1], username, 20);
				send(client_socket, buf, sizeof(buf), 0);
				break;
			case '1':   //表示通信，向服务器发送对方的名字和信息
				cout << "input name: " << endl;
				cin >> name;
				cout << "input msg: " << endl;
				cin >> msg;
				strncpy(&buf[1], name, 20);
				strncpy(&buf[21], msg, 1000);
				send(client_socket, buf, sizeof(buf), 0);
				break;
			case '2':   //表示请求服务器返回在线用户
				send(client_socket, buf, sizeof(buf), 0);
				break;
			case '3':   //表示告知服务器下线
				send(client_socket, buf, sizeof(buf), 0);
				shutdown(client_socket, SHUT_WR);  //只关闭输出流
				return NULL;
		}
	}
	return NULL;
}