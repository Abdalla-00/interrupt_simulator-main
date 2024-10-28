#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdio.h>

#define VECTOR_TABLE_SIZE 25  // Size of the vector table
#define MAX_PARTITIONS 6      // Number of memory partitions
#define MAX_PROGRAM_NAME 20    // Maximum program name length

// Structure to hold activity data from trace.txt
typedef struct {
    char activity[10];   // Activity type (FORK, EXEC)
    char programName[MAX_PROGRAM_NAME];     // Program name for EXEC/ null for FORK
    int duration;        // Duration of FORK/EXEC overhead
} TraceEntry;

// Structure to hold activity data from program#.txt
typedef struct {
    char activity[10];   // Activity type (CPU, SYSCALL, END_IO)
    int position;        // Position for SYSCALL or END_IO (if applicable)
    int duration;        // Duration of the activity in milliseconds
} SystemEntry;


// Structure to represent an entry in the vector table
typedef struct {
    int interruptNumber;   // Interrupt number (SYSCALL or END_IO)
    int memoryAddress;     // ISR memory address in hex
} VectorEntry;

// Structure to represent memory partitions
typedef struct {
    int partitionNumber;    // Partition number (1 to 6)
    unsigned int size;      // Size in MB
    unsigned int freeSpace; // Available space in the partition
    char status[20];        // "free", "init" or program name
} MemoryPartition;

// Linked list node for PCB
typedef struct PCB {
    int pid;                          // Process ID
    char programName[20];             // Program name
    int partition;                    // Partition number used
    int state;                        // State of the process (e.g., READY, RUNNING)
    int size;                         // Size of program
    struct PCB* parent;               // Pointer to parent PCB
    struct PCB* children;             // Pointer to the list of child PCBs
} PCB;

// Structure to represent an external file entry
typedef struct {
    char programName[20];    // Program name (up to 20 characters)
    unsigned int size;       // Size of the program in memory
} ExternalFile;


// A1 Function declarations
void useCPU(int duration, FILE* logFile, int* currentTime); // Simulates CPU work
void handleSysCall(int position, int duration, FILE* logFile, int* currentTime); // Simulates a system call
void handleEndIO(int position, int duration, FILE* logFile, int* currentTime); // Simulates handling of an I/O interrupt

// A2 function declarations
void initMemoryPartitions(MemoryPartition* partitions); // Initializes memory partitions
void findBestPartition(int programSize, MemoryPartition* partitions, int* bestPartition, int* bestsPartitionIndex);
void randomSplit(int originalValue, int numParts, int* parts);
int findProgramSize(ExternalFile* externalFiles, int externalFileCount, char* programName);

PCB* createPCB(int pid, char* programName, int partition, int size, PCB* parent);

int loadTrace(const char* filename, TraceEntry** traceEntries);
int loadExternalFiles(const char* filename, ExternalFile** externalFiles);
void loadProgram(TraceEntry execEntry, PCB* currentPCB, MemoryPartition* partitions, ExternalFile* externalFiles, int externalFileCount, FILE* logFile, int* currentTime);
void updateSystemStatus(FILE* statusFile, PCB* pcbList, int* currentTime);


void handleFork(PCB* parentPCB, FILE* logFile, TraceEntry forkEntry, int* currentTime);
void handleExec(TraceEntry execEntry, PCB* currentPCB, MemoryPartition* partitions, ExternalFile* externalFiles, int externalFileCount, FILE* logFile, int* currentTime);

void printTraceEntries(TraceEntry* traceEntries, int traceCount);
void printExternalFiles(ExternalFile* externalFiles, int externalFileCount);

#endif