#ifndef QUEUE_H
#define QUEUE_H
#include <pthread.h>  

// Define trainThread structure
typedef struct trainThread {
    int num;
    char direction;
    char *dirStr;
    int priority;
    int load;
    int cross;
} trainThread;

// Define the Node structure using trainThread
typedef struct Node {
    trainThread info;
    struct Node* next;
} Node;

typedef struct allTrains {
    struct trainThread *all;
    int num_trains;
} allTrains;

Node* createNode(trainThread* trainData);
Node* insert(Node* head, trainThread* trainData);
int isQEmpty(Node* head);
Node* removeByNum(Node* head, int num);
Node* removeHead(Node* head);
void printList(Node* head);

#endif /* QUEUE_H */
