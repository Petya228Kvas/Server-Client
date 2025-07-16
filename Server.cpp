#include <iostream>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <string>
#include <vector>
#pragma warning(disable:4996)

using namespace std;

vector<SOCKET> Users_Connected;
int counter;


enum Packet {
	P_ChatMessage
};

bool ProcessPacket(int index, Packet packettype) {
	switch (packettype){
	case P_ChatMessage:
	{
		int msg_size = 0;
		int result = recv(Users_Connected[index], (char*)&msg_size, sizeof(int), NULL);//принятие размера сообщения

		if (result <= 0) {
			cout << "User" << index << "disconected or error.";
			closesocket(Users_Connected[index]);
			Users_Connected.erase(Users_Connected.begin() + index);
			counter--;
			return false;
		}

		char* msg = new char[msg_size + 1];//выделение памяти под сообщение
		msg[msg_size] = '\0';// для конца строки
		result = recv(Users_Connected[index], msg, msg_size, NULL);//Принятие самого сообщения

		if (result <= 0) {
			cout << "User" << index << "disconected or error." << endl;
			closesocket(Users_Connected[index]);
			Users_Connected.erase(Users_Connected.begin() + index);
			counter--;
			delete[] msg;
			return false;
		}

		for (int i = 0; i < counter; i++) {
			if (i == index ) continue;
			if (Users_Connected[i] == INVALID_SOCKET) continue;
			Packet msgpacket = P_ChatMessage;
			send(Users_Connected[i], (char*)&msgpacket, sizeof(Packet), NULL);
			send(Users_Connected[i], (char*)&msg_size, sizeof(int), NULL);//отправка размера сообщения
			send(Users_Connected[i], msg, msg_size, NULL);//отправка сообщения
		}
		delete[] msg;
		break;
	}
	default:
		cout << "Unknown packet: " << packettype << endl;
	}
	return true;
}

void ClientHandler(int index) {

	Packet packettype;
	while (true) {
		int result = recv(Users_Connected[index], (char*)&packettype, sizeof(Packet), NULL);
		if (result <= 0) {
			cout << "User " << index << " disconected or error."<< endl;
			closesocket(Users_Connected[index]);
			Users_Connected.erase(Users_Connected.begin() + index);
			counter--;
			break;
		}
		if (!ProcessPacket(index, packettype)) {
			break;
		}
	}
	//closesocket(Users_Connected[index]);
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
	while(true) {
		newconnection = accept(slisten, (SOCKADDR*)&addr, &size_of_len);
		if (newconnection == INVALID_SOCKET)
			cout << "User is not connected." << endl;
		else {
			cout << "New User connected." << endl;
			Users_Connected.push_back(newconnection);
			counter++;
			CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandler, LPVOID(counter-1), NULL, NULL);
		}
	}
	
	WaitForMultipleObjects(counter, NULL, TRUE, INFINITY);
	closesocket(slisten);
	WSACleanup();


	return 0;
}