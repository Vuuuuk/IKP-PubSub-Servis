#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>

#define SERVER_SLEEP_TIME 50
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT 28000
struct DataToPublish
{
    char topic[6];
    char type[3];
    char message[DEFAULT_BUFLEN];
};

bool InitializeWindowsSockets();
void GenerateStringWithDelimiter(char* string, int stringLength, char* stringWithDelimiter, int* stringWithDelimiterLength);


int __cdecl main(int argc, char **argv) 
{
    SOCKET connectSocket = INVALID_SOCKET;
    int iResult;
    char* messageToSend = "This is a test message from a Publisher.";
    char* topic = "STATUS";
    char* type = "MER";

    // STRINGS WITH DELIMITERS
    int len =(int)strlen(topic);
    char* topicWithDelimiter = (char*)malloc(len + 1 + 1);
    int topicWithDelimiterLength = 0;
    GenerateStringWithDelimiter(topic,len, topicWithDelimiter, &topicWithDelimiterLength);
    //printf("Topic with delimiter-> %s\n", topicWithDelimiter);
    //printf("Topic with delimiter length-> %d\n",topicWithDelimiterLength);

    len = (int)strlen(type);
    int typeWithDelimiterLength =0;
    char* typeWithDelimiter = (char*)malloc(len + 1 + 1);
    GenerateStringWithDelimiter(type, len, typeWithDelimiter, &typeWithDelimiterLength);
    //printf("Type with delimiter %s\n", typeWithDelimiter);
    //printf("Type with delimiter length -> %d\n", typeWithDelimiterLength);
   
    int messageToSendLength = (int)strlen(messageToSend);
   // printf("Message-> %d\n", messageToSendLength);

    //CONCATENATED STRINGS WITH DELIMITERS
    int size = topicWithDelimiterLength + typeWithDelimiterLength + messageToSendLength;
    //printf("Size of data to sent %d\n",size);
    char* dataToSent = (char*)malloc(topicWithDelimiterLength + typeWithDelimiterLength + messageToSendLength);
    int offset = 0;
    memcpy(dataToSent, topicWithDelimiter, topicWithDelimiterLength);
    offset += topicWithDelimiterLength;
    memcpy(dataToSent+ offset, typeWithDelimiter, typeWithDelimiterLength);
    offset += typeWithDelimiterLength;
    memcpy(dataToSent+offset, messageToSend, messageToSendLength+1);

    printf("String to send %s\n", dataToSent);

    //DATA TO SENT WITH MESSAGE SIZE AND CONTETNT
    int length = size+1;
    char* headerAndData = (char*)malloc(length + 4);
    *((int*)headerAndData) = length;
    memcpy(headerAndData + 4, dataToSent, length);

    if(InitializeWindowsSockets() == false)
		return 1;

    //CREATING A SOCKET
    connectSocket = socket(AF_INET,
                           SOCK_STREAM,
                           IPPROTO_TCP);

    if (connectSocket == INVALID_SOCKET)
    {
        printf("socket - [PUBLISHER] failed with error: %ld\n", WSAGetLastError());
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
        printf("Unable - [PUBLISHER] to connect to server.\n");
        closesocket(connectSocket);
        WSACleanup();
    }

    //SETTING UP NON-BLOCKING MODE
    unsigned long int nonBlockingMode = 1;
    iResult = ioctlsocket(connectSocket, FIONBIO, &nonBlockingMode);

   
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
    iResult = send(connectSocket, headerAndData, length+4, 0);
    if (iResult == SOCKET_ERROR)
    {
        printf("send - [PUBLISHER] failed with error: %d\n", WSAGetLastError());
        closesocket(connectSocket);
        WSACleanup();
        return 1;
    }

    printf("[PUBLISHER] - Message Sent.\n");
    
    getchar();

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

void GenerateStringWithDelimiter(char* string, int stringLength, char* stringWithDelimiter, int* stringWithDelimiterLength)
{
    strcpy(stringWithDelimiter, string);
    stringWithDelimiter[stringLength] = '+';
    stringWithDelimiter[stringLength + 1] = '\0';
    *stringWithDelimiterLength = (int)strlen(stringWithDelimiter);
    //printf("String with delimiter: %s\n", stringWithDelimiter);
    //printf("Length of string with delimiter: %d\n", *stringWithDelimiterLength);
}

