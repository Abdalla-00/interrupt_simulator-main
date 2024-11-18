#include "interrupts.hpp"

// Helper functions to initialize

// Initializes the memory partitions, smallest partition at last index
void initMemoryPartitions(MemoryPartition (&partitions)[MAX_PARTITIONS]) {
    int sizes[MAX_PARTITIONS] = {40, 25, 15, 10, 8, 2};
    for (int i = 0; i < MAX_PARTITIONS; i++) {
        partitions[i].partitionNumber = i + 1;
        partitions[i].size = sizes[i];
        partitions[i].OccupiedBy = -1;  // initialize all partitions to free
    }
}

void initializeMemoryStatus() {
    if (!memoryStatus) {
        memoryStatus = new MemoryStatus;
        memoryStatus->totalFreeMemory = 0;
        memoryStatus->usableFreeMemory = 0;

        // Calculate initial free memory
        for (int i = 0; i < MAX_PARTITIONS; i++) {
            memoryStatus->totalFreeMemory += memoryPartitions[i].size;
            memoryStatus->usableFreeMemory += memoryPartitions[i].size;
        }
    }
}

void initializeHeadTail(HeadTailPCB& list) {
    list.head = nullptr;
    list.tail = nullptr;
    list.processCount = 0;
}

vector<int> splitToNumbers (const string& str, char delim){

    vector<int> result; // store the result in here
    string token;                       // Temporarily stores token (part of string) as split by delimiter
    std::istringstream stream(str);         // Create a stream of input data to read from one piece at a time

    while (getline(stream, token, delim)){ // reads token from stream until reaching delim
        result.push_back(std::stoi(token));
    }     
    return result;
}

void loadTrace(HeadTailPCB& list, ifstream& inputFile, string& line){

    while(getline(inputFile, line)){
        vector<int> pcbRow = splitToNumbers(line, ',');        // Parse data assign to vector
        addPCBNode(list, pcbRow);                       // Create newPCB with data
    }
}

void addPCBNode(HeadTailPCB& list, const vector<int>& PCBinfo){

    // create PCB
    PCB* newPCB = new PCB{
        static_cast<unsigned int>(PCBinfo[0]),  // PID
        static_cast<unsigned int>(PCBinfo[1]),  // Arrival Time
        static_cast<unsigned int>(PCBinfo[2]),  // Total CPU Time
        static_cast<unsigned int>(PCBinfo[2]),  // Remaining CPU Time (initialize to Total CPU Time)
        static_cast<unsigned int>(PCBinfo[3]),  // I/O Frequency
        static_cast<unsigned int>(PCBinfo[4]),  // I/O Duration
        -1,                                     // Place in memory partition (initialize to -1)
        NEW,                                    // Current State (initialize to NEW)
        NEW,                                    // Previous State (initialize to NEW)
        static_cast<int>(PCBinfo[5]),           // Memory Size
        0,                                      // Wait Time (initialize to 0)
        0,                                      // Turnaround Time (initialize to 0)
        0,                                      // Response Time (initialize to 0)
        nullptr,                                // Next pointer
        nullptr                                 // Prev pointer
    };

    // Case 1: List is empty
    if (list.head == nullptr){
        list.head = newPCB;
        list.tail = newPCB;
    }
    // Case 2: List is not empty
    else{
        newPCB->prev = list.tail;  // Link the new node's prev to the current tail
        list.tail->next = newPCB;  // Link the current tail's next to the new node
        list.tail = newPCB;        // Update the tail pointer to the new node
    }
    list.processCount++;
}

void freePCB(HeadTailPCB& list){

    PCB* current = list.head;
    while (current) {
        PCB* temp = current;  // Store the current node
        current = current->next;  // Move to the next node
        delete(temp);  // Free the memory of the current node
    }

    // Reset the head, tail, and process count
    list.head = nullptr;
    list.tail = nullptr;
    list.processCount = 0;
}

// Helper functions for algorithms
int findBestPartition(const int& memorySize){

    for (int i = MAX_PARTITIONS - 1; i < 0; i--){
        MemoryPartition partition = memoryPartitions[i];
        if(partition.OccupiedBy == -1 && memorySize <= partition.size) return i + 1;
    }
    return -1;
}

void updateMemoryStatus(int partitionIndex, const PCB* pcb, MemoryAction action) {
    if (!memoryStatus) return;  // Ensure memoryStatus is initialized

    switch (action){
        case ALLOCATE:
            // Allocate the partition
            memoryStatus->totalFreeMemory -= pcb->memorySize;
            memoryStatus->usableFreeMemory -= memoryPartitions[partitionIndex].size;
            memoryPartitions[partitionIndex].OccupiedBy = pcb->pid;        
            break;

        case FREE:
            // Free the partition
            memoryStatus->totalFreeMemory += pcb->memorySize;
            memoryStatus->usableFreeMemory += memoryPartitions[partitionIndex].size;
            memoryPartitions[partitionIndex].OccupiedBy = -1;
            break;
    }
}


// Helper to print
void transitionState(PCB* process, ProcessState newState, int currentTime, ofstream& executionFile) {
    static bool headerWritten = false;

    // Write the table header once
    if (!headerWritten) {
        executionFile << "+------------------+--------+----------------+----------------+\n";
        executionFile << "| Time (ms)       | PID    | Old State      | New State      |\n";
        executionFile << "+------------------+--------+----------------+----------------+\n";
        headerWritten = true;
    }

    // Write the transition data in table format
    executionFile << "| " << setw(16) << currentTime
                  << " | " << setw(6) << process->pid
                  << " | " << setw(14) << process->currentState
                  << " | " << setw(14) << newState
                  << " |\n";

    // Update the PCB's state
    process->prevState = process->currentState;
    process->currentState = newState;
}

void displayMemoryStatus(int currentTime, ofstream& memoryStatusFile) {
    if (!memoryStatus) return;  // Ensure memoryStatus is initialized

    // Header
    memoryStatusFile << "+------------------+----------------+----------------+------------------------+\n";
    memoryStatusFile << "| Time (ms)       | Total Free Mem | Usable Free Mem| Partition Status       |\n";
    memoryStatusFile << "+------------------+----------------+----------------+------------------------+\n";

    // Data Row
    memoryStatusFile << "| " << setw(16) << currentTime
                     << " | " << setw(14) << memoryStatus->totalFreeMemory
                     << " | " << setw(14) << memoryStatus->usableFreeMemory
                     << " | ";

    for (int i = 0; i < MAX_PARTITIONS; i++) {
        memoryStatusFile << setw(2) << memoryPartitions[i].OccupiedBy;
        if (i < MAX_PARTITIONS - 1) memoryStatusFile << ", ";
    }

    memoryStatusFile << " |\n";
    memoryStatusFile << "+------------------+----------------+----------------+------------------------+\n";
}

// Algorithms
void runFCFSScheduler(HeadTailPCB& list, ofstream& executionFile, ofstream& memoryStatusFile) {
    PCB* current = list.head;  // Start from the head of the PCB list
    int currentTime = 0;       // Initialize simulation time

    while (current) {
        // Try to allocate memory for the current process
        int partition = findBestPartition(current->memorySize);
        if (partition != -1) {
            // Memory successfully allocated; update memory status
            updateMemoryStatus(partition - 1, current, ALLOCATE);
            displayMemoryStatus(currentTime, memoryStatusFile);

            // Transition process to READY state
            transitionState(current, READY, currentTime, executionFile);

            // Transition process to RUNNING state
            transitionState(current, RUNNING, currentTime, executionFile);

            // Simulate process execution
            currentTime += current->totalCPUTime;

            // Simulate I/O operations if applicable
            if (current->ioFrequency > 0) {
                int ioOccurrences = current->totalCPUTime / current->ioFrequency;
                currentTime += ioOccurrences * current->ioDuration;
            }

            // Transition process to TERMINATED state
            transitionState(current, TERMINATED, currentTime, executionFile);

            // Release memory after process completion
            updateMemoryStatus(partition - 1, current, FREE);
            displayMemoryStatus(currentTime, memoryStatusFile);
        } else {
            // Memory not available; transition process to WAITING state
            transitionState(current, WAITING, currentTime, executionFile);
        }

        // Move to the next process in the queue
        current = current->next;
    }
}


// Main function to load files and process the trace
int main(int argc, char* argv[]) {

    // File is passed
    if (argc < 1){
        cerr << "Error loading input file" << endl;
        return 0;
    }

    // Opening File
    ifstream inputFile(argv[1]);
    if (!inputFile){
        cerr << "Error openning file" << argv[1] << endl;
        return 0;
    }

    // Initialize Memory
    initMemoryPartitions(memoryPartitions);
    initializeMemoryStatus();

    // Load Data in Double Linked list
    string trace;
    HeadTailPCB list;
    initializeHeadTail(list);
    loadTrace(list, inputFile, trace);

    // Create files
    ofstream output_execution_file("execution.txt");
    if(!output_execution_file){
        cerr << "Error generating execution.txt file" << endl;
        return 1;
    }
    ofstream output_memory_status("memory_status.txt");
        if(!output_memory_status){
        cerr << "Error generating memory_status.txt file" << endl;
        return 1;
    }

    // Run FCFS
    runFCFSScheduler(list, output_execution_file, output_memory_status);

    // Close files
    output_execution_file.close();
    output_memory_status.close();
    inputFile.close();

    // Delete allocated memory
    freePCB(list);
    delete memoryStatus;

    return 0;
}



