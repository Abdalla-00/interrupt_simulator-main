#include "interrupts.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // For usleep()
#include <time.h>    // For random numbers

// The vector table will be hardcoded
VectorEntry vectorTable[VECTOR_TABLE_SIZE] = {
    {1, 0x01A3}, {2, 0x024D}, {3, 0x03F6}, {4, 0x05A9}, {5, 0x0467},
    {6, 0x022B}, {7, 0x03D1}, {8, 0x0659}, {9, 0x027A}, {10, 0x04F2},
    {11, 0x01EC}, {12, 0x0398}, {13, 0x05B4}, {14, 0x06AD}, {15, 0x02C9},
    {16, 0x04C1}, {17, 0x071D}, {18, 0x0603}, {19, 0x0387}, {20, 0x06C4},
    {21, 0x073E}, {22, 0x02B2}, {23, 0x03FD}, {24, 0x0568}, {25, 0x019F}
};

// Function to load the input trace data
int loadTrace(const char* filename, TraceEntry** traceEntries){
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open trace file: %s\n", filename);
        return -1;
    }

    char line[50]; // Buffer
    int count = 0;

    // Count the number of entries in the file
    while (fgets(line, sizeof(line), file)) {
        count++;
    }

    // Allocate memory for the trace entries
    *traceEntries = malloc(count * sizeof(TraceEntry));
    if (!*traceEntries) {
        printf("Error: Memory allocation failed\n");
        fclose(file);
        return -1;
    }

    rewind(file);  // Go back to the start of the file

    // Load the trace entries into memory
    int index = 0;
    while (fgets(line, sizeof(line), file)) {
        TraceEntry entry;
        sscanf(line, "%[^,], %d", entry.activity, &entry.duration);

        // If SYSCALL or END_IO, extract the position
        if (strncmp(entry.activity, "SYSCALL", 7) == 0 || strncmp(entry.activity, "END_IO", 6) == 0) {
            sscanf(line, "%s %d, %d", entry.activity, &entry.position, &entry.duration);
        } else {
            entry.position = -1;  // No position for CPU activities
        }

        (*traceEntries)[index++] = entry;
    }

    fclose(file);
    return count;
}

// Function to simulate CPU usage
void useCPU(int duration, FILE* logFile, int* currentTime) {
    fprintf(logFile, "%d, %d, CPU execution\n", *currentTime, duration);
    *currentTime += duration;
}

// Function to simulate a system call (SYSCALL)
void handleSysCall(int position, int duration, FILE* logFile, int* currentTime) {
    fprintf(logFile, "%d, 1, switch to kernel mode\n", *currentTime);
    *currentTime += 1;

    // Save context (random between 1-3 ms)
    int contextSaveDuration = rand() % 3 + 1;
    fprintf(logFile, "%d, %d, context saved\n", *currentTime, contextSaveDuration);
    *currentTime += contextSaveDuration;

    // Find vector in memory and get ISR address
    int vectorIndex = position - 1;
    int isrAddress = vectorTable[vectorIndex].memoryAddress;

    fprintf(logFile, "%d, 1, find vector %d in memory position 0x%04X\n", *currentTime, position, vectorIndex * 2);
    *currentTime += 1;
    fprintf(logFile, "%d, 1, load address 0x%04X into the PC\n", *currentTime, isrAddress);
    *currentTime += 1;

    // Simulate ISR execution
    int isrDuration = rand() % 301 + 100; // Random ISR duration between 100 and 400 ms
    fprintf(logFile, "%d, %d, SYSCALL: run the ISR\n", *currentTime, isrDuration);
    *currentTime += isrDuration;

    // Simulate data transfer and error checking
    int dataTransferDuration = rand() % 100 + 50; // Random data transfer time
    fprintf(logFile, "%d, %d, transfer data\n", *currentTime, dataTransferDuration);
    *currentTime += dataTransferDuration;

    int errorCheckDuration = rand() % 100 + 50; // Random error check time
    fprintf(logFile, "%d, %d, check for errors\n", *currentTime, errorCheckDuration);
    *currentTime += errorCheckDuration;

    // IRET (interrupt return)
    fprintf(logFile, "%d, 1, IRET\n", *currentTime);
    *currentTime += 1;
}

// Function to handle END_IO interrupts
void handleEndIO(int position, int duration, FILE* logFile, int* currentTime) {
    fprintf(logFile, "%d, 1, check priority of interrupt\n", *currentTime);
    *currentTime += 1;
    fprintf(logFile, "%d, 1, check if masked\n", *currentTime);
    *currentTime += 1;

    fprintf(logFile, "%d, 1, switch to kernel mode\n", *currentTime);
    *currentTime += 1;

    // Save context (random between 1-3 ms)
    int contextSaveDuration = rand() % 3 + 1;
    fprintf(logFile, "%d, %d, context saved\n", *currentTime, contextSaveDuration);
    *currentTime += contextSaveDuration;

    // Find vector in memory and get ISR address
    int vectorIndex = position - 1;
    int isrAddress = vectorTable[vectorIndex].memoryAddress;

    fprintf(logFile, "%d, 1, find vector %d in memory position 0x%04X\n", *currentTime, position, vectorIndex * 2);
    *currentTime += 1;
    fprintf(logFile, "%d, 1, load address 0x%04X into the PC\n", *currentTime, isrAddress);
    *currentTime += 1;

    // Simulate END_IO interrupt
    fprintf(logFile, "%d, %d, END_IO\n", *currentTime, duration);
    *currentTime += duration;

    // IRET (interrupt return)
    fprintf(logFile, "%d, 1, IRET\n", *currentTime);
    *currentTime += 1;
}

// argc (argument count) represents the number of command line arguments passed to the main function
// argv (argument vector) is an array of strings (array of char pointers) containing the actual command line arguments
int main (int argc, char *argv[]){
   srand(time(NULL));  // Set random seed based on the current time, essentially sets the starting point for the random num gen to be current time

    // Check if the correct number of arguments is passed (trace file as an argument, along with this file name)
    if (argc < 2) {
        printf("Error: No trace file specified.\n");
        return 1;
    }

    TraceEntry* traceEntries;
    // Load the trace file input from the path specified in the command line argument
    int traceCount = loadTrace(argv[1], &traceEntries);
    if (traceCount == -1) {
        return 1;  // Error loading trace
    }

    FILE* logFile = fopen("execution.txt", "w");
    if (!logFile) {
        printf("Error: Could not create execution.txt\n");
        return 1;
    }

    int currentTime = 0;  // Simulated time in milliseconds

    // Process each trace entry depending on its activity
    for (int i = 0; i < traceCount; i++) {
        if (strcmp(traceEntries[i].activity, "CPU") == 0) {
            useCPU(traceEntries[i].duration, logFile, &currentTime);
        } else if (strcmp(traceEntries[i].activity, "SYSCALL") == 0) {
            handleSysCall(traceEntries[i].position, traceEntries[i].duration, logFile, &currentTime);
        } else if (strcmp(traceEntries[i].activity, "END_IO") == 0) {
            handleEndIO(traceEntries[i].position, traceEntries[i].duration, logFile, &currentTime);
        }
    }

    fclose(logFile);
    free(traceEntries);
    return 0;
}
