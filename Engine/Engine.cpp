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
void RingBufferGet(struct RingBuffer* buffer, int size,char* returnBuffer);
void RingBufferPut(struct RingBuffer* buffer, char* c, int size);
void parseRecievedData(SOCKET* acceptedSocket, char* recvBuffer, int* dataLength)
{
    int iResultPublisher = 0;
    SOCKET acceptedSocketPublisher = *acceptedSocket;
    int helpLength = 0;
    int messageLength = 0;

    while (helpLength < 4)
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
        iResultPublisher = recv(acceptedSocketPublisher, recvBuffer, 4, 0);
        if (iResultPublisher > 0)
        {

            helpLength += iResultPublisher;

        }
        else if (iResultPublisher == 0)
        {
            printf("Connection with client closed.\n");
            closesocket(acceptedSocketPublisher);
            break;
        }
        else
        {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(acceptedSocketPublisher);
            break;
        }
        if (helpLength == 4)
        {
            messageLength = *((int*)recvBuffer);
        }
    }
    helpLength = 0;
    iResultPublisher = 0;
    //recvBuffer = (char*)realloc(recvBuffer, messageLength);

    while (helpLength < messageLength)
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
        iResultPublisher = recv(acceptedSocketPublisher, recvBuffer, messageLength, 0);
        if (iResultPublisher > 0)
        {

            helpLength += iResultPublisher;

        }
        else if (iResultPublisher == 0)
        {
            printf("Connection with client closed.\n");
            closesocket(acceptedSocketPublisher);
            break;
        }
        else
        {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(acceptedSocketPublisher);
            break;
        }

    }

    if (helpLength == messageLength)
    {
        *dataLength = messageLength;

    }

}

struct RingBuffer
{
    int head;
    int tail;
    char data[DEFAULT_BUFLEN];
};

struct Parameters
{
    SOCKET listenSocketPublisher;
    SOCKET acceptedSocketPublisher;
    HANDLE empty;
    HANDLE full;
    CRITICAL_SECTION bufferAccess;
    RingBuffer* ringBuffer;
    int* dataLength;
};

int main(void) 
{

    SOCKET listenSocketSubscriber = INVALID_SOCKET;
    SOCKET listenSocketPublisher = INVALID_SOCKET;
    SOCKET acceptedSocketSubscriber = INVALID_SOCKET;
    SOCKET acceptedSocketPublisher = INVALID_SOCKET;
    int iResultSubscriber;
    int iResultPublisher;
    char recvBuf[DEFAULT_BUFLEN];

    HANDLE threadProducer;
    DWORD threadIDProducer;

    HANDLE threadConsumer;
    DWORD threadIDConsumer;

    //CREATING SEMAPOHORES FOR SINHRONIZATION
    HANDLE Empty = CreateSemaphore(0, 100, 100, NULL);
    HANDLE Full = CreateSemaphore(0, 0, 100, NULL);

    //RING BUFFER INICIALIZATION 
    RingBuffer ringBuffer;
    ringBuffer.head = 0;
    ringBuffer.tail = 0;
    ringBuffer.data[0] = 't';

    int dataLength=0;

    //SAFE ACCESS OVER SHARED DATA
    CRITICAL_SECTION bufferAccess;
    
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

    //CRITICAL SECTION INICIALIZATION
    InitializeCriticalSection(&bufferAccess);

    //PUBLISHER THREAD PARAMETERS
    Parameters pp;
    pp.acceptedSocketPublisher = acceptedSocketPublisher;
    pp.listenSocketPublisher = listenSocketPublisher;
    pp.empty = Empty;
    pp.full = Full;
    pp.ringBuffer = &ringBuffer;
    pp.bufferAccess = bufferAccess;
    pp.dataLength = &dataLength;
    Parameters* pokpp = &pp;

    //PUBLISHER THREAD
    threadProducer = CreateThread(NULL, 0, &ReceiveMessageProducer,pokpp, 0, &threadIDProducer);

    //SUBSCRIBER THREAD PARAMETERS
    Parameters sp;
    sp.acceptedSocketPublisher = acceptedSocketSubscriber;
    sp.listenSocketPublisher = listenSocketSubscriber;
    sp.empty = Empty;
    sp.full = Full;
    sp.ringBuffer = &ringBuffer;
    sp.bufferAccess = bufferAccess;
    sp.dataLength = &dataLength;
    Parameters* poksp = &sp;

    //SUBSCRIBER THREAD
    threadConsumer = CreateThread(NULL, 0, &ReceiveMessageConsumer, poksp, 0, &threadIDConsumer);

    //WAITING FOR THREADS
    if (threadConsumer)
        WaitForSingleObject(threadConsumer, INFINITE);
    if (threadProducer)
        WaitForSingleObject(threadProducer, INFINITE);

    //DELETEING CRITICAL SECTION
    DeleteCriticalSection(&bufferAccess);

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
    char* recvBuffer=NULL;
    int helpLength = 0;
    int messageLength = 0;

    SOCKET acceptedSocketPublisher = ((Parameters*)lpParameter)->acceptedSocketPublisher;
    SOCKET listenSocketPublisher = ((Parameters*)lpParameter)->listenSocketPublisher;
    HANDLE empty = ((Parameters*)lpParameter)->empty;
    HANDLE full = ((Parameters*)lpParameter)->full;
    RingBuffer* ringBuffer = ((Parameters*)lpParameter)->ringBuffer;
    CRITICAL_SECTION bufferAccess = ((Parameters*)lpParameter)->bufferAccess;
    int* dataLength = ((Parameters*)lpParameter)->dataLength;
   
    //BUFFER SINHRONIZATION - PRODUCER THREAD
    while (WaitForSingleObject(empty, INFINITE) == WAIT_OBJECT_0)
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

        recvBuffer = (char*)malloc(DEFAULT_BUFLEN);
        //PUBLISHER RECEIVE - PARSING MESSAGE SIZE AND CONTENT
        parseRecievedData(&acceptedSocketPublisher, recvBuffer, dataLength);
        printf("Received message from Publisher : %s\n", recvBuffer);
        EnterCriticalSection(&bufferAccess);
        RingBufferPut(ringBuffer, recvBuffer, *dataLength);
        LeaveCriticalSection(&bufferAccess);
        free(recvBuffer);
        ReleaseSemaphore(full, 1, NULL);
    }
    return 0;
}

DWORD WINAPI ReceiveMessageConsumer(LPVOID lpParameter)
{
    int iResultSubscriber;
    char* recvbufSubscriber=(char*)malloc(DEFAULT_BUFLEN);

    SOCKET acceptedSocketSubscriber = ((Parameters*)lpParameter)->acceptedSocketPublisher;
    SOCKET listenSocketSubscriber = ((Parameters*)lpParameter)->listenSocketPublisher;
    HANDLE empty = ((Parameters*)lpParameter)->empty;
    HANDLE full = ((Parameters*)lpParameter)->full;
    CRITICAL_SECTION bufferAccess = ((Parameters*)lpParameter)->bufferAccess;
    RingBuffer* ringBuffer = ((Parameters*)lpParameter)->ringBuffer;
    int* dataLength = ((Parameters*)lpParameter)->dataLength;

   
    //ADED FOR FAKE RELEASE OF WHILE
    bool releaseWhile = false;

    //BUFFER SINHRONIZATION - CONSUMER THREAD
    while(releaseWhile == true || WaitForSingleObject(full, INFINITE) == WAIT_OBJECT_0)
    {
        int length = *dataLength;
       // recvbufSubscriber = (char*)malloc(length);
        releaseWhile = false;
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
            //printf("Select accept sub waiting...\n");
            releaseWhile = true;
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
       
            
        FD_SET set1;
        FD_ZERO(&set1);
        FD_SET(acceptedSocketSubscriber, &set1);

        timeVal.tv_sec = 0;
        timeVal.tv_usec = 0;

        iResultSubscriber = select(0, NULL, &set1, NULL, &timeVal);

        if (iResultSubscriber == SOCKET_ERROR)
        {
            fprintf(stderr, "select - [SUBSCRIBER] failed with error: %ld\n", WSAGetLastError());
            continue;
        }

        if (iResultSubscriber == 0)
        {
            Sleep(SERVER_SLEEP_TIME);
            // printf("Select send sub waiting...\n");
            continue;
        }

        //DATA TO SENT WITH MESSAGE SIZE AND CONTETNT
        EnterCriticalSection(&bufferAccess);
        RingBufferGet(ringBuffer, length, recvbufSubscriber);
        char* headerAndData = (char*)malloc(length + 4);
        *((int*)headerAndData) = length;
        //READING DATA FROM RING BUFFER
        printf("Read from ring buffer: %s\n", recvbufSubscriber);
        memcpy(headerAndData + 4, recvbufSubscriber, length);
        LeaveCriticalSection(&bufferAccess);

        //SUBSCRIBER SEND
        iResultSubscriber = send(acceptedSocketSubscriber, headerAndData, length + 4, 0);

        if (iResultSubscriber == SOCKET_ERROR)
        {
            printf("send - [SUBSCRIBER] failed with error: %d\n", WSAGetLastError());
            closesocket(acceptedSocketSubscriber);
            WSACleanup();
            return 1;
        }

        printf("[PUBLISHER] - Message Sent.\n");
        free(recvbufSubscriber);
        free(headerAndData);
        ReleaseSemaphore(empty, 1, NULL);
    }

    return 0;
}
void RingBufferGet(struct RingBuffer* buffer, int size, char* returnBuffer)
{
    int index;
    char retData[DEFAULT_BUFLEN];
    for (int i = 0; i < size; i++)
    {
        index = buffer->head;
        returnBuffer[i] = buffer->data[index];
        buffer->head = (buffer->head + 1) % 100;
    }
   
   // return returnBuffer;

}

void RingBufferPut(struct RingBuffer* buffer, char* c, int size)
{
    for (int i = 0; i < size; i++)
    {
        buffer->data[buffer->tail] = c[i];
        buffer->tail = (buffer->tail + 1) % 100;
    }

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


