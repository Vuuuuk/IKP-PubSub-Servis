#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#define DEFAULT_BUFLEN 512
#define SERVER_SLEEP_TIME 50
#define SUBSCRIBER_PORT_RECV "26000"
#define SUBSCRIBER_PORT_SEND "27000"
#define PUBLISHER_PORT "28000"

bool InitializeWindowsSockets();
DWORD WINAPI ReceiveMessageProducer(LPVOID lpParameter);
DWORD WINAPI ReceiveMessageConsumer(LPVOID lpParameter);
DWORD WINAPI ConsumerThread(LPVOID lpParameter);
char RingBufferGet(struct RingBuffer* buffer);
void RingBufferPut(struct RingBuffer* buffer, char c);

struct PublishParametars
{
    char* topic;
    char* type;
    char* message;

};

struct RingBuffer
{
    int head;
    int tail;
    char data[1];
};

struct Parameters
{
    SOCKET listenSocket;
    //SOCKET acceptedSocketPublisher;
    bool end;
    HANDLE empty;
    HANDLE full;
    HANDLE finishSignal;
    RingBuffer* ringBuffer;
    CRITICAL_SECTION bufferAccess;
    int messageLength;
 
};

int main(void) 
{
    // Createing engine threads
    HANDLE threadProducer=NULL;
    HANDLE threadConsumer=NULL;
    HANDLE threadSubRecv=NULL;
    DWORD threadIDProducer;
    DWORD threadIDConsumer;
    DWORD threadIDSubRecv;

    
    HANDLE Empty = CreateSemaphore(0, 100, 100, NULL);
    HANDLE Full = CreateSemaphore(0, 0, 100, NULL);
    HANDLE FinishSignal = CreateSemaphore(0, 0, 2, NULL);

    SOCKET listenSocketSubscriberRecv = INVALID_SOCKET;
    SOCKET listenSocketSubscriberSend = INVALID_SOCKET;
    SOCKET listenSocketPublisher = INVALID_SOCKET;

    
    CRITICAL_SECTION bufferAccess;


  /*  SOCKET acceptedSocketSubscriber = INVALID_SOCKET;
    SOCKET acceptedSocketPublisher = INVALID_SOCKET;*/
    int iResultSubscriber;
    int iResultPublisher; 

    // Inicialization of ring buffer
    RingBuffer ringBuffer;
    ringBuffer.head = 0;
    ringBuffer.tail = 0;
    ringBuffer.data[0]='t';
    //char recvbufPublisher[DEFAULT_BUFLEN];
    
    if(InitializeWindowsSockets() == false)
		return 1;
    
    addrinfo* resultingAddressSubscriberRecv = NULL;
    addrinfo* resultingAddressSubscriberSend = NULL;
    addrinfo* resultingAddressPublisher= NULL;
    addrinfo hints;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_protocol = IPPROTO_TCP; 
    hints.ai_flags = AI_PASSIVE;     

    //SUBSCRIBER INIT
    iResultSubscriber = getaddrinfo(NULL, SUBSCRIBER_PORT_RECV, &hints, &resultingAddressSubscriberRecv);
    if (iResultSubscriber != 0 )
    {
        printf("getaddrinfo-[SUBSCRIBER] failed with error: %d\n", iResultSubscriber);
        WSACleanup();
        return 1;
    }

    iResultSubscriber = getaddrinfo(NULL, SUBSCRIBER_PORT_SEND, &hints, &resultingAddressSubscriberSend);
    if (iResultSubscriber != 0)
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

    //SUBSCRIBER SOCKETS
    listenSocketSubscriberRecv = socket(AF_INET,      
                          SOCK_STREAM,  
                          IPPROTO_TCP);

    if (listenSocketSubscriberRecv == INVALID_SOCKET)
    {
        printf("socket-[SUBSCRIBER] failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(resultingAddressSubscriberRecv);
        WSACleanup();
        return 1;
    }

    listenSocketSubscriberSend = socket(AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP);

    if (listenSocketSubscriberSend == INVALID_SOCKET)
    {
        printf("socket-[SUBSCRIBER] failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(resultingAddressSubscriberSend);
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

    //SETTING UP LISTEN SOCKETS - SUBSCRIBER
    iResultSubscriber = bind(listenSocketSubscriberRecv, resultingAddressSubscriberRecv->ai_addr, (int)resultingAddressSubscriberRecv->ai_addrlen);
    iResultSubscriber = bind(listenSocketSubscriberSend, resultingAddressSubscriberSend->ai_addr, (int)resultingAddressSubscriberSend->ai_addrlen);

    //SETTING UP LISTEN SOCKET - PUBLISHER
    iResultPublisher = bind(listenSocketPublisher, resultingAddressPublisher->ai_addr, (int)resultingAddressPublisher->ai_addrlen);

    //SETTING UP NON-BLOCKING MODE - SUBSCRIBER
    unsigned long int nonBlockingMode = 1;
    iResultSubscriber = ioctlsocket(listenSocketSubscriberRecv, FIONBIO, &nonBlockingMode);

    if (iResultSubscriber == SOCKET_ERROR)
    {
        printf("bind - [SUBSCRIBER] failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(resultingAddressSubscriberRecv);
        closesocket(listenSocketSubscriberRecv);
        WSACleanup();
        return 1;
    }

    iResultSubscriber = ioctlsocket(listenSocketSubscriberSend, FIONBIO, &nonBlockingMode);

    if (iResultSubscriber == SOCKET_ERROR)
    {
        printf("bind - [SUBSCRIBER] failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(resultingAddressSubscriberSend);
        closesocket(listenSocketSubscriberSend);
        WSACleanup();
        return 1;
    }

    //SETTING UP NON-BLOCKING MODE - PUBLISHER
    iResultPublisher = ioctlsocket(listenSocketPublisher, FIONBIO, &nonBlockingMode);

    if (iResultPublisher == SOCKET_ERROR)
    {
        printf("bind - [PUBLISHER] failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(resultingAddressPublisher);
        closesocket(listenSocketPublisher);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(resultingAddressSubscriberRecv);
    freeaddrinfo(resultingAddressSubscriberSend);
    freeaddrinfo(resultingAddressPublisher);

    //SUBSCRIBER - LISTEN MODE
    iResultSubscriber = listen(listenSocketSubscriberRecv, SOMAXCONN);
    if (iResultSubscriber == SOCKET_ERROR)
    {
        printf("listen - [SUBSCRIBER] failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocketSubscriberRecv);
        WSACleanup();
        return 1;
    }

    iResultSubscriber = listen(listenSocketSubscriberSend, SOMAXCONN);
    if (iResultSubscriber == SOCKET_ERROR)
    {
        printf("listen - [SUBSCRIBER] failed with error: %d\n", WSAGetLastError());
        closesocket(listenSocketSubscriberSend);
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

    bool end=false;
    char c = NULL;
    int messageLength = 0;
	printf("Engine initialized, waiting for Clients/Publishers.\n\n");

    Parameters subRecvParam;
    subRecvParam.listenSocket = listenSocketSubscriberRecv;
    subRecvParam.end = end;
    Parameters* pokSuRecvParam = &subRecvParam;
    
    threadSubRecv = CreateThread(NULL, 0, &ReceiveMessageConsumer, (LPVOID)pokSuRecvParam, 0, &threadIDSubRecv);
/*
*   
    while (1)
    {
       */
 
       /* if (Empty && FinishSignal && Full)
        {*/
            

            InitializeCriticalSection(&bufferAccess);

            //PUBLISHER RECEIVE
            struct Parameters pp;
            //   pp.acceptedSocketPublisher = acceptedSocketPublisher;
            pp.listenSocket = listenSocketPublisher;
            pp.end = end;
            pp.empty = Empty;
            pp.full = Full;
            pp.finishSignal = FinishSignal;
            pp.ringBuffer = &ringBuffer;
            pp.bufferAccess = bufferAccess;
            pp.messageLength = messageLength;
            Parameters* pokpp = &pp;

            threadProducer = CreateThread(NULL, 0, &ReceiveMessageProducer, (LPVOID)pokpp, 0, &threadIDProducer);

        //    //SUBSCRIBER RECEIVE
           struct Parameters sp;
            //// sp.acceptedSocketPublisher = acceptedSocketSubscriber;
            sp.listenSocket = listenSocketSubscriberSend;
            sp.end = end;
            sp.empty = Empty;
            sp.full = Full;
            sp.finishSignal = FinishSignal;
            sp.ringBuffer = &ringBuffer;
            sp.bufferAccess = bufferAccess;
            sp.messageLength = messageLength;
            Parameters* poksp = &sp;
            threadConsumer = CreateThread(NULL, 0, &ConsumerThread, (LPVOID)poksp, 0, &threadIDConsumer);
            if (!threadConsumer || !threadProducer || c == 'q')
                ReleaseSemaphore(FinishSignal, 2, NULL);

            if (threadConsumer)
                WaitForSingleObject(threadConsumer, INFINITE);
            if (threadProducer)
                WaitForSingleObject(threadProducer, INFINITE);

        
      ///  }
        if (threadSubRecv)
            WaitForSingleObject(threadSubRecv, INFINITE);
        
  /*      char c = getchar();
    }*/
   // HANDLE hSemaphore = CreateSemaphore(0, 0, 1, NULL);
  

    //while (1)
    //{
    //    if (kbhit())
    //    {
    //        c = getchar();
    //        if (c == 'q')
    //        {
    //          //  end = true;
    //            break;
    //        }
    //       
    //    }
    //   
    //    ReleaseSemaphore(hSemaphore, 1, NULL);
    //    Sleep(100);
    //}
    

   /* if (hThreadConsumer)
        WaitForSingleObject(hThreadConsumer, INFINITE);
    if (hThreadProducer)
        WaitForSingleObject(hThreadProducer, INFINITE);*/

  
    CloseHandle(threadProducer);
    CloseHandle(threadConsumer);
    CloseHandle(Empty);
    CloseHandle(Full);
    CloseHandle(FinishSignal);
    DeleteCriticalSection(&bufferAccess);

    WSACleanup();

    return 0;
}

DWORD WINAPI ReceiveMessageProducer(LPVOID lpParameter)
{
    int iResultPublisher;
    SOCKET acceptedSocketPublisher = INVALID_SOCKET;//((Parameters*)lpParameter)->acceptedSocketPublisher;
    SOCKET listenSocketPublisher = ((Parameters*)lpParameter)->listenSocket;
    bool end = ((Parameters*)lpParameter)->end;
   
    HANDLE empty = ((Parameters*)lpParameter)->empty;
    HANDLE full = ((Parameters*)lpParameter)->full;
    HANDLE finishSignal = ((Parameters*)lpParameter)->finishSignal;
    RingBuffer* ringBuffer = ((Parameters*)lpParameter)->ringBuffer;
    CRITICAL_SECTION bufferAccess = ((Parameters*)lpParameter)->bufferAccess;

    const int semaphore_num = 2;
    HANDLE semaphores[semaphore_num] = {finishSignal,empty};

    //char* recvBufferPublisher = (char*)malloc(4);
    char recvBufferPublisher[520];
    int iResult = 0;
    int headerLength = 0;
    int messageLength = 0;

    while (WaitForMultipleObjects(semaphore_num, semaphores, FALSE,
        INFINITE) == WAIT_OBJECT_0 + 1)
    {
        FD_SET set;
        timeval timeVal;

        FD_ZERO(&set);
        FD_SET(listenSocketPublisher ,&set);

        timeVal.tv_sec = 0;
        timeVal.tv_usec = 0;

        //SUBSCRIBER NON BLOCKING
        iResultPublisher = select(0, &set, NULL, NULL, &timeVal);

        if (iResultPublisher == SOCKET_ERROR)
        {
            fprintf(stderr, "select - [PUBLISHER] failed with error: %ld\n", WSAGetLastError());
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
                fprintf(stderr, "select- [PUBLISHER] failed with error: %ld\n", WSAGetLastError());
                continue;
            }

            if (iResultPublisher == 0)
            {
                Sleep(SERVER_SLEEP_TIME);
                continue;
            }
            iResultPublisher = recv(acceptedSocketPublisher, recvBufferPublisher, 1, 0);

            if (iResultPublisher > 0)
            {
                printf("Message - received from PUBLISHER: %c\n", recvBufferPublisher[0]);
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
            EnterCriticalSection(&bufferAccess);
            RingBufferPut(ringBuffer, recvBufferPublisher[0]);
            LeaveCriticalSection(&bufferAccess);


            ReleaseSemaphore(full, 1, NULL);
           

        //    char* generisanBuffer;
        //    char* primljeniBuffer = (char*)malloc(4);

        //    int iResult = 0;
        //    int primljenaDuzina = 0;
        //    int duzinaPoruke = 0;

        //    while (primljenaDuzina < 4)
        //    {
        //        // Receive data until the client shuts down the connection
        //        iResult = recv(acceptedSocketPublisher, primljeniBuffer, 4, 0);
        //        if (iResult > 0)
        //        {

        //            primljenaDuzina += iResult;

        //        }
        //        else if (iResult == 0)
        //        {
        //            // connection was closed gracefully
        //            printf("Connection with client closed.\n");
        //            closesocket(acceptedSocketPublisher);
        //            break;
        //        }
        //        else
        //        {
        //            // there was an error during recv
        //            printf("recv1 failed with error: %d\n", WSAGetLastError());
        //            closesocket(acceptedSocketPublisher);
        //            break;
        //        }
        //    }
        //    if (primljenaDuzina == 4)
        //    {
        //        //printf("Message received from client: %d.\n", *((int*)primljeniBuffer));
        //        duzinaPoruke = *((int*)primljeniBuffer);
        //    }
        //    primljenaDuzina = 0;
        //    iResult = 0;
        //    //generisanBuffer = (char*)malloc(duzinaPoruke);
        //    primljeniBuffer = (char*)realloc(primljeniBuffer, duzinaPoruke);
        //    // pok = primljeniBuffer + 4;

        //    while (primljenaDuzina < duzinaPoruke)
        //    {
        //        // Receive data until the client shuts down the connection
        //        iResult = recv(acceptedSocketPublisher, primljeniBuffer, duzinaPoruke, 0);
        //        if (iResult > 0)
        //        {

        //            primljenaDuzina += iResult;

        //        }
        //        else if (iResult == 0)
        //        {
        //            // connection was closed gracefully
        //            printf("Connection with client closed.\n");
        //            closesocket(acceptedSocketPublisher);
        //            break;
        //        }
        //        else
        //        {
        //            // there was an error during recv
        //            printf("recv failed with error: %d\n", WSAGetLastError());
        //            closesocket(acceptedSocketPublisher);
        //            break;
        //        }
        //    }

        //    if (primljenaDuzina == duzinaPoruke)
        //    {
        //        //printf("Message received from client: %d.\n", *((int*)primljeniBuffer));
        //        printf("Primljena poruka je %s\n", primljeniBuffer);
        //        // return primljeniBuffer;
        //    }

        ///*    ringBuffer->data = (char*)realloc(ringBuffer->data, messageLength);
        //    ((Parameters*)lpParameter)->messageLength = messageLength;*/
    

        } while (iResultPublisher > 0);

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

    closesocket(listenSocketPublisher);
    closesocket(acceptedSocketPublisher);

    return 0;
}
DWORD WINAPI ReceiveMessageConsumer(LPVOID lpParameter)
{
    int iResultSubscriber;
    char recvbufSubscriber[DEFAULT_BUFLEN];


    SOCKET acceptedSocketSubscriber = INVALID_SOCKET;//((Parameters*)lpParameter)->acceptedSocketPublisher;
    SOCKET listenSocketSubscriber = ((Parameters*)lpParameter)->listenSocket;
    bool end = ((Parameters*)lpParameter)->end;


    do
    {
        //WaitForSingleObject(semaphore, INFINITE);
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
      
            fprintf(stderr, "select- [SUBSCRIBER] failed with error: %ld\n", WSAGetLastError());
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
                fprintf(stderr, "select- [SUBSCRIBER] failed with error: %ld\n", WSAGetLastError());
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

    //SUBSCRIBER CONNNECTION CLOSE
    iResultSubscriber = shutdown(acceptedSocketSubscriber, SD_SEND);
    if (iResultSubscriber == SOCKET_ERROR)
    {
        printf("shutdown - [SUBSCRIBER] failed with error: %d\n", WSAGetLastError());
        closesocket(acceptedSocketSubscriber);
        WSACleanup();
        return 1;
    }

    closesocket(listenSocketSubscriber);
    closesocket(acceptedSocketSubscriber);

    return 0;
}

DWORD WINAPI ConsumerThread(LPVOID lpParameter)
{
    int iResultSubscriber;
    char recvbufSubscriber[500];

    SOCKET acceptedSocketSubscriber = INVALID_SOCKET;//((Parameters*)lpParameter)->acceptedSocketPublisher;
    SOCKET listenSocketSubscriber = ((Parameters*)lpParameter)->listenSocket;
    bool end = ((Parameters*)lpParameter)->end;
    HANDLE empty = ((Parameters*)lpParameter)->empty;
    HANDLE full = ((Parameters*)lpParameter)->full;
    HANDLE finishSignal = ((Parameters*)lpParameter)->finishSignal;
    RingBuffer* ringBuffer = ((Parameters*)lpParameter)->ringBuffer;
    CRITICAL_SECTION bufferAccess = ((Parameters*)lpParameter)->bufferAccess;

    const int semaphore_num = 2;
    HANDLE semaphores[semaphore_num] = {finishSignal,full};

    int messageLength = ((Parameters*)lpParameter)->messageLength;
    char* messageWthDataAndHeader = NULL;

    while (WaitForMultipleObjects(semaphore_num, semaphores, FALSE,
        INFINITE) == WAIT_OBJECT_0 + 1)
    {
        //printf("USAO\n");
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
            fprintf(stderr, "select- [SUBSCRIBER] failed with error: %ld\n", WSAGetLastError());
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
            printf("accept2 - [SUBSCRIBER] failed with error: %d\n", WSAGetLastError());
            closesocket(listenSocketSubscriber);
            WSACleanup();
            return 1;
        }

        /*  do
          {*/
         
        FD_SET set1;

        FD_ZERO(&set1);
        FD_SET(acceptedSocketSubscriber, &set1);


        iResultSubscriber = select(0, NULL, &set1, NULL, &timeVal);

        if (iResultSubscriber == SOCKET_ERROR)
        {
            fprintf(stderr, "select- [SUBSCRIBER] failed with error: %ld\n", WSAGetLastError());
            continue;
        }

        if (iResultSubscriber == 0)
        {
            Sleep(SERVER_SLEEP_TIME);
            continue;
        }
        EnterCriticalSection(&bufferAccess);
        recvbufSubscriber[0] = RingBufferGet(ringBuffer);
        LeaveCriticalSection(&bufferAccess);
        printf("Procitano iz bafera: %c\n", recvbufSubscriber[0]);
        iResultSubscriber = send(acceptedSocketSubscriber, recvbufSubscriber, 2, 0);
        if (iResultSubscriber == SOCKET_ERROR)
        {
            printf("send - [SUBSCRIBER] failed with error: %d\n", WSAGetLastError());
            closesocket(acceptedSocketSubscriber);
            WSACleanup();
            return 1;
        }
       /* messageWthDataAndHeader = (char*)malloc(messageLength + 4);
        *((int*)messageWthDataAndHeader) = messageLength;
        memcpy(messageWthDataAndHeader + 4, recvbufSubscriber, messageLength);
        iResultSubscriber = send(acceptedSocketSubscriber, messageWthDataAndHeader, messageLength + 4, 0);*/
     

        ReleaseSemaphore(empty, 1, NULL);
       
    }

    //SUBSCRIBER CONNNECTION CLOSE
    iResultSubscriber = shutdown(acceptedSocketSubscriber, SD_SEND);
    if (iResultSubscriber == SOCKET_ERROR)
    {
        printf("shutdown - [SUBSCRIBER] failed with error: %d\n", WSAGetLastError());
        closesocket(acceptedSocketSubscriber);
        WSACleanup();
        return 1;
    }

    closesocket(listenSocketSubscriber);
    closesocket(acceptedSocketSubscriber);


    return 0;
}
char RingBufferGet(struct RingBuffer* buffer)
{
    int index;
    index = buffer->head;
    buffer->head = (buffer->head + 1) % 100;
    return buffer->data[index];

}

void RingBufferPut(struct RingBuffer* buffer, char c)
{
    buffer->data[buffer->tail] = c;
    buffer->tail = (buffer->tail + 1) % 100;
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
