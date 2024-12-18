#define _REENTRANT  //函数自动转化为线程安全函数
#include <iostream>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <map>
#include <semaphore.h>
using namespace std;

#define MAXCLIENT 2  //最大连接客户端数量

void addUser(int, char *, map<int, char *> &); //用户上线
void removeUser(int, map<int, char *> &); //用户下线
int findUser(const char *, const map<int, char *> &); //用户查找
char *findName(int, map<int, char *> &); //用户名查找
void showUser(char *, const map<int, char *> &); //展示在线用户
void *contact(void *); //线程的主函数

map<int, char *> users; //记录客户端套接字和用户的对应关系，设置为全局变量
sem_t modmap, cnum;  //声明信号量，一个用于修改字典，一个用于修改客户端数量

int main() {
	char *ip = "127.0.0.1";
	char *port = "9955";
	int opt = 1;

	pthread_t pcontact;

	sem_init(&modmap, 0, 1); //初始化信号量
	sem_init(&cnum, 0, MAXCLIENT);

	int server_socket, client_socket;
	struct sockaddr_in server_addr, client_addr;
	socklen_t server_addr_len = sizeof(server_addr), client_addr_len = sizeof(client_addr);

	server_socket = socket(PF_INET, SOCK_STREAM, 0);
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (void *)&opt, sizeof(opt)); //创建套接字并避免timeout

	memset(&server_addr, 0, sizeof(server_addr)); //设置地址
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(atoi(port));

	if (bind(server_socket, (struct sockaddr *)&server_addr, server_addr_len) == -1) //分配套接字地址
		cout << "bind error" << endl;

	if (listen(server_socket, 5) == -1) //开启监听
		cout << "listen error" << endl;

	while (1) {
		client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);

		sem_wait(&cnum);  //客户端数量不为最大值则创建线程
		pthread_create(&pcontact, NULL, contact, (void *)&client_socket); //创建线程
		pthread_detach(pcontact);  //线程结束时销毁线程
		cout << "connect..." << client_socket << endl;
	}

	close(server_socket);
	sem_destroy(&modmap);  //销毁信号量
	sem_destroy(&cnum);
	return 0;
}

void *contact(void *arg) {
	int client_socket = *((int *)arg);
	int to;   //接收方
	char choice;  //记录客户端请求的服务，0：注册、1：通信、2：查看在线用户、3：下线
	char name[20];  //要通信的用户名
	char msg[1000]; //信息
	char buf[1024]; //存储所有数据

	while (1) {
		memset(buf, 0, sizeof(buf));
		recv(client_socket, buf, sizeof(buf), 0);
		choice = buf[0];
		switch (choice) {
			case '0':
				strncpy(name, &buf[1], 20);
				if (findUser(name, users) == -1) { //检查是否已经登陆
					addUser(client_socket, name, users); //添加用户
					send(client_socket, "log in", 7, 0);
				} else
					send(client_socket, "already log in", 15, 0);
				continue;
			case '1':
				strncpy(name, &buf[1], 20);
				strncpy(msg, &buf[21], 1000);
				to = findUser(name, users);
				if (to == -1)
					send(client_socket, "not found user", 15, 0);
				else {
					int len = 0;   //构造消息
					char *fromname = findName(client_socket, users);
					strncpy(&buf[len], fromname, strlen(fromname));
					len += strlen(fromname);
					buf[len++] = ':';
					strncpy(&buf[len], msg, strlen(msg));
					if (send(to, buf, sizeof(buf), 0) == -1)
						send(client_socket, "user gone", 10, 0);
					else
						send(client_socket, "send successfully", 18, 0);
				}
				continue;
			case '2':
				showUser(buf, users); //展示用户
				send(client_socket, buf, sizeof(buf), 0);
				continue;
			case '3':
				removeUser(client_socket, users); //删除用户
				send(client_socket, "log out", 8, 0);
				close(client_socket);
				sem_post(&cnum);     //信号量加
				return NULL;
		}
	}
	return NULL;
}

void addUser(int client_socket, char *username, map<int, char *> &users) {
	char *name = new char(20);    //创建一个新地址存放名字并存储到map字典中
	strcpy(name, username);
	if (users.count(client_socket))
		return;
	else {
		sem_wait(&modmap);   //信号量
		users.insert(pair<int, char *>(client_socket, name));
		cout << "client " << username << " log in" << endl;
		sem_post(&modmap);
	}
}

void removeUser(int client_socket, map<int, char *> &users) {
	if (users.count(client_socket)) {
		sem_wait(&modmap);   //信号量
		delete[] users.find(client_socket)->second;   //释放内存
		users.erase(client_socket);
		cout << "client " << client_socket << " log out" << endl;
		sem_post(&modmap);
	} else
		return;
}

int findUser(const char *username, const map<int, char *> &users) {
	sem_wait(&modmap);   //信号量
	map<int, char *>::const_iterator it = users.begin();
	for (; it != users.end(); it++) {
		if (!strcmp(it->second, username)) {
			sem_post(&modmap);
			return it->first;
		}
	}
	sem_post(&modmap);
	return -1;
}

void showUser(char *buf, const map<int, char *> &users) {
	int len = 0;
	sem_wait(&modmap);   //信号量
	map<int, char *>::const_iterator it = users.begin();
	for (; it != users.end(); it++) {
		strncpy(&buf[len], it->second, strlen(it->second));
		len += strlen(it->second);
		buf[len++] = ' ';
	}
	sem_post(&modmap);
}

char *findName(int client_socket, map<int, char *> &users) {
	sem_wait(&modmap);   //信号量
	map<int, char *>::iterator it;
	it = users.find(client_socket);
	sem_post(&modmap);
	return it->second;
}