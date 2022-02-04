#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>


#define SERVER_SLEEP_TIME 50
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT_RECV 27000
#define DEFAULT_PORT_SEND 26000

bool InitializeWindowsSockets();
DWORD WINAPI ReceiveMessage(LPVOID lpParameter);
DWORD WINAPI MessageSend(LPVOID lpParameter);

struct Parameters
{
    SOCKET connectSocket;
    bool end;
};

int __cdecl main(int argc, char **argv) 
{
    SOCKET connectSocketRecv = INVALID_SOCKET;
    SOCKET connectSocketSend = INVALID_SOCKET;
    int iResult;
    char recvbufSubscriber[DEFAULT_BUFLEN];

    HANDLE threadRecv;
    HANDLE threadSend;
    DWORD  threadRecvID;
    DWORD  threadSendID;
   
    //flag for gracefull shutdown
    bool end = false;

    if(InitializeWindowsSockets() == false)
		return 1;

    //CREATING A SOCKET
    connectSocketRecv = socket(AF_INET,
                           SOCK_STREAM,
                           IPPROTO_TCP);
    connectSocketSend = socket(AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP);

    if (connectSocketRecv == INVALID_SOCKET)
    {
        printf("socket - [SUBSCRIBER] failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    if (connectSocketSend== INVALID_SOCKET)
    {
        printf("socket - [SUBSCRIBER] failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    
    //CREATING AND INITIALISING ADDRESS STRUCT
    sockaddr_in serverAddressRecv;
    serverAddressRecv.sin_family = AF_INET;
    serverAddressRecv.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddressRecv.sin_port = htons(DEFAULT_PORT_RECV);

    sockaddr_in serverAddressSend;
    serverAddressSend.sin_family = AF_INET;
    serverAddressSend.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddressSend.sin_port = htons(DEFAULT_PORT_SEND);
    
    //CONNECTING TO ENGINE
    if (connect(connectSocketRecv, (SOCKADDR*)&serverAddressRecv, sizeof(serverAddressRecv)) == SOCKET_ERROR)
    {
        printf("Unable - [SUBSCRIBER] to connect to server.\n");
        closesocket(connectSocketRecv);
        WSACleanup();
    }

    if (connect(connectSocketSend, (SOCKADDR*)&serverAddressSend, sizeof(serverAddressSend)) == SOCKET_ERROR)
    {
        printf("Unable - [SUBSCRIBER] to connect to server.\n");
        closesocket(connectSocketSend);
        WSACleanup();
    }
 
    //SETTING UP NON-BLOCKING MODE
    unsigned long int nonBlockingMode = 1;
    iResult = ioctlsocket(connectSocketRecv, FIONBIO, &nonBlockingMode);
    if (iResult == SOCKET_ERROR)
    {
        printf("connect - [SUBSCRIBER] failed with error: %d\n", WSAGetLastError());
        closesocket(connectSocketRecv);
        WSACleanup();
        return 1;
    }

    iResult = ioctlsocket(connectSocketSend, FIONBIO, &nonBlockingMode);
    if (iResult == SOCKET_ERROR)
    {
        printf("connect - [SUBSCRIBER] failed with error: %d\n", WSAGetLastError());
        closesocket(connectSocketSend);
        WSACleanup();
        return 1;
    }
   
    //    do
    //    {
    //        FD_SET set;
    //        timeval timeVal;

    //        FD_ZERO(&set);
    //        FD_SET(connectSocket, &set);

    //        timeVal.tv_sec = 0;
    //        timeVal.tv_usec = 0;

    //        iResult = select(0, &set, NULL, NULL, &timeVal);

    //        if (iResult == SOCKET_ERROR)
    //        {
    //           /* if (WSAGetLastError() == 10093)
    //                Sleep(1000);
    //            else*/
    //                fprintf(stderr, "select- [SUBSCRIBER] failed with error: %ld\n", WSAGetLastError());
    //            continue;
    //        }

    //        if (iResult == 0)
    //        {
    //            Sleep(SERVER_SLEEP_TIME);
    //            continue;
    //        }

    //        iResult = recv(connectSocket, recvbufSubscriber, 1, 0);

    //        if (iResult > 0)
    //        {
    //            printf("Message - received from PUBLISHER: %c\n", recvbufSubscriber[0]);
    //        }
    //        else if (iResult == 0)
    //        {
    //            printf("Connection with SUBSCRIBER closed.\n");
    //            closesocket(connectSocket);
    //        }
    //        else
    //        {
    //            printf("recv failed with error: %d\n", WSAGetLastError());
    //            closesocket(connectSocket);
    //        }
    //    } while (iResult > 0);
    //}
   
    Parameters threadParamsRecv;
    threadParamsRecv.connectSocket = connectSocketRecv;
    threadParamsRecv.end = end;
    Parameters* pointerThreadParamRecv = &threadParamsRecv;

    Parameters threadParamsSend;
    threadParamsSend.connectSocket = connectSocketSend;
    threadParamsSend.end = end;
    Parameters* pointerThreadParamSend = &threadParamsSend;
    //    
    threadSend = CreateThread(NULL, 0, &MessageSend, (LPVOID)pointerThreadParamSend, 0, &threadSendID);
    threadRecv = CreateThread(NULL, 0, &ReceiveMessage, (LPVOID)pointerThreadParamRecv, 0, &threadRecvID);
    //
    //char c = getchar();
    //if (c == 'q')
    //{
    //    end = true;
    //}
    if (threadSend)
        WaitForSingleObject(threadSend, INFINITE);
    if (threadRecv)
        WaitForSingleObject(threadRecv, INFINITE);
    char c = getchar();
    closesocket(connectSocketRecv);
    closesocket(connectSocketSend);
    CloseHandle(threadSend);
    CloseHandle(threadRecv);
   // CloseHandle(threadRecv);
   // CloseHandle(threadSend);
    WSACleanup();
    return 0;
}

DWORD WINAPI ReceiveMessage(LPVOID lpParameter)
{
    int iResult;
    char* recvBuffer = (char*)malloc(4);
    int headerLength = 0;
    int messageLength = 0;

    SOCKET connectSocket = ((Parameters*)lpParameter)->connectSocket;
    bool end = ((Parameters*)lpParameter)->end;

  /*  while (end == false)
    {*/
    while (1)
    {
        do
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

            iResult = recv(connectSocket, recvBuffer, 2, 0);

            if (iResult > 0)
            {
                printf("Message - received from PUBLISHER: %c\n", recvBuffer[0]);
            }
            else if (iResult == 0)
            {
                printf("Connection with SUBSCRIBER closed.\n");
                closesocket(connectSocket);
            }
            else
            {
                printf("recv failed with error: %d\n", WSAGetLastError());
                closesocket(connectSocket);
            }

            //int iResult = 0;


            //while (headerLength < 4)
            //{
            //    // Receive data until the client shuts down the connection
            //    iResult = recv(connectSocket, recvBuffer, 4, 0);
            //    if (iResult > 0)
            //    {

            //        headerLength += iResult;

            //    }
            //    else if (iResult == 0)
            //    {
            //        // connection was closed gracefully
            //        printf("Connection with client closed.\n");
            //        closesocket(connectSocket);
            //        break;
            //    }
            //    else
            //    {
            //        // there was an error during recv
            //        printf("recv failed with error: %d\n", WSAGetLastError());
            //        closesocket(connectSocket);
            //        break;
            //    }
            //}
            //if (headerLength == 4)
            //{
            //    messageLength = *((int*)recvBuffer);
            //}

            //headerLength = 0;
            //iResult = 0;
            //recvBuffer = (char*)realloc(recvBuffer, messageLength);

            //while (headerLength < messageLength)
            //{
            //    // Receive data until the client shuts down the connection
            //    iResult = recv(connectSocket, recvBuffer, messageLength, 0);
            //    if (iResult > 0)
            //    {

            //        headerLength += iResult;

            //    }
            //    else if (iResult == 0)
            //    {
            //        // connection was closed gracefully
            //        printf("Connection with client closed.\n");
            //        closesocket(connectSocket);
            //        break;
            //    }
            //    else
            //    {
            //        // there was an error during recv
            //        printf("recv failed with error: %d\n", WSAGetLastError());
            //        closesocket(connectSocket);
            //        break;
            //    }
            //}

            //if (headerLength == messageLength)
            //{
            //    printf("Recv message from Publisher -> %s\n", recvBuffer);
            //}


        } while (iResult > 0);
    }
  
   /* }*/

      // char c = getchar();


    return 0;
}

DWORD WINAPI MessageSend(LPVOID lpParameter)
{

    char* messageToSend = "This is a test message from a Subscriber.";
    int iResult;

    SOCKET connectSocket = ((Parameters*)lpParameter)->connectSocket;
    bool end = ((Parameters*)lpParameter)->end;

  /*  while (end == false)
    {*/
   /* while (1)
    {*/
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
            //continue;
        }

        if (iResult == 0)
        {
            Sleep(SERVER_SLEEP_TIME);
            //continue;
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

        char c = getchar();

   // }

    //}

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
