#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define singleDigitMultFactor 50
#define doubleDigitMultFactor 5

int tableSize;

typedef struct keyValuePair
{
    char* key;
	int value;
	struct keyValuePair* next;
}keyValuePair;

char* InitHashMap(keyValuePair** hashMap){
	for(int i = 0; i < tableSize; i++)
		hashMap[i] = NULL;
	return "Init() completed without any errors.\n";
}

int SimpleHash(char* key)
{
	int hashedKey = 0;
	for(int i = 0; key[i] != '\0'; i++)
	{
		hashedKey += key[i];
		hashedKey = (hashedKey * key[i]) % tableSize;
	}
	return hashedKey;
}

//djb2 
int AdvancedHash(char *key)
{
    unsigned long hash = 5381;
    int c;

    while (c = *key++)
        hash = (((hash << 5) + hash) + c) % tableSize; 

    return hash;
}

char* Insert(keyValuePair *kvp, keyValuePair** hashMap)
{
	//Dodavanje na pocetak liste
	kvp->next = hashMap[SimpleHash(kvp->key)];
	hashMap[SimpleHash(kvp->key)] = kvp;
	return "Insert() completed without any errors.\n";
}

//Dobra implementacija za koliziju sa vise razlicitih kljuceva
keyValuePair* FindKey(keyValuePair** hashMap, char *key)
{
    keyValuePair* tmp = hashMap[SimpleHash(key)];
    while (tmp != NULL && strcmp(key, tmp->key) != 0)
        tmp=tmp->next;

    return tmp;
}

char* RemoveKey(keyValuePair** hashMap, char* key)
{
    keyValuePair *tmp = hashMap[SimpleHash(key)];
    keyValuePair *prev = NULL;
    if (tmp != NULL && strcmp(key, tmp->key) != 0)
    {
        prev = tmp;
        tmp = tmp->next;
    }
    if (tmp == NULL) 
    	return "Error -> Remove() could not be completed, no such key in hashMap.\n";
    if (prev == NULL)
        hashMap[SimpleHash(key)] = tmp->next;
    else
        prev->next = tmp->next;
    return "Remove() completed without any errors.\n";
}

void PrintHashMap(keyValuePair** hashMap)
{
	for(int i = 0; i < tableSize; i++)
	{
		if(hashMap[i] == NULL)
			printf("\t[%d]\t---\n", i);
		else
		{
			keyValuePair *tmp = hashMap[i];
			printf("\t[%d]\t[%s]\t", i, tmp->key);
			while(tmp != NULL)
			{
				printf("%d -> ", tmp->value);
				tmp = tmp -> next;
			}
		printf("\n");
		}
	}
}

void main()
{

	keyValuePair analog = {.key = "analog", .value = 123};
	keyValuePair status = {.key = "status", .value = 345};
	keyValuePair status1 = {.key = "status", .value = 567};
	keyValuePair status2 = {.key = "status", .value = 789};
	keyValuePair test = {.key = "testtt", .value = 333};

	printf("Unesite oocekivan broj tema za obradu -> ");
	scanf("%d", &tableSize);

	if(tableSize >= 0 && tableSize <= 9)
		tableSize *= singleDigitMultFactor;

	else 
		tableSize *= doubleDigitMultFactor;

	keyValuePair** hashMap = malloc(tableSize * sizeof(keyValuePair));
	InitHashMap(hashMap);

	Insert(&analog, hashMap);
	Insert(&status, hashMap);
	Insert(&status1, hashMap);
	Insert(&status2, hashMap);
	Insert(&test, hashMap);

	PrintHashMap(hashMap);

	keyValuePair *tmp = FindKey(hashMap, "status");
	if(tmp != NULL)
		printf("\n\tFindKey() completed without any errors.\n");
	else
		printf("\n\t Error -> FindKey() could not locate the target key.\n");

	tmp = FindKey(hashMap, "nesto");
	if(tmp != NULL)
		printf("\n\tFind() completed without any errors.\n");
	else
		printf("\n\tError -> FindKey() could not locate the target key.\n");

	printf("\n\t%s", RemoveKey(hashMap, "analog"));
	printf("\n\t%s\n", RemoveKey(hashMap, "status"));

	PrintHashMap(hashMap);

	free(hashMap);
}