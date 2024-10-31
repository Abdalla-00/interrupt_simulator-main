#include "interrupts.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // For usleep()
#include <time.h>    // For random numbers

// Constants for process states
#define READY 0
#define RUNNING 1
#define FORK_ISR_ADDRESS 2
#define EXEC_ISR_ADDRESS 3

// The vector table will be hardcoded
VectorEntry vectorTable[VECTOR_TABLE_SIZE] = {
    {0, 0x01E3}, {1, 0x029C}, {2, 0x0695}, {3, 0x042B}, {4, 0x0292},
    {5, 0x048B}, {6, 0x0639}, {7, 0x00BD}, {8, 0x06EF}, {9, 0x036C},
    {10, 0x07B0}, {11, 0x01F8}, {12, 0x03B9}, {13, 0x06C7}, {14, 0x0165},
    {15, 0x0584}, {16, 0x02DF}, {17, 0x05B3}, {18, 0x060A}, {19, 0x0765},
    {20, 0x07B7}, {21, 0x0523}, {22, 0x03B7}, {23, 0x028C}, {24, 0x05E8},
    {25, 0x05D3}
};


// Initializes the memory partitions
void initMemoryPartitions(MemoryPartition* partitions) {
    int sizes[MAX_PARTITIONS] = {40, 25, 15, 10, 8, 2};
    for (int i = 0; i < MAX_PARTITIONS; i++) {
        partitions[i].partitionNumber = i + 1;
        partitions[i].freeSpace = sizes[i];  // Initially, the entire partition is free
        partitions[i].size = sizes[i];
        strcpy(partitions[i].status, "free");
    }
}

// Function to find the best-fit memory partition for a program, checking lowest memory first
void findBestPartition(int programSize, MemoryPartition* partitions, int* bestPartition, int* bestsPartitionIndex) {
    int partition = -1;
    int minSizeDiff = 100; // Arbitrary large number for initial comparison

    // Loop through the partitions from the end to the beginning
    for (int i = MAX_PARTITIONS - 1; i >= 0; i--) {
        // Check if the partition is free and large enough
        if (strcmp(partitions[i].status, "free") == 0 && partitions[i].freeSpace >= programSize) {
            int sizeDiff = partitions[i].freeSpace - programSize;
            // Update the best partition if a smaller size difference is found
            if (sizeDiff < minSizeDiff) {
                minSizeDiff = sizeDiff;
                partition = i;
            }
        }
    }
    *bestsPartitionIndex = partition;
    *bestPartition= partitions[partition].partitionNumber;
}

// Function to randomly split an integer into 'numParts' parts
void randomSplit(int originalValue, int numParts, int* parts) {
    if (numParts <= 0 || originalValue <= 0) {
        return; // Return if inputs are invalid
    }

    int remainingValue = originalValue;

    // Distribute values randomly among the parts
    for (int i = 0; i < numParts - 1; i++) {
        // Generate a random value between 1 and remainingValue - (numParts - i - 1)
        // This ensures there is enough left for the remaining parts
        int maxSplit = remainingValue - (numParts - i - 1);
        parts[i] = (maxSplit > 1) ? (rand() % (maxSplit - 1) + 1) : 1;
        remainingValue -= parts[i];
    }

    // Assign the remaining value to the last part
    parts[numParts - 1] = remainingValue;
}

// Find memory size of program from external program list
int findProgramSize(ExternalFile* externalFiles,int externalFileCount, char* programName){
    for (size_t i = 0; i < externalFileCount; i++){
        if (strcmp(externalFiles[i].programName, programName) == 0) return externalFiles[i].size;
    }
    return -1;    
} 

// Creates a new PCB and while assigning it to its parent
PCB* createPCB(int pid, char* programName, int partition, int size, PCB* parent) {
    PCB* newPCB = (PCB*)malloc(sizeof(PCB));
    if (!newPCB) {
        printf("Error: Memory allocation failed.\n");
        exit(1);
    }
    newPCB->pid = pid;
    strcpy(newPCB->programName, programName);
    newPCB->partition = partition;
    newPCB->size = size;
    newPCB->state = READY;
    newPCB->children = NULL;

    // If parent is not NULL, add the new PCB as a child
    if (parent != NULL) {
        if (parent->children == NULL) {
            parent->children = newPCB;
        } else {
            // Find the last sibling in the children list
            PCB* currentChild = parent->children;
            while (currentChild->children != NULL) {
                currentChild = currentChild->children;
            }
            currentChild->children = newPCB;
        }
    }

    return newPCB;
}

// Function to load the trace file and fill the TraceEntry structure
int loadTrace(const char* filename, TraceEntry** traceEntries) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open trace file: %s\n", filename);
        return -1;
    }

    char line[100]; // Buffer for reading lines
    int count = 0;

    // Count the number of entries in the file
    while (fgets(line, sizeof(line), file)) {
        count++;
    }

    // Allocate memory for trace entries
    *traceEntries = malloc(count * sizeof(TraceEntry));
    if (!*traceEntries) {
        printf("Error: Memory allocation failed.\n");
        fclose(file);
        return -1;
    }

    rewind(file);  // Reset file pointer to the start

    // Load the trace entries into memory
    int index = 0;
    while (fgets(line, sizeof(line), file)) {
        TraceEntry entry;
        char activity[10] = "";
        char programName[MAX_PROGRAM_NAME] = "";
        int duration = 0;

        // Check if the first 4 characters are "FORK"
        if (strncmp(line, "FORK", 4) == 0) {
            sscanf(line, "%[^,], %d", activity, &duration);  // Parse activity and duration
            strncpy(entry.activity, activity, sizeof(entry.activity) - 1);
            entry.activity[sizeof(entry.activity) - 1] = '\0';  // Null-terminate
            strcpy(entry.programName, "");  // No program name for FORK
            entry.duration = duration;
        }
        // Check if the first 4 characters are "EXEC"
        else if (strncmp(line, "EXEC", 4) == 0) {
            sscanf(line, "%[^ ] %[^,], %d", activity, programName, &duration);  // Parse activity, program name, and duration
            strncpy(entry.activity, activity, sizeof(entry.activity) - 1);
            entry.activity[sizeof(entry.activity) - 1] = '\0';  // Null-terminate
            strncpy(entry.programName, programName, sizeof(entry.programName) - 1);
            entry.programName[sizeof(entry.programName) - 1] = '\0';  // Null-terminate
            entry.duration = duration;
        }

        // Store the entry
        (*traceEntries)[index++] = entry;
    }

    fclose(file);
    return count;
}

// Loads the external files into memory from external_files.txt
int loadExternalFiles(const char* filename, ExternalFile** externalFiles) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open external files: %s\n", filename);
        return -1;
    }

    char line[100];
    int count = 0;

    // Count the number of entries in the file
    while (fgets(line, sizeof(line), file)) {
        count++;
    }

    // Allocate memory for external files
    *externalFiles = malloc(count * sizeof(ExternalFile));
    if (!*externalFiles) {
        printf("Error: Memory allocation failed.\n");
        fclose(file);
        return -1;
    }

    rewind(file);  // Reset file pointer to the start

    // Load external files into memory
    int index = 0;
    while (fgets(line, sizeof(line), file)) {
        ExternalFile extFile;
        char programName[20] = "";

        // Replace commas with spaces in the line
        for (int i = 0; line[i] != '\0'; i++) {
            if (line[i] == ',' || line[i] == ' ') {
                line[i] = ' ';
            }
        }

        // Parse program name and size
        sscanf(line, "%19s %u", programName, &extFile.size); // Restrict to 19 chars for safety
        strncpy(extFile.programName, programName, sizeof(extFile.programName) - 1);
        extFile.programName[sizeof(extFile.programName) - 1] = '\0';  // Null-terminate

        // Store external file entry
        (*externalFiles)[index++] = extFile;
    }

    fclose(file);
    return count;
}

// Dynamically loads the program from the program.txt file
void loadProgram(TraceEntry execEntry, PCB** currentPCB, MemoryPartition* partitions, 
                 ExternalFile* externalFiles, int externalFileCount, FILE* logFile, int* currentTime, FILE* statusFile, PCB* headPCB) {
    char programFileName[30];
    snprintf(programFileName, sizeof(programFileName), "%s.txt", execEntry.programName);

    FILE* programFile = fopen(programFileName, "r");
    if (!programFile) {
        fprintf(logFile, "Error: Could not load program file %s\n", programFileName);
        return;
    }

    char line[100];
    while (fgets(line, sizeof(line), programFile)) {
        // Remove newline character
        line[strcspn(line, "\n")] = 0;

        // FORK and EXEC commands use TraceEntry structure
        if (strncmp(line, "FORK", 4) == 0 || strncmp(line, "EXEC", 4) == 0) {
            TraceEntry entry;
            char command[10] = "";
            char execProgramName[MAX_PROGRAM_NAME] = "";

            // Parse FORK
            if (strncmp(line, "FORK", 4) == 0) {
                sscanf(line, "%[^,], %d", entry.activity, &entry.duration);
                strcpy(entry.programName, "");  // Set program name to empty
                TraceEntry forkEntry = entry;
                handleFork(currentPCB, partitions, logFile, forkEntry, currentTime);
                updateSystemStatus(statusFile, headPCB, currentTime);
            } 
            // Parse EXEC
            else if (strncmp(line, "EXEC", 4) == 0) {
                sscanf(line, "%[^ ] %[^,], %d", entry.activity, execProgramName, &entry.duration);
                strncpy(entry.programName, execProgramName, sizeof(entry.programName) - 1);
                entry.programName[sizeof(entry.programName) - 1] = '\0';  // Null-terminate
                handleExec(entry, *currentPCB, partitions, externalFiles, externalFileCount, logFile, currentTime);
                updateSystemStatus(statusFile, headPCB, currentTime);
                loadProgram(entry, currentPCB, partitions, externalFiles, externalFileCount, logFile, currentTime, statusFile, headPCB);
                // updateSystemStatus(statusFile, headPCB, currentTime);

            }
        }
        // CPU, SYSCALL, and END_IO commands use SystemEntry structure
        else {
            SystemEntry entry;
            char command[10] = "";

            // Parse CPU command
            if (strncmp(line, "CPU", 3) == 0) {
                sscanf(line, "%s %d", entry.activity, &entry.duration);
                useCPU(entry.duration, logFile, currentTime);
            }
            // Parse SYSCALL command
            else if (strncmp(line, "SYSCALL", 7) == 0) {
                sscanf(line, "%s %d, %d", entry.activity, &entry.position, &entry.duration);
                handleSysCall(entry.position, entry.duration, logFile, currentTime);
            }
            // Parse END_IO command
            else if (strncmp(line, "END_IO", 6) == 0) {
                sscanf(line, "%s %d, %d", entry.activity, &entry.position, &entry.duration);
                handleEndIO(entry.position, entry.duration, logFile, currentTime);
            }
        }
    }

    fclose(programFile);
}

// Handles fork, creating a child PCB from the current process
void handleFork(PCB** currentPCB, MemoryPartition* partitions, FILE* logFile, TraceEntry forkEntry, int* currentTime) {
    fprintf(logFile, "%d, 1, switch to kernel mode\n", *currentTime);
    *currentTime += 1;

    // Save context (random between 1-3 ms)
    int contextSaveDuration = rand() % 3 + 1;
    fprintf(logFile, "%d, %d, context saved\n", *currentTime, contextSaveDuration);
    *currentTime += contextSaveDuration;

    fprintf(logFile, "%d, 1, find vector %d in memory position 0x%04X\n", *currentTime, FORK_ISR_ADDRESS, FORK_ISR_ADDRESS * 2);
    *currentTime += 1;
    
    int isrAddress = vectorTable[FORK_ISR_ADDRESS].memoryAddress;
    fprintf(logFile, "%d, 1, load address 0x%04X into the PC\n", *currentTime, isrAddress);
    *currentTime += 1;

    // find best suited partition and then modify the partition
    int bestPartition, partitionIndex;
    findBestPartition(1, partitions, &bestPartition, &partitionIndex);
        if (bestPartition == -1) {
        return;
    }

    // modify partition to show it's occupied
    partitions[partitionIndex].freeSpace -= 1; // should this always be size of 1 or should it be current size of process?
    if (partitions[partitionIndex].freeSpace == 0) // check if memory is full
        strcpy(partitions[partitionIndex].status, "occupied");
    else strcpy(partitions[partitionIndex].status, "free");


    // will have to modify the fork function to pass the partitions as a parameter and everything else findBestPartition needs
    // should we copy the size of the current parent first before creating a new process it?

    // Create a new child PCB by copying the current PCB and add it to parent
    PCB* childPCB = createPCB((*currentPCB)->pid + 1, "init", bestPartition, 1, *currentPCB);
    *currentPCB = (*currentPCB)->children;
    // Split duration of execution
    int numParts = 2;
    int parts[2];
    randomSplit(forkEntry.duration, numParts, parts);

    fprintf(logFile, "%d, %d, FORK: copy parent PCB to child PCB\n", *currentTime, parts[0]);
    *currentTime += parts[0];

    fprintf(logFile, "%d, %d, scheduler called\n", *currentTime, parts[1]);
    *currentTime += parts[1];

    fprintf(logFile, "%d, 1, IRET\n", *currentTime);
    *currentTime += 1;
}

// Function to handle the EXEC system call
void handleExec(TraceEntry execEntry, PCB* currentPCB, MemoryPartition* partitions, ExternalFile* externalFiles, int externalFileCount, FILE* logFile, int* currentTime) {
    fprintf(logFile, "%d, 1, switch to kernel mode\n", *currentTime);
    *currentTime += 1;

    // Save context (random between 1-3 ms)
    int contextSaveDuration = rand() % 3 + 1;
    fprintf(logFile, "%d, %d, context saved\n", *currentTime, contextSaveDuration);
    *currentTime += contextSaveDuration;

    // Find vector in memory and get ISR address for EXEC (found at vector 3)
    fprintf(logFile, "%d, 1, find vector %d in memory position 0x%04X\n", *currentTime, EXEC_ISR_ADDRESS, EXEC_ISR_ADDRESS * 2);
    *currentTime += 1;

    int isrAddress = vectorTable[EXEC_ISR_ADDRESS].memoryAddress;
    fprintf(logFile, "%d, 1, load address 0x%04X into the PC\n", *currentTime, isrAddress);
    *currentTime += 1;

    // Split duration of EXEC call 
    int numParts = 5;
    int execDurations[5];
    randomSplit(execEntry.duration, numParts, execDurations);

    // 1. find program size of programName 
    int programSize = findProgramSize(externalFiles, externalFileCount, execEntry.programName);
    if (programSize == -1) {
        fprintf(logFile, "Error: Program %s not found in external files.\n", execEntry.programName);
        return;
    }

    fprintf(logFile, "%d, %d, EXEC: load %s of size %d MB\n", *currentTime, execDurations[0] , execEntry.programName, programSize);
    *currentTime += execDurations[0];

    // 2. Find the best-fit memory partition for the program
    int bestPartition, partitionIndex;
    findBestPartition(programSize, partitions, &bestPartition, &partitionIndex);
        if (bestPartition == -1) {
        fprintf(logFile, "Error: No suitable partition found for program %s.\n", execEntry.programName);
        return;
    }
    
    fprintf(logFile, "%d, %d, found partition %d with %dMb of space\n", *currentTime, execDurations[1] , bestPartition, programSize);
    *currentTime += execDurations[1];

    //3. free up previously occupied memory and size
    for (int i = 0; i < MAX_PARTITIONS; i++){
        if (partitions[i].partitionNumber == currentPCB->partition){
            partitions[i].freeSpace += currentPCB->size;
            strcpy(partitions[i].status, "free");
        }
    }
    
    // mark the new partition status in (memory)
    partitions[partitionIndex].freeSpace -= programSize; 
    if (partitions[partitionIndex].freeSpace == 0) // check if memory is full
        strcpy(partitions[partitionIndex].status, "occupied");
    else strcpy(partitions[partitionIndex].status, "free");

    fprintf(logFile, "%d, %d, partition %d marked as occupied\n", *currentTime, execDurations[2], bestPartition);
    *currentTime += execDurations[2];

    // 4. updates the currentPCB with the new information
    strcpy(currentPCB->programName, execEntry.programName);
    currentPCB->partition = bestPartition;
    currentPCB->size = programSize;
    currentPCB->state = RUNNING;

    fprintf(logFile, "%d, %d, updating PCB with new information\n", *currentTime, execDurations[3]);
    *currentTime += execDurations[3];

    // 5. Call the scheduler (empty for now)
    fprintf(logFile, "%d, %d, scheduler called\n", *currentTime, execDurations[4]);
    *currentTime += execDurations[4];

    // Log return from interrupt (IRET)
    fprintf(logFile, "%d, 1, IRET\n", *currentTime);
    *currentTime += 1;
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
    int vectorIndex = position;
    int isrAddress = vectorTable[vectorIndex].memoryAddress;

    fprintf(logFile, "%d, 1, find vector %d in memory position 0x%04X\n", *currentTime, position, vectorIndex * 2);
    *currentTime += 1;
    fprintf(logFile, "%d, 1, load address 0x%04X into the PC\n", *currentTime, isrAddress);
    *currentTime += 1;

    // Split duration of ISR across running ISR, transferring data, and checking error time
    int numParts = 3;
    int parts[3];
    randomSplit(duration, numParts, parts);

    // Simulate ISR execution
    fprintf(logFile, "%d, %d, SYSCALL: run the ISR\n", *currentTime, parts[0]);
    *currentTime += parts[0];

    // Simulate data transfer and error checking
    fprintf(logFile, "%d, %d, transfer data\n", *currentTime, parts[1]);
    *currentTime += parts[1];
    
    // check for errors
    fprintf(logFile, "%d, %d, check for errors\n", *currentTime, parts[2]);
    *currentTime += parts[2];

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

// Update system_status.txt after each fork/exec
void updateSystemStatus(FILE* statusFile, PCB* pcbList, int* currentTime) {
    // Write the header to system_status.txt
    fprintf(statusFile, "!-----------------------------------------------------------!\n");
    fprintf(statusFile, "Save Time: %d ms\n", *currentTime);
    fprintf(statusFile, "+--------------------------------------------+\n");
    fprintf(statusFile, "| PID | Program Name | Partition Number | size |\n");
    fprintf(statusFile, "+--------------------------------------------+\n");

    // Iterate through all PCBs and print each one
    PCB* currentPCB = pcbList;
    while (currentPCB != NULL) {
        // Print the current PCB information
        fprintf(statusFile, "| %3d | %12s | %15d | %4d |\n",
                currentPCB->pid,
                currentPCB->programName,
                currentPCB->partition,
                currentPCB->size);

        // Move to the next child PCB
        currentPCB = currentPCB->children;
    }

    fprintf(statusFile, "+--------------------------------------------+\n");
    fprintf(statusFile, "!-----------------------------------------------------------!\n");
    fflush(statusFile);  // Ensure data is written to the file immediately
}

// Function to print all trace entries
void printTraceEntries(TraceEntry* traceEntries, int traceCount) {
    printf("\nLoaded Trace Entries:\n");
    printf("+-------------------------------------------+\n");
    printf("| Activity | Program Name  | Duration       |\n");
    printf("+-------------------------------------------+\n");

    for (int i = 0; i < traceCount; i++) {
        printf("| %8s | %12s | %4d ms        |\n",
               traceEntries[i].activity,
               traceEntries[i].programName,
               traceEntries[i].duration);
    }

    printf("+-------------------------------------------+\n");
}

// Function to print all external files
void printExternalFiles(ExternalFile* externalFiles, int externalFileCount) {
    printf("\nLoaded External Files:\n");
    printf("+--------------------------------+\n");
    printf("| Program Name  | Size (MB)      |\n");
    printf("+--------------------------------+\n");

    for (int i = 0; i < externalFileCount; i++) {
        printf("| %12s | %4u MB         |\n",
               externalFiles[i].programName,
               externalFiles[i].size);
    }

    printf("+--------------------------------+\n");
}


// argc (argument count) represents the number of command line arguments passed to the main function
// argv (argument vector) is an array of strings (array of char pointers) containing the actual command line arguments
// Main function to load files and process the trace
int main(int argc, char* argv[]) {
    srand(time(NULL));  // Initialize random number generator

    if (argc < 3) {
        printf("Usage: %s <trace_file> <external_files>\n", argv[0]);
        return 1;
    }

    // Load trace entries
    TraceEntry* traceEntries;
    int traceCount = loadTrace(argv[1], &traceEntries);
    if (traceCount == -1) {
        return 1;  // Error loading trace
    }
    printTraceEntries(traceEntries, traceCount);

    // Load external files
    ExternalFile* externalFiles;
    int externalFileCount = loadExternalFiles(argv[2], &externalFiles);
    if (externalFileCount == -1) {
        free(traceEntries);
        return 1;  // Error loading external files
    }
    printExternalFiles(externalFiles, externalFileCount);

    // Initialize memory partitions
    MemoryPartition partitions[MAX_PARTITIONS];
    initMemoryPartitions(partitions);

    // Create output files
    FILE* executionFile = fopen("execution.txt", "w");
    FILE* statusFile = fopen("system_status.txt", "w");
    if (!executionFile || !statusFile) {
        printf("Error: Could not create output files.\n");
        free(traceEntries);
        free(externalFiles);
        return 1;
    }

    // Initialize the PCB list
    PCB* headPCB = createPCB(0, "init", 6, 1, NULL);  // Initial process
    PCB* currentPCB = headPCB;
    partitions[5].freeSpace -= 1;
    int currentTime = 0;
    updateSystemStatus(statusFile, headPCB, &currentTime);


    // Process each trace entry
    for (int i = 0; i < traceCount; i++) {
        if (strcmp(traceEntries[i].activity, "FORK") == 0) {
            handleFork(&currentPCB, partitions, executionFile, traceEntries[i], &currentTime); //pass currentPCB by refrence
            updateSystemStatus(statusFile, headPCB, &currentTime);
        } else if (strcmp(traceEntries[i].activity, "EXEC") == 0) {
            handleExec(traceEntries[i], currentPCB, partitions, externalFiles, externalFileCount, executionFile, &currentTime);
            updateSystemStatus(statusFile, headPCB, &currentTime);
            loadProgram(traceEntries[i], &currentPCB, partitions, externalFiles, externalFileCount, executionFile, &currentTime, statusFile, headPCB);
        }
    }

    // Problems:
    // see how the output of nexted FORK AND EXEC differ from non nested

    // Clean up
    fclose(executionFile);
    fclose(statusFile);
    free(traceEntries);
    free(externalFiles);

    return 0;
}