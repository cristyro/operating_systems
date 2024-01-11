#include <pthread.h>
#include <stdio.h>
#include <unistd.h> 
#include <stdlib.h>
#include <string.h>
#include <ctype.h> 
#include <time.h>
#include "queue.h"
/*----------Constants and global variables -----------*/
#define BUF 3000
#define INIT_GUESS 256
#define NUMSTATIONS 4
#define BILLION  1000000000L
int trainsDone=0;
struct timespec start;
typedef struct trainStation{// an access to all the stations queues
    Node* EastPriority;
    Node* east;
    Node* WestPriority;
    Node* west;
}trainStation;
Node * trainsFinished=NULL; //global station queue
struct allTrains infoTrains;
trainStation trainDirection;
pthread_mutex_t oneTrain= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t finish = PTHREAD_COND_INITIALIZER;
pthread_cond_t trainSent = PTHREAD_COND_INITIALIZER;
pthread_mutex_t numUpdate= PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t loadBarrier;
int done=0;
int countEast=0;
int countWest=0;
char lastDir='n';

/*----------Secondary helper functions -----------*/
//Counts the number of lines we have in the file 
int countLines(char *fileCount){
    int count=0;
    FILE *ptr;
    if (fileCount!=NULL){
    ptr= fopen(fileCount, "r");
    if (ptr==NULL){
        printf("Could not read from file \n");
        exit(1);
    }
    for (char c = getc(ptr); c != EOF; c = getc(ptr)){
        if (c == '\n') // Increment count if this character is newline
        count = count + 1;
    }
    }
    fclose(ptr);
    return count;
}

//Returns 1 if we have a priority train
int isPriority (char letter){
    if (isupper(letter)){
        return 1;
    }
    return 0;
}


//returns 1 if they are opposite, 0 if they arent. -1 if could not compare
int areOpposite(Node *compare, Node *contrast){
    if (compare==NULL ||contrast==NULL){
        return -1;
    } else if (compare->info.direction == contrast->info.direction){
        return 0;
    }else{
        return 1;
    }
}

//Initializes simulation time
void startTime(){
    if (clock_gettime(CLOCK_REALTIME, &start) == -1) {
        exit(1);
    }
}

//Calculates elapsed time since the simulation start
double elapsedTime() {
    struct timespec currentTime;
    if (clock_gettime(CLOCK_REALTIME, &currentTime) == -1) {
        exit(1);
    }
    return (double)(currentTime.tv_sec - start.tv_sec) + 
           (double)(currentTime.tv_nsec - start.tv_nsec) / BILLION;
}

//Prints the time in hours:min:sec format
void printTime(double curTime){
    int hours= (int)(curTime/3600);
    int minutes = (int)((curTime - hours * 3600) / 60);
    double seconds = curTime - hours * 3600 - minutes * 60;
    printf("%02d:%02d:%04.1lf ", hours, minutes, seconds);
}

/*Adds to 5 Queues: one queue for each direction (E, e,W,w) and a global queue named trainsFinished keeping
    track of all the trains added to the queue.
*/    
void addToQueue(trainThread* train){
    pthread_mutex_lock(&oneTrain);
    trainsFinished=insert(trainsFinished, train);
    pthread_cond_signal(&trainSent);
    pthread_mutex_unlock(&oneTrain);

    switch(train->direction){
        case 'E':
            pthread_mutex_lock(&oneTrain);
            trainDirection.EastPriority= insert(trainDirection.EastPriority, train);
            pthread_mutex_unlock(&oneTrain);
            break;
        case 'e':
            pthread_mutex_lock(&oneTrain);
            trainDirection.east= insert(trainDirection.east, train);
            pthread_mutex_unlock(&oneTrain);
            break;           
        case 'W':
            pthread_mutex_lock(&oneTrain);
            trainDirection.WestPriority= insert(trainDirection.WestPriority, train);
            pthread_mutex_unlock(&oneTrain);
            break;            
        case 'w':
            pthread_mutex_lock(&oneTrain);
            trainDirection.west= insert(trainDirection.west, train);
            pthread_mutex_unlock(&oneTrain);
            break; 
    }
}

/* Removes according to the direction*/
void removeDirQ(Node *ready){
        switch(ready->info.direction){
        case 'E':
            trainDirection.EastPriority= removeHead(trainDirection.EastPriority);
            break;
        case 'e':
            trainDirection.east= removeHead(trainDirection.east);
            break;           
        case 'W':
            trainDirection.WestPriority= removeHead(trainDirection.WestPriority);
            break;            
        case 'w':
            trainDirection.west= removeHead(trainDirection.west);
            break; 
    }
}

/*----Load and train sending functions -------*/
//Loads all the trains simultaneosly. To do so, I use a barrier waiting for all threads read in the file.
void* loadTrain(void* arg) {
    struct trainThread* train = (struct trainThread*)arg;
    pthread_barrier_wait(&loadBarrier); //wait for all threads to reach here!
    usleep(train->load * 100000); //usleep in micro seconds to tenths of a second
    double curTime= elapsedTime(); 
    printTime(curTime);
    printf("Train %2d is ready to go %s \n", train->num, train->dirStr);
    addToQueue(train); //returning from adding to queue
    return NULL;
}

//It is used by the main track thread to send a train once in main track
void trainGo(Node* ready){ 
    int crosstime= ready->info.cross;
    usleep(crosstime* 100000);
    double curTime= elapsedTime(); 
    printTime(curTime);
    printf("Train %2d is OFF the main track after going %s \n", ready->info.num, ready->info.dirStr);
    done++;

    if (done==infoTrains.num_trains){
        pthread_cond_signal(&finish);
    }
    return;
}

/*--------Helper functions for dispatcher algorithm. This functions are used to decide which train will be picked next by the dispatcher--------*/
//Function in charge of checking for trains waiting going east, as well as updating the last direction to east and updating countEast and countWest.
Node * eastStation(Node *next){
    pthread_mutex_lock(&numUpdate);
    lastDir='e';
     countEast++;
     countWest=0;
     if (isQEmpty(trainDirection.EastPriority)==0){//has something
        next->info= trainDirection.EastPriority->info;
     }else if(isQEmpty(trainDirection.east)==0){
        next->info= trainDirection.east->info;
     }
     return next;
}

//Function in charge of checking for trains waiting going west, as well as updating the last direction to east and updating countEast and countWest.
Node *westStation(Node *next){
    pthread_mutex_lock(&numUpdate);
    lastDir='w';
    countWest++;
    countEast=0;
    if (isQEmpty(trainDirection.WestPriority)==0){//has something
        next->info= trainDirection.WestPriority->info;
     }  else if (isQEmpty(trainDirection.west)==0){
        next->info= trainDirection.west->info;
     }
     
     return next;
}

//Handles the case of having 3 trains back to back in one direction. 
int avoidStarve(Node *goingTrack){
    int flag=0;
    if (countEast==3 && (!isQEmpty(trainDirection.west) || !isQEmpty(trainDirection.WestPriority))){
        flag=1;
        goingTrack= westStation(goingTrack);
        pthread_mutex_unlock(&numUpdate);
    }
    if (countWest==3 && (!isQEmpty(trainDirection.east) || !isQEmpty(trainDirection.EastPriority))){
        flag=1;
        goingTrack= eastStation(goingTrack);
        pthread_mutex_unlock(&numUpdate);
    }
    return flag;
}

//Handles the case of having a single train waiting in the queue waiting to be dispatched
void basicCase(Node *goingTrack){
    goingTrack->info= trainsFinished->info;
    int compareEast= goingTrack->info.direction=='e' || goingTrack->info.direction=='E';
    if (compareEast){
        goingTrack=eastStation(goingTrack);
        pthread_mutex_unlock(&numUpdate);
    } else{
         goingTrack=westStation(goingTrack);
        pthread_mutex_unlock(&numUpdate);
    }
}

//Manages the case of having 2 trains waiting for main track with the same load time and priority going in opposite directions.
void alternateDirections(Node *goingTrack){
    if (areOpposite(trainsFinished, trainsFinished->next)){
        if (lastDir=='n' || lastDir=='e'){
            goingTrack= westStation(goingTrack);
            pthread_mutex_unlock(&numUpdate);
        }else{ //go into here if lastDir is W
            goingTrack= eastStation(goingTrack);
            pthread_mutex_unlock(&numUpdate);
        }
    }
}

/*  -----Primary helper functions ----------*/
//Implements the helper functions to determine which train goes into main track.
Node * mainTrack(){
    Node *goingTrack=(Node*)malloc(sizeof(Node));
    if (goingTrack==NULL){
        printf("memory could not be allocated \n");
        exit(1); 
    }
    goingTrack->info.num=-1;
    goingTrack->info.dirStr="blank";

   if (countEast==3 || countWest==3){
    int flag= avoidStarve(goingTrack);
    if (!flag){
        basicCase(goingTrack);
    }
    } else{
        if (trainsFinished!=NULL){ //only that one train waiting
        basicCase(goingTrack);
        } else if (trainsFinished!=NULL && trainsFinished->next!=NULL){//we have 2 trains in q 
            alternateDirections(goingTrack);
        }
    }
    double curTime= elapsedTime(); 
    printTime(curTime);
    printf("Train %2d is ON the main track going %s \n",  goingTrack->info.num, goingTrack->info.dirStr);
    trainGo(goingTrack);
    return goingTrack;
}


/* This function calls other helper functions with the following functionality: 
 - Waits for conditional variable to signal that we have a train to process
 - Dispatches the trains ready to mainTrack
 - Updates the queues (global and direction specific) after the train has been dispatched
 - Waits for all the trains to be sent to the main track 
*/
void* dispatchTrains(void* arg) {
    while(1){
        pthread_mutex_lock(&oneTrain);
        while(trainsFinished==NULL){
            pthread_cond_wait(&trainSent, &oneTrain); //wait for one train to be sent through signal
        }        
        Node *ready= mainTrack();
        removeDirQ(ready);
        if (trainsFinished!=NULL){
            trainsFinished= removeByNum(trainsFinished, ready->info.num);
        }
        pthread_mutex_unlock(&oneTrain);
        if (done==infoTrains.num_trains){
            break;
        }
    }
    return NULL;
}

//Initializes and joins all train threads, as well as the dispatcher thread
void organizer(struct allTrains info){
    pthread_t ptid;
    pthread_t *trains= (pthread_t *)calloc(info.num_trains,sizeof(pthread_t)); 
    pthread_barrier_init(&loadBarrier, NULL, info.num_trains); 
    for (int i=0;  i<info.num_trains; i++){
        pthread_create(&trains[i], NULL, &loadTrain, &info.all[i]);
    }
    pthread_mutex_lock(&lock);
    pthread_create(&ptid,NULL,dispatchTrains,NULL); //can change first attribute if we need to by the struct of the thread we want to have
    pthread_cond_wait(&finish, &lock);
    pthread_mutex_unlock(&lock);
    pthread_join(ptid, NULL);
    for (int i=0;  i<info.num_trains; i++){
        pthread_join(trains[i], NULL);
    }
    pthread_barrier_destroy(&loadBarrier);
}

//Reads file input and initializes each line read as a train struct.
struct allTrains FileProcess(FILE *file, int num){
    char ch;
    int crossTime, loadTime, numThreads=0;
    infoTrains.all=(struct trainThread *)calloc(num, sizeof(struct trainThread)); //the struct to be given to threads
     if (infoTrains.all==NULL){
        printf("Memory could not be allocated \n");
        exit(1);
    }
    while(fscanf(file,"%c %d %d", &ch, &loadTime, &crossTime)!=EOF){
        if (isspace(ch)==0){ 
            infoTrains.all[numThreads].num=numThreads;
            infoTrains.all[numThreads].direction=ch;
            infoTrains.all[numThreads].dirStr = (ch == 'e' || ch == 'E') ? strdup("East") : strdup("West"); //assigns a direction string according to east or west
            infoTrains.all[numThreads].priority= isPriority(ch);
            infoTrains.all[numThreads].load= loadTime;
            infoTrains.all[numThreads].cross=crossTime;
            numThreads++;
        }
    }
    infoTrains.num_trains= numThreads;
    return infoTrains;        
}

//Train dispatcher simulator 
int main(int argc, char *argv[] ){
    char *fileName= argv[1];
    int num= countLines(argv[1]);
    if (argc>2){
        printf("only 2 inputs accepted, retry");
        exit(1);
    }
    if(fileName!=NULL){ //if provided an argument to terminal 
        FILE *file= fopen(fileName, "r");
        if (file==NULL){
            printf("Could not open file \n");
            exit(1);
        }
        struct allTrains trainsFromFile=FileProcess(file, num);
        startTime();
        organizer(trainsFromFile);
        for (int i = 0; i < trainsFromFile.num_trains; i++) {
        free(trainsFromFile.all[i].dirStr);
        }
        free(trainsFromFile.all);
        fclose(file);
    }
    return 0;
}
