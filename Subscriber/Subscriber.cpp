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
void parseRecievedData(SOCKET connectSocket);

int __cdecl main(int argc, char **argv) 
{
    SOCKET connectSocket = INVALID_SOCKET;
    int iResult;
   // char recvBuf[DEFAULT_BUFLEN];
    char* recvBuffer = (char*)malloc(4);
    int helpLength = 0;
    int messageLength = 0;
   

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
 
    while(1)
    {
        parseRecievedData(connectSocket);

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

// PARSING MESSAGE SIZE AND CONTENT
void parseRecievedData(SOCKET connectSocket)
{
    int iResult;
    // char recvBuf[DEFAULT_BUFLEN];
    char* recvBuffer = (char*)malloc(4);
    int helpLength = 0;
    int messageLength = 0;

    //SUBSCRIBER RECEIVE
    while (helpLength < 4)
    {
        FD_SET set;
        timeval timeVal;

        FD_ZERO(&set);
        FD_SET(connectSocket, &set);

        timeVal.tv_sec = 0;
        timeVal.tv_usec = 0;

        iResult = select(0, &set, NULL, NULL, &timeVal);

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
        iResult = recv(connectSocket, recvBuffer, 4, 0);
        if (iResult > 0)
        {

            helpLength += iResult;

        }
        else if (iResult == 0)
        {
            printf("Connection with client closed.\n");
            closesocket(connectSocket);
            break;
        }
        else
        {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(connectSocket);
            break;
        }
        if (helpLength == 4)
        {
            messageLength = *((int*)recvBuffer);
        }
    }
    helpLength = 0;
    iResult = 0;
    recvBuffer = (char*)realloc(recvBuffer, messageLength);

    while (helpLength < messageLength)
    {
        FD_SET set;
        timeval timeVal;

        FD_ZERO(&set);
        FD_SET(connectSocket, &set);

        timeVal.tv_sec = 0;
        timeVal.tv_usec = 0;

        iResult = select(0, &set, NULL, NULL, &timeVal);

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
        iResult = recv(connectSocket, recvBuffer, messageLength, 0);
        if (iResult > 0)
        {

            helpLength += iResult;

        }
        else if (iResult == 0)
        {
            printf("Connection with client closed.\n");
            closesocket(connectSocket);
            break;
        }
        else
        {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(connectSocket);
            break;
        }

    }

    if (helpLength == messageLength)
    {
        printf("Message received from Publisher : %s\n", recvBuffer);
    }

}
