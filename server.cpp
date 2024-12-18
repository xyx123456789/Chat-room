#define _REENTRANT  //�����Զ�ת��Ϊ�̰߳�ȫ����
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

#define MAXCLIENT 2  //������ӿͻ�������

void addUser(int, char *, map<int, char *> &); //�û�����
void removeUser(int, map<int, char *> &); //�û�����
int findUser(const char *, const map<int, char *> &); //�û�����
char *findName(int, map<int, char *> &); //�û�������
void showUser(char *, const map<int, char *> &); //չʾ�����û�
void *contact(void *); //�̵߳�������

map<int, char *> users; //��¼�ͻ����׽��ֺ��û��Ķ�Ӧ��ϵ������Ϊȫ�ֱ���
sem_t modmap, cnum;  //�����ź�����һ�������޸��ֵ䣬һ�������޸Ŀͻ�������

int main() {
	char *ip = "127.0.0.1";
	char *port = "9955";
	int opt = 1;

	pthread_t pcontact;

	sem_init(&modmap, 0, 1); //��ʼ���ź���
	sem_init(&cnum, 0, MAXCLIENT);

	int server_socket, client_socket;
	struct sockaddr_in server_addr, client_addr;
	socklen_t server_addr_len = sizeof(server_addr), client_addr_len = sizeof(client_addr);

	server_socket = socket(PF_INET, SOCK_STREAM, 0);
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (void *)&opt, sizeof(opt)); //�����׽��ֲ�����timeout

	memset(&server_addr, 0, sizeof(server_addr)); //���õ�ַ
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(atoi(port));

	if (bind(server_socket, (struct sockaddr *)&server_addr, server_addr_len) == -1) //�����׽��ֵ�ַ
		cout << "bind error" << endl;

	if (listen(server_socket, 5) == -1) //��������
		cout << "listen error" << endl;

	while (1) {
		client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);

		sem_wait(&cnum);  //�ͻ���������Ϊ���ֵ�򴴽��߳�
		pthread_create(&pcontact, NULL, contact, (void *)&client_socket); //�����߳�
		pthread_detach(pcontact);  //�߳̽���ʱ�����߳�
		cout << "connect..." << client_socket << endl;
	}

	close(server_socket);
	sem_destroy(&modmap);  //�����ź���
	sem_destroy(&cnum);
	return 0;
}

void *contact(void *arg) {
	int client_socket = *((int *)arg);
	int to;   //���շ�
	char choice;  //��¼�ͻ�������ķ���0��ע�ᡢ1��ͨ�š�2���鿴�����û���3������
	char name[20];  //Ҫͨ�ŵ��û���
	char msg[1000]; //��Ϣ
	char buf[1024]; //�洢��������

	while (1) {
		memset(buf, 0, sizeof(buf));
		recv(client_socket, buf, sizeof(buf), 0);
		choice = buf[0];
		switch (choice) {
			case '0':
				strncpy(name, &buf[1], 20);
				if (findUser(name, users) == -1) { //����Ƿ��Ѿ���½
					addUser(client_socket, name, users); //����û�
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
					int len = 0;   //������Ϣ
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
				showUser(buf, users); //չʾ�û�
				send(client_socket, buf, sizeof(buf), 0);
				continue;
			case '3':
				removeUser(client_socket, users); //ɾ���û�
				send(client_socket, "log out", 8, 0);
				close(client_socket);
				sem_post(&cnum);     //�ź�����
				return NULL;
		}
	}
	return NULL;
}

void addUser(int client_socket, char *username, map<int, char *> &users) {
	char *name = new char(20);    //����һ���µ�ַ������ֲ��洢��map�ֵ���
	strcpy(name, username);
	if (users.count(client_socket))
		return;
	else {
		sem_wait(&modmap);   //�ź���
		users.insert(pair<int, char *>(client_socket, name));
		cout << "client " << username << " log in" << endl;
		sem_post(&modmap);
	}
}

void removeUser(int client_socket, map<int, char *> &users) {
	if (users.count(client_socket)) {
		sem_wait(&modmap);   //�ź���
		delete[] users.find(client_socket)->second;   //�ͷ��ڴ�
		users.erase(client_socket);
		cout << "client " << client_socket << " log out" << endl;
		sem_post(&modmap);
	} else
		return;
}

int findUser(const char *username, const map<int, char *> &users) {
	sem_wait(&modmap);   //�ź���
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
	sem_wait(&modmap);   //�ź���
	map<int, char *>::const_iterator it = users.begin();
	for (; it != users.end(); it++) {
		strncpy(&buf[len], it->second, strlen(it->second));
		len += strlen(it->second);
		buf[len++] = ' ';
	}
	sem_post(&modmap);
}

char *findName(int client_socket, map<int, char *> &users) {
	sem_wait(&modmap);   //�ź���
	map<int, char *>::iterator it;
	it = users.find(client_socket);
	sem_post(&modmap);
	return it->second;
}