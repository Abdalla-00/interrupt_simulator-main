#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <stdio.h>

#define VECTOR_TABLE_SIZE 25  // Size of the vector table

// Structure to hold activity data from trace.txt
typedef struct {
    char activity[10];   // Activity type (CPU, SYSCALL, END_IO)
    int position;        // Position for SYSCALL or END_IO (if applicable)
    int duration;        // Duration of the activity in milliseconds
} TraceEntry;

// Structure to represent an entry in the vector table
typedef struct {
    int interruptNumber;   // Interrupt number (SYSCALL or END_IO)
    int memoryAddress;     // ISR memory address in hex
} VectorEntry;

// Function declarations
int loadTrace(const char* filename, TraceEntry** traceEntries); // Loads trace input data
void useCPU(int duration, FILE* logFile, int* currentTime); // Simulates CPU work
void handleSysCall(int position, int duration, FILE* logFile, int* currentTime); // Simulates a system call
void handleEndIO(int position, int duration, FILE* logFile, int* currentTime); // Simulates handling of an I/O interrupt

#endif
