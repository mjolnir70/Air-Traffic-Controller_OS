#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

#define MAX_PASSENGERS 10
#define CREW_WEIGHT 75
#define CREW_COUNT_PASSENGER 7
#define CREW_COUNT_CARGO 2
#define MAX_PLANES 100

typedef struct {
    int luggageWeight;
    int bodyWeight;
} Passenger;

typedef struct {
    int planeID;
    int totalWeight;
    int planeType; 
    int numPassengers; 
    int departureAirport;
    int arrivalAirport;
} PlaneDetails;

typedef struct {
    long int msg_type;
    char msg_text[1024];
} PlaneMsg;

void handlePassengerProcess(int passengerNum, int writePipe[]);

int main() {
    int planeCount = 0;
    PlaneDetails planes[MAX_PLANES];
    int planeID, planeType;
    int totalW = 0;
    int numOccupiedSeats = 0;
    int numCargoItems = 0;
    int avgCargoWeight = 0;

    printf("Enter Plane ID: ");
    scanf("%d", &planeID);

    printf("Enter Type of Plane (1 for Passenger, 0 for Cargo): ");
    scanf("%d", &planeType);

    PlaneDetails msg;
    PlaneMsg message;

    planes[planeCount].planeID = planeID;
    planes[planeCount].planeType = planeType;

    if (planeType == 1) {
        printf("Enter Number of Occupied Seats: ");
        scanf("%d", &numOccupiedSeats);
        planes[planeCount].numPassengers = numOccupiedSeats;

        printf("Enter Airport Number for Departure: ");
        scanf("%d", &planes[planeCount].departureAirport);
        if (planes[planeCount].departureAirport < 1 || planes[planeCount].departureAirport > 10) {
            fprintf(stderr, "Invalid airport number! Please enter a number between 1 and 10.\n");
            exit(EXIT_FAILURE);
        }

        printf("Enter Airport Number for Arrival: ");
        scanf("%d", &planes[planeCount].arrivalAirport);
        if (planes[planeCount].arrivalAirport < 1 || planes[planeCount].arrivalAirport > 10) {
            fprintf(stderr, "Invalid airport number! Please enter a number between 1 and 10.\n");
            exit(EXIT_FAILURE);
        }

        int pipes[numOccupiedSeats][2]; 
        for (int i = 0; i < numOccupiedSeats; ++i) {
            if (pipe(pipes[i]) == -1) {
                perror("pipe failed");
                exit(EXIT_FAILURE);
            }
        }

        int lug[numOccupiedSeats];
        int weight[numOccupiedSeats];
        
        for(int i=0; i<numOccupiedSeats; i++){
            printf("Enter Weight of Your Luggage: \n");
            scanf("%d", &lug[i]);}
        for(int i=0; i<numOccupiedSeats; i++) {
            printf("Enter Your Body Weight: \n");
            scanf("%d", &weight[i]);
            totalW += weight[i] + lug[i];
        }

        for (int i = 0; i < numOccupiedSeats; ++i) {
            pid_t pid = fork();
            if (pid == 0) {
                close(pipes[i][0]);
                exit(0);
            } else if (pid > 0) {
                close(pipes[i][1]);
            } else {
                printf("Fork failed\n");
                exit(EXIT_FAILURE);
            }
        }

        for (int i = 0; i < numOccupiedSeats; ++i) {
            Passenger p;
            read(pipes[i][0], &p, sizeof(Passenger));
            totalW += p.luggageWeight + p.bodyWeight;
            close(pipes[i][0]);
        }

        totalW += CREW_COUNT_PASSENGER * CREW_WEIGHT;
    } else if (planeType == 0) {
        printf("Enter Number of Cargo Items: ");
        scanf("%d", &numCargoItems);

        printf("Enter Average Weight of Cargo Items: ");
        scanf("%d", &avgCargoWeight);

        totalW = numCargoItems * avgCargoWeight + CREW_COUNT_CARGO * CREW_WEIGHT;

        printf("Enter Airport Number for Departure: ");
        scanf("%d", &planes[planeCount].departureAirport);
        if (planes[planeCount].departureAirport < 1 || planes[planeCount].departureAirport > 10) {
            fprintf(stderr, "Invalid airport number! Please enter a number between 1 and 10.\n");
            exit(EXIT_FAILURE);
        }

        printf("Enter Airport Number for Arrival: ");
        scanf("%d", &planes[planeCount].arrivalAirport);
        if (planes[planeCount].arrivalAirport < 1 || planes[planeCount].arrivalAirport > 10) {
            fprintf(stderr, "Invalid airport number! Please enter a number between 1 and 10.\n");
            exit(EXIT_FAILURE);
        }
    }

    key_t key;
    int msgid;
    key = ftok("plane.c", 65);
    msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget failed");
        exit(EXIT_FAILURE);
    }

    message.msg_type = planeID;
    msg.planeID = planeID;
    msg.departureAirport = planes[planeCount].departureAirport;
    msg.arrivalAirport = planes[planeCount].arrivalAirport;
    msg.totalWeight = totalW;
    sprintf(message.msg_text, "%d %d %d %d %d %d", msg.planeID, msg.totalWeight, msg.planeType, msg.numPassengers, msg.departureAirport, msg.arrivalAirport);
    msgsnd(msgid, &message, sizeof(message), 0);

    while (1) {
        if (msgrcv(msgid, &message, sizeof(message), planeID + 50, 0) == -1) {
            perror("msgrcv failed");
            exit(EXIT_FAILURE);
        }
        if (msg.planeID == planeID) {
        sleep(30);
            printf("Plane %d has successfully traveled from Airport %d to Airport %d!\n", planeID, msg.departureAirport, msg.arrivalAirport);
            break;
        }
    }

    exit(EXIT_SUCCESS);
}

void handlePassengerProcess(int passengerNum, int writePipe[]) {
    int luggageWeight, bodyWeight;
    printf("Passenger %d, Enter Weight of Your Luggage: ", passengerNum);
    fflush(stdout);
    scanf("%d", &luggageWeight);
    printf("Passenger %d, Enter Your Body Weight: ", passengerNum);
    fflush(stdout);
    scanf("%d", &bodyWeight);
    Passenger p = {luggageWeight, bodyWeight};
    write(writePipe[1], &p, sizeof(Passenger));
    close(writePipe[1]);
}

