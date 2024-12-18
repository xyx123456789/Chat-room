#define _REENTRANT  //�����Զ�ת��Ϊ�̰߳�ȫ����
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
	cin >> username;    //�����û���

	char *ip = "127.0.0.1";
	char *port = "9955";

	pthread_t pread, pcontact;  //��������д���߳�ID����
	void *t_res;   //��¼�̷߳���ֵ

	int sock = socket(PF_INET, SOCK_STREAM, 0); //����TCP�׽���
	struct sockaddr_in server_addr;

	memset(&server_addr, 0, sizeof(server_addr)); //��д��������ַ��Ϣ
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(atoi(port));

	if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) //��������
		cout << "connect error" << endl;

	pthread_create(&pcontact, NULL, contact, (void *)&sock); //�����߳�
	pthread_create(&pread, NULL, read, (void *)&sock);
	pthread_join(pcontact, &t_res); //��ΪҪ�ȵ������̶߳�������close�����Ա�����join
	pthread_join(pread, &t_res);

	close(sock);
	return 0;
}

void *read(void *arg) {
	int client_socket = *((int *)arg);
	char buf[1024];
	while (recv(client_socket, buf, sizeof(buf), 0)) //����˹ر��׽��ֺ󣬿ͻ��˹رս���
		cout << buf << endl;
	return NULL;
}

void *contact(void *arg) {
	int client_socket = *((int *)arg);
	char choice;  //����˹���ѡ��
	char name[20];  //���շ�����
	char msg[1000];  //��Ϣ
	char buf[1024];  //����˴�����Ϣ
	while (1) {
		memset(buf, 0, sizeof(buf));
		cout << "0:login | 1:send | 2:show | 3:logout: " << endl;
		cin >> choice;
		buf[0] = choice;
		switch (choice) {
			case '0':   //��ʾ��½�������˷����Լ�������
				strncpy(&buf[1], username, 20);
				send(client_socket, buf, sizeof(buf), 0);
				break;
			case '1':   //��ʾͨ�ţ�����������ͶԷ������ֺ���Ϣ
				cout << "input name: " << endl;
				cin >> name;
				cout << "input msg: " << endl;
				cin >> msg;
				strncpy(&buf[1], name, 20);
				strncpy(&buf[21], msg, 1000);
				send(client_socket, buf, sizeof(buf), 0);
				break;
			case '2':   //��ʾ������������������û�
				send(client_socket, buf, sizeof(buf), 0);
				break;
			case '3':   //��ʾ��֪����������
				send(client_socket, buf, sizeof(buf), 0);
				shutdown(client_socket, SHUT_WR);  //ֻ�ر������
				return NULL;
		}
	}
	return NULL;
}