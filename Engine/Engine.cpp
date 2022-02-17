#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_BUFLEN 512
#define SERVER_SLEEP_TIME 50
#define SUBSCRIBER_PORT "27000"
#define PUBLISHER_PORT "28000"

bool InitializeWindowsSockets();
DWORD WINAPI ReceiveMessageProducer(LPVOID lpParameter);
DWORD WINAPI ReceiveMessageConsumer(LPVOID lpParameter);

struct Parameters
{
    SOCKET listenSocketPublisher;
    SOCKET acceptedSocketPublisher;
};

int main(void) 
{
    HANDLE threadProducer;
    DWORD threadIDProducer;

    HANDLE threadConsumer;
    DWORD threadIDConsumer;

    SOCKET listenSocketSubscriber = INVALID_SOCKET;
    SOCKET listenSocketPublisher = INVALID_SOCKET;
    SOCKET acceptedSocketSubscriber = INVALID_SOCKET;
    SOCKET acceptedSocketPublisher = INVALID_SOCKET;
    int iResultSubscriber;
    int iResultPublisher;
    char recvbufSubscriber[DEFAULT_BUFLEN];
    char recvbufPublisher[DEFAULT_BUFLEN];
    
    if(InitializeWindowsSockets() == false)
		return 1;
    
    addrinfo *resultingAddressSubscriber = NULL;
    addrinfo* resultingAddressPublisher= NULL;
    addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_protocol = IPPROTO_TCP; 
    hints.ai_flags = AI_PASSIVE;     

    //SUBSCRIBER INIT
    iResultSubscriber = getaddrinfo(NULL, SUBSCRIBER_PORT, &hints, &resultingAddressSubscriber);
    if (iResultSubscriber != 0 )
    {
        printf("getaddrinfo-[SUBSCRIBER] failed with error: %d\n", iResultSubscriber);
        WSACleanup();
        return 1;
    }

    //PUBLISHER INIT
    iResultPublisher = getaddrinfo(NULL, PUBLISHER_PORT, &hints, &resultingAddressPublisher);
    if (iResultPublisher != 0)
    {
        printf("getaddrinfo-[PUBLISHER] failed with error: %d\n", iResultPublisher);
        WSACleanup();
        return 1;
    }

    //SUBSCRIBER SOCKET
    listenSocketSubscriber = socket(AF_INET,      
                          SOCK_STREAM,  
                          IPPROTO_TCP);

    if (listenSocketSubscriber == INVALID_SOCKET)
    {
        printf("socket-[SUBSCRIBER] failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(resultingAddressSubscriber);
        WSACleanup();
        return 1;
    }

    //PUBLISHER SOCKET
    listenSocketPublisher = socket(AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP);

    if (listenSocketPublisher == INVALID_SOCKET)
    {
        printf("socket-[PUBLISHER] failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(resultingAddressPublisher);
        WSACleanup();
        return 1;
    }

    //SETTING UP LISTEN SOCKET - SUBSCRIBER
    iResultSubscriber = bind(listenSocketSubscriber, resultingAddressSubscriber->ai_addr, (int)resultingAddressSubscriber->ai_addrlen);

    //SETTING UP LISTEN SOCKET - PUBLISHER
    iResultPublisher = bind(listenSocketPublisher, resultingAddressPublisher->ai_addr, (int)resultingAddressPublisher->ai_addrlen);

    //SETTING UP NON-BLOCKING MODE
    unsigned long int nonBlockingMode = 1;
    iResultSubscriber = ioctlsocket(listenSocketSubscriber, FIONBIO, &nonBlockingMode);

    if (iResultSubscriber == SOCKET_ERROR)
    {
        printf("bind - [SUBSCRIBER] failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(resultingAddressSubscriber);
        closesocket(listenSocketSubscriber);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(resultingAddressSubscriber);
    freeaddrinfo(resultingAddressPublisher);

    //SUBSCRIBER - LISTEN MODE
    iResultSubscriber = listen(listenSocketSubscriber, SOMAXCONN);
    if (iResultSubscriber == SOCKET_ERROR)
    {
        printf("listen - [SUBSCRIBER] failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocketSubscriber);
        WSACleanup();
        return 1;
    }

    //PUBLISHER - LISTEN MODE
    iResultPublisher = listen(listenSocketPublisher, SOMAXCONN);
    if (iResultPublisher == SOCKET_ERROR)
    {
        printf("listen - [PUBLISHER] failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocketPublisher);
        WSACleanup();
        return 1;
    }

	printf("Engine initialized, waiting for Clients/Publishers.\n\n");

    // PUBLISHER THREAD PARAMETERS
    Parameters pp;
    pp.acceptedSocketPublisher = acceptedSocketPublisher;
    pp.listenSocketPublisher = listenSocketPublisher;
    Parameters* pokpp = &pp;
    // PUBLISHER THREAD
    threadProducer = CreateThread(NULL, 0, &ReceiveMessageProducer,pokpp, 0, &threadIDProducer);

    // SUBSCRIBER THREAD PARAMETERS
    Parameters sp;
    sp.acceptedSocketPublisher = acceptedSocketSubscriber;
    sp.listenSocketPublisher = listenSocketSubscriber;
    Parameters* poksp = &sp;
    // SUBSCRIBER THREAD
    threadConsumer = CreateThread(NULL, 0, &ReceiveMessageConsumer, poksp, 0, &threadIDConsumer);

    // Waiting for threads
    if (threadConsumer)
        WaitForSingleObject(threadConsumer, INFINITE);
    if (threadProducer)
        WaitForSingleObject(threadProducer, INFINITE);
   

    //SUBSCRIBER CONNNECTION CLOSE
    iResultSubscriber = shutdown(acceptedSocketSubscriber, SD_SEND);
    if (iResultSubscriber == SOCKET_ERROR)
    {
        printf("shutdown - [SUBSCRIBER] failed with error: %d\n", WSAGetLastError());
        closesocket(acceptedSocketSubscriber);
        WSACleanup();
        return 1;
    }

    //PUBLISHER CONNNECTION CLOSE
    iResultPublisher = shutdown(acceptedSocketPublisher, SD_SEND);
    if (iResultPublisher == SOCKET_ERROR)
    {
        printf("shutdown - [PUBLISHER] failed with error: %d\n", WSAGetLastError());
        closesocket(acceptedSocketPublisher);
        WSACleanup();
        return 1;
    }

    closesocket(listenSocketSubscriber);
    closesocket(listenSocketPublisher);
    closesocket(acceptedSocketSubscriber);
    closesocket(acceptedSocketPublisher);
    WSACleanup();

    return 0;
}

DWORD WINAPI ReceiveMessageProducer(LPVOID lpParameter)
{
    int iResultPublisher;
    char recvbufPublisher[DEFAULT_BUFLEN];

    SOCKET acceptedSocketPublisher = ((Parameters*)lpParameter)->acceptedSocketPublisher;
    SOCKET listenSocketPublisher = ((Parameters*)lpParameter)->listenSocketPublisher;
   
    //PUBLISHER RECEIVE
   do
   {
       FD_SET set;
       timeval timeVal;

       FD_ZERO(&set);
       FD_SET(listenSocketPublisher, &set);

       timeVal.tv_sec = 0;
       timeVal.tv_usec = 0;

       //PUBLISHER NON BLOCKING
       iResultPublisher = select(0, &set, NULL, NULL, &timeVal);

       if (iResultPublisher == SOCKET_ERROR)
       {
           fprintf(stderr, "select - [SUBSCRIBER] failed with error: %ld\n", WSAGetLastError());
           continue;
       }

       if (iResultPublisher == 0)
       {
           Sleep(SERVER_SLEEP_TIME);
           continue;
       }
        acceptedSocketPublisher = accept(listenSocketPublisher, NULL, NULL);

        if (acceptedSocketPublisher == INVALID_SOCKET)
        {
            printf("accept - [PUBLISHER] failed with error: %d\n", WSAGetLastError());
            closesocket(listenSocketPublisher);
            WSACleanup();
            return 1;
        }

        do
        {
            FD_SET set;
            timeval timeVal;

            FD_ZERO(&set);
            FD_SET(acceptedSocketPublisher, &set);

            timeVal.tv_sec = 0;
            timeVal.tv_usec = 0;

            iResultPublisher = select(0, &set, NULL, NULL, &timeVal);

            if (iResultPublisher == SOCKET_ERROR)
            {
                fprintf(stderr, "select - [SUBSCRIBER] failed with error: %ld\n", WSAGetLastError());
                continue;
            }

            if (iResultPublisher == 0)
            {
                Sleep(SERVER_SLEEP_TIME);
                continue;
            }
            iResultPublisher = recv(acceptedSocketPublisher, recvbufPublisher, 41, 0);

            if (iResultPublisher > 0)
            {
                printf("Message - received from PUBLISHER: %s\n", recvbufPublisher);
            }
            else if (iResultPublisher == 0)
            {
                printf("Connection with PUBLISHER closed.\n");
                closesocket(acceptedSocketPublisher);
            }
            else
            {
                printf("recv  - [PUBLISHER] failed with error: %d\n", WSAGetLastError());
                closesocket(acceptedSocketPublisher);
            }
        } while (iResultPublisher > 0);

   } while (1);

    return 0;
}

DWORD WINAPI ReceiveMessageConsumer(LPVOID lpParameter)
{
    int iResultSubscriber;
    char recvbufSubscriber[DEFAULT_BUFLEN];

    SOCKET acceptedSocketSubscriber = ((Parameters*)lpParameter)->acceptedSocketPublisher;
    SOCKET listenSocketSubscriber = ((Parameters*)lpParameter)->listenSocketPublisher;

     //SUBSCRIBER RECEIVE
    do
    {
        FD_SET set;
        timeval timeVal;

        FD_ZERO(&set);
        FD_SET(listenSocketSubscriber, &set);

        timeVal.tv_sec = 0;
        timeVal.tv_usec = 0;

        //SUBSCRIBER NON BLOCKING
        iResultSubscriber = select(0, &set, NULL, NULL, &timeVal);

        if (iResultSubscriber == SOCKET_ERROR)
        {
            fprintf(stderr, "select - [SUBSCRIBER] failed with error: %ld\n", WSAGetLastError());
            continue;
        }

        if (iResultSubscriber == 0)
        {
            Sleep(SERVER_SLEEP_TIME);
            continue;
        }

        acceptedSocketSubscriber = accept(listenSocketSubscriber, NULL, NULL);

        if (acceptedSocketSubscriber == INVALID_SOCKET)
        {
            printf("accept - [SUBSCRIBER] failed with error: %d\n", WSAGetLastError());
            closesocket(listenSocketSubscriber);
            WSACleanup();
            return 1;
        }

        do
        {
            FD_SET set;
            timeval timeVal;

            FD_ZERO(&set);
            FD_SET(acceptedSocketSubscriber, &set);

            timeVal.tv_sec = 0;
            timeVal.tv_usec = 0;

            iResultSubscriber = select(0, &set, NULL, NULL, &timeVal);

            if (iResultSubscriber == SOCKET_ERROR)
            {
                fprintf(stderr, "select - [SUBSCRIBER] failed with error: %ld\n", WSAGetLastError());
                continue;
            }

            if (iResultSubscriber == 0)
            {
                Sleep(SERVER_SLEEP_TIME);
                continue;
            }

            iResultSubscriber = recv(acceptedSocketSubscriber, recvbufSubscriber, 42, 0);

            if (iResultSubscriber > 0)
            {
                printf("Message - received from SUBSCRIBER: %s\n", recvbufSubscriber);
              
            }
            else if (iResultSubscriber == 0)
            {
                printf("Connection with SUBSCRIBER closed.\n");
                closesocket(acceptedSocketSubscriber);
            }
            else
            {
                printf("recv failed with error: %d\n", WSAGetLastError());
                closesocket(acceptedSocketSubscriber);
            }
        } while (iResultSubscriber > 0);
     
    } while (1);
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
