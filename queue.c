#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include "queue.h"

Node* createNode(trainThread *trainData) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }
    newNode->info = *trainData;
    newNode->next = NULL;
    return newNode;
}


Node* insert(Node* head, trainThread * trainData) {
    Node* newNode = createNode(trainData);
    if (newNode == NULL) {
        return head; // Failed to create a new node, return the current head
    }

    if (head == NULL) {
        return newNode; // If the list is empty, the new node becomes the head
    }

    Node* cur = head;
    while (cur->next != NULL) { //iterate untill end of list
        cur = cur->next;
    }
    cur->next = newNode;
    return head; //stays the same
}

int isQEmpty(Node *head){
    if (head==NULL){
        return 1;
    } 
    return 0;
}

Node * removeByNum(Node *head, int num){
    Node * cur= head;
    Node *prev= NULL;
    
    if (head->info.num==num){
        head=removeHead(head);
        return head;
    }
    while(cur!=NULL && cur->info.num!=num){ //keep searching until a match 
        prev=cur;
        cur=cur->next;
    }
    if (cur==NULL){
        return NULL;
    }
    prev->next= cur->next; //update pointers
    free(cur);
    return prev;
 
}

Node* removeHead(Node* head) {
    if (head == NULL) {
        return NULL;
    }

    Node* newHead = head->next; // Point to the next node
    free(head); // Free the memory of the old head
    return newHead; // Return the new head
}


void printList(Node* head) {
    Node* current = head;
    while (current != NULL) {
        printf("Num: %d, Direction: %c, Priority: %d, Load: %d, Cross: %d\n",
        current->info.num, current->info.direction, current->info.priority, current->info.load, current->info.cross);
        current = current->next;
    }
}
