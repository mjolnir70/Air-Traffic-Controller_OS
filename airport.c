#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <limits.h>

#define MAX_TEXT 512
#define MAX_RUNWAYS 10
#define BACKUP_LOAD_CAPACITY 15000

typedef struct {
    long int msg_type;
    char msg_text[1024];
} PlaneDetails;

typedef struct {
    int number;
    int loadCapacity;
    pthread_mutex_t mutex;
} Runway;

int planeID;
int totalWeight;
int planeType;
int numPassengers;
int departureAirport;
int arrivalAirport;
int airportNumber;
int numRunways;

Runway runways[MAX_RUNWAYS + 1];
pthread_mutex_t messageQueueMutex = PTHREAD_MUTEX_INITIALIZER;

void* handlePlane(void* arg);
void informAirTrafficController(PlaneDetails* msg, int departureOrArrival);
int findBestFitRunway(int totalWeight);
void performBoardingOrDeboarding(int planeID, int runwayNumber, int isDeparture);

int msgid;

int main() {
    key_t key;

    key = ftok("plane.c", 65);
    msgid = msgget(key, 0666 | IPC_CREAT);

    for (int i = 1; i <= MAX_RUNWAYS; ++i) {
        runways[i].number = i;
        pthread_mutex_init(&runways[i].mutex, NULL);
    }

    printf("Enter Airport Number: ");
    scanf("%d", &airportNumber);

    printf("Enter number of Runways: ");
    scanf("%d", &numRunways);

    printf("Enter loadCapacity of Runways (give as a space separated list in a single line): ");
    for (int i = 1; i <= numRunways; ++i) {
        scanf("%d", &runways[i].loadCapacity);
    }

    runways[numRunways + 1].number = numRunways + 1;
    runways[numRunways + 1].loadCapacity = BACKUP_LOAD_CAPACITY;

    while (1) {
        PlaneDetails msg;

        if (msgrcv(msgid, &msg, sizeof(msg), airportNumber + 40, 0) == -1) {
            perror("msgrcv failed");
            exit(EXIT_FAILURE);
        }

        sscanf(msg.msg_text, "%d %d %d %d %d %d", &planeID, &totalWeight, &planeType, &numPassengers, &departureAirport, &arrivalAirport);

        pthread_t tid;
        pthread_create(&tid, NULL, handlePlane, (void*)&msg);
        pthread_detach(tid);
    }

    return 0;
}

void* handlePlane(void* arg) {
    PlaneDetails* plane = (PlaneDetails*)arg;
    int isDeparture = (departureAirport == airportNumber) ? 1 : 0;

    informAirTrafficController(plane, isDeparture);

    int runwayNumber = findBestFitRunway(totalWeight);

    pthread_mutex_lock(&runways[runwayNumber].mutex);
    performBoardingOrDeboarding(planeID, runwayNumber, isDeparture);
    pthread_mutex_unlock(&runways[runwayNumber].mutex);

    informAirTrafficController(plane, isDeparture);

    return NULL;
}

void informAirTrafficController(PlaneDetails* msg, int departureOrArrival) {
    pthread_mutex_lock(&messageQueueMutex);
    PlaneDetails message = *msg;
    message.msg_type = departureOrArrival ? airportNumber + 10 : airportNumber + 20;
    sprintf(message.msg_text, "%d %d %d %d %d %d", planeID, totalWeight, planeType, numPassengers, departureAirport, arrivalAirport);

    if (msgsnd(msgid, &message, sizeof(message), 0) == -1) {
        perror("msgsnd failed");
        exit(EXIT_FAILURE);
    }

    //printf("departure=%d msgtype=%ld\n", departureOrArrival, message.msg_type);

    pthread_mutex_unlock(&messageQueueMutex);
}

int findBestFitRunway(int totalWeight) {
    int bestFitRunway = -1;
    int minDifference = INT_MAX;

    for (int i = 1; i <= numRunways + 1; ++i) {
        int difference = runways[i].loadCapacity - totalWeight;

        if (difference >= 0 && difference < minDifference) {
            minDifference = difference;
            bestFitRunway = i;
        }
    }

    return bestFitRunway;
}

void performBoardingOrDeboarding(int planeID, int runwayNumber, int isDeparture) {
    if (isDeparture) {
        printf("Plane %d has completed boarding/loading and taken off from Runway No. %d of Airport No. %d.\n",
               planeID, runways[runwayNumber].number, airportNumber);
        sleep(3); // Simulate boarding process
    } else {
        printf("Plane %d has landed on Runway No. %d of Airport No. %d and has completed deboarding/unloading.\n",
               planeID, runways[runwayNumber].number, airportNumber);
        sleep(3); // Simulate deboarding process
    }
}

