#include <iostream>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#pragma warning(disable:4996)

using namespace std;

vector<SOCKET> Users_Connected;
mutex mtx;
int counter = 0;



void ClientHandler(SOCKET clientsocket) {

	while (true) {
		int msg_size = 0;
		int result = recv(clientsocket, (char*)&msg_size, sizeof(int), NULL);//принятие размера сообщения
		if (result <= 0) {
			cout << "User " << clientsocket << " disconected or error.";
			mtx.lock();
			for (auto it = Users_Connected.begin(); it != Users_Connected.end(); ++it) {
				if (*it == clientsocket) {
					Users_Connected.erase(it);
					break;
				}
			}
			counter--;
			mtx.unlock();
			closesocket(clientsocket);
			break;
		}
		char* msg = new char[msg_size + 1];//выделение памяти под сообщение
		msg[msg_size] = '\0';// для конца строки
		result = recv(clientsocket, msg, msg_size, NULL);//Принятие самого сообщения ///////Точка ошибки
		if (result <= 0) {
			cout << "User " << clientsocket << " disconected or error." << endl;
			mtx.lock();
			for (auto it = Users_Connected.begin(); it != Users_Connected.end(); ++it) {
				if (*it == clientsocket) {
					Users_Connected.erase(it);
					break;
				}
			}
			counter--;
			mtx.unlock();
			closesocket(clientsocket);
			delete[] msg;
			break;
		}
		mtx.lock();
		for (SOCKET other_client : Users_Connected) {
			if (other_client == clientsocket) continue;
			if (other_client == INVALID_SOCKET) continue;
			send(other_client, (char*)&msg_size, sizeof(int), NULL);//отправка размера сообщения
			send(other_client, msg, msg_size, NULL);//отправка сообщения
		}
		mtx.unlock();
		delete[] msg;
	}

}

int main() {

	WSAData wsadata;
	WORD WinSockVer = MAKEWORD(2, 2);

	if (WSAStartup(WinSockVer, &wsadata) != 0) {
		cout << "Error";
		exit(1);
	}

	SOCKADDR_IN addr;
	int size_of_len = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(7777);
	addr.sin_family = AF_INET;

	SOCKET slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	bind(slisten, (SOCKADDR*)&addr, sizeof(addr));
	listen(slisten, SOMAXCONN);
	cout << "Server start listen client..." << endl;

	SOCKET newconnection;
	vector<thread> client_threads;
	while (true) {
		newconnection = accept(slisten, (SOCKADDR*)&addr, &size_of_len);
		if (newconnection == INVALID_SOCKET)
			cout << "User is not connected." << endl;
		else {
			cout << "New User connected." << endl;
			mtx.lock();
			Users_Connected.push_back(newconnection);
			counter++;
			mtx.unlock();
			client_threads.emplace_back(ClientHandler, newconnection);
			client_threads.back().detach();
		}
	}

	WaitForMultipleObjects(counter, NULL, TRUE, INFINITY);
	closesocket(slisten);
	WSACleanup();


	return 0;
}
