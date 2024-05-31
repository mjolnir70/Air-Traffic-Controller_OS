#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>

#define MAX_TEXT 512

typedef struct {
    long int msg_type;	
    char msg_text[1024];
} PlaneDetails;

int planeID;
int totalWeight;
int planeType; // 1 for Passenger, 0 for Cargo
int numPassengers; // 0 for Cargo
int departureAirport;
int arrivalAirport;

void logPlaneActivity(const char* logMessage);
void informDepartureAirport(int msgid, PlaneDetails* msg);
void informArrivalAirport(int msgid, PlaneDetails* msg);
void informPlaneProcess(int msgid, PlaneDetails* msg);
void informAirportTermination(int msgid, int airport_id);
int checkAllPlanesDeparted(int msgid, int num_planes);

int main() {
    key_t key;
    int msgid;
    int numAirports;
    int numPlanes;

    printf("Enter the number of airports to be handled/managed: ");
    scanf("%d", &numAirports);

    key = ftok("plane.c", 65);
    msgid = msgget(key, 0666 | IPC_CREAT);

    PlaneDetails msg;

    while (1) {
        if (msgrcv(msgid, &msg, sizeof(msg), -31, 0) == -1) {
            perror("msgrcv failed");
           // printf("message size %ld", sizeof(msg)); 
            exit(EXIT_FAILURE);
        }

       // printf("msg received\n");
        fflush(stdin);

        if (strcmp(msg.msg_text, "TERMINATE") == 0) {
            printf("Termination signal received. Shutting down...\n");

            for (int i = 1; i <= numAirports; i++) {
                informAirportTermination(msgid, i);
            }

            if (checkAllPlanesDeparted(msgid, numPlanes)) {
                break; // Exit the loop and terminate
            }
        }

        sscanf(msg.msg_text, "%d %d %d %d %d %d", &planeID, &totalWeight, &planeType, &numPassengers, &departureAirport, &arrivalAirport);

        FILE *fp = fopen("AirTrafficController.txt", "a");
        if (fp == NULL) {
            perror("Error opening log file.");
            exit(EXIT_FAILURE);
        }

        fprintf(fp, "Plane %d has departed from Airport %d and will land at Airport %d.\n", planeID, departureAirport, arrivalAirport);
        fclose(fp);

       // printf("ID= %ld\n",msg.msg_type);
        //fflush(stdin);

        if (msg.msg_type <= 10) {
            informDepartureAirport(msgid, &msg);
        } else if (msg.msg_type <= 20) {
            informArrivalAirport(msgid, &msg);
        } else if (msg.msg_type <= 30) {
            informPlaneProcess(msgid, &msg);
        }
    }

    msgctl(msgid, IPC_RMID, NULL);

    return 0;
}

void logPlaneActivity(const char* logMessage) {
    FILE* logFile = fopen("AirTrafficController.txt", "a");
    if (logFile == NULL) {
        perror("Error opening log file.");
        return;
    }

    fprintf(logFile, "%s\n", logMessage);
    fclose(logFile);
}

void informDepartureAirport(int msgid, PlaneDetails* msg) {
    PlaneDetails message = *msg;
    message.msg_type = departureAirport + 40;
    sprintf(message.msg_text, "%d %d %d %d %d %d", planeID, totalWeight, planeType, numPassengers, departureAirport, arrivalAirport);

    if (msgsnd(msgid, &message, sizeof(message), 0) == -1) {
        perror("msgsnd failed");
        exit(EXIT_FAILURE);
    }
}

void informArrivalAirport(int msgid, PlaneDetails* msg) {
    PlaneDetails message = *msg;
    message.msg_type = arrivalAirport + 40;
    sprintf(message.msg_text, "%d %d %d %d %d %d", planeID, totalWeight, planeType, numPassengers, departureAirport, arrivalAirport);

    if (msgsnd(msgid, &message, sizeof(message), 0) == -1) {
        perror("msgsnd failed");
        exit(EXIT_FAILURE);
    }
}

void informPlaneProcess(int msgid, PlaneDetails* msg) {
    PlaneDetails message = *msg;
    message.msg_type = planeID + 50;
    strcpy(message.msg_text, "Process complete");

    if (msgsnd(msgid, &message, sizeof(message), 0) == -1) {
        perror("msgsnd failed");
        exit(EXIT_FAILURE);
    }
}

void informAirportTermination(int msgid, int airport_id) {
    PlaneDetails message;
    message.msg_type = airport_id + 40;
    strcpy(message.msg_text, "Termination");

    if (msgsnd(msgid, &message, sizeof(message), 0) == -1) {
        perror("msgsnd failed");
        exit(EXIT_FAILURE);
    }
}

int checkAllPlanesDeparted(int msgid, int num_planes) {
    int planes_departed = 0;
    for (int i = 1; i <= num_planes; i++) {
        PlaneDetails message;
        if (msgrcv(msgid, &message, sizeof(message), i, IPC_NOWAIT) != -1) {
            planes_departed++;
        }
    }
    return planes_departed == num_planes;
}

