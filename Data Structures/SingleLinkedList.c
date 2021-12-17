#include <stdio.h>
#include <stdlib.h>

struct Node
{
	int data;
	struct Node *next;
};

void printList(struct Node *head)
{
	struct Node *pomocna = head;

	while(pomocna != NULL)
	{
		printf("%d => ", pomocna->data);
		pomocna = pomocna->next;
	}

	printf("\n\n");
}

void printAtIndex(int i, struct Node *head)
{
	struct Node *pomocna = head;
	int brojac = 0;

	while(pomocna != NULL)
	{
		++brojac;
		if(brojac == i)
		{
			printf("%d => ", pomocna->data);
			break;
		}
		else
			pomocna = pomocna->next;
	}

	printf("\n\n");
}

void insertAtBeginning(struct Node **head, int data)
{
	struct Node *link = (struct Node*)malloc(sizeof(struct Node));
	link->data = data;
	link->next = *head;
	*head = link;
}

void insertAtTheEnd(struct Node **head, int data)
{
	struct Node *link = (struct Node*)malloc(sizeof(struct Node));
	link->data = data;
	link->next = NULL;

	if(*head == NULL)
		*head = link;

	else
	{
		struct Node *pomocna = *head;

		while(pomocna->next != NULL)
			pomocna = pomocna->next;

		pomocna->next = link;
	}
}

void listSearch(struct Node **head, int data)
{
	struct Node *pomocna = *head;
	while(pomocna->next != NULL)
	{
		if(pomocna->data == data)
		{
			printf("BROJ PRONADJEN! \n\n");
			break;
		}
		else
			pomocna = pomocna->next;
	}
}

void listEmpty(struct Node **head)
{
	struct Node *pomocna;
	while(*head != NULL)
	{
		pomocna = *head;
		*head = (*head)->next;
		free(pomocna);
	}
}

int main()
{

	struct Node *head = NULL;
	struct Node *current = NULL;

	for(int i = 0; i < 100000; i++)
		insertAtBeginning(&head, rand()%1000001);

	listEmpty(&head);

	for(int i = 0; i < 500; i++)
		insertAtTheEnd(&head, rand()%1000001);

	printAtIndex(250, head);

	return 0;
}
