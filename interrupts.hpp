#ifndef INTERRUPTS_HPP
#define INTERRUPTS_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
using namespace std;


#define VECTOR_TABLE_SIZE 26  // Size of the vector table
#define MAX_PARTITIONS 6      // Number of memory partitions

// make sure to state assumption of how the trace information must be in order of process arival time
// assumption is that we assume all process are created as if they hasven't arived yet (ie loaded into the ready queue)
// then they are currently at the new state before arrival time
// if a process is finished we terminate (partition is 0, we dont remove from PCB) (maybe ask question), if yet to be worked on, complete 


typedef enum {NEW, READY, RUNNING, WAITING, TERMINATED} ProcessState;

// Structure to represent memory partitions
typedef struct MemoryPartition{
    unsigned int partitionNumber;   // Partition number (1 to 6)
    unsigned int size;              // Size in MB
    int OccupiedBy;                 // PID in process OR -1 if free 
} MemoryPartition;

static MemoryPartition memoryPartitions[MAX_PARTITIONS]; // Fixed memory partition

// PCB
typedef struct PCB {
    unsigned int pid;               // Process ID
    unsigned int arrivalTime;       // Arrival time in milliseconds
    unsigned int totalCPUTime;      // Total CPU time required
    unsigned int remainingCPUTime;  // CPU time remaining
    unsigned int ioFrequency;       // Frequency of I/O operations
    unsigned int initialIODuration; // Duration of each I/O operation
    unsigned int ioDuration;        // Remaining Duration of each I/O operation
    int partition;                  // Partition number assigned (initialize to -1)
    ProcessState currentState;      // Current state of the process
    ProcessState prevState;         // Previous state of the process
    int memorySize;                 // Memory size required
    unsigned int waitTime;          // Total time spent waiting in the ready queue
    unsigned int turnaroundTime;    // Total time from arrival to termination
    unsigned int responseTime;      // Time from arrival to first CPU execution
    PCB* next;               // Pointer to the next PCB in the list
    PCB* prev;               // Pointer to the prev PCD to implemnet double linked list
} PCB;

typedef struct HeadTailPCB{
    PCB* head;   // Pointer to the head of the list
    PCB* tail;   // Pointer to the tail of the list
    unsigned int processCount;  // Number of processes in the list
} HeadTailPCB;

enum MemoryAction { ALLOCATE, FREE };  // Define actions

typedef struct MemoryStatus {
    int totalFreeMemory;
    int usableFreeMemory;
} MemoryStatus;

// Static pointer to a single instance of MemoryStatus
static MemoryStatus* memoryStatus = nullptr;

// Helper to setup
void initMemoryPartitions(MemoryPartition (&partitions)[MAX_PARTITIONS]);
void initializeHeadTail(HeadTailPCB& list);
vector<int> splitToNumbers (const string& str, char delim);
void loadTrace(HeadTailPCB& list, ifstream& inputFile, string& line);
void addPCBNode(HeadTailPCB& list, const vector<int>& PCBinfo);
void freePCB(HeadTailPCB& list);
void printPCB(const HeadTailPCB& list, ofstream& file);

string processStateToString(ProcessState state);

#endif