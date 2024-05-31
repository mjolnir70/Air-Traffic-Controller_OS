    #include <stdio.h>

    #include <stdlib.h>

    #include <sys/ipc.h>

    #include <sys/msg.h>

    #include <unistd.h>

    #include <string.h>

    
    #include <sys/ipc.h>

    // Define the message buffer structure

    typedef struct msg_buffer {

        long msg_type;

        char msg_text[512];

    } message_buffer;

     

    int main() {

        key_t key;

        int msgid;

        message_buffer message;

     

        // Generate a unique key for the message queue

        key = ftok("plane.c", 65);

     

        // Connect to the existing message queue

        msgid = msgget(key, 0666|IPC_CREAT);

     

        if (msgid == -1) {

            perror("msgget failed");

            exit(EXIT_FAILURE);

        }

     

        char userInput;

        do {

            printf("Do you want the Air Traffic Control System to terminate? (Y for Yes / N for No): ");

            scanf("%c", &userInput);
		//printf("%c",userInput);
     

            // Use a space in the format string before %c to automatically skip any white spaces or newlines.

     

            if (userInput == 'Y' || userInput == 'y') {

                // Prepare and send the termination message to the ATC

                message.msg_type = 31; // Message type 1 for termination messages

                strcpy(message.msg_text, "TERMINATE");

     

                // Send the message to the message queue

                if (msgsnd(msgid, &message, sizeof(message.msg_text), 0) == -1) {

                    perror("msgsnd failed");

                    exit(EXIT_FAILURE);

                }

     

                printf("Termination signal sent to ATC.\n");

                break; // Exit the loop as we have sent the termination signal

            } else if (userInput == 'N' || userInput == 'n') {

                continue; // Continue to ask the user if they want to terminate

            } else {

                printf("Invalid input. Please enter 'Y' for Yes or 'N' for No.\n");

            }

        } while (userInput != 'Y' && userInput != 'y'); // Loop until 'Y' or 'y' is entered

     

        return 0;

    }
