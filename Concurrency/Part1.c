#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <time.h>

#define NUM_TAS 5
#define DATABASE "database.txt"

// Function Prototypes
void semaphore_lock(int semid, int sem_num);
void semaphore_unlock(int semid, int sem_num);
void ta_process(int ta_id, int semid);

// Helper Function to generate random nums
int generate_random(int min, int max) {
    return rand() % (max - min + 1) + min;
}

int main() {
    // Set up semaphores and TA processes
    key_t sem_key = 1234; // Semaphore key
    int semid;
    pid_t pids[NUM_TAS];

    // Create semaphore set with 5 semaphores
    semid = semget(sem_key, NUM_TAS, IPC_CREAT | 0666);
    if (semid < 0) {
        perror("semget failed");
        exit(EXIT_FAILURE);
    }

    // Initialize all semaphores to 1 (unlocked)
    for (int i = 0; i < NUM_TAS; i++) {
        if (semctl(semid, i, SETVAL, 1) < 0) {
            perror("semctl failed");
            exit(EXIT_FAILURE);
        }
    }

    // Fork 5 processes, one for each TA
    for (int i = 0; i < NUM_TAS; i++) {
        if ((pids[i] = fork()) == 0) {
            // Child process: TA functionality
            ta_process(i + 1, semid);
            exit(0);
        } else if (pids[i] < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
    }

    // Parent process waits for all child processes to complete
    for (int i = 0; i < NUM_TAS; i++) {
        waitpid(pids[i], NULL, 0);
    }

    // Remove semaphores
    semctl(semid, 0, IPC_RMID, 0);

    return 0;
}

// Function: TA Process
void ta_process(int ta_id, int semid) {
    char filename[20];
    snprintf(filename, sizeof(filename), "TA%d.txt", ta_id); // Create TA file
    FILE *ta_file = fopen(filename, "w"); // Open the files for writing and clear their current contents
    if (!ta_file) {
        perror("Failed to create TA file");
        exit(EXIT_FAILURE);
    }

    FILE *db_file = fopen(DATABASE, "r");
    if (!db_file) {
        perror("Failed to open database file");
        exit(EXIT_FAILURE);
    }

    int cycle_count = 0;
    char student_number[5]; // String to hold student ID (4 digits + null terminator)

    while (cycle_count < 3) { // Process each student 3 times
        // Lock current and next semaphore
        semaphore_lock(semid, ta_id - 1);
        semaphore_lock(semid, ta_id % NUM_TAS);

        // Access the database
        if (fscanf(db_file, "%4s", student_number) == EOF) {
            // Rewind if end of file is reached
            rewind(db_file);
            cycle_count++;
            continue;
        }
        printf("TA %d accessing database: Student %s\n", ta_id, student_number);

        // Simulate delay while accessing database
        sleep(generate_random(1, 4));

        // Unlock semaphores
        semaphore_unlock(semid, ta_id - 1);
        semaphore_unlock(semid, ta_id % NUM_TAS);

        // Mark the student
        int mark = generate_random(0, 10);
        printf("TA %d marking Student %s with grade %d\n", ta_id, student_number, mark);
        fprintf(ta_file, "Student %s: Grade %d\n", student_number, mark);
        fflush(ta_file); // Ensure data is written to the file immediately

        // Simulate marking delay
        sleep(generate_random(1, 10));
    }

    fclose(ta_file); // Close the TA file
    fclose(db_file); // Close the database file
    printf("TA %d finished marking.\n", ta_id);
}


// Semaphore Lock
void semaphore_lock(int semid, int sem_num) {
    struct sembuf sb = {sem_num, -1, 0}; // Decrement semaphore
    semop(semid, &sb, 1);
}

// Semaphore Unlock
void semaphore_unlock(int semid, int sem_num) {
    struct sembuf sb = {sem_num, 1, 0}; // Increment semaphore
    semop(semid, &sb, 1);
}
