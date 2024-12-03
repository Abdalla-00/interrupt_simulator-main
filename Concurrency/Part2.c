#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>

#define NUM_TAS 5
#define NUM_STUDENTS 20
#define SHM_KEY 1234
#define SEM_KEY 5678

// Semaphore lock and unlock functions
void lock(int semid, int semnum) {
    struct sembuf op = {semnum, -1, 0}; // Decrement semaphore
    semop(semid, &op, 1);
}

void unlock(int semid, int semnum) {
    struct sembuf op = {semnum, 1, 0}; // Increment semaphore
    semop(semid, &op, 1);
}

// TA process logic
void TA_process(int id, int semid, int* students) {
    char filename[20];
    sprintf(filename, "TA%d.txt", id);

    FILE* output = fopen(filename, "w");
    if (!output) {
        perror("Failed to open TA file");
        exit(1);
    }

    int rounds = 0;
    int index = 0; // All TA's start at the first student (previously was id - 1)
    srand(time(NULL) ^ getpid()); // Initialize random seed for delays. By providing a dynamic value like in this case the current time or process id, this ensures numbers are more random

    while (rounds < 3) {
        // Lock semaphores for current TA and the next TA in the circular table
        lock(semid, id - 1);          // Lock TA's own semaphore
        lock(semid, id % NUM_TAS);    // Lock the next TA's semaphore

        // Access the shared memory
        printf("TA%d is currently accessing the database\n", id);
        int student = students[index]; // Access "database" and store student in local var

        // Simulate access database delay for 1-4 seconds
        sleep(rand() % 4 + 1);

        // Release semaphores before marking
        unlock(semid, id - 1);        // Unlock TA's own semaphore
        unlock(semid, id % NUM_TAS); // Unlock the next TA's semaphore

        // Mark the student's test
        int mark = rand() % 11; // Random mark between 0 and 10
        fprintf(output, "TA%d marked student %04d with %d\n", id, student, mark);
        fflush(output); // Ensure output is written immediately
        printf("TA%d is marking student %04d with %d\n", id, student, mark);

        // Simulate marking delay for 1-10 seconds
        sleep(rand() % 10 + 1);

        // Move to the next student
        index = (index + 1) % NUM_STUDENTS;

        // Check if the end of the list (student 9999) was just marked
        if (student == 9999) {
            rounds++; // Increment rounds after marking the last student
        }
    }

    fclose(output);
    printf("TA%d has finished marking.\n", id);
}



int main() {
    // Create shared memory for student list
    int shmid = shmget(SHM_KEY, NUM_STUDENTS * sizeof(int), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget failed");
        exit(1);
    }

    int* students = (int*)shmat(shmid, NULL, 0);
    if (students == (void*)-1) {
        perror("shmat failed");
        exit(1);
    }

    // Initialize the student list in shared memory
    for (int i = 0; i < NUM_STUDENTS - 1; i++) {
        students[i] = 0000 + (i+1); // Students numbered from 0001
    }
    students[NUM_STUDENTS - 1] = 9999; // End marker

    // Create semaphores
    int semid = semget(SEM_KEY, NUM_TAS, IPC_CREAT | 0666);
    if (semid < 0) {
        perror("semget failed");
        exit(1);
    }

    // Initialize all semaphores to 1 (unlocked state)
    for (int i = 0; i < NUM_TAS; i++) {
        if (semctl(semid, i, SETVAL, 1) < 0) {
            perror("semctl failed");
            exit(1);
        }
    }

    // Create TA processes
    for (int i = 1; i <= NUM_TAS; i++) {
        if (fork() == 0) {
            // Child process executes the TA logic
            TA_process(i, semid, students);
            exit(0); // Exit child process after work is done
        }
    }

    // Wait for all TA processes to finish
    for (int i = 0; i < NUM_TAS; i++) {
        wait(NULL);
    }

    // Cleanup: detach and remove shared memory
    shmdt(students);
    shmctl(shmid, IPC_RMID, NULL);

    // Remove semaphores
    semctl(semid, 0, IPC_RMID);

    printf("All TAs have finished marking.\n");
    return 0;
}
