#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>


#define SERVER_SLEEP_TIME 50
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 27000

bool InitializeWindowsSockets();

int __cdecl main(int argc, char **argv) 
{
    SOCKET connectSocket = INVALID_SOCKET;
    int iResult;
    char *messageToSend = "This is a test message from a Subscriber.";
   

    if(InitializeWindowsSockets() == false)
		return 1;

    //CREATING A SOCKET
    connectSocket = socket(AF_INET,
                           SOCK_STREAM,
                           IPPROTO_TCP);

    if (connectSocket == INVALID_SOCKET)
    {
        printf("socket - [SUBSCRIBER] failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    
    //CREATING AND INITIALISING ADDRESS STRUCT
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(DEFAULT_PORT);
    
    //CONNECTING TO ENGINE
    if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
    {
        printf("Unable - [SUBSCRIBER] to connect to server.\n");
        closesocket(connectSocket);
        WSACleanup();
    }
 
    //SETTING UP NON-BLOCKING MODE
    unsigned long int nonBlockingMode = 1;
    iResult = ioctlsocket(connectSocket, FIONBIO, &nonBlockingMode);
    while (1)
    {
        
        FD_SET set;
        timeval timeVal;

        FD_ZERO(&set);
        FD_SET(connectSocket, &set);

        timeVal.tv_sec = 0;
        timeVal.tv_usec = 0;

        iResult = select(0, NULL, &set, NULL, &timeVal);

        if (iResult == SOCKET_ERROR)
        {
            fprintf(stderr, "select - [SUBSCRIBER] failed with error: %ld\n", WSAGetLastError());
            continue;
        }

        if (iResult == 0)
        {
            Sleep(SERVER_SLEEP_TIME);
            continue;
        }

        //MESSAGE PREPARATION
        iResult = send(connectSocket, messageToSend, (int)strlen(messageToSend) + 1, 0);

        if (iResult == SOCKET_ERROR)
        {
            printf("send - [SUBSCRIBER] failed with error: %d\n", WSAGetLastError());
            closesocket(connectSocket);
            WSACleanup();
            return 1;
        }

        printf("[SUBSCRIBER] - Message Sent.\n");
    
    }
    closesocket(connectSocket);
    WSACleanup();

    return 0;
}

bool InitializeWindowsSockets()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
    {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return false;
    }
	return true;
}
